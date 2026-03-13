/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef DATASHARESERVICE_DATA_SHARE_SA_CONNECTION_H
#define DATASHARESERVICE_DATA_SHARE_SA_CONNECTION_H

#include <mutex>

#include "datashare_sa_provider_info.h"
#include "if_local_ability_manager.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability_load_callback_stub.h"

namespace OHOS::DataShare {

class SAConnection : public SystemAbilityExtensionPara {
public:
    SAConnection(int32_t systemAbilityId, uint32_t waitTime) : systemAbilityId_(systemAbilityId),
        waitTime_(waitTime), code_(0), descriptor_(u"") {}
    bool InputParaSet(MessageParcel& data) override;
    bool OutputParaGet(MessageParcel& reply) override;
    std::pair<int32_t, ConnectionInterfaceInfo> GetConnectionInterfaceInfo();
    bool CheckAndLoadSystemAbility();
    sptr<ILocalAbilityManager> GetLocalAbilityManager();
    class SALoadCallback : public SystemAbilityLoadCallbackStub {
    public:
        void OnLoadSystemAbilitySuccess(int32_t systemAbilityId, const sptr<IRemoteObject> &remoteObject) override;
        void OnLoadSystemAbilityFail(int32_t systemAbilityId) override;

    public:
        std::condition_variable proxyConVar_;
        std::atomic<bool> isLoadSuccess_ = {false};
    };
private:
    int32_t systemAbilityId_ = 0;
    uint32_t waitTime_;
    uint32_t code_;
    std::u16string descriptor_;
    std::mutex mutex_;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_DATA_SHARE_SA_CONNECTION_H