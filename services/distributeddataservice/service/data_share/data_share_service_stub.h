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

#ifndef DATASHARESERVICE_DATA_SHARE_SERVICE_STUB_H
#define DATASHARESERVICE_DATA_SHARE_SERVICE_STUB_H

#include <iremote_stub.h>
#include "idata_share_service.h"
#include "feature/feature_system.h"
namespace OHOS {
namespace DataShare {
class DataShareServiceStub : public IDataShareService, public DistributedData::FeatureSystem::Feature {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply) override;

private:
    static constexpr std::chrono::milliseconds TIME_THRESHOLD = std::chrono::milliseconds(500);
    static bool CheckInterfaceToken(MessageParcel& data);
    int32_t OnInsert(MessageParcel& data, MessageParcel& reply);
    int32_t OnUpdate(MessageParcel& data, MessageParcel& reply);
    int32_t OnDelete(MessageParcel& data, MessageParcel& reply);
    int32_t OnQuery(MessageParcel& data, MessageParcel& reply);
    int32_t OnAddTemplate(MessageParcel& data, MessageParcel& reply);
    int32_t OnDelTemplate(MessageParcel& data, MessageParcel& reply);
    int32_t OnPublish(MessageParcel& data, MessageParcel& reply);
    int32_t OnGetData(MessageParcel& data, MessageParcel& reply);
    int32_t OnSubscribeRdbData(MessageParcel& data, MessageParcel& reply);
    int32_t OnUnsubscribeRdbData(MessageParcel& data, MessageParcel& reply);
    int32_t OnEnableRdbSubs(MessageParcel& data, MessageParcel& reply);
    int32_t OnDisableRdbSubs(MessageParcel& data, MessageParcel& reply);
    int32_t OnSubscribePublishedData(MessageParcel& data, MessageParcel& reply);
    int32_t OnUnsubscribePublishedData(MessageParcel& data, MessageParcel& reply);
    int32_t OnEnablePubSubs(MessageParcel& data, MessageParcel& reply);
    int32_t OnDisablePubSubs(MessageParcel& data, MessageParcel& reply);
    int32_t OnNotifyConnectDone(MessageParcel& data, MessageParcel& reply);
    int32_t OnNotifyObserver(MessageParcel& data, MessageParcel& reply);
    int32_t OnSetSilentSwitch(MessageParcel& data, MessageParcel& reply);
    int32_t OnGetSilentProxyStatus(MessageParcel& data, MessageParcel& reply);
    int32_t OnRegisterObserver(MessageParcel& data, MessageParcel& reply);
    int32_t OnUnregisterObserver(MessageParcel& data, MessageParcel& reply);
    using RequestHandle = int (DataShareServiceStub::*)(MessageParcel &, MessageParcel &);
    static constexpr RequestHandle HANDLERS[DATA_SHARE_SERVICE_CMD_MAX] = {
        &DataShareServiceStub::OnInsert,
        &DataShareServiceStub::OnDelete,
        &DataShareServiceStub::OnUpdate,
        &DataShareServiceStub::OnQuery,
        &DataShareServiceStub::OnAddTemplate,
        &DataShareServiceStub::OnDelTemplate,
        &DataShareServiceStub::OnPublish,
        &DataShareServiceStub::OnGetData,
        &DataShareServiceStub::OnSubscribeRdbData,
        &DataShareServiceStub::OnUnsubscribeRdbData,
        &DataShareServiceStub::OnEnableRdbSubs,
        &DataShareServiceStub::OnDisableRdbSubs,
        &DataShareServiceStub::OnSubscribePublishedData,
        &DataShareServiceStub::OnUnsubscribePublishedData,
        &DataShareServiceStub::OnEnablePubSubs,
        &DataShareServiceStub::OnDisablePubSubs,
        &DataShareServiceStub::OnNotifyConnectDone,
        &DataShareServiceStub::OnNotifyObserver,
        &DataShareServiceStub::OnSetSilentSwitch,
        &DataShareServiceStub::OnGetSilentProxyStatus,
        &DataShareServiceStub::OnRegisterObserver,
        &DataShareServiceStub::OnUnregisterObserver};
};
} // namespace DataShare
} // namespace OHOS
#endif // DATASHARESERVICE_DATA_SHARE_SERVICE_STUB_H
