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
#include "cloud_types_util.h"
#include "cloud/cloud_config_manager.h"
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
    &CloudServiceStub::OnQueryStatistics,
    &CloudServiceStub::OnQueryLastSyncInfo,
    &CloudServiceStub::OnSetGlobalCloudStrategy,
    &CloudServiceStub::OnAllocResourceAndShare,
    &CloudServiceStub::OnShare,
    &CloudServiceStub::OnUnshare,
    &CloudServiceStub::OnExit,
    &CloudServiceStub::OnChangePrivilege,
    &CloudServiceStub::OnQuery,
    &CloudServiceStub::OnQueryByInvitation,
    &CloudServiceStub::OnConfirmInvitation,
    &CloudServiceStub::OnChangeConfirmation,
    &CloudServiceStub::OnSetCloudStrategy,
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

    if (((code >= TRANS_CONFIG_HEAD && code < TRANS_CONFIG_BUTT) ||
        (code >= TRANS_SHARE_HEAD && code < TRANS_SHARE_BUTT)) &&
        !TokenIdKit::IsSystemAppByFullTokenID(IPCSkeleton::GetCallingFullTokenID())) {
        ZLOGE("permission denied! code:%{public}u", code);
        auto result = static_cast<int32_t>(PERMISSION_DENIED);
        return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
    }

    if (code >= TRANS_CONFIG_HEAD && code < TRANS_CONFIG_BUTT &&
        !DistributedKv::PermissionValidator::GetInstance().IsCloudConfigPermit(IPCSkeleton::GetCallingTokenID())) {
        ZLOGE("cloud config permission denied! code:%{public}u, BUTT:%{public}d", code, TRANS_BUTT);
        auto result = static_cast<int32_t>(CLOUD_CONFIG_PERMISSION_DENIED);
        return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
    }
    return (this->*HANDLERS[code])(data, reply);
}

