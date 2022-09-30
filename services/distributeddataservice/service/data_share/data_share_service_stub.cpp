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

#define LOG_TAG "DataShareServiceStub"

#include "data_share_service_stub.h"

#include <ipc_skeleton.h>
#include "ishared_result_set.h"
#include "itypes_util.h"
#include "log_print.h"

namespace OHOS {
namespace DataShare {
bool DataShareServiceStub::CheckInterfaceToken(MessageParcel &data)
{
    auto localDescriptor = IDataShareService::GetDescriptor();
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (remoteDescriptor != localDescriptor) {
        ZLOGE("interface token is not equal");
        return false;
    }
    return true;
}

int32_t DataShareServiceStub::OnRemoteInsert(MessageParcel &data, MessageParcel &reply)
{
    std::string uri;
    DataShareValuesBucket bucket;
    if (!DistributedKv::ITypesUtil::Unmarshal(data, uri, bucket)) {
        ZLOGW("read device list failed.");
        return -1;
    }
    int32_t status = Insert(uri, bucket);
    if (!reply.WriteInt32(static_cast<int>(status))) {
        ZLOGE("OnRemoteInsert fail %d", static_cast<int>(status));
        return -1;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteUpdate(MessageParcel &data, MessageParcel &reply)
{
    std::string uri;
    DataSharePredicates predicate;
    DataShareValuesBucket bucket;
    if (!DistributedKv::ITypesUtil::Unmarshal(data, uri, predicate, bucket)) {
        ZLOGW("read device list failed.");
        return -1;
    }
    int32_t status = Update(uri, predicate, bucket);
    if (!reply.WriteInt32(static_cast<int>(status))) {
        ZLOGE("OnRemoteUpdate fail %d", static_cast<int>(status));
        return -1;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteDelete(MessageParcel &data, MessageParcel &reply)
{
    std::string uri;
    DataSharePredicates predicate;
    if (!DistributedKv::ITypesUtil::Unmarshal(data, uri, predicate)) {
        ZLOGW("read device list failed.");
        return -1;
    }
    int32_t status = Delete(uri, predicate);
    if (!reply.WriteInt32(static_cast<int>(status))) {
        ZLOGE("OnRemoteDelete fail %d", static_cast<int>(status));
        return -1;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteQuery(MessageParcel &data, MessageParcel &reply)
{
    std::string uri;
    DataSharePredicates predicate;
    std::vector<std::string> columns;
    if (!DistributedKv::ITypesUtil::Unmarshal(data, uri, predicate, columns)) {
        ZLOGW("read device list failed.");
        return -1;
    }
    auto result = ISharedResultSet::WriteToParcel(Query(uri, predicate, columns), reply);
    if (result == nullptr) {
        ZLOGW("!resultSet->Marshalling(reply)");
        return -1;
    }
    return 0;
}

int DataShareServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    ZLOGD("code:%{public}u, callingPid:%{public}d", code, IPCSkeleton::GetCallingPid());
    if (!CheckInterfaceToken(data)) {
        return DATA_SHARE_ERROR;
    }
    if (code >= 0 && code < DATA_SHARE_SERVICE_CMD_MAX) {
        return (this->*HANDLERS[code])(data, reply);
    }
    return -1;
}
} // namespace DataShare
} // namespace OHOS