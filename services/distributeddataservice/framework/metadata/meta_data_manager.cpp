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

#include <utility>

#include "kv_store_nb_delegate.h"
#include "log/log_print.h"

namespace OHOS::DistributedData {
class MetaObserver : public DistributedDB::KvStoreObserver {
public:
    MetaObserver(const std::shared_ptr<MetaDataManager::MetaStore> &metaStore, const std::string &prefix,
        const MetaDataManager::Observer &observer);
    ~MetaObserver() override;

    // Database change callback
    void OnChange(const DistributedDB::KvStoreChangedData &data) override;

private:
    std::shared_ptr<MetaDataManager::MetaStore> metaStore_;
    MetaDataManager::Observer observer_;
    std::string prefix_;
};

MetaObserver::MetaObserver(const std::shared_ptr<MetaDataManager::MetaStore> &metaStore, const std::string &prefix,
    const MetaDataManager::Observer &observer)
    : metaStore_(metaStore), observer_(observer), prefix_(prefix)
{
    if (metaStore_ != nullptr) {
        int mode = DistributedDB::OBSERVER_CHANGES_NATIVE | DistributedDB::OBSERVER_CHANGES_FOREIGN;
        auto status = metaStore_->RegisterObserver(DistributedDB::Key(), mode, this);
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

void MetaObserver::OnChange(const DistributedDB::KvStoreChangedData &data)
{
    auto entriesGroups = { &data.GetEntriesInserted(), &data.GetEntriesUpdated(), &data.GetEntriesDeleted() };
    int32_t next = MetaDataManager::INSERT;
    for (auto entries : entriesGroups) {
        int32_t action = next++;
        if (entries->empty()) {
            continue;
        }
        for (auto &[key, value] : *entries) {
            std::string keyStr(key.begin(), key.end());
            if (keyStr.find(prefix_) != 0) {
                continue;
            }
            observer_(keyStr, { value.begin(), value.end() }, action);
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

void MetaDataManager::SetMetaStore(std::shared_ptr<MetaStore> metaStore)
{
    if (metaStore == nullptr) {
        return;
    }

    std::lock_guard<decltype(mutex_)> lg(mutex_);
    if (inited_) {
        return;
    }
    metaStore_ = std::move(metaStore);
    inited_ = true;
}

bool MetaDataManager::SaveMeta(const std::string &key, const Serializable &value)
{
    if (!inited_) {
        ZLOGE("not init");
        return false;
    }

    auto data = Serializable::Marshall(value);
    auto status = metaStore_->Put({ key.begin(), key.end() }, { data.begin(), data.end() });
    return status == DistributedDB::DBStatus::OK;
}

bool MetaDataManager::LoadMeta(const std::string &key, Serializable &value)
{
    if (!inited_) {
        return false;
    }

    DistributedDB::Value data;
    auto status = metaStore_->Get({ key.begin(), key.end() }, data);
    if (status != DistributedDB::DBStatus::OK) {
        return false;
    }
    Serializable::Unmarshall({ data.begin(), data.end() }, value);
    return true;
}

bool MetaDataManager::SubscribeMeta(const std::string &prefix, Observer observer)
{
    if (!inited_) {
        return false;
    }

    return metaObservers_.ComputeIfAbsent(
        prefix, [ this, &observer ](const std::string &key) -> auto {
            return std::make_shared<MetaObserver>(metaStore_, key, observer);
        });
}
} // namespace OHOS::DistributedData