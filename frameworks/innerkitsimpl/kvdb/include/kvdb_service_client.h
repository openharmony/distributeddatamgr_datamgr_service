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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SERVICE_CLIENT_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SERVICE_CLIENT_H
#include <functional>

#include "concurrent_map.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "kvdb_service.h"

namespace OHOS::DistributedKv {
class KVDBServiceClient : public IRemoteProxy<KVDBService> {
public:
    static std::shared_ptr<KVDBServiceClient> CreateInstance();
    Status GetStoreIds(const AppId &appId, std::vector<StoreId> &storeIds) override;

    Status Delete(const std::string &path, const AppId &appId, const StoreId &storeId) override;
    Status Sync(const std::vector<std::string> &devices, const AppId &appId, const StoreId &storeId) override;
    std::shared_ptr<SingleKvStore> GetKVStore(
        const std::string &path, const Options &options, const AppId &appId, const StoreId &storeId, Status &status);
    Status CloseKVStore(const AppId &appId, const StoreId &storeId);
    Status CloseKVStore(const AppId &appId, std::shared_ptr<SingleKVStore> &kvStore);
    Status CloseAllKVStore(const AppId &appId);

protected:
    Status BeforeCreate(const Options &options, const AppId &appId, const StoreId &storeId) override;
    Status AfterCreate(const Options &options, const AppId &appId, const StoreId &storeId,
        const std::vector<uint8_t> &password) override;

private:
    explicit KVDBServiceClient(const sptr<IRemoteObject> &object);
    virtual ~KVDBServiceClient() = default;
    friend class BrokerCreator<KVDBServiceClient>;
    static BrokerDelegator<KVDBServiceClient> delegator_;
    std::function<void(int, int)> value;
    sptr<IRemoteObject> remote_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SERVICE_CLIENT_H
