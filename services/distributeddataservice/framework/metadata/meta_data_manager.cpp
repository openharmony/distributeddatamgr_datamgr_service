/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "metadata/meta_data_manager.h"
#include <csignal>
#define LOG_TAG "MetaDataManager"

#include "directory/directory_manager.h"
#include "kv_store_nb_delegate.h"
#include "log_print.h"
#include "utils/anonymous.h"
#include "utils/corrupt_reporter.h"

namespace OHOS::DistributedData {
class MetaObserver : public DistributedDB::KvStoreObserver {
public:
    using Filter = MetaDataManager::Filter;
    using MetaStore = MetaDataManager::MetaStore;
    using Observer = MetaDataManager::Observer;
    using DBOrigin = DistributedDB::Origin;
    using DBChangeData = DistributedDB::ChangedData;
    using Type = DistributedDB::Type;
    MetaObserver(std::shared_ptr<MetaStore> metaStore, std::shared_ptr<Filter> filter, Observer observer,
        bool isLocal = false);
    virtual ~MetaObserver();

    // Database change callback
    void OnChange(const DistributedDB::KvStoreChangedData &data) override;
    void OnChange(DBOrigin origin, const std::string &originalId, DBChangeData &&data) override;

    void HandleChanges(int32_t flag, std::vector<std::vector<Type>> &priData);

private:
    std::shared_ptr<MetaStore> metaStore_;
    std::shared_ptr<Filter> filter_;
    Observer observer_;
};

MetaObserver::MetaObserver(
    std::shared_ptr<MetaStore> metaStore, std::shared_ptr<Filter> filter, Observer observer, bool isLocal)
    : metaStore_(std::move(metaStore)), filter_(std::move(filter)), observer_(std::move(observer))
{
    if (metaStore_ != nullptr) {
        int mode = isLocal ? DistributedDB::OBSERVER_CHANGES_LOCAL_ONLY
                           : (DistributedDB::OBSERVER_CHANGES_NATIVE | DistributedDB::OBSERVER_CHANGES_FOREIGN);
        auto status = metaStore_->RegisterObserver(filter_->GetKey(), mode, this);
        if (!isLocal) {
            status = metaStore_->RegisterObserver(filter_->GetKey(), DistributedDB::OBSERVER_CHANGES_CLOUD, this);
        }
        if (status != DistributedDB::DBStatus::OK) {
            ZLOGE("register meta observer failed :%{public}d.", status);
        }
    }
}

MetaObserver::~MetaObserver()
{
    if (metaStore_ != nullptr) {
        metaStore_->UnRegisterObserver(this);
    }
}

bool MetaDataManager::Filter::operator()(const std::string &key) const
{
    return key.find(pattern_) == 0;
}

std::vector<uint8_t> MetaDataManager::Filter::GetKey() const
{
    return std::vector<uint8_t>();
}

MetaDataManager::Filter::Filter(const std::string &pattern) : pattern_(pattern)
{
}

void MetaObserver::OnChange(const DistributedDB::KvStoreChangedData &data)
{
    if (filter_ == nullptr) {
        ZLOGE("filter_ is nullptr!");
        return;
    }
    auto values = { &data.GetEntriesInserted(), &data.GetEntriesUpdated(), &data.GetEntriesDeleted() };
    int32_t next = MetaDataManager::INSERT;
    for (auto value : values) {
        int32_t action = next++;
        if (value->empty()) {
            continue;
        }
        for (const auto &entry : *value) {
            std::string key(entry.key.begin(), entry.key.end());
            if (!(*filter_)(key)) {
                continue;
            }
            observer_(key, { entry.value.begin(), entry.value.end() }, action);
        }
    }
}

void MetaObserver::OnChange(DBOrigin origin, const std::string &originalId, DBChangeData &&data)
{
    (void)origin;
    (void)originalId;
    HandleChanges(MetaDataManager::INSERT, data.primaryData[MetaDataManager::INSERT]);
    HandleChanges(MetaDataManager::UPDATE, data.primaryData[MetaDataManager::UPDATE]);
    HandleChanges(MetaDataManager::DELETE, data.primaryData[MetaDataManager::DELETE]);
}

void MetaObserver::HandleChanges(int32_t flag, std::vector<std::vector<Type>> &priData)
{
    if (priData.empty()) {
        return;
    }
    if (filter_ == nullptr) {
        ZLOGE("filter_ is nullptr!");
        return;
    }
    for (const auto &priKey : priData) {
        if (priKey.empty()) {
            continue;
        }
        auto strValue = std::get_if<std::string>(&priKey[0]);
        if (strValue != nullptr) {
            auto key = *strValue;
            if (!(*filter_)(key)) {
                continue;
            }
            observer_(key, "", flag);
        }
    }
}

MetaDataManager &MetaDataManager::GetInstance()
{
    static MetaDataManager instance;
    return instance;
}

MetaDataManager::MetaDataManager() = default;

MetaDataManager::~MetaDataManager()
{
    metaObservers_.Clear();
}

void MetaDataManager::Initialize(std::shared_ptr<MetaStore> metaStore, const Backup &backup, const std::string &storeId)
{
    if (metaStore == nullptr) {
        return;
    }

    std::lock_guard<decltype(mutex_)> lg(mutex_);
    if (inited_) {
        return;
    }
    metaStore_ = std::move(metaStore);
    backup_ = backup;
    storeId_ = storeId;
    inited_ = true;
}

void MetaDataManager::SetSyncer(const Syncer &syncer)
{
    if (metaStore_ == nullptr) {
        return;
    }
    syncer_ = syncer;
}

void MetaDataManager::SetCloudSyncer(const CloudSyncer &cloudSyncer)
{
    if (metaStore_ == nullptr) {
        return;
    }
    cloudSyncer_ = cloudSyncer;
}

bool MetaDataManager::SaveMeta(const std::string &key, const Serializable &value, bool isLocal)
{
    if (!inited_) {
        return false;
    }

    auto data = Serializable::Marshall(value);
    auto status = isLocal ? metaStore_->PutLocal({ key.begin(), key.end() }, { data.begin(), data.end() })
                          : metaStore_->Put({ key.begin(), key.end() }, { data.begin(), data.end() });
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        ZLOGE("db corrupted! status:%{public}d isLocal:%{public}d, key:%{public}s",
            status, isLocal, Anonymous::Change(key).c_str());
        CorruptReporter::CreateCorruptedFlag(DirectoryManager::GetInstance().GetMetaStorePath(), storeId_);
        StopSA();
        return false;
    }
    if (status == DistributedDB::DBStatus::OK && backup_) {
        backup_(metaStore_);
    }
    if (!isLocal && cloudSyncer_) {
        cloudSyncer_();
    }
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("failed! status:%{public}d isLocal:%{public}d, key:%{public}s", status, isLocal,
            Anonymous::Change(key).c_str());
    }
    DelCacheMeta(key, isLocal);
    return status == DistributedDB::DBStatus::OK;
}

