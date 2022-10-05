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

#include "itypes_util.h"
#include "log_print.h"

namespace OHOS::DistributedObject {
using namespace DistributedKv;
int32_t ObjectServiceStub::ObjectStoreSaveOnRemote(MessageParcel &data, MessageParcel &reply)
{
    std::string sessionId;
    std::string bundleName;
    std::string deviceId;
    std::map<std::string, std::vector<uint8_t>> objectData;
    sptr<IRemoteObject> obj;
    if (!ITypesUtil::Unmarshal(data, bundleName, sessionId, deviceId, objectData, obj)) {
        ZLOGW("read device list failed.");
        return -1;
    }
    if (obj == nullptr) {
        ZLOGW("callback null");
        return -1;
    }
    sptr<IObjectSaveCallback> callback = iface_cast<IObjectSaveCallback>(obj);
    int32_t status = ObjectStoreSave(bundleName, sessionId, deviceId, objectData, callback);
    if (!reply.WriteInt32(static_cast<int>(status))) {
        ZLOGE("ObjectStoreSaveOnRemote fail %d", static_cast<int>(status));
        return -1;
    }
    return 0;
}

int32_t ObjectServiceStub::ObjectStoreRevokeSaveOnRemote(MessageParcel &data, MessageParcel &reply)
{
    std::string sessionId;
    std::string bundleName;
    sptr<IRemoteObject> obj;
    if (!ITypesUtil::Unmarshal(data, bundleName, sessionId, obj)) {
        ZLOGW("read device list failed.");
        return -1;
    }
    if (obj == nullptr) {
        ZLOGW("callback null");
        return -1;
    }
    sptr<IObjectRevokeSaveCallback> callback = iface_cast<IObjectRevokeSaveCallback>(obj);
    int32_t status = ObjectStoreRevokeSave(bundleName, sessionId, callback);
    if (!reply.WriteInt32(static_cast<int>(status))) {
        ZLOGE("ObjectStoreRevokeSaveOnRemote fail %d", static_cast<int>(status));
        return -1;
    }
    return 0;
}

int32_t ObjectServiceStub::ObjectStoreRetrieveOnRemote(MessageParcel &data, MessageParcel &reply)
{
    std::string sessionId;
    std::string bundleName;
    sptr<IRemoteObject> obj;
    if (!ITypesUtil::Unmarshal(data, bundleName, sessionId, obj)) {
        ZLOGW("read device list failed.");
        return -1;
    }
    if (obj == nullptr) {
        ZLOGW("callback null");
        return -1;
    }
    sptr<IObjectRetrieveCallback> callback = iface_cast<IObjectRetrieveCallback>(obj);
    int32_t status = ObjectStoreRetrieve(bundleName, sessionId, callback);
    if (!reply.WriteInt32(static_cast<int>(status))) {
        ZLOGE("ObjectStoreRetrieveOnRemote fail %d", static_cast<int>(status));
        return -1;
    }
    return 0;
}

int32_t ObjectServiceStub::OnSubscribeRequest(MessageParcel &data, MessageParcel &reply)
{
    std::string sessionId;
    std::string bundleName;
    sptr<IRemoteObject> obj;
    if (!ITypesUtil::Unmarshal(data, bundleName, sessionId, obj)) {
        ZLOGW("read device list failed.");
        return -1;
    }
    if (obj == nullptr) {
        ZLOGW("callback null");
        return -1;
    }
    sptr<IObjectChangeCallback> callback = iface_cast<IObjectChangeCallback>(obj);
    int32_t status = RegisterDataObserver(bundleName, sessionId, callback);
    if (!reply.WriteInt32(static_cast<int>(status))) {
        ZLOGE("OnSubscribeRequest fail %d", static_cast<int>(status));
        return -1;
    }
    return 0;
}

int32_t ObjectServiceStub::OnUnsubscribeRequest(MessageParcel &data, MessageParcel &reply)
{
    std::string sessionId;
    std::string bundleName;
    if (!ITypesUtil::Unmarshal(data, bundleName, sessionId)) {
        ZLOGW("read device list failed.");
        return -1;
    }
    int32_t status = UnregisterDataChangeObserver(bundleName, sessionId);
    if (!reply.WriteInt32(static_cast<int>(status))) {
        ZLOGE("OnSubscribeRequest fail %d", static_cast<int>(status));
        return -1;
    }
    return 0;
}

bool ObjectServiceStub::CheckInterfaceToken(MessageParcel& data)
{
    auto localDescriptor = ObjectService::GetDescriptor();
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
    if (code >= 0 && code < OBJECTSTORE_SERVICE_CMD_MAX) {
        return (this->*HANDLERS[code])(data, reply);
    }
    return -1;
}
} // namespace OHOS::DistributedRdb
