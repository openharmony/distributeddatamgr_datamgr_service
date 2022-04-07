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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_META_DATA_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_META_DATA_MANAGER_H
#include <functional>
#include <memory>
#include <mutex>

#include "concurrent_map.h"
#include "serializable/serializable.h"
namespace DistributedDB {
class KvStoreNbDelegate;
}
namespace OHOS::DistributedData {
class MetaObserver;
class MetaDataManager {
public:
    enum Action : int32_t {
        INSERT,
        UPDATE,
        DELETE,
    };
    class API_EXPORT Filter {
    public:
        Filter() = default;
        Filter(const std::string &pattern);
        virtual ~Filter() = default;
        virtual bool operator()(const std::string &key) const;
        virtual std::vector<uint8_t> GetKey() const;

    private:
        std::string pattern_;
    };
    using MetaStore = DistributedDB::KvStoreNbDelegate;
    using Observer = std::function<bool(const std::string &, const std::string &, int32_t)>;
    using Syncer = std::function<void(const std::shared_ptr<MetaStore> &, int32_t)>;
    using Backup = std::function<int32_t(const std::shared_ptr<MetaStore> &)>;
    using Bytes = std::vector<uint8_t>;
    API_EXPORT static MetaDataManager &GetInstance();
    API_EXPORT void Initialize(std::shared_ptr<MetaStore> metaStore, const Backup &backup, const Syncer &syncer);
    API_EXPORT bool SaveMeta(const std::string &key, const Serializable &value, bool isLocal = false);
    API_EXPORT bool LoadMeta(const std::string &key, Serializable &value, bool isLocal = false);
    template<class T>
    API_EXPORT bool LoadMeta(const std::string &prefix, std::vector<T> &values, bool isLocal = false)
    {
        if (!inited_) {
            return false;
        }
        std::vector<Bytes> entries;
        if (!GetEntries(prefix, entries, isLocal)) {
            return false;
        }
        values.resize(entries.size());
        for (size_t i = 0; i < entries.size(); ++i) {
            Serializable::Unmarshall({ entries[i].begin(), entries[i].end() }, values[i]);
        }
        return true;
    }

    API_EXPORT bool DelMeta(const std::string &key, bool isLocal = false);
    API_EXPORT bool Subscribe(std::shared_ptr<Filter> filter, Observer observer);
    API_EXPORT bool Subscribe(std::string prefix, Observer observer);
    API_EXPORT bool Unsubscribe(std::string filter);
private:
    MetaDataManager();
    ~MetaDataManager();

    API_EXPORT bool GetEntries(const std::string &prefix, std::vector<Bytes> &entries, bool isLocal);

    bool inited_ = false;
    std::mutex mutex_;
    std::shared_ptr<MetaStore> metaStore_;
    ConcurrentMap<std::string, std::shared_ptr<MetaObserver>> metaObservers_;
    Backup backup_;
    Syncer syncer_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_META_DATA_MANAGER_H
