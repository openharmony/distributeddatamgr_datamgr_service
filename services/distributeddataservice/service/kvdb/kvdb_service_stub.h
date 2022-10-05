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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_SERVICE_STUB_H
#define OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_SERVICE_STUB_H
#include "iremote_stub.h"
#include "kvdb_service.h"
#include "feature/feature_system.h"
namespace OHOS::DistributedKv {
class API_EXPORT KVDBServiceStub : public KVDBService, public DistributedData::FeatureSystem::Feature {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply) override;

private:
    using Handler = int32_t (KVDBServiceStub::*)(
        const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnGetStoreIds(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnBeforeCreate(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnAfterCreate(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnDelete(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnSync(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnRegisterCallback(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnUnregisterCallback(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnSetSyncParam(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnGetSyncParam(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnEnableCap(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnDisableCap(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnSetCapability(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnAddSubInfo(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnRmvSubInfo(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnSubscribe(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnUnsubscribe(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    int32_t OnGetBackupPassword(const AppId &appId, const StoreId &storeId, MessageParcel &data, MessageParcel &reply);
    static const Handler HANDLERS[TRANS_BUTT];
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_SERVICE_KVDB_SERVICE_STUB_H
