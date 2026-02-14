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

#ifndef NATIVE_AND_HAP_TOKEN_MOCK_H
#define NATIVE_AND_HAP_TOKEN_MOCK_H

#include <gtest/gtest.h>
#include "access_token.h"
#include "accesstoken_kit.h"
#include "token_setproc.h"

namespace OHOS {
namespace DistributedData {
using namespace OHOS;

// Adapted from security_access_token/blob/master/interfaces/innerkits/privacy/test/unittest/src/privacy_test_common
class MockNativeToken {
public:
    explicit MockNativeToken(const std::string& process);
    ~MockNativeToken();
private:
    uint64_t selfToken_;
};

class MockToken {
public:
    // Call in SetUp to save the current token id. Can't be called concurrently to avoid mismatching tokens
    static void SetTestEnvironment();
    // Call in TearDown to reset the saved current token id to 0 and avoid impact on other tests
    static void ResetTestEnvironment();
    // Call by shell tokenid. Other processes don't have permission to dump info
    static Security::AccessToken::AccessTokenID GetTokenByProcName(const std::string& process);
    static Security::AccessToken::AccessTokenIDEx AllocTestHapToken(
        const Security::AccessToken::HapInfoParams& hapInfo, Security::AccessToken::HapPolicyParams& hapPolicy
    );
    // Don't call on real apps
    static int32_t DeleteTestHapToken(Security::AccessToken::AccessTokenID tokenID);
};
}
}
#endif // NATIVE_AND_HAP_TOKEN_MOCK_H