int32_t CloudServiceStub::OnEnableCloud(MessageParcel &data, MessageParcel &reply)
{
    std::string id;
    std::map<std::string, int32_t> switches;
    if (!ITypesUtil::Unmarshal(data, id, switches)) {
        ZLOGE("Unmarshal id:%{public}s", Anonymous::Change(id).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    std::map<std::string, int32_t> localSwitches;
    for (auto &[bundle, status] : switches) {
        localSwitches.insert_or_assign(CloudConfigManager::GetInstance().ToLocal(bundle), status);
    }
    auto result = EnableCloud(id, localSwitches);
    return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnDisableCloud(MessageParcel &data, MessageParcel &reply)
{
    std::string id;
    if (!ITypesUtil::Unmarshal(data, id)) {
        ZLOGE("Unmarshal id:%{public}s", Anonymous::Change(id).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto result = DisableCloud(id);
    return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnChangeAppSwitch(MessageParcel &data, MessageParcel &reply)
{
    std::string id;
    std::string bundleName;
    int32_t appSwitch = SWITCH_OFF;
    if (!ITypesUtil::Unmarshal(data, id, bundleName, appSwitch)) {
        ZLOGE("Unmarshal id:%{public}s", Anonymous::Change(id).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto result = ChangeAppSwitch(id, CloudConfigManager::GetInstance().ToLocal(bundleName), appSwitch);
    return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnClean(MessageParcel &data, MessageParcel &reply)
{
    std::string id;
    std::map<std::string, int32_t> actions;
    if (!ITypesUtil::Unmarshal(data, id, actions)) {
        ZLOGE("Unmarshal id:%{public}s", Anonymous::Change(id).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    std::map<std::string, int32_t> localActions;
    for (auto &[bundle, action] : actions) {
        localActions.insert_or_assign(CloudConfigManager::GetInstance().ToLocal(bundle), action);
    }
    auto result = Clean(id, localActions);
    return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnNotifyDataChange(MessageParcel &data, MessageParcel &reply)
{
    std::string id;
    std::string bundleName;
    if (!ITypesUtil::Unmarshal(data, id, bundleName)) {
        ZLOGE("Unmarshal id:%{public}s", Anonymous::Change(id).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto result = NotifyDataChange(id, CloudConfigManager::GetInstance().ToLocal(bundleName));
    return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnQueryLastSyncInfo(MessageParcel &data, MessageParcel &reply)
{
    std::string id;
    std::string bundleName;
    std::string storeId;
    if (!ITypesUtil::Unmarshal(data, id, bundleName, storeId)) {
        ZLOGE("Unmarshal id:%{public}s, bundleName:%{public}s, storeId:%{public}s", Anonymous::Change(id).c_str(),
            bundleName.c_str(), Anonymous::Change(storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto [status, results] = QueryLastSyncInfo(id, bundleName, storeId);
    return ITypesUtil::Marshal(reply, status, results) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnSetGlobalCloudStrategy(MessageParcel &data, MessageParcel &reply)
{
    Strategy strategy;
    std::vector<CommonType::Value> values;
    if (!ITypesUtil::Unmarshal(data, strategy, values)) {
        ZLOGE("Unmarshal strategy:%{public}d, values size:%{public}zu", strategy, values.size());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto status = SetGlobalCloudStrategy(strategy, values);
    return ITypesUtil::Marshal(reply, status) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnAllocResourceAndShare(MessageParcel &data, MessageParcel &reply)
{
    std::string storeId;
    DistributedRdb::PredicatesMemo predicatesMemo;
    std::vector<std::string> columns;
    std::vector<Participant> participants;
    if (!ITypesUtil::Unmarshal(data, storeId, predicatesMemo, columns, participants)) {
        ZLOGE("Unmarshal storeId:%{public}s", Anonymous::Change(storeId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto [status, resultSet] = AllocResourceAndShare(storeId, predicatesMemo, columns, participants);
    return ITypesUtil::Marshal(reply, status, resultSet) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnNotifyChange(MessageParcel &data, MessageParcel &reply)
{
    std::string eventId;
    std::string extraData;
    int32_t userId;
    if (!ITypesUtil::Unmarshal(data, eventId, extraData, userId)) {
        ZLOGE("Unmarshal eventId:%{public}s", Anonymous::Change(eventId).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto result = NotifyDataChange(eventId, extraData, userId);
    return ITypesUtil::Marshal(reply, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnQueryStatistics(MessageParcel &data, MessageParcel &reply)
{
    std::string id;
    std::string bundleName;
    std::string storeId;
    if (!ITypesUtil::Unmarshal(data, id, bundleName, storeId)) {
        ZLOGE("Unmarshal id:%{public}s", Anonymous::Change(id).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto result = QueryStatistics(id, bundleName, storeId);
    return ITypesUtil::Marshal(reply, result.first, result.second) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnShare(MessageParcel &data, MessageParcel &reply)
{
    std::string sharingRes;
    Participants participants;
    if (!ITypesUtil::Unmarshal(data, sharingRes, participants)) {
        ZLOGE("Unmarshal sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    Results results;
    auto status = Share(sharingRes, participants, results);
    return ITypesUtil::Marshal(reply, status, results) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnUnshare(MessageParcel &data, MessageParcel &reply)
{
    std::string sharingRes;
    Participants participants;
    if (!ITypesUtil::Unmarshal(data, sharingRes, participants)) {
        ZLOGE("Unmarshal sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    Results results;
    auto status = Unshare(sharingRes, participants, results);
    return ITypesUtil::Marshal(reply, status, results) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnExit(MessageParcel &data, MessageParcel &reply)
{
    std::string sharingRes;
    if (!ITypesUtil::Unmarshal(data, sharingRes)) {
        ZLOGE("Unmarshal sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    std::pair<int32_t, std::string> result;
    auto status = Exit(sharingRes, result);
    return ITypesUtil::Marshal(reply, status, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnChangePrivilege(MessageParcel &data, MessageParcel &reply)
{
    std::string sharingRes;
    Participants participants;
    if (!ITypesUtil::Unmarshal(data, sharingRes, participants)) {
        ZLOGE("Unmarshal sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    Results results;
    auto status = ChangePrivilege(sharingRes, participants, results);
    return ITypesUtil::Marshal(reply, status, results) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnQuery(MessageParcel &data, MessageParcel &reply)
{
    std::string sharingRes;
    if (!ITypesUtil::Unmarshal(data, sharingRes)) {
        ZLOGE("Unmarshal sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    QueryResults results;
    auto status = Query(sharingRes, results);
    return ITypesUtil::Marshal(reply, status, results) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnQueryByInvitation(MessageParcel &data, MessageParcel &reply)
{
    std::string invitation;
    if (!ITypesUtil::Unmarshal(data, invitation)) {
        ZLOGE("Unmarshal invitation:%{public}s", Anonymous::Change(invitation).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    QueryResults results;
    auto status = QueryByInvitation(invitation, results);
    return ITypesUtil::Marshal(reply, status, results) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnConfirmInvitation(MessageParcel &data, MessageParcel &reply)
{
    std::string invitation;
    int32_t confirmation;
    if (!ITypesUtil::Unmarshal(data, invitation, confirmation)) {
        ZLOGE("Unmarshal invitation:%{public}s", Anonymous::Change(invitation).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    std::tuple<int32_t, std::string, std::string> result;
    auto status = ConfirmInvitation(invitation, confirmation, result);
    return ITypesUtil::Marshal(reply, status, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnChangeConfirmation(MessageParcel &data, MessageParcel &reply)
{
    std::string sharingRes;
    int32_t confirmation;
    if (!ITypesUtil::Unmarshal(data, sharingRes, confirmation)) {
        ZLOGE("Unmarshal sharingRes:%{public}s", Anonymous::Change(sharingRes).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    std::pair<int32_t, std::string> result;
    auto status = ChangeConfirmation(sharingRes, confirmation, result);
    return ITypesUtil::Marshal(reply, status, result) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}

int32_t CloudServiceStub::OnSetCloudStrategy(MessageParcel &data, MessageParcel &reply)
{
    Strategy strategy;
    std::vector<CommonType::Value> values;
    if (!ITypesUtil::Unmarshal(data, strategy, values)) {
        ZLOGE("Unmarshal strategy:%{public}d, values size:%{public}zu", strategy, values.size());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    auto status = SetCloudStrategy(strategy, values);
    return ITypesUtil::Marshal(reply, status) ? ERR_NONE : IPC_STUB_WRITE_PARCEL_ERR;
}
} // namespace OHOS::CloudData