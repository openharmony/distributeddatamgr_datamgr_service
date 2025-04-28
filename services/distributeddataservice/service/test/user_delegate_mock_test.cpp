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
#include "user_delegate.h"
#include "device_manager_adapter_mock.h"
#include "account_delegate_mock.h"
#include "metadata/meta_data_manager.h"

using namespace OHOS::DistributedData;
using namespace testing::ext;
using namespace testing;
using namespace std;
bool MetaDataManager::Subscribe(std::string prefix, Observer observer, bool isLocal)
{
    if (observer) {
        for (int i = 0; i <= (MetaDataManager::DELETE + 1); ++i) {
            static int32_t flag = MetaDataManager::INSERT;
            static std::string key = prefix + "1";
            static std::string value = "";
            observer(key, value, flag);
            ++flag;
        }
        return true;
    }
    return false;
}

bool MetaDataManager::SaveMeta(const std::string &key, const Serializable &value, bool isLocal)
{
    return true;
}

namespace OHOS::Test {
namespace DistributedDataTest {
class UserDelegateMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() {}
    void TearDown() {}
    static inline shared_ptr<DeviceManagerAdapterMock> devMgrAdapterMock = nullptr;
};

void UserDelegateMockTest::SetUpTestCase(void)
{
    devMgrAdapterMock = make_shared<DeviceManagerAdapterMock>();
    BDeviceManagerAdapter::deviceManagerAdapter = devMgrAdapterMock;
}

void UserDelegateMockTest::TearDownTestCase(void)
{
    BDeviceManagerAdapter::deviceManagerAdapter = nullptr;
    devMgrAdapterMock = nullptr;
}

/**
* @tc.name: GetLocalUserStatus
* @tc.desc: test GetLocalUserStatus normal branch.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UserDelegateMockTest, GetLocalUserStatus, TestSize.Level0)
{
    DeviceInfo devInfo = { .uuid = "AFAGA45WF3663FAGA" };
    EXPECT_CALL(*devMgrAdapterMock, GetLocalDevice()).WillOnce(Return(devInfo));
    std::vector<UserStatus> vec = UserDelegate::GetInstance().GetLocalUserStatus();
    EXPECT_TRUE(vec.empty());
}

/**
* @tc.name: InitLocalUserMeta
* @tc.desc: test InitLocalUserMeta
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UserDelegateMockTest, InitLocalUserMeta, TestSize.Level0)
{
    EXPECT_CALL(AccountDelegateMock::Init(), QueryUsers(_)).WillOnce(Return(false));
    bool ret = UserDelegate::GetInstance().InitLocalUserMeta();
    EXPECT_FALSE(ret);

    std::vector<int> users;
    EXPECT_CALL(AccountDelegateMock::Init(), QueryUsers(_))
        .WillRepeatedly(DoAll(SetArgReferee<0>(users), Return(true)));
    ret = UserDelegate::GetInstance().InitLocalUserMeta();
    UserDelegate::GetInstance().DeleteUsers("users");
    EXPECT_FALSE(ret);
}

/**
* @tc.name: GetLocalUsers
* @tc.desc: test GetLocalUsers
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UserDelegateMockTest, GetLocalUsers, TestSize.Level0)
{
    DeviceInfo devInfo = { .uuid = "TESTGHJJ46785FAGA9323FGAGATEST" };
    EXPECT_CALL(*devMgrAdapterMock, GetLocalDevice()).WillOnce(Return(devInfo));
    std::map<int, bool> value;
    bool ret = UserDelegate::GetInstance().deviceUser_.Insert(devInfo.uuid, value);
    EXPECT_TRUE(ret);
    std::set<std::string> user = UserDelegate::GetInstance().GetLocalUsers();
    EXPECT_TRUE(user.empty());
}

/**
* @tc.name: GetUsers
* @tc.desc: test GetUsers
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UserDelegateMockTest, GetUsers, TestSize.Level0)
{
    std::string deviceId = "45677afatghfrttWISKKM";
    std::map<int, bool> val;
    bool ret = UserDelegate::GetInstance().deviceUser_.Insert(deviceId, val);
    EXPECT_TRUE(ret);
    std::vector<UserStatus> statusVec = UserDelegate::GetInstance().GetUsers(deviceId);
    EXPECT_TRUE(statusVec.empty());
}

/**
* @tc.name: Init
* @tc.desc: test Init
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UserDelegateMockTest, Init, TestSize.Level0)
{
    EXPECT_CALL(AccountDelegateMock::Init(), Subscribe(_)).WillRepeatedly(Return(0));
    DeviceInfo devInfo = { .uuid = "HJJ4FGAGAAGA45WF3663FAGA" };
    EXPECT_CALL(*devMgrAdapterMock, GetLocalDevice()).WillRepeatedly(Return(devInfo));
    std::shared_ptr<ExecutorPool> poolPtr = std::make_shared<ExecutorPool>(0, 1);
    ASSERT_NE(poolPtr, nullptr);
    UserDelegate::GetInstance().executors_ = poolPtr;
    ASSERT_NE(UserDelegate::GetInstance().executors_, nullptr);

    std::vector<int> users = { 0 };
    EXPECT_CALL(AccountDelegateMock::Init(), QueryUsers(_))
        .WillRepeatedly(([&users](std::vector<int> &out) {
            out = users;
        }));
    EXPECT_TRUE(UserDelegate::GetInstance().InitLocalUserMeta());
    UserDelegate::GetInstance().Init(poolPtr);
    ASSERT_TRUE(UserDelegate::GetInstance().executors_ != nullptr);
}
}
}