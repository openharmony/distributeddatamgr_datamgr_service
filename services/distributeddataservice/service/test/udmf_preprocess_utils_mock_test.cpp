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

#include "preprocess_utils.h"
#include "gtest/gtest.h"
#include "access_token_mock.h"

namespace OHOS::UDMF {
using namespace testing;
using namespace std;
using namespace testing::ext;
using namespace OHOS::Security::AccessToken;
class UdmfPreProcessUtilsMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() {}
    void TearDown() {}
    static inline shared_ptr<AccessTokenKitMock> accessTokenKitMock = nullptr;
};

void UdmfPreProcessUtilsMockTest::SetUpTestCase(void)
{
    accessTokenKitMock = make_shared<AccessTokenKitMock>();
    BAccessTokenKit::accessTokenkit = accessTokenKitMock;
}

void UdmfPreProcessUtilsMockTest::TearDownTestCase(void)
{
    BAccessTokenKit::accessTokenkit = nullptr;
    accessTokenKitMock = nullptr;
}

/**
* @tc.name: GetHapUidByToken001
* @tc.desc: Abnormal test of GetHapUidByToken, AccessTokenKit GetHapTokenInfo failed
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, GetHapUidByToken001, TestSize.Level1)
{
    uint32_t tokenId = 0;
    int userId = 0;
    PreProcessUtils preProcessUtils;
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillOnce(Return(RET_SUCCESS));
    int32_t ret = preProcessUtils.GetHapUidByToken(tokenId, userId);
    EXPECT_EQ(ret, E_OK);
}

/**
* @tc.name: GetInstIndex001
* @tc.desc: Abnormal test of GetInstIndex, AccessTokenKit GetInstIndex failed
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfPreProcessUtilsMockTest, GetInstIndex001, TestSize.Level1)
{
    uint32_t tokenId = 0;
    int32_t instIndex = 0;
    PreProcessUtils preProcessUtils;
    EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).WillOnce(Return(TOKEN_HAP));
    EXPECT_CALL(*accessTokenKitMock, GetHapTokenInfo(_, _)).WillOnce(Return(RET_SUCCESS));
    bool ret = preProcessUtils.GetInstIndex(tokenId, instIndex);
    EXPECT_EQ(ret, true);
}
}; // namespace UDMF