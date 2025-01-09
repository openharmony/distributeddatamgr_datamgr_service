/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef UDMF_DIALOG_H
#define UDMF_DIALOG_H

#include "error_code.h"
#include "ability_manager_interface.h"
#include "refbase.h"

namespace OHOS::UDMF {

class ProgressDialog {
public:
    struct FocusedAppInfo {
        int32_t windowId = 0;
        sptr<IRemoteObject> abilityToken = nullptr;
    };
    struct ProgressMessageInfo {
        std::string promptText{ DEFAULT_LABEL };
        std::string remoteDeviceName{ DEFAULT_LABEL };
        std::string progressKey{ DEFAULT_LABEL };
        std::string signalKey{ DEFAULT_LABEL };
        bool isRemote { false };
        int32_t windowId { 0 };
        sptr<IRemoteObject> callerToken { nullptr };
    };

    static ProgressDialog &GetInstance();
    int32_t ShowProgress(const ProgressMessageInfo &message);

private:
    static sptr<OHOS::AAFwk::IAbilityManager> GetAbilityManagerService();
    FocusedAppInfo GetFocusedAppInfo(void) const;

    static constexpr const char *DEFAULT_LABEL = "unknown";
    static constexpr const char *PASTEBOARD_DIALOG_APP = "com.ohos.pasteboarddialog";
    static constexpr const char *PASTEBOARD_PROGRESS_ABILITY = "PasteboardProgressAbility";
};
} // namespace UDMF::OHOS
#endif // UDMF_DIALOG_H