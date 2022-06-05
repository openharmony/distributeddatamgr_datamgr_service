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
#include "kvstore_sync_callback_client.h"
namespace OHOS::DistributedKv {
class API_EXPORT KVDBServiceClient : public IRemoteProxy<KVDBService> {
public:
    static std::shared_ptr<KVDBServiceClient> GetInstance();
    Status GetStoreIds(const AppId &appId, std::vector<StoreId> &storeIds) override;
    Status BeforeCreate(const AppId &appId, const StoreId &storeId, const Options &options) override;
    Status AfterCreate(const AppId &appId, const StoreId &storeId, const Options &options,
        const std::vector<uint8_t> &password) override;
    Status Delete(const AppId &appId, const StoreId &storeId) override;
    Status Sync(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo) override;
    Status RegisterSyncCallback(
        const AppId &appId, const StoreId &storeId, sptr<IKvStoreSyncCallback> callback) override;
    Status UnregisterSyncCallback(const AppId &appId, const StoreId &storeId) override;
    Status SetSyncParam(const AppId &appId, const StoreId &storeId, const KvSyncParam &syncParam) override;
    Status GetSyncParam(const AppId &appId, const StoreId &storeId, KvSyncParam &syncParam) override;
    Status EnableCapability(const AppId &appId, const StoreId &storeId) override;
    Status DisableCapability(const AppId &appId, const StoreId &storeId) override;
    Status SetCapability(const AppId &appId, const StoreId &storeId, const std::vector<std::string> &local,
        const std::vector<std::string> &remote) override;
    Status AddSubscribeInfo(const AppId &appId, const StoreId &storeId, const std::vector<std::string> &devices,
        const std::string &query) override;
    Status RmvSubscribeInfo(const AppId &appId, const StoreId &storeId, const std::vector<std::string> &devices,
        const std::string &query) override;
    Status Subscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer) override;
    Status Unsubscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer) override;
    sptr<KvStoreSyncCallbackClient> GetSyncAgent(const AppId &appId);

private:
    explicit KVDBServiceClient(const sptr<IRemoteObject> &object);
    virtual ~KVDBServiceClient() = default;
    class ServiceDeath : public KvStoreDeathRecipient {
    public:
        ServiceDeath() = default;
        virtual ~ServiceDeath() = default;
        void OnRemoteDied() override;
    };
    static std::mutex mutex_;
    static std::shared_ptr<KVDBServiceClient> instance_;
    static std::atomic_bool isWatched_;
    sptr<IRemoteObject> remote_;
    ConcurrentMap<std::string, sptr<KvStoreSyncCallbackClient>> syncAgents_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SERVICE_CLIENT_H
