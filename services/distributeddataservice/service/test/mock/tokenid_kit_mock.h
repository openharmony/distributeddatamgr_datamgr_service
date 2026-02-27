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

#include "tokenid_kit.h"

bool g_isSystemAppByFullTokenID = true;

void MockIsSystemAppByFullTokenID(bool mockRet)
{
    g_isSystemAppByFullTokenID = mockRet;
}

namespace OHOS {
namespace Security {
namespace AccessToken {
bool TokenIdKit::IsSystemAppByFullTokenID(uint64_t tokenId)
{
    return g_isSystemAppByFullTokenID;
}
}
}
}
#endif
