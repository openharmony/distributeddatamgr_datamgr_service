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

#ifndef OHOS_NETWORK_MANAGER_H
#define OHOS_NETWORK_MANAGER_H

#include "net_all_capabilities.h"
#include "net_conn_callback_stub.h"
#include "net_handle.h"
#include "kvstore_data_service.h"
namespace OHOS::DistributedKv {
using namespace NetManagerStandard;
class NetWorkManager {
public:
    bool RegOnNetworkChange(KvStoreDataService *kvStoreDataService);
    static NetWorkManager &GetInstance();

    class NetConnCallbackObserver : public NetConnCallbackStub {
    public:
        explicit NetConnCallbackObserver(KvStoreDataService *kvStoreDataService)
            : kvStoreDataService_(kvStoreDataService)
        {
        }
        ~NetConnCallbackObserver() override = default;
        int32_t NetAvailable(sptr<NetHandle> &netHandle) override;
        int32_t NetCapabilitiesChange(sptr<NetHandle> &netHandle, const sptr<NetAllCapabilities> &netAllCap) override;
        int32_t NetConnectionPropertiesChange(sptr<NetHandle> &netHandle, const sptr<NetLinkInfo> &info) override;
        int32_t NetLost(sptr<NetHandle> &netHandle) override;
        int32_t NetUnavailable() override;
        int32_t NetBlockStatusChange(sptr<NetHandle> &netHandle, bool blocked) override;

    private:
        KvStoreDataService *kvStoreDataService_;
    };

private:
    NetWorkManager();
    ~NetWorkManager();
};
} // namespace OHOS::DistributedKv
#endif // OHOS_NETWORK_MANAGER_H
