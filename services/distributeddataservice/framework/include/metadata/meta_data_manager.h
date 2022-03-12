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
    using MetaStore = DistributedDB::KvStoreNbDelegate;
    using Observer = std::function<bool(const std::string &, const std::string &, int32_t)>;
    API_EXPORT static MetaDataManager &GetInstance();
    API_EXPORT void SetMetaStore(std::shared_ptr<MetaStore> metaStore);
    API_EXPORT bool SaveMeta(const std::string &key, const Serializable &value);
    API_EXPORT bool LoadMeta(const std::string &key, Serializable &value);
    API_EXPORT bool SubscribeMeta(const std::string &prefix, Observer observer);

private:
    MetaDataManager();
    ~MetaDataManager();
    bool inited_ = false;
    std::mutex mutex_;
    std::shared_ptr<MetaStore> metaStore_;
    ConcurrentMap<std::string, std::shared_ptr<MetaObserver>> metaObservers_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_METADATA_META_DATA_MANAGER_H
