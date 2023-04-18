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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_SERVICE_IMPL_H
#define OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_SERVICE_IMPL_H
#include <set>
#include <vector>

#include "concurrent_map.h"
#include "device_matrix.h"
#include "kv_store_nb_delegate.h"
#include "kvdb_service_stub.h"
#include "kvstore_sync_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "metadata/strategy_meta_data.h"
#include "ref_count.h"
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
    Status AddSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo) override;
    Status RmvSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo) override;
    Status Subscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer) override;
    Status Unsubscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer) override;
    Status GetBackupPassword(const AppId &appId, const StoreId &storeId, std::vector<uint8_t> &password) override;
    
    int32_t OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &appId) override;
    int32_t ResolveAutoLaunch(const std::string &identifier, DBLaunchParam &param) override;
    int32_t OnUserChange(uint32_t code, const std::string &user, const std::string &account) override;
    int32_t OnReady(const std::string &device) override;

private:
    using StoreMetaData = OHOS::DistributedData::StoreMetaData;
    using StrategyMeta = OHOS::DistributedData::StrategyMeta;
    using StoreMetaDataLocal = OHOS::DistributedData::StoreMetaDataLocal;
    using DBStore = DistributedDB::KvStoreNbDelegate;
    using DBManager = DistributedDB::KvStoreDelegateManager;
    using SyncEnd = KvStoreSyncManager::SyncEnd;
    using DBResult = std::map<std::string, DistributedDB::DBStatus>;
    using DBStatus = DistributedDB::DBStatus;
    using DBMode = DistributedDB::SyncMode;
    enum SyncAction {
        ACTION_SYNC,
        ACTION_SUBSCRIBE,
        ACTION_UNSUBSCRIBE,
    };
    struct SyncAgent {
        pid_t pid_ = 0;
        AppId appId_;
        sptr<IKvStoreSyncCallback> callback_;
        std::map<std::string, uint32_t> delayTimes_;
        std::map<std::string, std::shared_ptr<StoreCache::Observers>> observers_;
        void ReInit(pid_t pid, const AppId &appId);
    };
    class Factory {
    public:
        Factory();
        ~Factory();
    };

    void AddOptions(const Options &options, StoreMetaData &metaData);
    StoreMetaData GetStoreMetaData(const AppId &appId, const StoreId &storeId);
    StrategyMeta GetStrategyMeta(const AppId &appId, const StoreId &storeId);
    int32_t GetInstIndex(uint32_t tokenId, const AppId &appId);
    Status DoSync(const StoreMetaData &meta, const SyncInfo &info, const SyncEnd &complete, int32_t type);
    Status DoComplete(const StoreMetaData &meta, const SyncInfo &info, RefCount refCount, const DBResult &dbResult);
    uint32_t GetSyncDelayTime(uint32_t delay, const StoreId &storeId);
    Status ConvertDbStatus(DBStatus status) const;
    DBMode ConvertDBMode(SyncMode syncMode) const;
    std::vector<std::string> ConvertDevices(const std::vector<std::string> &deviceIds) const;
    std::shared_ptr<StoreCache::Observers> GetObservers(uint32_t tokenId, const std::string &storeId);
    void SaveLocalMetaData(const Options &options, const StoreMetaData &metaData);
    static Factory factory_;
    ConcurrentMap<uint32_t, SyncAgent> syncAgents_;
    StoreCache storeCache_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_SERVICE_IMPL_H
