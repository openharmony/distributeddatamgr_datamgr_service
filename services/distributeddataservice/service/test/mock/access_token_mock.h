/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef OHOS_ACCESS_TOKEN_KIT_MOCK_H
#define OHOS_ACCESS_TOKEN_KIT_MOCK_H

#include <gmock/gmock.h>
#include "accesstoken_kit.h"
#include "access_token.h"

namespace OHOS {
namespace Security {
namespace AccessToken {
class BAccessTokenKit {
public:
    virtual ATokenTypeEnum GetTokenTypeFlag(AccessTokenID) = 0;
    virtual int GetHapTokenInfo(AccessTokenID, HapTokenInfo&) = 0;
    BAccessTokenKit() = default;
    virtual ~BAccessTokenKit() = default;
private:
    static inline std::shared_ptr<BAccessTokenKit> accessTokenkit = nullptr;
};

class AccessTokenKitMock : public BAccessTokenKit {
public:
    MOCK_METHOD(ATokenTypeEnum, GetTokenTypeFlag, (AccessTokenID));
    MOCK_METHOD(int, GetHapTokenInfo, (AccessTokenID, HapTokenInfo&));
};
}
}
}
#endif //OHOS_ACCESS_TOKEN_KIT_MOCK_H