bool MetaDataManager::SaveMeta(const std::vector<Entry> &values, bool isLocal)
{
    if (!inited_) {
        return false;
    }
    if (values.empty()) {
        return true;
    }
    std::vector<DistributedDB::Entry> entries;
    entries.reserve(values.size());
    for (const auto &[key, value] : values) {
        entries.push_back({ { key.begin(), key.end() }, { value.begin(), value.end() } });
        DelCacheMeta(key, isLocal);
    }
    auto status = isLocal ? metaStore_->PutLocalBatch(entries) : metaStore_->PutBatch(entries);
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        ZLOGE("db corrupted! status:%{public}d isLocal:%{public}d, size:%{public}zu", status, isLocal, values.size());
        CorruptReporter::CreateCorruptedFlag(DirectoryManager::GetInstance().GetMetaStorePath(), storeId_);
        StopSA();
        return false;
    }
    if (status == DistributedDB::DBStatus::OK && backup_) {
        backup_(metaStore_);
    }
    if (!isLocal && cloudSyncer_) {
        cloudSyncer_();
    }
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("failed! status:%{public}d isLocal:%{public}d, size:%{public}zu", status, isLocal, values.size());
    }
    return status == DistributedDB::DBStatus::OK;
}

bool MetaDataManager::LoadMeta(const std::string &key, Serializable &value, bool isLocal)
{
    if (!inited_) {
        return false;
    }
    if (LoadCacheMeta(key, value, isLocal)) {
        return true;
    }
    DistributedDB::Value data;
    auto status = isLocal ? metaStore_->GetLocal({ key.begin(), key.end() }, data)
                          : metaStore_->Get({ key.begin(), key.end() }, data);
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        ZLOGE("db corrupted! status:%{public}d isLocal:%{public}d, key:%{public}s",
            status, isLocal, Anonymous::Change(key).c_str());
        CorruptReporter::CreateCorruptedFlag(DirectoryManager::GetInstance().GetMetaStorePath(), storeId_);
        StopSA();
        return false;
    }
    if (status != DistributedDB::DBStatus::OK) {
        return false;
    }
    std::string tempdata(data.begin(), data.end());
    SaveCacheMeta(key, tempdata, isLocal);
    Serializable::Unmarshall(tempdata, value);
    if (isLocal) {
        data.assign(data.size(), 0);
    }
    return true;
}

bool MetaDataManager::GetEntries(const std::string &prefix, std::vector<Bytes> &entries, bool isLocal)
{
    std::vector<DistributedDB::Entry> dbEntries;
    auto status = isLocal ? metaStore_->GetLocalEntries({ prefix.begin(), prefix.end() }, dbEntries)
                          : metaStore_->GetEntries({ prefix.begin(), prefix.end() }, dbEntries);
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        ZLOGE("db corrupted! status:%{public}d isLocal:%{public}d", status, isLocal);
        CorruptReporter::CreateCorruptedFlag(DirectoryManager::GetInstance().GetMetaStorePath(), storeId_);
        StopSA();
        return false;
    }
    if (status != DistributedDB::DBStatus::OK && status != DistributedDB::DBStatus::NOT_FOUND) {
        ZLOGE("failed! prefix:%{public}s status:%{public}d isLocal:%{public}d", Anonymous::Change(prefix).c_str(),
            status, isLocal);
        return false;
    }
    entries.resize(dbEntries.size());
    for (size_t i = 0; i < dbEntries.size(); ++i) {
        entries[i] = std::move(dbEntries[i].value);
    }
    return true;
}

