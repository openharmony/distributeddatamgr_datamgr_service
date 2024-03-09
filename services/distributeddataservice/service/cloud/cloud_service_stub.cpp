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
#define LOG_TAG "CloudServiceStub"
#include "cloud_service_stub.h"

#include "ipc_skeleton.h"
#include "itypes_util.h"
#include "log_print.h"
#include "permission/permission_validator.h"
#include "rdb_types.h"
#include "utils/anonymous.h"
#include "tokenid_kit.h"
namespace OHOS::CloudData {
using namespace DistributedData;
using namespace OHOS::Security::AccessToken;
const CloudServiceStub::Handler CloudServiceStub::HANDLERS[TRANS_BUTT] = {
    &CloudServiceStub::OnEnableCloud,
    &CloudServiceStub::OnDisableCloud,
    &CloudServiceStub::OnChangeAppSwitch,
    &CloudServiceStub::OnClean,
    &CloudServiceStub::OnNotifyDataChange,
    &CloudServiceStub::OnNotifyChange,
    &CloudServiceStub::OnAllocResourceAndShare,
    &CloudServiceStub::OnShare,
    &CloudServiceStub::OnUnshare,
    &CloudServiceStub::OnExit,
    &CloudServiceStub::OnChangePrivilege,
    &CloudServiceStub::OnQuery,
    &CloudServiceStub::OnQueryByInvitation,
    &CloudServiceStub::OnConfirmInvitation,
    &CloudServiceStub::OnChangeConfirmation,
};

int CloudServiceStub::OnRemoteRequest(uint32_t code, OHOS::MessageParcel &data, OHOS::MessageParcel &reply)
{
    ZLOGI("code:%{public}u callingPid:%{public}u", code, IPCSkeleton::GetCallingPid());
    std::u16string local = CloudServiceStub::GetDescriptor();
    std::u16string remote = data.ReadInterfaceToken();
    if (local != remote) {
        ZLOGE("local is not equal to remote");
        return -1;
    }

    if (TRANS_HEAD > code || code >= TRANS_BUTT || HANDLERS[code] == nullptr) {
        ZLOGE("not support code:%{public}u, BUTT:%{public}d", code, TRANS_BUTT);
        return -1;
    }

    if (!TokenIdKit::IsSystemAppByFullTokenID(IPCSkeleton::GetCallingFullTokenID())) {
        ZLOGE("permission denied! code:%{public}u, BUTT:%{public}d", code, TRANS_BUTT);
        auto result = static_cast<int32_t>(PERMISSION_DENIED);
        return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
    }

    if (code >= TRANS_HEAD && code < TRANS_ALLOC_RESOURCE_AND_SHARE) {
        auto permit =
            DistributedKv::PermissionValidator::GetInstance().IsCloudConfigPermit(IPCSkeleton::GetCallingTokenID());
        if (!permit) {
            ZLOGE("cloud config permission denied! code:%{public}u, BUTT:%{public}d", code, TRANS_BUTT);
            auto result = static_cast<int32_t>(CLOUD_CONFIG_PERMISSION_DENIED);
            return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
        }
    }

    std::string id;
    if (!ITypesUtil::Unmarshal(data, id)) {
        ZLOGE("Unmarshal id:%{public}s", Anonymous::Change(id).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    return (this->*HANDLERS[code])(id, data, reply);
}

int32_t CloudServiceStub::OnEnableCloud(const std::string &id, MessageParcel &data, MessageParcel &reply)
{
    std::map<std::string, int32_t> switches;
    if (!ITypesUtil::Unmarshal(data, switches)) {
        ZLOGE("Unmarshal id:%{public}s", Anonymous::Change(id).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto result = EnableCloud(id, switches);
    return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnDisableCloud(const std::string &id, MessageParcel &data, MessageParcel &reply)
{
    auto result = DisableCloud(id);
    return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnChangeAppSwitch(const std::string &id, MessageParcel &data, MessageParcel &reply)
{
    std::string bundleName;
    int32_t appSwitch = SWITCH_OFF;
    if (!ITypesUtil::Unmarshal(data, bundleName, appSwitch)) {
        ZLOGE("Unmarshal id:%{public}s", Anonymous::Change(id).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto result = ChangeAppSwitch(id, bundleName, appSwitch);
    return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnClean(const std::string &id, MessageParcel &data, MessageParcel &reply)
{
    std::map<std::string, int32_t> actions;
    if (!ITypesUtil::Unmarshal(data, actions)) {
        ZLOGE("Unmarshal id:%{public}s", Anonymous::Change(id).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto result = Clean(id, actions);
    return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnNotifyDataChange(const std::string &id, MessageParcel &data, MessageParcel &reply)
{
    std::string bundleName;
    if (!ITypesUtil::Unmarshal(data, bundleName)) {
        ZLOGE("Unmarshal id:%{public}s", Anonymous::Change(id).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto result = NotifyDataChange(id, bundleName);
    return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnAllocResourceAndShare(const std::string& storeId, MessageParcel& data,
    MessageParcel& reply)
{
    DistributedRdb::PredicatesMemo predicatesMemo;
    std::vector<std::string> columns;
    std::vector<Participant> participants;
    if (!ITypesUtil::Unmarshal(data, predicatesMemo, columns, participants)) {
        ZLOGE("Unmarshal storeId:%{public}s", Anonymous::Change(storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto [status, resultSet] = AllocResourceAndShare(storeId, predicatesMemo, columns, participants);
    return ITypesUtil::Marshal(reply, status, resultSet) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnNotifyChange(const std::string &id, MessageParcel &data, MessageParcel &reply)
{
    std::string extraData;
    int32_t userId;
    if (!ITypesUtil::Unmarshal(data, extraData, userId)) {
        ZLOGE("Unmarshal id:%{public}s", Anonymous::Change(id).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto result = NotifyDataChange(id, extraData, userId);
    return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnShare(const std::string &sharingRes, MessageParcel &data, MessageParcel &reply)
{
    Participants participants;
    if (!ITypesUtil::Unmarshal(data, participants)) {
        ZLOGE("Unmarshal sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    Results results;
    auto status = Share(sharingRes, participants, results);
    return ITypesUtil::Marshal(reply, status, results) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnUnshare(const std::string &sharingRes, MessageParcel &data, MessageParcel &reply)
{
    Participants participants;
    if (!ITypesUtil::Unmarshal(data, participants)) {
        ZLOGE("Unmarshal sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    Results results;
    auto status = Unshare(sharingRes, participants, results);
    return ITypesUtil::Marshal(reply, status, results) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnExit(const std::string &sharingRes, MessageParcel &data, MessageParcel &reply)
{
    std::pair<int32_t, std::string> result;
    auto status = Exit(sharingRes, result);
    return ITypesUtil::Marshal(reply, status, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnChangePrivilege(const std::string &sharingRes, MessageParcel &data, MessageParcel &reply)
{
    Participants participants;
    if (!ITypesUtil::Unmarshal(data, participants)) {
        ZLOGE("Unmarshal sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    Results results;
    auto status = ChangePrivilege(sharingRes, participants, results);
    return ITypesUtil::Marshal(reply, status, results) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnQuery(const std::string &sharingRes, MessageParcel &data, MessageParcel &reply)
{
    QueryResults results;
    auto status = Query(sharingRes, results);
    return ITypesUtil::Marshal(reply, status, results) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnQueryByInvitation(
    const std::string &invitation, MessageParcel &data, MessageParcel &reply)
{
    QueryResults results;
    auto status = QueryByInvitation(invitation, results);
    return ITypesUtil::Marshal(reply, status, results) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnConfirmInvitation(
    const std::string &invitation, MessageParcel &data, MessageParcel &reply)
{
    int32_t confirmation;
    if (!ITypesUtil::Unmarshal(data, confirmation)) {
        ZLOGE("Unmarshal invitation:%{public}s", Anonymous::Change(invitation).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    std::tuple<int32_t, std::string, std::string> result;
    auto status = ConfirmInvitation(invitation, confirmation, result);
    return ITypesUtil::Marshal(reply, status, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnChangeConfirmation(
    const std::string &sharingRes, MessageParcel &data, MessageParcel &reply)
{
    int32_t confirmation;
    if (!ITypesUtil::Unmarshal(data, confirmation)) {
        ZLOGE("Unmarshal sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    std::pair<int32_t, std::string> result;
    auto status = ChangeConfirmation(sharingRes, confirmation, result);
    return ITypesUtil::Marshal(reply, status, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}
} // namespace OHOS::CloudData