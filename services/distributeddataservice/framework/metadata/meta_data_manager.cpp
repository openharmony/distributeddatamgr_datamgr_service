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
#define LOG_TAG "MetaDataManager"

#include "kv_store_nb_delegate.h"
#include "log_print.h"
#include "utils/anonymous.h"

namespace OHOS::DistributedData {
class MetaObserver : public DistributedDB::KvStoreObserver {
public:
    using Filter = MetaDataManager::Filter;
    using MetaStore = MetaDataManager::MetaStore;
    using Observer = MetaDataManager::Observer;
    MetaObserver(std::shared_ptr<MetaStore> metaStore, std::shared_ptr<Filter> filter, Observer observer);
    virtual ~MetaObserver();

    // Database change callback
    void OnChange(const DistributedDB::KvStoreChangedData &data) override;

private:
    std::shared_ptr<MetaStore> metaStore_;
    std::shared_ptr<Filter> filter_;
    Observer observer_;
};

MetaObserver::MetaObserver(std::shared_ptr<MetaStore> metaStore, std::shared_ptr<Filter> filter, Observer observer)
    : metaStore_(std::move(metaStore)), filter_(std::move(filter)), observer_(std::move(observer))
{
    if (metaStore_ != nullptr) {
        int mode = DistributedDB::OBSERVER_CHANGES_NATIVE | DistributedDB::OBSERVER_CHANGES_FOREIGN;
        auto status = metaStore_->RegisterObserver(filter_->GetKey(), mode, this);
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

MetaDataManager::Filter::Filter(const std::string &pattern)
    : pattern_(pattern)
{
}

void MetaObserver::OnChange(const DistributedDB::KvStoreChangedData &data)
{
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

void MetaDataManager::Initialize(std::shared_ptr<MetaStore> metaStore, const Backup &backup, const Syncer &syncer)
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
    syncer_ = syncer;
    inited_ = true;
}

bool MetaDataManager::SaveMeta(const std::string &key, const Serializable &value, bool isLocal)
{
    if (!inited_) {
        return false;
    }

    auto data = Serializable::Marshall(value);
    auto status = isLocal ? metaStore_->PutLocal({ key.begin(), key.end() }, { data.begin(), data.end() })
                          : metaStore_->Put({ key.begin(), key.end() }, { data.begin(), data.end() });
    if (status == DistributedDB::DBStatus::OK && backup_) {
        backup_(metaStore_);
    }
    if (!isLocal && syncer_) {
        syncer_(metaStore_, status);
    }
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("failed! status:%{public}d isLocal:%{public}d, key:%{public}s",
            status, isLocal, Anonymous::Change(key).c_str());
    }
    return status == DistributedDB::DBStatus::OK;
}

bool MetaDataManager::LoadMeta(const std::string &key, Serializable &value, bool isLocal)
{
    if (!inited_) {
        return false;
    }

    DistributedDB::Value data;
    auto status = isLocal ? metaStore_->GetLocal({ key.begin(), key.end() }, data)
                          : metaStore_->Get({ key.begin(), key.end() }, data);
    if (status != DistributedDB::DBStatus::OK) {
        return false;
    }
    Serializable::Unmarshall({ data.begin(), data.end() }, value);
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
    if (status != DistributedDB::DBStatus::OK && status != DistributedDB::DBStatus::NOT_FOUND) {
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

    DistributedDB::Value data;
    auto status = isLocal ? metaStore_->DeleteLocal({ key.begin(), key.end() })
                          : metaStore_->Delete({ key.begin(), key.end() });
    if (status == DistributedDB::DBStatus::OK && backup_) {
        backup_(metaStore_);
    }
    if (!isLocal && syncer_) {
        syncer_(metaStore_, status);
    }
    return ((status == DistributedDB::DBStatus::OK) || (status == DistributedDB::DBStatus::NOT_FOUND));
}

bool MetaDataManager::Subscribe(std::shared_ptr<Filter> filter, Observer observer)
{
    if (!inited_) {
        return false;
    }

    return metaObservers_.ComputeIfAbsent(
        "", [ this, &observer, &filter ](const std::string &key) -> auto {
            return std::make_shared<MetaObserver>(metaStore_, filter, observer);
        });
}

bool MetaDataManager::Subscribe(std::string prefix, Observer observer)
{
    if (!inited_) {
        return false;
    }

    return metaObservers_.ComputeIfAbsent(
        prefix, [ this, &observer, &prefix ](const std::string &key) -> auto {
            return std::make_shared<MetaObserver>(metaStore_, std::make_shared<Filter>(prefix), observer);
        });
}

bool MetaDataManager::Unsubscribe(std::string filter)
{
    if (!inited_) {
        return false;
    }

    return metaObservers_.Erase(filter);
}
} // namespace OHOS::DistributedData