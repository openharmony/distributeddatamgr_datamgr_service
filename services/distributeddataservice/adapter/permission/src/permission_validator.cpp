/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#define LOG_TAG "PermissionValidator"

#include "permission_validator.h"
#include "accesstoken_kit.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
using namespace Security::AccessToken;
PermissionValidator &PermissionValidator::GetInstance()
{
    static PermissionValidator permissionValidator;
    return permissionValidator;
}

// check whether the client process have enough privilege to share data with the other devices.
bool PermissionValidator::CheckSyncPermission(uint32_t tokenId)
{
    auto type = AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (type == TOKEN_NATIVE || type == TOKEN_SHELL) {
        return true;
    }
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) == TOKEN_HAP) {
        return (AccessTokenKit::VerifyAccessToken(tokenId, DISTRIBUTED_DATASYNC) == PERMISSION_GRANTED);
    }

    ZLOGI("invalid tokenid:%u", tokenId);
    return false;
}
} // namespace DistributedKv
} // namespace OHOS
