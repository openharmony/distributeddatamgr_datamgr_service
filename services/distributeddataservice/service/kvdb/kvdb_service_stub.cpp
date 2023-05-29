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
#define LOG_TAG "KVDBServiceStub"
#include "kvdb_service_stub.h"

#include "checker/checker_manager.h"
#include "ipc_skeleton.h"
#include "itypes_util.h"
#include "log_print.h"
#include "utils/anonymous.h"
#include "utils/constant.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedData;
const KVDBServiceStub::Handler KVDBServiceStub::HANDLERS[TRANS_BUTT] = {
    &KVDBServiceStub::OnGetStoreIds,
    &KVDBServiceStub::OnBeforeCreate,
    &KVDBServiceStub::OnAfterCreate,
    &KVDBServiceStub::OnDelete,
    &KVDBServiceStub::OnSync,
    &KVDBServiceStub::OnRegisterCallback,
    &KVDBServiceStub::OnUnregisterCallback,
    &KVDBServiceStub::OnSetSyncParam,
    &KVDBServiceStub::OnGetSyncParam,
    &KVDBServiceStub::OnEnableCap,
    &KVDBServiceStub::OnDisableCap,
    &KVDBServiceStub::OnSetCapability,
    &KVDBServiceStub::OnAddSubInfo,
    &KVDBServiceStub::OnRmvSubInfo,
    &KVDBServiceStub::OnSubscribe,
    &KVDBServiceStub::OnUnsubscribe,
    &KVDBServiceStub::OnGetBackupPassword,
};

int KVDBServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    ZLOGI("code:%{public}u callingPid:%{public}u", code, IPCSkeleton::GetCallingPid());
    std::u16string local = KVDBServiceStub::GetDescriptor();
    std::u16string remote = data.ReadInterfaceToken();
    if (local != remote) {
        ZLOGE("local is not equal to remote");
        return -1;
    }

    if (TRANS_HEAD > code || code >= TRANS_BUTT || HANDLERS[code] == nullptr) {
        ZLOGE("not support code:%{public}u, BUTT:%{public}d", code, TRANS_BUTT);
        return -1;
    }

    AppId appId;
    StoreId storeId;
    if (!ITypesUtil::Unmarshal(data, appId, storeId)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    appId.appId = Constant::TrimCopy(appId.appId);
    storeId.storeId = Constant::TrimCopy(storeId.storeId);

    CheckerManager::StoreInfo info;
    info.uid = IPCSkeleton::GetCallingUid();
    info.tokenId = IPCSkeleton::GetCallingTokenID();
    info.bundleName = appId.appId;
    info.storeId = storeId.storeId;
    if (CheckerManager::GetInstance().IsValid(info)) {
        return (this->*HANDLERS[code])(appId, storeId, data, reply);
    }
    ZLOGE("PERMISSION_DENIED uid:%{public}d appId:%{public}s storeId:%{public}s", info.uid, info.bundleName.c_str(),
        Anonymous::Change(info.storeId).c_str());

    if (!ITypesUtil::Marshal(reply, PERMISSION_DENIED)) {
        ZLOGE("Marshal PERMISSION_DENIED code:%{public}u appId:%{public}s storeId:%{public}s", code,
            appId.appId.c_str(), Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnGetStoreIds(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    std::vector<StoreId> storeIds;
    int32_t status = GetStoreIds(appId, storeIds);
    if (!ITypesUtil::Marshal(reply, status, storeIds)) {
        ZLOGE("Marshal status:0x%{public}d storeIds:%{public}zu", status, storeIds.size());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnBeforeCreate(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    Options options;
    if (!ITypesUtil::Unmarshal(data, options)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = BeforeCreate(appId, storeId, options);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnAfterCreate(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    Options options;
    std::vector<uint8_t> password;
    if (!ITypesUtil::Unmarshal(data, options, password)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = AfterCreate(appId, storeId, options, password);
    password.assign(password.size(), 0);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnDelete(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    int32_t status = Delete(appId, storeId);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnSync(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    SyncInfo syncInfo;
    if (!ITypesUtil::Unmarshal(data, syncInfo.seqId, syncInfo.mode, syncInfo.devices, syncInfo.delay, syncInfo.query)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = Sync(appId, storeId, syncInfo);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnRegisterCallback(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> remoteObj;
    if (!ITypesUtil::Unmarshal(data, remoteObj)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto syncCallback = (remoteObj == nullptr) ? nullptr : iface_cast<IKvStoreSyncCallback>(remoteObj);
    int32_t status = RegisterSyncCallback(appId, syncCallback);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnUnregisterCallback(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    int32_t status = UnregisterSyncCallback(appId);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnSetSyncParam(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    KvSyncParam syncParam;
    if (!ITypesUtil::Unmarshal(data, syncParam.allowedDelayMs)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = SetSyncParam(appId, storeId, syncParam);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnGetSyncParam(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    KvSyncParam syncParam;
    int32_t status = GetSyncParam(appId, storeId, syncParam);
    if (!ITypesUtil::Marshal(reply, status, syncParam.allowedDelayMs)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnEnableCap(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    int32_t status = EnableCapability(appId, storeId);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnDisableCap(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    int32_t status = DisableCapability(appId, storeId);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnSetCapability(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    std::vector<std::string> local;
    std::vector<std::string> remote;
    if (!ITypesUtil::Unmarshal(data, local, remote)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = SetCapability(appId, storeId, local, remote);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnAddSubInfo(const AppId &appId, const StoreId &storeId, MessageParcel &data,
    MessageParcel &reply)
{
    SyncInfo syncInfo;
    if (!ITypesUtil::Unmarshal(data, syncInfo.seqId, syncInfo.devices, syncInfo.query)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = AddSubscribeInfo(appId, storeId, syncInfo);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnRmvSubInfo(const AppId &appId, const StoreId &storeId, MessageParcel &data,
    MessageParcel &reply)
{
    SyncInfo syncInfo;
    if (!ITypesUtil::Unmarshal(data, syncInfo.seqId, syncInfo.devices, syncInfo.query)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = RmvSubscribeInfo(appId, storeId, syncInfo);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnSubscribe(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> remoteObj;
    if (!ITypesUtil::Unmarshal(data, remoteObj)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto observer = (remoteObj == nullptr) ? nullptr : iface_cast<IKvStoreObserver>(remoteObj);
    int32_t status = Subscribe(appId, storeId, observer);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnUnsubscribe(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> remoteObj;
    if (!ITypesUtil::Unmarshal(data, remoteObj)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto observer = (remoteObj == nullptr) ? nullptr : iface_cast<IKvStoreObserver>(remoteObj);
    int32_t status = Unsubscribe(appId, storeId, observer);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnGetBackupPassword(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    std::vector<uint8_t> password;
    int32_t status = GetBackupPassword(appId, storeId, password);
    if (!ITypesUtil::Marshal(reply, status, password)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        password.assign(password.size(), 0);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    password.assign(password.size(), 0);
    return ERR_NONE;
}
} // namespace OHOS::DistributedKv
