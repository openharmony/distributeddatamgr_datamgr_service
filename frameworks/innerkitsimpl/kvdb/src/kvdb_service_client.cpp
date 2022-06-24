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
#define LOG_TAG "KVDBServiceClient"
#include "kvdb_service_client.h"

#include <cinttypes>

#include "itypes_util.h"
#include "kvstore_observer_client.h"
#include "kvstore_service_death_notifier.h"
#include "log_print.h"
#include "security_manager.h"
#include "single_store_impl.h"
#include "store_factory.h"
#include "store_util.h"
namespace OHOS::DistributedKv {
#define IPC_SEND(code, reply, ...)                                              \
    ({                                                                          \
        int32_t __status = SUCCESS;                                             \
        do {                                                                    \
            MessageParcel request;                                              \
            if (!request.WriteInterfaceToken(GetDescriptor())) {                \
                __status = IPC_PARCEL_ERROR;                                    \
                break;                                                          \
            }                                                                   \
            if (!ITypesUtil::Marshal(request, ##__VA_ARGS__)) {                 \
                __status = IPC_PARCEL_ERROR;                                    \
                break;                                                          \
            }                                                                   \
            MessageOption option;                                               \
            auto result = remote_->SendRequest((code), request, reply, option); \
            if (result != 0) {                                                  \
                __status = IPC_ERROR;                                           \
                break;                                                          \
            }                                                                   \
                                                                                \
            ITypesUtil::Unmarshal(reply, __status);                             \
        } while (0);                                                            \
        __status;                                                               \
    })

std::mutex KVDBServiceClient::mutex_;
std::shared_ptr<KVDBServiceClient> KVDBServiceClient::instance_;
std::atomic_bool KVDBServiceClient::isWatched_(false);

std::shared_ptr<KVDBServiceClient> KVDBServiceClient::GetInstance()
{
    if (!isWatched_.exchange(true)) {
        KvStoreServiceDeathNotifier::AddServiceDeathWatcher(std::make_shared<ServiceDeath>());
    }

    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    if (instance_ != nullptr) {
        return instance_;
    }

    sptr<IKvStoreDataService> ability = KvStoreServiceDeathNotifier::GetDistributedKvDataService();
    if (ability == nullptr) {
        return nullptr;
    }

    sptr<IRemoteObject> service = ability->GetKVdbService();
    if (service == nullptr) {
        return nullptr;
    }

    sptr<KVDBServiceClient> client = new (std::nothrow) KVDBServiceClient(service);
    if (client == nullptr) {
        return nullptr;
    }

    instance_.reset(client.GetRefPtr(), [client](auto *) mutable { client = nullptr; });
    return instance_;
}

void KVDBServiceClient::ServiceDeath::OnRemoteDied()
{
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    instance_ = nullptr;
}

KVDBServiceClient::KVDBServiceClient(const sptr<IRemoteObject> &handle) : IRemoteProxy(handle)
{
    remote_ = Remote();
}

Status KVDBServiceClient::GetStoreIds(const AppId &appId, std::vector<StoreId> &storeIds)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_GET_STORE_IDS, reply, appId, StoreId(), storeIds);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, appId:%{public}s", status, appId.appId.c_str());
    }
    ITypesUtil::Unmarshal(reply, storeIds);
    return static_cast<Status>(status);
}

