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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "access_token_mock.h"
#include "permission_validator.h"

namespace OHOS {
namespace DistributedKv {
using namespace std;
using namespace testing;
using namespace OHOS::Security::AccessToken;
class PermissionValidatorMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
public:
    static inline shared_ptr<AccessTokenKitMock> accessTokenKitMock = nullptr;
};

void PermissionValidatorMockTest::SetUpTestCase(void)
{
    accessTokenKitMock = make_shared<AccessTokenKitMock>();
    BAccessTokenKit::accessTokenkit = accessTokenKitMock;
}

void PermissionValidatorMockTest::TearDownTestCase(void)
{
    BAccessTokenKit::accessTokenkit = nullptr;
    accessTokenKitMock = nullptr;
}

void PermissionValidatorMockTest::SetUp(void)
{}

void PermissionValidatorMockTest::TearDown(void)
{}

/**
 * @tc.name: CheckSyncPermission
 * @tc.desc: check sync premisson.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caozhijun
 */
HWTEST_F(PermissionValidatorMockTest, CheckSyncPermission, testing::ext::TestSize.Level0)
{
    uint32_t tokenId = 0;
    EXPECT_CALL(*accessTokenKitMock, VerifyAccessToken(_, _))
	    .WillOnce(Return(TypePermissionState::PERMISSION_GRANTED));
    EXPECT_TRUE(PermissionValidator::GetInstance().CheckSyncPermission(tokenId));

    EXPECT_CALL(*accessTokenKitMock, VerifyAccessToken(_, _))
	    .WillOnce(Return(TypePermissionState::PERMISSION_DENIED));
    EXPECT_FALSE(PermissionValidator::GetInstance().CheckSyncPermission(tokenId));
}

/**
 * @tc.name: IsCloudConfigPermit
 * @tc.desc: is loudConfig permit.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: caozhijun
 */
HWTEST_F(PermissionValidatorMockTest, IsCloudConfigPermit, testing::ext::TestSize.Level0)
{
    uint32_t tokenId = 0;
    EXPECT_CALL(*accessTokenKitMock, VerifyAccessToken(_, _))
	    .WillOnce(Return(TypePermissionState::PERMISSION_GRANTED));
    EXPECT_TRUE(PermissionValidator::GetInstance().IsCloudConfigPermit(tokenId));

    EXPECT_CALL(*accessTokenKitMock, VerifyAccessToken(_, _))
	    .WillOnce(Return(TypePermissionState::PERMISSION_DENIED));
    EXPECT_FALSE(PermissionValidator::GetInstance().IsCloudConfigPermit(tokenId));
}
}
}