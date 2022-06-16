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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_KVDB_SERVICE_IMPL_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_KVDB_SERVICE_IMPL_H
#include <set>
#include <vector>

#include "concurrent_map.h"
#include "kv_store_nb_delegate.h"
#include "kvdb_service_stub.h"
#include "kvstore_sync_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/strategy_meta_data.h"
#include "store_cache.h"
namespace OHOS::DistributedKv {
class API_EXPORT KVDBServiceImpl final : public KVDBServiceStub {
public:
    using DBLaunchParam = DistributedDB::AutoLaunchParam;
    API_EXPORT KVDBServiceImpl();
    virtual ~KVDBServiceImpl();
    Status GetStoreIds(const AppId &appId, std::vector<StoreId> &storeIds) override;
    Status BeforeCreate(const AppId &appId, const StoreId &storeId, const Options &options) override;
    Status AfterCreate(const AppId &appId, const StoreId &storeId, const Options &options,
        const std::vector<uint8_t> &password) override;
    Status Delete(const AppId &appId, const StoreId &storeId) override;
    Status Sync(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo) override;
    Status RegisterSyncCallback(const AppId &appId, sptr<IKvStoreSyncCallback> callback) override;
    Status UnregisterSyncCallback(const AppId &appId) override;
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
    Status AppExit(pid_t uid, pid_t pid, uint32_t tokenId, const AppId &appId);
    Status ResolveAutoLaunch(const std::string &identifier, DBLaunchParam &param);

private:
    using StoreMetaData = OHOS::DistributedData::StoreMetaData;
    using StrategyMeta = OHOS::DistributedData::StrategyMeta;
    using DBStore = DistributedDB::KvStoreNbDelegate;
    using SyncEnd = KvStoreSyncManager::SyncEnd;
    using DBResult = std::map<std::string, DistributedDB::DBStatus>;
    void AddOptions(const Options &options, StoreMetaData &metaData);
    StoreMetaData GetStoreMetaData(const AppId &appId, const StoreId &storeId);
    StrategyMeta GetStrategyMeta(const AppId &appId, const StoreId &storeId);
    int32_t GetInstIndex(uint32_t tokenId, const AppId &appId);
    Status DoSync(const StoreMetaData &metaData, const SyncInfo &syncInfo, const SyncEnd &complete);
    Status DoComplete(const StoreMetaData &metaData, const SyncInfo &syncInfo, const DBResult &dbResult);
    uint32_t GetSyncDelayTime(uint32_t delay, const StoreId &storeId);
    Status ConvertDbStatus(DistributedDB::DBStatus status);

    template<class T>
    struct Less {
    public:
        bool operator()(const sptr<T> &__x, const sptr<T> &__y) const
        {
            return __x.GetRefPtr() < __y.GetRefPtr();
        }
    };
    struct SyncAgent {
        pid_t pid_ = 0;
        AppId appId_;
        sptr<IKvStoreSyncCallback> callback_;
        std::map<std::string, uint32_t> delayTimes_;
        std::map<std::string, std::set<sptr<IKvStoreObserver>, Less<IKvStoreObserver>>> observers_;
        std::map<std::string, std::pair<std::vector<std::string>, std::string>> conditions_;
        void ReInit(pid_t pid, const AppId &appId)
        {
            pid_ = pid;
            appId_ = appId;
            callback_ = nullptr;
            delayTimes_.clear();
            observers_.clear();
            conditions_.clear();
        }
    };
    ConcurrentMap<uint32_t, SyncAgent> syncAgents_;
    StoreCache storeCache_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_KVDB_SERVICE_IMPL_H
