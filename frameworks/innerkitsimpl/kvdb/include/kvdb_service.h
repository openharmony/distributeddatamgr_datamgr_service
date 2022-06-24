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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SERVICE_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SERVICE_H
#include <memory>
#include <limits>
#include "ikvstore_observer.h"
#include "ikvstore_sync_callback.h"
#include "iremote_broker.h"
#include "kvstore_death_recipient.h"
#include "single_kvstore.h"
#include "types.h"
#include "visibility.h"
namespace OHOS::DistributedKv {
class API_EXPORT KVDBService : public IRemoteBroker {
public:
    using SingleKVStore = SingleKvStore;
    struct SyncInfo {
        uint64_t seqId = std::numeric_limits<uint64_t>::max();
        int32_t mode = PUSH_PULL;
        uint32_t delay = 0;
        std::vector<std::string> devices;
        std::string query;
    };
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedKv.KVDBService");

    API_EXPORT KVDBService() = default;

    API_EXPORT virtual ~KVDBService() = default;

    virtual Status GetStoreIds(const AppId &appId, std::vector<StoreId> &storeIds) = 0;
    virtual Status BeforeCreate(const AppId &appId, const StoreId &storeId, const Options &options) = 0;
    virtual Status AfterCreate(
        const AppId &appId, const StoreId &storeId, const Options &options, const std::vector<uint8_t> &password) = 0;
    virtual Status Delete(const AppId &appId, const StoreId &storeId) = 0;
    virtual Status Sync(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo) = 0;
    virtual Status RegisterSyncCallback(const AppId &appId, sptr<IKvStoreSyncCallback> callback) = 0;
    virtual Status UnregisterSyncCallback(const AppId &appId) = 0;
    virtual Status SetSyncParam(const AppId &appId, const StoreId &storeId, const KvSyncParam &syncParam) = 0;
    virtual Status GetSyncParam(const AppId &appId, const StoreId &storeId, KvSyncParam &syncParam) = 0;
    virtual Status EnableCapability(const AppId &appId, const StoreId &storeId) = 0;
    virtual Status DisableCapability(const AppId &appId, const StoreId &storeId) = 0;
    virtual Status SetCapability(const AppId &appId, const StoreId &storeId, const std::vector<std::string> &local,
        const std::vector<std::string> &remote) = 0;
    virtual Status AddSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo) = 0;
    virtual Status RmvSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo) = 0;
    virtual Status Subscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer) = 0;
    virtual Status Unsubscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer) = 0;

protected:
    enum TransId : int32_t {
        TRANS_HEAD,
        TRANS_GET_STORE_IDS = TRANS_HEAD,
        TRANS_BEFORE_CREATE,
        TRANS_AFTER_CREATE,
        TRANS_DELETE,
        TRANS_SYNC,
        TRANS_REGISTER_CALLBACK,
        TRANS_UNREGISTER_CALLBACK,
        TRANS_SET_SYNC_PARAM,
        TRANS_GET_SYNC_PARAM,
        TRANS_ENABLE_CAP,
        TRANS_DISABLE_CAP,
        TRANS_SET_CAP,
        TRANS_ADD_SUB,
        TRANS_RMV_SUB,
        TRANS_SUB,
        TRANS_UNSUB,
        TRANS_BUTT,
    };
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_SERVICE_H
