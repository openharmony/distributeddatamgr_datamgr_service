/*
* Copyright (c) 2023 Huawei Device Co., Ltd.
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

#define LOG_TAG "KvStoreDataServiceStub"

#include "kvstore_data_service_stub.h"
#include <ipc_skeleton.h>
#include "itypes_util.h"
#include "message_parcel.h"
#include "types.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
constexpr KvStoreDataServiceStub::RequestHandler
    KvStoreDataServiceStub::HANDLERS[static_cast<uint32_t>(KvStoreDataServiceInterfaceCode::SERVICE_CMD_LAST)];

int32_t KvStoreDataServiceStub::RegisterClientDeathObserverOnRemote(MessageParcel &data, MessageParcel &reply)
{
    AppId appId = { data.ReadString() };
    sptr<IRemoteObject> kvStoreClientDeathObserverProxy = data.ReadRemoteObject();
    if (kvStoreClientDeathObserverProxy == nullptr) {
        return -1;
    }
    Status status = RegisterClientDeathObserver(appId, std::move(kvStoreClientDeathObserverProxy));
    if (!reply.WriteInt32(static_cast<int>(status))) {
        return -1;
    }
    return 0;
}

int32_t KvStoreDataServiceStub::GetFeatureInterfaceOnRemote(MessageParcel &data, MessageParcel &reply)
{
    std::string name;
    if (!ITypesUtil::Unmarshal(data, name)) {
        return -1;
    }
    auto remoteObject = GetFeatureInterface(name);
    if (!ITypesUtil::Marshal(reply, remoteObject)) {
        return -1;
    }
    return 0;
}

int32_t KvStoreDataServiceStub::ClearAppStorageOnRemote(MessageParcel &data, MessageParcel &reply)
{
    std::string bundleName;
    int32_t userId;
    int32_t appIndex;
    int32_t tokenId;
    if (!ITypesUtil::Unmarshal(data, bundleName, userId, appIndex, tokenId)) {
        return -1;
    }
    auto remoteObject = ClearAppStorage(bundleName, userId, appIndex, tokenId);
    if (!ITypesUtil::Marshal(reply, remoteObject)) {
        return -1;
    }
    return 0;
}

int32_t KvStoreDataServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    ZLOGD("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    std::u16string descriptor = KvStoreDataServiceStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        ZLOGE("local descriptor is not equal to remote");
        return -1;
    }
    if (code >= 0 && code < static_cast<uint32_t>(KvStoreDataServiceInterfaceCode::SERVICE_CMD_LAST)) {
        return (this->*HANDLERS[code])(data, reply);
    } else {
        MessageOption mo{ MessageOption::TF_SYNC };
        return IPCObjectStub::OnRemoteRequest(code, data, reply, mo);
    }
}
}  // namespace DistributedKv
}  // namespace OHOS
