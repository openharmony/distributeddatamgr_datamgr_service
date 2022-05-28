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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_OBSERVER_BRIDGE_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_OBSERVER_BRIDGE_H
#include "kv_store_observer.h"
#include "kvstore_observer.h"
#include "visibility.h"
namespace OHOS::DistributedKv {
class API_EXPORT ObserverBridge : public DistributedDB::KvStoreObserver {
public:
    using Convert = std::function<Key(const DistributedDB::Key &key, std::string &deviceId)>;

    ObserverBridge(std::shared_ptr<DistributedKv::KvStoreObserver> observer, Convert convert = nullptr);
    void OnChange(const DistributedDB::KvStoreChangedData &data) override;

private:
    std::vector<Entry> ConvertDB(const std::list<DistributedDB::Entry> &dbEntries, std::string &deviceId) const;
    std::shared_ptr<DistributedKv::KvStoreObserver> observer_;
    Convert convert_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_OBSERVER_BRIDGE_H
