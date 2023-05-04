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
    static bool CheckInterfaceToken(MessageParcel& data);
    int32_t OnRemoteInsert(MessageParcel& data, MessageParcel& reply);
    int32_t OnRemoteUpdate(MessageParcel& data, MessageParcel& reply);
    int32_t OnRemoteDelete(MessageParcel& data, MessageParcel& reply);
    int32_t OnRemoteQuery(MessageParcel& data, MessageParcel& reply);
    int32_t OnRemoteAddTemplate(MessageParcel& data, MessageParcel& reply);
    int32_t OnRemoteDelTemplate(MessageParcel& data, MessageParcel& reply);
    int32_t OnRemotePublish(MessageParcel& data, MessageParcel& reply);
    int32_t OnRemoteGetData(MessageParcel& data, MessageParcel& reply);
    int32_t OnRemoteSubscribeRdbData(MessageParcel& data, MessageParcel& reply);
    int32_t OnRemoteUnSubscribeRdbData(MessageParcel& data, MessageParcel& reply);
    int32_t OnRemoteEnableSubscribeRdbData(MessageParcel& data, MessageParcel& reply);
    int32_t OnRemoteDisableSubscribeRdbData(MessageParcel& data, MessageParcel& reply);
    int32_t OnRemoteSubscribePublishedData(MessageParcel& data, MessageParcel& reply);
    int32_t OnRemoteUnSubscribePublishedData(MessageParcel& data, MessageParcel& reply);
    int32_t OnRemoteEnableSubscribePublishedData(MessageParcel& data, MessageParcel& reply);
    int32_t OnRemoteDisableSubscribePublishedData(MessageParcel& data, MessageParcel& reply);
    using RequestHandle = int (DataShareServiceStub::*)(MessageParcel &, MessageParcel &);
    static constexpr RequestHandle HANDLERS[DATA_SHARE_SERVICE_CMD_MAX] = {
        &DataShareServiceStub::OnRemoteInsert,
        &DataShareServiceStub::OnRemoteDelete,
        &DataShareServiceStub::OnRemoteUpdate,
        &DataShareServiceStub::OnRemoteQuery,
        &DataShareServiceStub::OnRemoteAddTemplate,
        &DataShareServiceStub::OnRemoteDelTemplate,
        &DataShareServiceStub::OnRemotePublish,
        &DataShareServiceStub::OnRemoteGetData,
        &DataShareServiceStub::OnRemoteSubscribeRdbData,
        &DataShareServiceStub::OnRemoteUnSubscribeRdbData,
        &DataShareServiceStub::OnRemoteEnableSubscribeRdbData,
        &DataShareServiceStub::OnRemoteDisableSubscribeRdbData,
        &DataShareServiceStub::OnRemoteSubscribePublishedData,
        &DataShareServiceStub::OnRemoteUnSubscribePublishedData,
        &DataShareServiceStub::OnRemoteEnableSubscribePublishedData,
        &DataShareServiceStub::OnRemoteDisableSubscribePublishedData
    };
};
} // namespace DataShare
} // namespace OHOS
#endif
