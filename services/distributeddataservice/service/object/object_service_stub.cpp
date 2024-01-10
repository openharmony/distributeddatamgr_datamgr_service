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

#define LOG_TAG "ObjectServiceStub"

#include "object_service_stub.h"

#include <ipc_skeleton.h>

#include "common_types.h"
#include "object_types_utils.h"
#include "log_print.h"
#include "utils/anonymous.h"

namespace OHOS::DistributedObject {
using namespace DistributedKv;
int32_t ObjectServiceStub::ObjectStoreSaveOnRemote(MessageParcel &data, MessageParcel &reply)
{
    std::string sessionId;
    std::string bundleName;
    std::string deviceId;
    std::map<std::string, std::vector<uint8_t>> objectData;
    sptr<IRemoteObject> obj;
    if (!ITypesUtil::Unmarshal(data, bundleName, sessionId, deviceId, objectData, obj) || obj == nullptr) {
        ZLOGE("Unmarshal sessionId:%{public}s bundleName:%{public}s deviceId:%{public}s objectData size:%{public}zu, "
              "callback is nullptr:%{public}d",
            DistributedData::Anonymous::Change(sessionId).c_str(), bundleName.c_str(),
            DistributedData::Anonymous::Change(deviceId).c_str(), objectData.size(), obj == nullptr);
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = ObjectStoreSave(bundleName, sessionId, deviceId, objectData, obj);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return 0;
}

int32_t ObjectServiceStub::OnAssetChangedOnRemote(MessageParcel &data, MessageParcel &reply)
{
    std::string sessionId;
    std::string bundleName;
    std::string deviceId;
    if (!ITypesUtil::Unmarshal(data, bundleName, sessionId, deviceId)) {
        ZLOGE("Unmarshal fail sessionId:%{public}s bundleName:%{public}s deviceId:%{public}s",
            DistributedData::Anonymous::Change(sessionId).c_str(), bundleName.c_str(),
            DistributedData::Anonymous::Change(deviceId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    ObjectStore::Asset assetValue;
    if (!ITypesUtil::Unmarshalling(assetValue, data)) {
        ZLOGE("Unmarshal assetValue fail, asset name: %{public}s", assetValue.name.c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = OnAssetChanged(bundleName, sessionId, deviceId, assetValue);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return status;
}

int32_t ObjectServiceStub::ObjectStoreBindAssetOnRemote(MessageParcel &data, MessageParcel &reply)
{
    std::string sessionId;
    std::string bundleName;
    ObjectStore::Asset asset;
    ObjectStore::AssetBindInfo bindInfo;
    if (!ITypesUtil::Unmarshal(data, bundleName, sessionId, asset, bindInfo)) {
        ZLOGE("Unmarshal sessionId:%{public}s bundleName:%{public}s assetName:%{public}s bindStore:%{public}s",
              DistributedData::Anonymous::Change(sessionId).c_str(), bundleName.c_str(),
              asset.name.c_str(), bindInfo.storeName.c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }

    int32_t status = BindAssetStore(bundleName, sessionId, asset, bindInfo);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return 0;
}

int32_t ObjectServiceStub::ObjectStoreRevokeSaveOnRemote(MessageParcel &data, MessageParcel &reply)
{
    std::string sessionId;
    std::string bundleName;
    sptr<IRemoteObject> obj;
    if (!ITypesUtil::Unmarshal(data, bundleName, sessionId, obj) || obj == nullptr) {
        ZLOGE("Unmarshal sessionId:%{public}s bundleName:%{public}s, callback is nullptr: %{public}d",
            DistributedData::Anonymous::Change(sessionId).c_str(), bundleName.c_str(), obj == nullptr);
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = ObjectStoreRevokeSave(bundleName, sessionId, obj);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return 0;
}

int32_t ObjectServiceStub::ObjectStoreRetrieveOnRemote(MessageParcel &data, MessageParcel &reply)
{
    std::string sessionId;
    std::string bundleName;
    sptr<IRemoteObject> obj;
    if (!ITypesUtil::Unmarshal(data, bundleName, sessionId, obj) || obj == nullptr) {
        ZLOGE("Unmarshal sessionId:%{public}s bundleName:%{public}s, callback is nullptr: %{public}d",
            DistributedData::Anonymous::Change(sessionId).c_str(), bundleName.c_str(), obj == nullptr);
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = ObjectStoreRetrieve(bundleName, sessionId, obj);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return 0;
}

int32_t ObjectServiceStub::OnSubscribeRequest(MessageParcel &data, MessageParcel &reply)
{
    std::string sessionId;
    std::string bundleName;
    sptr<IRemoteObject> obj;
    if (!ITypesUtil::Unmarshal(data, bundleName, sessionId, obj) || obj == nullptr) {
        ZLOGE("Unmarshal sessionId:%{public}s bundleName:%{public}s, callback is nullptr: %{public}d",
            DistributedData::Anonymous::Change(sessionId).c_str(), bundleName.c_str(), obj == nullptr);
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = RegisterDataObserver(bundleName, sessionId, obj);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return 0;
}

int32_t ObjectServiceStub::OnUnsubscribeRequest(MessageParcel &data, MessageParcel &reply)
{
    std::string sessionId;
    std::string bundleName;
    if (!ITypesUtil::Unmarshal(data, bundleName, sessionId)) {
        ZLOGE("Unmarshal sessionId:%{public}s bundleName:%{public}s",
            DistributedData::Anonymous::Change(sessionId).c_str(), bundleName.c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = UnregisterDataChangeObserver(bundleName, sessionId);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return 0;
}

int32_t ObjectServiceStub::OnDeleteSnapshot(MessageParcel &data, MessageParcel &reply)
{
    std::string sessionId;
    std::string bundleName;
    if (!ITypesUtil::Unmarshal(data, bundleName, sessionId)) {
        ZLOGE("Unmarshal sessionId:%{public}s bundleName:%{public}s",
              DistributedData::Anonymous::Change(sessionId).c_str(), bundleName.c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = DeleteSnapshot(bundleName, sessionId);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return 0;
}

bool ObjectServiceStub::CheckInterfaceToken(MessageParcel& data)
{
    auto localDescriptor = IObjectService::GetDescriptor();
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (remoteDescriptor != localDescriptor) {
        ZLOGE("interface token is not equal");
        return false;
    }
    return true;
}

int ObjectServiceStub::OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply)
{
    ZLOGD("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    if (!CheckInterfaceToken(data)) {
        return -1;
    }
    if (code >= 0 && code < static_cast<uint32_t>(ObjectCode::OBJECTSTORE_SERVICE_CMD_MAX)) {
        return (this->*HANDLERS[code])(data, reply);
    }
    return -1;
}
} // namespace OHOS::DistributedRdb
