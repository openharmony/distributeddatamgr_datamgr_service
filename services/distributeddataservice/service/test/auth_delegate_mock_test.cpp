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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "user_delegate_mock.h"
#include "device_manager_adapter_mock.h"
#include "auth_delegate.h"
#include "types.h"

namespace OHOS::DistributedData {
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using DistributedData::UserStatus;
using AclParams = OHOS::AppDistributedKv::AclParams;
using namespace testing;
using namespace std;
class AuthDelegateMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
public:
    shared_ptr<DeviceManagerAdapterMock> devMgrAdapterMock = nullptr;
    shared_ptr<UserDelegateMock> userDelegateMock = nullptr;
    AuthHandler *authHandler = nullptr;
};

void AuthDelegateMockTest::SetUpTestCase(void)
{}

void AuthDelegateMockTest::TearDownTestCase(void)
{}

void AuthDelegateMockTest::SetUp(void)
{
    authHandler = AuthDelegate::GetInstance();
    devMgrAdapterMock = make_shared<DeviceManagerAdapterMock>();
    BDeviceManagerAdapter::deviceManagerAdapter = devMgrAdapterMock;
    userDelegateMock = make_shared<UserDelegateMock>();
    BUserDelegate::userDelegate = userDelegateMock;
}

void AuthDelegateMockTest::TearDown(void)
{
    BDeviceManagerAdapter::deviceManagerAdapter = nullptr;
    devMgrAdapterMock = nullptr;
    userDelegateMock = nullptr;
    BUserDelegate::userDelegate = nullptr;
    authHandler = nullptr;
}

/**
* @tc.name: CheckAccess001
* @tc.desc: Check access.
* @tc.type: FUNC
* @tc.require:
* @tc.author: caozhijun
*/
HWTEST_F(AuthDelegateMockTest, CheckAccess001, testing::ext::TestSize.Level0)
{
    ASSERT_NE(authHandler, nullptr);
    int localUserId = 1;
    int peerUserId = 1;
    std::string peerDevId = "CheckAccess001";
    AclParams aclParams;
    UserStatus local;
    local.id = localUserId;
    local.isActive = true;
    UserStatus remote;
    remote.id = peerUserId;
    remote.isActive = true;
    std::vector<UserStatus> localUsers;
    std::vector<UserStatus> peerUsers;
    localUsers.push_back(local);
    peerUsers.push_back(remote);
    EXPECT_CALL(*userDelegateMock, GetLocalUserStatus()).WillOnce(Return(localUsers));
    EXPECT_CALL(*userDelegateMock, GetRemoteUserStatus(_)).WillOnce(Return(peerUsers));
    auto result = authHandler->CheckAccess(localUserId, peerUserId, peerDevId, aclParams);
    EXPECT_FALSE(result.first);
}

/**
* @tc.name: CheckAccess002
* @tc.desc: Check access.
* @tc.type: FUNC
* @tc.require:
* @tc.author: caozhijun
*/
HWTEST_F(AuthDelegateMockTest, CheckAccess002, testing::ext::TestSize.Level0)
{
    ASSERT_NE(authHandler, nullptr);
    int localUserId = 1;
    int peerUserId = 1;
    std::string peerDevId = "CheckAccess002";
    AclParams aclParams;
    aclParams.authType = static_cast<int32_t>(DistributedKv::AuthType::DEFAULT);
    UserStatus local;
    local.id = localUserId;
    local.isActive = true;
    std::vector<UserStatus> localUsers = { local };
    std::vector<UserStatus> peerUsers(localUsers);
    EXPECT_CALL(*userDelegateMock, GetLocalUserStatus()).WillOnce(Return(localUsers));
    EXPECT_CALL(*userDelegateMock, GetRemoteUserStatus(_)).WillOnce(Return(peerUsers));
    EXPECT_CALL(*devMgrAdapterMock, IsSameAccount(_, _)).WillOnce(Return(true));
    auto result = authHandler->CheckAccess(localUserId, peerUserId, peerDevId, aclParams);
    EXPECT_TRUE(result.first);

    EXPECT_CALL(*userDelegateMock, GetLocalUserStatus()).WillOnce(Return(localUsers));
    EXPECT_CALL(*userDelegateMock, GetRemoteUserStatus(_)).WillOnce(Return(peerUsers));
    EXPECT_CALL(*devMgrAdapterMock, IsSameAccount(_, _)).WillOnce(Return(false));
    EXPECT_CALL(*devMgrAdapterMock, CheckAccessControl(_, _)).WillOnce(Return(true));
    result = authHandler->CheckAccess(localUserId, peerUserId, peerDevId, aclParams);
    EXPECT_TRUE(result.first);

    aclParams.accCaller.bundleName = "com.AuthDelegateMockTest.app";
    EXPECT_CALL(*userDelegateMock, GetLocalUserStatus()).WillOnce(Return(localUsers));
    EXPECT_CALL(*userDelegateMock, GetRemoteUserStatus(_)).WillOnce(Return(peerUsers));
    EXPECT_CALL(*devMgrAdapterMock, IsSameAccount(_, _)).WillOnce(Return(false));
    EXPECT_CALL(*devMgrAdapterMock, CheckAccessControl(_, _)).WillOnce(Return(false));
    result = authHandler->CheckAccess(localUserId, peerUserId, peerDevId, aclParams);
    EXPECT_FALSE(result.first);
}

/**
* @tc.name: CheckAccess003
* @tc.desc: Check access.
* @tc.type: FUNC
* @tc.require:
* @tc.author: caozhijun
*/
HWTEST_F(AuthDelegateMockTest, CheckAccess003, testing::ext::TestSize.Level0)
{
    ASSERT_NE(authHandler, nullptr);
    int localUserId = 1;
    int peerUserId = 1;
    std::string peerDevId = "CheckAccess003";
    AclParams aclParams;
    aclParams.authType = static_cast<int32_t>(DistributedKv::AuthType::IDENTICAL_ACCOUNT);
    UserStatus local;
    local.id = localUserId;
    local.isActive = true;
    std::vector<UserStatus> localUsers = { local };
    std::vector<UserStatus> peerUsers(localUsers);
    EXPECT_CALL(*userDelegateMock, GetLocalUserStatus()).WillOnce(Return(localUsers));
    EXPECT_CALL(*userDelegateMock, GetRemoteUserStatus(_)).WillOnce(Return(peerUsers));
    EXPECT_CALL(*devMgrAdapterMock, IsSameAccount(_, _)).WillOnce(Return(true));
    auto result = authHandler->CheckAccess(localUserId, peerUserId, peerDevId, aclParams);
    EXPECT_TRUE(result.first);

    aclParams.authType += 1;
    aclParams.accCaller.bundleName = "com.AuthDelegateTest.app";
    EXPECT_CALL(*userDelegateMock, GetLocalUserStatus()).WillOnce(Return(localUsers));
    EXPECT_CALL(*userDelegateMock, GetRemoteUserStatus(_)).WillOnce(Return(peerUsers));
    result = authHandler->CheckAccess(localUserId, peerUserId, peerDevId, aclParams);
    EXPECT_FALSE(result.second);
}
}