Status KVDBServiceClient::BeforeCreate(const AppId &appId, const StoreId &storeId, const Options &options)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_BEFORE_CREATE, reply, appId, storeId, options);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x appId:%{public}s, storeId:%{public}s", status, appId.appId.c_str(),
            storeId.storeId.c_str());
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::AfterCreate(
    const AppId &appId, const StoreId &storeId, const Options &options, const std::vector<uint8_t> &password)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_AFTER_CREATE, reply, appId, storeId, options, password);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x appId:%{public}s, storeId:%{public}s, encrypt:%{public}d", status,
            appId.appId.c_str(), storeId.storeId.c_str(), options.encrypt);
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::Delete(const AppId &appId, const StoreId &storeId)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_DELETE, reply, appId, storeId);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x appId:%{public}s, storeId:%{public}s", status, appId.appId.c_str(),
            storeId.storeId.c_str());
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::Sync(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_SYNC, reply, appId, storeId, syncInfo.seqId, syncInfo.mode, syncInfo.devices,
        syncInfo.delay, syncInfo.query);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, appId:%{public}s, storeId:%{public}s, sequenceId:%{public}" PRIu64, status,
            appId.appId.c_str(), storeId.storeId.c_str(), syncInfo.seqId);
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::RegisterSyncCallback(const AppId &appId, sptr<IKvStoreSyncCallback> callback)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_REGISTER_CALLBACK, reply, appId, StoreId(), callback->AsObject().GetRefPtr());
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, appId:%{public}s, callback:0x%{public}x", status, appId.appId.c_str(),
            StoreUtil::Anonymous(callback.GetRefPtr()));
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::UnregisterSyncCallback(const AppId &appId)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_UNREGISTER_CALLBACK, reply, appId, StoreId());
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, appId:%{public}s", status, appId.appId.c_str());
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::SetSyncParam(const AppId &appId, const StoreId &storeId, const KvSyncParam &syncParam)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_SET_SYNC_PARAM, reply, appId, storeId, syncParam.allowedDelayMs);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, appId:%{public}s, storeId:%{public}s", status, appId.appId.c_str(),
            storeId.storeId.c_str());
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::GetSyncParam(const AppId &appId, const StoreId &storeId, KvSyncParam &syncParam)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_GET_SYNC_PARAM, reply, appId, storeId);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, appId:%{public}s, storeId:%{public}s", status, appId.appId.c_str(),
            storeId.storeId.c_str());
        return SUCCESS;
    }
    ITypesUtil::Unmarshal(reply, syncParam.allowedDelayMs);
    return static_cast<Status>(status);
}

Status KVDBServiceClient::EnableCapability(const AppId &appId, const StoreId &storeId)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_ENABLE_CAP, reply, appId, storeId);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, appId:%{public}s, storeId:%{public}s", status, appId.appId.c_str(),
            storeId.storeId.c_str());
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::DisableCapability(const AppId &appId, const StoreId &storeId)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_DISABLE_CAP, reply, appId, storeId);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, appId:%{public}s, storeId:%{public}s", status, appId.appId.c_str(),
            storeId.storeId.c_str());
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::SetCapability(const AppId &appId, const StoreId &storeId,
    const std::vector<std::string> &local, const std::vector<std::string> &remote)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_SET_CAP, reply, appId, storeId, local, remote);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, appId:%{public}s, storeId:%{public}s", status, appId.appId.c_str(),
            storeId.storeId.c_str());
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::AddSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_ADD_SUB, reply, appId, storeId, syncInfo.seqId, syncInfo.devices, syncInfo.query);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, appId:%{public}s, storeId:%{public}s, query:%{public}s", status,
            appId.appId.c_str(), storeId.storeId.c_str(), StoreUtil::Anonymous(syncInfo.query).c_str());
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::RmvSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_RMV_SUB, reply, appId, storeId, syncInfo.seqId, syncInfo.devices, syncInfo.query);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, appId:%{public}s, storeId:%{public}s, query:%{public}s", status,
            appId.appId.c_str(), storeId.storeId.c_str(), StoreUtil::Anonymous(syncInfo.query).c_str());
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::Subscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_SUB, reply, appId, storeId, observer->AsObject().GetRefPtr());
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, appId:%{public}s, storeId:%{public}s, observer:0x%{public}x", status,
            appId.appId.c_str(), storeId.storeId.c_str(), StoreUtil::Anonymous(observer.GetRefPtr()));
    }
    return static_cast<Status>(status);
}

Status KVDBServiceClient::Unsubscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer)
{
    MessageParcel reply;
    int32_t status = IPC_SEND(TRANS_UNSUB, reply, appId, storeId, observer->AsObject().GetRefPtr());
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, appId:%{public}s, storeId:%{public}s, observer:0x%{public}x", status,
            appId.appId.c_str(), storeId.storeId.c_str(), StoreUtil::Anonymous(observer.GetRefPtr()));
    }
    return static_cast<Status>(status);
}

sptr<KvStoreSyncCallbackClient> KVDBServiceClient::GetSyncAgent(const AppId &appId)
{
    std::lock_guard<decltype(agentMtx_)> lockGuard(agentMtx_);
    if (syncAgent_ != nullptr) {
        return syncAgent_;
    }

    sptr<KvStoreSyncCallbackClient> syncAgent = new (std::nothrow) KvStoreSyncCallbackClient();
    auto status = RegisterSyncCallback(appId, syncAgent);
    if (status == SUCCESS) {
        syncAgent_ = std::move(syncAgent);
    }
    return syncAgent_;
}
} // namespace OHOS::DistributedKv