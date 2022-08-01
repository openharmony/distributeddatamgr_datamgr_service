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

#ifndef DISTRIBUTEDDATAMGR_DATAMGR_GRANT_DATA_SYNC_PERMISSION_SERVICE_H
#define DISTRIBUTEDDATAMGR_DATAMGR_GRANT_DATA_SYNC_PERMISSION_SERVICE_H

#include <cstdint>
#include <string>

#include "access_token.h"
#include "hap_token_info.h"
#include "accesstoken_kit.h"
#include "token_setproc.h"

namespace OHOS {
namespace DistributedKv {
using namespace OHOS::Security;
class GrantDataSyncPermission {
public:
    static void GrantPermission(const std::string &appId)
    {
        int32_t ret = -1;
        std::string permissionName = "ohos.permission.DISTRIBUTED_DATASYNC";

        AccessToken::HapInfoParams hapInfoParams = {
            .userID = 1,
            .bundleName = appId,
            .instIndex = 0,
            .appIDDesc = "app need sync permission"
        };

        AccessToken::PermissionDef permissionDef = {
            .permissionName = permissionName,
            .bundleName = appId,
            .grantMode = 1,
            .availableLevel = AccessToken::ATokenAplEnum::APL_NORMAL,
            .label = "label",
            .labelId = 1,
            .description = "permission define",
            .descriptionId = 1
        };

        AccessToken::PermissionStateFull permissionStateFull = {
            .permissionName = permissionName,
            .isGeneral = true,
            .resDeviceID = { "local" },
            .grantStatus = { AccessToken::PermissionState::PERMISSION_GRANTED },
            .grantFlags = { 1 }
        };

        AccessToken::HapPolicyParams hapPolicyParams = {
            .apl = AccessToken::ATokenAplEnum::APL_NORMAL,
            .domain = "test.domain",
            .permList = { permissionDef },
            .permStateList = { permissionStateFull }
        };

        AccessToken::AccessTokenIDEx tokenIdEx = AccessToken::AccessTokenKit::AllocHapToken(hapInfoParams,
                                                                                            hapPolicyParams);
        if (tokenIdEx.tokenIdExStruct.tokenID == 0) {
            unsigned int tokenIdOld = 0;
            tokenIdOld = AccessToken::AccessTokenKit::GetHapTokenID(hapInfoParams.userID, hapInfoParams.bundleName,
                                                                    hapInfoParams.instIndex);
            if (tokenIdOld == 0) {
                return;
            }
            ret = AccessToken::AccessTokenKit::DeleteToken(tokenIdOld);
            if (ret != 0) {
                return;
            }

            /* Retry the token allocation again */
            tokenIdEx = AccessToken::AccessTokenKit::AllocHapToken(hapInfoParams, hapPolicyParams);
            if (tokenIdEx.tokenIdExStruct.tokenID == 0) {
                return;
            }
        }

        (void) SetSelfTokenID(tokenIdEx.tokenIdExStruct.tokenID);

        AccessToken::AccessTokenKit::GrantPermission(tokenIdEx.tokenIdExStruct.tokenID, permissionName,
                                                     AccessToken::PERMISSION_USER_FIXED);
    }
};
} // namespace DistributedKv
} // namespace OHOS

#endif // DISTRIBUTEDDATAMGR_DATAMGR_GRANT_DATA_SYNC_PERMISSION_SERVICE_H
