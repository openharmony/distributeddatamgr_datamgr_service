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

#include "ipc_skeleton.h"
#include "kv_types_util.h"
#include "log_print.h"
#include "utils/anonymous.h"
#include "utils/constant.h"
namespace OHOS::DistributedKv {
using namespace OHOS::DistributedData;
const KVDBServiceStub::Handler
    KVDBServiceStub::HANDLERS[static_cast<uint32_t>(KVDBServiceInterfaceCode::TRANS_BUTT)] = {
    &KVDBServiceStub::OnGetStoreIds,
    &KVDBServiceStub::OnBeforeCreate,
    &KVDBServiceStub::OnAfterCreate,
    &KVDBServiceStub::OnDelete,
    &KVDBServiceStub::OnSync,
    &KVDBServiceStub::OnRegServiceNotifier,
    &KVDBServiceStub::OnUnregServiceNotifier,
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
    &KVDBServiceStub::OnCloudSync,
    &KVDBServiceStub::OnNotifyDataChange,
    &KVDBServiceStub::OnSetConfig,
    &KVDBServiceStub::OnPutSwitch,
    &KVDBServiceStub::OnGetSwitch,
    &KVDBServiceStub::OnSubscribeSwitchData,
    &KVDBServiceStub::OnUnsubscribeSwitchData,
    &KVDBServiceStub::OnClose,
    &KVDBServiceStub::OnRemoveDeviceData,
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

    if (static_cast<uint32_t>(KVDBServiceInterfaceCode::TRANS_HEAD) > code ||
        code >= static_cast<uint32_t>(KVDBServiceInterfaceCode::TRANS_BUTT) || HANDLERS[code] == nullptr) {
        ZLOGE("not support code:%{public}u, BUTT:%{public}d", code,
              static_cast<uint32_t>(KVDBServiceInterfaceCode::TRANS_BUTT));
        return -1;
    }
    auto [status, storeInfo] = GetStoreInfo(code, data);
    if (status != ERR_NONE) {
        return status;
    }
    if (CheckPermission(code, storeInfo)) {
        return (this->*HANDLERS[code])({ storeInfo.bundleName }, { storeInfo.storeId }, data, reply);
    }
    ZLOGE("PERMISSION_DENIED uid:%{public}d appId:%{public}s storeId:%{public}s", storeInfo.uid,
        storeInfo.bundleName.c_str(), Anonymous::Change(storeInfo.storeId).c_str());

    if (!ITypesUtil::Marshal(reply, static_cast<int32_t>(PERMISSION_DENIED))) {
        ZLOGE("Marshal PERMISSION_DENIED code:%{public}u appId:%{public}s storeId:%{public}s", code,
            storeInfo.bundleName.c_str(), Anonymous::Change(storeInfo.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

std::pair<int32_t, KVDBServiceStub::StoreInfo> KVDBServiceStub::GetStoreInfo(uint32_t code, MessageParcel &data)
{
    AppId appId;
    StoreId storeId;
    if (!ITypesUtil::Unmarshal(data, appId, storeId)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s",
            appId.appId.c_str(), Anonymous::Change(storeId.storeId).c_str());
        return { IPC_STUB_INVALID_DATA_ERR, StoreInfo() };
    }
    appId.appId = Constant::TrimCopy(appId.appId);
    storeId.storeId = Constant::TrimCopy(storeId.storeId);
    StoreInfo info;
    info.uid = IPCSkeleton::GetCallingUid();
    info.tokenId = IPCSkeleton::GetCallingTokenID();
    info.bundleName = std::move(appId.appId);
    info.storeId = std::move(storeId.storeId);
    return { ERR_NONE, info };
}

bool KVDBServiceStub::CheckPermission(uint32_t code, const StoreInfo &storeInfo)
{
    if (code >= static_cast<uint32_t>(KVDBServiceInterfaceCode::TRANS_PUT_SWITCH) &&
        code <= static_cast<uint32_t>(KVDBServiceInterfaceCode::TRANS_UNSUBSCRIBE_SWITCH_DATA)) {
        return CheckerManager::GetInstance().IsSwitches(storeInfo);
    }
    return CheckerManager::GetInstance().IsValid(storeInfo);
}

int32_t KVDBServiceStub::OnGetStoreIds(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    int32_t subUser;
    if (!ITypesUtil::Unmarshal(data, subUser)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    std::vector<StoreId> storeIds;
    int32_t status = GetStoreIds(appId, subUser, storeIds);
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
    int32_t subUser;
    if (!ITypesUtil::Unmarshal(data, subUser)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = Delete(appId, storeId, subUser);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnClose(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    int32_t subUser;
    if (!ITypesUtil::Unmarshal(data, subUser)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = Close(appId, storeId, subUser);
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
    int32_t subUser;
    if (!ITypesUtil::Unmarshal(data, syncInfo.seqId, syncInfo.mode, syncInfo.devices, syncInfo.delay, syncInfo.query,
        subUser)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = Sync(appId, storeId, subUser, syncInfo);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnCloudSync(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    SyncInfo syncInfo;
    if (!ITypesUtil::Unmarshal(data, syncInfo.seqId)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
              Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = CloudSync(appId, storeId, syncInfo);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnNotifyDataChange(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    uint64_t delay = 0;
    if (!ITypesUtil::Unmarshal(data, delay)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = NotifyDataChange(appId, storeId, delay);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s",
            status, appId.appId.c_str(), Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnRegServiceNotifier(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> remoteObj;
    if (!ITypesUtil::Unmarshal(data, remoteObj)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto notifier = (remoteObj == nullptr) ? nullptr : iface_cast<IKVDBNotifier>(remoteObj);
    int32_t status = RegServiceNotifier(appId, notifier);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnUnregServiceNotifier(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    int32_t status = UnregServiceNotifier(appId);
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
    int32_t subUser;
    if (!ITypesUtil::Unmarshal(data, remoteObj, subUser)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto observer = (remoteObj == nullptr) ? nullptr : iface_cast<IKvStoreObserver>(remoteObj);
    int32_t status = Subscribe(appId, storeId, subUser, observer);
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
    int32_t subUser;
    if (!ITypesUtil::Unmarshal(data, remoteObj, subUser)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto observer = (remoteObj == nullptr) ? nullptr : iface_cast<IKvStoreObserver>(remoteObj);
    int32_t status = Unsubscribe(appId, storeId, subUser, observer);
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
    int32_t passwordType;
    if (!ITypesUtil::Unmarshal(data, passwordType)) {
        ZLOGE("Unmarshal type failed, appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    std::vector<std::vector<uint8_t>> passwords;
    int32_t status = GetBackupPassword(appId, storeId, passwords, passwordType);
    if (!ITypesUtil::Marshal(reply, status, passwords)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s storeId:%{public}s", status, appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        for (auto &password : passwords) {
            password.assign(password.size(), 0);
        }
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    for (auto &password : passwords) {
        password.assign(password.size(), 0);
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnPutSwitch(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    SwitchData switchData;
    if (!ITypesUtil::Unmarshal(data, switchData)) {
        ZLOGE("Unmarshal appId:%{public}s", appId.appId.c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = PutSwitch(appId, switchData);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s", status, appId.appId.c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnGetSwitch(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    std::string networkId;
    if (!ITypesUtil::Unmarshal(data, networkId)) {
        ZLOGE("Unmarshal appId:%{public}s networkId:%{public}s",
            appId.appId.c_str(), Anonymous::Change(networkId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    SwitchData switchData;
    int32_t status = GetSwitch(appId, networkId, switchData);
    if (!ITypesUtil::Marshal(reply, status, switchData)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s networkId:%{public}s",
            status, appId.appId.c_str(), Anonymous::Change(networkId).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnSubscribeSwitchData(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    int32_t status = SubscribeSwitchData(appId);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s", status, appId.appId.c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnUnsubscribeSwitchData(
    const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply)
{
    int32_t status = UnsubscribeSwitchData(appId);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s", status, appId.appId.c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnSetConfig(const AppId &appId, const StoreId &storeId, MessageParcel &data,
    MessageParcel &reply)
{
    StoreConfig storeConfig;
    if (!ITypesUtil::Unmarshal(data, storeConfig)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = SetConfig(appId, storeId, storeConfig);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s", status, appId.appId.c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}

int32_t KVDBServiceStub::OnRemoveDeviceData(const AppId &appId, const StoreId &storeId, MessageParcel &data,
    MessageParcel &reply)
{
    std::string device;
    int32_t subUser;
    if (!ITypesUtil::Unmarshal(data, device, subUser)) {
        ZLOGE("Unmarshal appId:%{public}s storeId:%{public}s", appId.appId.c_str(),
            Anonymous::Change(storeId.storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = RemoveDeviceData(appId, storeId, subUser, device);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x appId:%{public}s", status, appId.appId.c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return ERR_NONE;
}
} // namespace OHOS::DistributedKv
