/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "UdmfDialog"

#include "udmf_dialog.h"

#include "ability_connect_callback_stub.h"
#include "ability_manager_proxy.h"
#include "error_code.h"
#include "in_process_call_wrapper.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "system_ability_definition.h"
#ifdef SCENE_BOARD_ENABLE
#include "window_manager_lite.h"
#else
#include "window_manager.h"
#endif

namespace OHOS::UDMF {
using namespace OHOS::AAFwk;

ProgressDialog &ProgressDialog::GetInstance()
{
    static ProgressDialog instance;
    return instance;
}

ProgressDialog::FocusedAppInfo ProgressDialog::GetFocusedAppInfo(void) const
{
    FocusedAppInfo appInfo = { 0 };
    Rosen::FocusChangeInfo info;
#ifdef SCENE_BOARD_ENABLE
    Rosen::WindowManagerLite::GetInstance().GetFocusWindowInfo(info);
#else
    Rosen::WindowManager::GetInstance().GetFocusWindowInfo(info);
#endif
    appInfo.windowId = info.windowId_;
    appInfo.abilityToken = info.abilityToken_;
    return appInfo;
}

sptr<IAbilityManager> ProgressDialog::GetAbilityManagerService()
{
    auto systemAbilityManager = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemAbilityManager == nullptr) {
        ZLOGE("Failed to get ability manager.");
        return nullptr;
    }
    sptr<IRemoteObject> remoteObject = systemAbilityManager->GetSystemAbility(ABILITY_MGR_SERVICE_ID);
    if (!remoteObject) {
        ZLOGE("Failed to get ability manager service.");
        return nullptr;
    }
    return iface_cast<IAbilityManager>(remoteObject);
}

int32_t ProgressDialog::ShowProgress(const ProgressMessageInfo &message)
{
    ZLOGD("Begin.");
    auto abilityManager = GetAbilityManagerService();
    if (abilityManager == nullptr) {
        ZLOGE("Get ability manager failed.");
        return E_ERROR;
    }
 
    Want want;
    want.SetElementName(PASTEBOARD_DIALOG_APP, PASTEBOARD_PROGRESS_ABILITY);
    want.SetAction(PASTEBOARD_PROGRESS_ABILITY);
    want.SetParam("promptText", message.promptText);
    want.SetParam("remoteDeviceName", message.remoteDeviceName);
    want.SetParam("progressKey", message.progressKey);
    want.SetParam("signalKey", message.signalKey);
    want.SetParam("isRemote", message.isRemote);
    want.SetParam("windowId", message.windowId);
    if (message.callerToken != nullptr) {
        want.SetParam("tokenKey", message.callerToken);
    } else {
        ZLOGW("CallerToken is nullptr.");
    }

    int32_t status = IN_PROCESS_CALL(abilityManager->StartAbility(want));
    if (status != 0) {
        ZLOGE("Start pasteboard progress failed, status:%{public}d.", status);
    }
    return status;
}
} // namespace OHOS::UDMF