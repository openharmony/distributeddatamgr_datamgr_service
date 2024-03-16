/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "DataSharePermission"

#include "data_share_permission.h"
#include "accesstoken_kit.h"
#include "log_print.h"

namespace OHOS::DataShare {
bool DataSharePermission::VerifyPermission(const std::string &permission,
    const uint32_t callerTokenId, const bool isProxyUri)
{
    if (isProxyUri && permission.empty()) {
        ZLOGE("reject permission, callerTokenId: 0x%{public}x", callerTokenId);
        return false;
    }
    if (!permission.empty()) {
        int status =
            Security::AccessToken::AccessTokenKit::VerifyAccessToken(callerTokenId, permission);
        if (status != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
            return false;
        }
    }
    return true;
}
} // namespace OHOS::DataShare
