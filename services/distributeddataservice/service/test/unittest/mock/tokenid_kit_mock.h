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

#ifndef OHOS_TOKENID_KIT_MOCK_H
#define OHOS_TOKENID_KIT_MOCK_H

#include <gtest/gtest.h>

#include "accesstoken_kit.h"
#include "tokenid_kit.h"

bool g_isSystemAppByFullTokenID = true;

void MockIsSystemAppByFullTokenID(bool mockRet)
{
    g_isSystemAppByFullTokenID = mockRet;
}

namespace OHOS {
namespace Security {
namespace AccessToken {
bool g_mockGetTokenTypeFlag = false;
ATokenTypeEnum g_mockTokenTypeFlag = TOKEN_HAP;

void MockGetTokenTypeFlag(bool mockEnable, ATokenTypeEnum mockTokenType)
{
    g_mockGetTokenTypeFlag = mockEnable;
    g_mockTokenTypeFlag = mockTokenType;
}

int32_t g_mockGetHapTokenInfoRet = 0;
HapTokenInfo g_mockHapTokenInfo{ .instIndex = 0, .userID = 0 };

void MockGetHapTokenInfo(int32_t ret, const HapTokenInfo& tokenInfo)
{
    g_mockGetHapTokenInfoRet = ret;
    g_mockHapTokenInfo = tokenInfo;
}

int AccessTokenKit::GetHapTokenInfo(uint32_t tokenId, HapTokenInfo& tokenInfo)
{
    tokenInfo = g_mockHapTokenInfo;
    return g_mockGetHapTokenInfoRet;
}

bool TokenIdKit::IsSystemAppByFullTokenID(uint64_t tokenId)
{
    return g_isSystemAppByFullTokenID;
}

ATokenTypeEnum AccessTokenKit::GetTokenTypeFlag(uint32_t tokenId)
{
    if (g_mockGetTokenTypeFlag) {
        return g_mockTokenTypeFlag;
    }
    return TOKEN_HAP;
}
}
}
}
#endif
