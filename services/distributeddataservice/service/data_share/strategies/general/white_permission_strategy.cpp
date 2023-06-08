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
#define LOG_TAG "WhitePermissionStrategy"

#include "white_permission_strategy.h"

#include "accesstoken_kit.h"
#include "log_print.h"

namespace OHOS::DataShare {
bool WhitePermissionStrategy::operator()(std::shared_ptr<Context> context)
{
    for (const auto &permission : whitePermissions_) {
        int status = Security::AccessToken::AccessTokenKit::VerifyAccessToken(context->callerTokenId, permission);
        if (status == Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
            context->isInWhite = true;
            return true;
        }
    }
    return true;
}
WhitePermissionStrategy::WhitePermissionStrategy(std::initializer_list<std::string> permissions)
{
    whitePermissions_.insert(whitePermissions_.end(), permissions.begin(), permissions.end());
}
} // namespace OHOS::DataShare