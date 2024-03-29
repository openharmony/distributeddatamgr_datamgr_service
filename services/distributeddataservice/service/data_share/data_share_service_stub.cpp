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

#define LOG_TAG "DataShareServiceStub"

#include "data_share_service_stub.h"

#include <cinttypes>
#include "data_share_obs_proxy.h"
#include "ipc_skeleton.h"
#include "ishared_result_set.h"
#include "itypes_util.h"
#include "log_print.h"
#include "utils/anonymous.h"

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
    if (!ITypesUtil::Unmarshal(data, uri, bucket.valuesMap)) {
        ZLOGE("Unmarshal uri:%{public}s bucket size:%{public}zu", DistributedData::Anonymous::Change(uri).c_str(),
            bucket.valuesMap.size());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = Insert(uri, bucket);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteUpdate(MessageParcel &data, MessageParcel &reply)
{
    std::string uri;
    DataSharePredicates predicate;
    DataShareValuesBucket bucket;
    if (!ITypesUtil::Unmarshal(data, uri, predicate, bucket.valuesMap)) {
        ZLOGE("Unmarshal uri:%{public}s bucket size:%{public}zu", DistributedData::Anonymous::Change(uri).c_str(),
            bucket.valuesMap.size());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = Update(uri, predicate, bucket);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteDelete(MessageParcel &data, MessageParcel &reply)
{
    std::string uri;
    DataSharePredicates predicate;
    if (!ITypesUtil::Unmarshal(data, uri, predicate)) {
        ZLOGE("Unmarshal uri:%{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = Delete(uri, predicate);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteQuery(MessageParcel &data, MessageParcel &reply)
{
    std::string uri;
    DataSharePredicates predicate;
    std::vector<std::string> columns;
    if (!ITypesUtil::Unmarshal(data, uri, predicate, columns)) {
        ZLOGE("Unmarshal uri:%{public}s columns size:%{public}zu", DistributedData::Anonymous::Change(uri).c_str(),
            columns.size());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int status = 0;
    auto result = ISharedResultSet::WriteToParcel(Query(uri, predicate, columns, status), reply);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteAddTemplate(MessageParcel &data, MessageParcel &reply)
{
    std::string uri;
    int64_t subscriberId;
    Template tpl;
    if (!ITypesUtil::Unmarshal(data, uri, subscriberId, tpl.predicates_,  tpl.scheduler_)) {
        ZLOGW("read device list failed.");
        return -1;
    }
    int32_t status = AddTemplate(uri, subscriberId, tpl);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteDelTemplate(MessageParcel &data, MessageParcel &reply)
{
    std::string uri;
    int64_t subscriberId;
    if (!ITypesUtil::Unmarshal(data, uri, subscriberId)) {
        ZLOGW("read device list failed.");
        return -1;
    }
    int32_t status = DelTemplate(uri, subscriberId);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemotePublish(MessageParcel &data, MessageParcel &reply)
{
    Data publishData;
    std::string bundleName;
    if (!ITypesUtil::Unmarshal(data, publishData.datas_, publishData.version_, bundleName)) {
        ZLOGW("read device list failed.");
        return -1;
    }
    std::vector<OperationResult> results = Publish(publishData, bundleName);
    if (!ITypesUtil::Marshal(reply, results)) {
        ZLOGE("ITypesUtil::Marshal(reply, results) failed");
        return -1;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteGetData(MessageParcel &data, MessageParcel &reply)
{
    std::string bundleName;
    if (!ITypesUtil::Unmarshal(data, bundleName)) {
        ZLOGW("read device list failed.");
        return -1;
    }
    ZLOGI("bundleName is %{public}s", bundleName.c_str());
    int errorCode = E_OK;
    auto results = GetData(bundleName, errorCode);
    if (!ITypesUtil::Marshal(reply, results.datas_, errorCode)) {
        ZLOGE("ITypesUtil::Marshal(reply, results) failed");
        return -1;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteSubscribeRdbData(MessageParcel &data, MessageParcel &reply)
{
    std::vector<std::string> uris;
    TemplateId templateId;
    if (!ITypesUtil::Unmarshal(data, uris, templateId)) {
        ZLOGE("read device list failed.");
        return -1;
    }
    auto remoteObj = data.ReadRemoteObject();
    sptr<IDataProxyRdbObserver> observer = new (std::nothrow)RdbObserverProxy(remoteObj);
    if (observer == nullptr) {
        ZLOGE("obServer is nullptr");
        return -1;
    }
    std::vector<OperationResult> results = SubscribeRdbData(uris, templateId, observer);
    if (!ITypesUtil::Marshal(reply, results)) {
        ZLOGE("ITypesUtil::Marshal(reply, results) failed");
        return -1;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteUnsubscribeRdbData(MessageParcel &data, MessageParcel &reply)
{
    std::vector<std::string> uris;
    TemplateId templateId;
    if (!ITypesUtil::Unmarshal(data, uris, templateId)) {
        ZLOGE("read device list failed.");
        return -1;
    }
    std::vector<OperationResult> results = UnsubscribeRdbData(uris, templateId);
    if (!ITypesUtil::Marshal(reply, results)) {
        ZLOGE("ITypesUtil::Marshal(reply, results) failed");
        return -1;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteEnableRdbSubs(MessageParcel &data, MessageParcel &reply)
{
    std::vector<std::string> uris;
    TemplateId templateId;
    if (!ITypesUtil::Unmarshal(data, uris, templateId)) {
        ZLOGE("read device list failed.");
        return -1;
    }
    std::vector<OperationResult> results = EnableRdbSubs(uris, templateId);
    if (!ITypesUtil::Marshal(reply, results)) {
        ZLOGE("ITypesUtil::Marshal(reply, results) failed");
        return -1;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteDisableRdbSubs(MessageParcel &data, MessageParcel &reply)
{
    std::vector<std::string> uris;
    TemplateId templateId;
    if (!ITypesUtil::Unmarshal(data, uris, templateId)) {
        ZLOGE("read device list failed.");
        return -1;
    }
    std::vector<OperationResult> results = DisableRdbSubs(uris, templateId);
    if (!ITypesUtil::Marshal(reply, results)) {
        ZLOGE("ITypesUtil::Marshal(reply, results) failed");
        return -1;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteSubscribePublishedData(MessageParcel &data, MessageParcel &reply)
{
    std::vector<std::string> uris;
    int64_t subscriberId;
    if (!ITypesUtil::Unmarshal(data, uris, subscriberId)) {
        ZLOGE("read device list failed.");
        return -1;
    }
    sptr<PublishedDataObserverProxy> observer = new (std::nothrow)PublishedDataObserverProxy(data.ReadRemoteObject());
    if (observer == nullptr) {
        ZLOGE("obServer is nullptr");
        return -1;
    }
    std::vector<OperationResult> results = SubscribePublishedData(uris, subscriberId, observer);
    if (!ITypesUtil::Marshal(reply, results)) {
        ZLOGE("ITypesUtil::Marshal(reply, results) failed");
        return -1;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteUnsubscribePublishedData(MessageParcel &data, MessageParcel &reply)
{
    std::vector<std::string> uris;
    int64_t subscriberId;
    if (!ITypesUtil::Unmarshal(data, uris, subscriberId)) {
        ZLOGE("read device list failed.");
        return -1;
    }
    std::vector<OperationResult> results = UnsubscribePublishedData(uris, subscriberId);
    if (!ITypesUtil::Marshal(reply, results)) {
        ZLOGE("ITypesUtil::Marshal(reply, results) failed");
        return -1;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteEnablePubSubs(MessageParcel &data, MessageParcel &reply)
{
    std::vector<std::string> uris;
    int64_t subscriberId;
    if (!ITypesUtil::Unmarshal(data, uris, subscriberId)) {
        ZLOGE("read device list failed.");
        return -1;
    }
    std::vector<OperationResult> results = EnablePubSubs(uris, subscriberId);
    if (!ITypesUtil::Marshal(reply, results)) {
        ZLOGE("ITypesUtil::Marshal(reply, results) failed");
        return -1;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteDisablePubSubs(MessageParcel &data, MessageParcel &reply)
{
    std::vector<std::string> uris;
    int64_t subscriberId;
    if (!ITypesUtil::Unmarshal(data, uris, subscriberId)) {
        ZLOGE("read device list failed.");
        return -1;
    }
    std::vector<OperationResult> results = DisablePubSubs(uris, subscriberId);
    if (!ITypesUtil::Marshal(reply, results)) {
        ZLOGE("ITypesUtil::Marshal(reply, results) failed");
        return -1;
    }
    return 0;
}

int32_t DataShareServiceStub::OnRemoteNotifyConnectDone(MessageParcel &data, MessageParcel &reply)
{
    OnConnectDone();
    return 0;
}

int DataShareServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply)
{
    auto callingPid = IPCSkeleton::GetCallingPid();
    ZLOGD("code:%{public}u, callingPid:%{public}d", code, callingPid);
    if (!CheckInterfaceToken(data)) {
        return DATA_SHARE_ERROR;
    }
    int res = -1;
    if (code < DATA_SHARE_SERVICE_CMD_MAX) {
        auto start = std::chrono::steady_clock::now();
        res = (this->*HANDLERS[code])(data, reply);
        auto finish = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start);
        if (duration >= TIME_THRESHOLD) {
            int64_t milliseconds = duration.count();
            ZLOGW("over time, code:%{public}u callingPid:%{public}d, cost:%{public}" PRIi64 "ms",
                code, callingPid, milliseconds);
        }
    }
    return res;
}

int32_t DataShareServiceStub::OnRemoteNotifyObserver(MessageParcel &data, MessageParcel &reply)
{
    std::string uri;
    if (!ITypesUtil::Unmarshal(data, uri)) {
        ZLOGE("read device list failed.");
        return -1;
    }
    NotifyObserver(uri);
    return 0;
}

int32_t DataShareServiceStub::OnRemoteSetSilentSwitch(MessageParcel &data, MessageParcel &reply)
{
    std::string uri;
    bool enable = false;
    if (!ITypesUtil::Unmarshal(data, uri, enable)) {
        ZLOGE("Unmarshal set silent switch failed. uri: %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = EnableSilentProxy(uri, enable);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal status:0x%{public}x", status);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return E_OK;
}

int32_t DataShareServiceStub::OnRemoteIsSilentProxyEnable(MessageParcel &data, MessageParcel &reply)
{
    std::string uri;
    if (!ITypesUtil::Unmarshal(data, uri)) {
        ZLOGE("Unmarshal silent enable failed. uri: %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    bool enable = IsSilentProxyEnable(uri);
    if (!ITypesUtil::Marshal(reply, enable)) {
        ZLOGE("Marshal enable:%{public}d failed.", enable);
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return E_OK;
}

int32_t DataShareServiceStub::OnRemoteRegisterObserver(MessageParcel &data, MessageParcel &reply)
{
    std::string uri;
    sptr<IRemoteObject> remoteObj;
    if (!ITypesUtil::Unmarshal(data, uri, remoteObj)) {
        ZLOGE("Unmarshal failed,uri: %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = RegisterObserver(uri, remoteObj);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal failed,status:0x%{public}x,uri:%{public}s", status,
            DistributedData::Anonymous::Change(uri).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return E_OK;
}

int32_t DataShareServiceStub::OnRemoteUnregisterObserver(MessageParcel &data, MessageParcel &reply)
{
    std::string uri;
    sptr<IRemoteObject> remoteObj;
    if (!ITypesUtil::Unmarshal(data, uri, remoteObj)) {
        ZLOGE("Unmarshal failed,uri: %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return IPC_STUB_INVALID_DATA_ERR;
    }
    int32_t status = UnregisterObserver(uri, remoteObj);
    if (!ITypesUtil::Marshal(reply, status)) {
        ZLOGE("Marshal failed,status:0x%{public}x,uri:%{public}s", status,
            DistributedData::Anonymous::Change(uri).c_str());
        return IPC_STUB_WRITE_PARCEL_ERR;
    }
    return E_OK;
}
} // namespace DataShare
} // namespace OHOS