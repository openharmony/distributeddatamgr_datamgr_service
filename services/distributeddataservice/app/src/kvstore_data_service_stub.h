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
#ifndef KVSTORE_DATA_SERVICE_STUB_H
#define KVSTORE_DATA_SERVICE_STUB_H
#include "ikvstore_data_service.h"
#include "distributeddata_ipc_interface_code.h"
namespace OHOS::DistributedKv {
class API_EXPORT KvStoreDataServiceStub : public IRemoteStub<IKvStoreDataService> {
public:
   int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
       MessageOption &option) override;

private:
   int32_t GetFeatureInterfaceOnRemote(MessageParcel &data, MessageParcel &reply);
   int32_t RegisterClientDeathObserverOnRemote(MessageParcel &data, MessageParcel &reply);
   int32_t ClearAppStorageOnRemote(MessageParcel &data, MessageParcel &reply);

   using RequestHandler = int32_t (KvStoreDataServiceStub::*)(MessageParcel &, MessageParcel &);
   using code = KvStoreDataServiceInterfaceCode;
   static constexpr RequestHandler HANDLERS[static_cast<uint32_t>(code::SERVICE_CMD_LAST)] = {
       &KvStoreDataServiceStub::GetFeatureInterfaceOnRemote,
       &KvStoreDataServiceStub::RegisterClientDeathObserverOnRemote,
       &KvStoreDataServiceStub::ClearAppStorageOnRemote,
   };
};
} // namespace OHOS::DistributedKv
#endif // KVSTORE_DATA_SERVICE_STUB_H
