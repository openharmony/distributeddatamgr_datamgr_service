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

#ifndef DISTRIBUTED_OBJECT_SERVICE_STUB_H
#define DISTRIBUTED_OBJECT_SERVICE_STUB_H

#include <iremote_stub.h>
#include "iobject_service.h"
#include "feature/feature_system.h"
namespace OHOS::DistributedObject {
class ObjectServiceStub : public ObjectService, public DistributedData::FeatureSystem::Feature {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply) override;

private:
    bool CheckInterfaceToken(MessageParcel& data);
    int32_t ObjectStoreSaveOnRemote(MessageParcel &data, MessageParcel &reply);
    int32_t ObjectStoreRevokeSaveOnRemote(MessageParcel &data, MessageParcel &reply);
    int32_t ObjectStoreRetrieveOnRemote(MessageParcel &data, MessageParcel &reply);
    int32_t OnSubscribeRequest(MessageParcel &data, MessageParcel &reply);
    int32_t OnUnsubscribeRequest(MessageParcel &data, MessageParcel &reply);
    
    using RequestHandle = int (ObjectServiceStub::*)(MessageParcel &, MessageParcel &);
    static constexpr RequestHandle HANDLERS[OBJECTSTORE_SERVICE_CMD_MAX] = {
        &ObjectServiceStub::ObjectStoreSaveOnRemote,
        &ObjectServiceStub::ObjectStoreRevokeSaveOnRemote,
        &ObjectServiceStub::ObjectStoreRetrieveOnRemote,
        &ObjectServiceStub::OnSubscribeRequest,
        &ObjectServiceStub::OnUnsubscribeRequest,
    };
};
} // namespace OHOS::DistributedRdb
#endif