bool MetaDataManager::DelMeta(const std::string &key, bool isLocal)
{
    if (!inited_) {
        return false;
    }
    DelCacheMeta(key, isLocal);
    auto status = isLocal ? metaStore_->DeleteLocal({ key.begin(), key.end() })
                          : metaStore_->Delete({ key.begin(), key.end() });
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        ZLOGE("db corrupted! status:%{public}d isLocal:%{public}d, key:%{public}s",
            status, isLocal, Anonymous::Change(key).c_str());
        CorruptReporter::CreateCorruptedFlag(DirectoryManager::GetInstance().GetMetaStorePath(), storeId_);
        StopSA();
        return false;
    }
    if (status == DistributedDB::DBStatus::OK && backup_) {
        backup_(metaStore_);
    }
    if (!isLocal && cloudSyncer_) {
        cloudSyncer_();
    }
    return ((status == DistributedDB::DBStatus::OK) || (status == DistributedDB::DBStatus::NOT_FOUND));
}

bool MetaDataManager::DelMeta(const std::vector<std::string> &keys, bool isLocal)
{
    if (!inited_) {
        return false;
    }
    if (keys.empty()) {
        return true;
    }
    std::vector<DistributedDB::Key> dbKeys;
    dbKeys.reserve(keys.size());
    for (auto &key : keys) {
        dbKeys.emplace_back(key.begin(), key.end());
        DelCacheMeta(key, isLocal);
    }
    auto status = isLocal ? metaStore_->DeleteLocalBatch(dbKeys) : metaStore_->DeleteBatch(dbKeys);
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        ZLOGE("db corrupted! status:%{public}d isLocal:%{public}d, key size:%{public}zu", status, isLocal,
            dbKeys.size());
        CorruptReporter::CreateCorruptedFlag(DirectoryManager::GetInstance().GetMetaStorePath(), storeId_);
        StopSA();
        return false;
    }
    if (status == DistributedDB::DBStatus::OK && backup_) {
        backup_(metaStore_);
    }
    if (!isLocal && cloudSyncer_) {
        cloudSyncer_();
    }
    return ((status == DistributedDB::DBStatus::OK) || (status == DistributedDB::DBStatus::NOT_FOUND));
}

bool MetaDataManager::Sync(const std::vector<std::string> &devices, OnComplete complete, bool wait)
{
    if (!inited_ || devices.empty()) {
        return false;
    }
    auto status = metaStore_->Sync(devices, DistributedDB::SyncMode::SYNC_MODE_PUSH_PULL, [complete](auto &dbResults) {
        if (complete == nullptr) {
            return;
        }
        std::map<std::string, int32_t> results;
        for (auto &[uuid, status] : dbResults) {
            results.insert_or_assign(uuid, static_cast<int32_t>(status));
        }
        complete(results);
    }, wait);
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        ZLOGE("db corrupted! status:%{public}d", status);
        CorruptReporter::CreateCorruptedFlag(DirectoryManager::GetInstance().GetMetaStorePath(), storeId_);
        StopSA();
        return false;
    }
    if (status != DistributedDB::OK) {
        ZLOGW("meta data sync error %{public}d.", status);
    }
    return status == DistributedDB::OK;
}

bool MetaDataManager::Subscribe(std::shared_ptr<Filter> filter, Observer observer)
{
    if (!inited_) {
        return false;
    }

    return metaObservers_.ComputeIfAbsent("", [this, &observer, &filter](const std::string &key) -> auto {
        return std::make_shared<MetaObserver>(metaStore_, filter, observer);
    });
}

bool MetaDataManager::Subscribe(std::string prefix, Observer observer, bool isLocal)
{
    if (!inited_) {
        return false;
    }

    return metaObservers_.ComputeIfAbsent(prefix, [this, isLocal, &observer, &prefix](const std::string &key) -> auto {
        return std::make_shared<MetaObserver>(metaStore_, std::make_shared<Filter>(prefix), observer, isLocal);
    });
}

bool MetaDataManager::Unsubscribe(std::string filter)
{
    if (!inited_) {
        return false;
    }

    return metaObservers_.Erase(filter);
}

void MetaDataManager::StopSA()
{
    ZLOGI("stop distributeddata");
    int err = raise(SIGKILL);
    if (err < 0) {
        ZLOGE("stop distributeddata failed, errCode: %{public}d", err);
    }
}
} // namespace OHOS::DistributedData