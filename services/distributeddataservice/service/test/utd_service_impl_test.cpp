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
#define LOG_TAG "UtdServiceImplTest"
#include "utd_service_impl.h"

#include <gtest/gtest.h>

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "bootstrap.h"
#include "device_manager_adapter.h"
#include "executor_pool.h"
#include "ipc_skeleton.h"
#include "kvstore_meta_manager.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

using namespace testing::ext;
using namespace OHOS::Security::AccessToken;
using namespace OHOS::UDMF;
using namespace OHOS::DistributedData;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
namespace OHOS::Test {
namespace DistributedDataTest {
constexpr const char *HAP_BUNDLE_NAME1 = "ohos.utd_service_test.demo1";
constexpr const char *MANAGE_DYNAMIC_UTD_TYPE = "ohos.permission.MANAGE_DYNAMIC_UTD_TYPE";

class UtdServiceImplTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        size_t max = 2;
        size_t min = 1;
        executors_ = std::make_shared<OHOS::ExecutorPool>(max, min);
    }
    static void TearDownTestCase(void)
    {
        DeleteTestHapToken();
    }
    void SetUp(){};
    void TearDown(){};
    static void AllocTestHapToken1();
    static void DeleteTestHapToken();

    static constexpr uint32_t userId = 100;
    static constexpr int instIndex = 0;
    static std::shared_ptr<ExecutorPool> executors_;
};
std::shared_ptr<ExecutorPool> UtdServiceImplTest::executors_ = nullptr;

void UtdServiceImplTest::AllocTestHapToken1()
{
    HapInfoParams info = {
        .userID = userId,
        .bundleName = HAP_BUNDLE_NAME1,
        .instIndex = instIndex,
        .appIDDesc = HAP_BUNDLE_NAME1
    };

    HapPolicyParams policy = {
        .apl = APL_NORMAL,
        .domain = "test.domain",
        .permList = {
            {
                .permissionName = "ohos.permission.test",
                .bundleName = HAP_BUNDLE_NAME1,
                .grantMode = 1,
                .availableLevel = APL_NORMAL,
                .label = "label",
                .labelId = 1,
                .description = "test1",
                .descriptionId = 1
            }
        },
        .permStateList = {
            {
                .permissionName = "ohos.permission.test",
                .isGeneral = true,
                .resDeviceID = { "local" },
                .grantStatus = { PermissionState::PERMISSION_GRANTED },
                .grantFlags = { 1 }
            }
        }
    };
    auto tokenID = AccessTokenKit::AllocHapToken(info, policy);
    SetSelfTokenID(tokenID.tokenIDEx);
}
void UtdServiceImplTest::DeleteTestHapToken()
{
    auto tokenId = AccessTokenKit::GetHapTokenID(userId, HAP_BUNDLE_NAME1, instIndex);
    AccessTokenKit::DeleteToken(tokenId);
}

/**
* @tc.name: RegServiceNotifier001
* @tc.desc: Abnormal test of RegServiceNotifier, iUtdNotifier is nullptr
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UtdServiceImplTest, RegServiceNotifier001, TestSize.Level1)
{
    UtdServiceImpl utdServiceImpl;
    EXPECT_EQ(utdServiceImpl.foundationTokenId_, 0);
    utdServiceImpl.RegServiceNotifier(nullptr);
    EXPECT_EQ(utdServiceImpl.foundationTokenId_, 0);
}

/**
* @tc.name: RegisterTypeDescriptors001
* @tc.desc: Abnormal test of RegisterTypeDescriptors
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UtdServiceImplTest, RegisterTypeDescriptors001, TestSize.Level1)
{
    UtdServiceImpl utdServiceImpl;
    std::vector<TypeDescriptorCfg> descriptors;
    int32_t ret = utdServiceImpl.RegisterTypeDescriptors(descriptors);
    EXPECT_TRUE(ret == E_NO_SYSTEM_PERMISSION || ret == E_NO_PERMISSION);
}

/**
* @tc.name: UnregisterTypeDescriptors001
* @tc.desc: Abnormal test of UnregisterTypeDescriptors001
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UtdServiceImplTest, UnregisterTypeDescriptors001, TestSize.Level1)
{
    UtdServiceImpl utdServiceImpl;
    std::vector<std::string> typeIds;
    int32_t ret = utdServiceImpl.UnregisterTypeDescriptors(typeIds);
    EXPECT_TRUE(ret == E_NO_SYSTEM_PERMISSION || ret == E_NO_PERMISSION);
}

/**
* @tc.name: OnBind001
* @tc.desc: Normal test of OnBind
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UtdServiceImplTest, OnBind001, TestSize.Level1)
{
    EXPECT_NE(executors_, nullptr);
    UtdServiceImpl utdServiceImpl;
    FeatureSystem::Feature::BindInfo bindInfo;
    bindInfo.executors = executors_;
    utdServiceImpl.OnBind(bindInfo);
    EXPECT_NE(utdServiceImpl.executors_, nullptr);
}

/**
* @tc.name: GetHapBundleNameByToken001
* @tc.desc: Normal test of GetHapBundleNameByToken
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UtdServiceImplTest, GetHapBundleNameByToken001, TestSize.Level1)
{
    AllocTestHapToken1();
    UtdServiceImpl utdServiceImpl;
    auto tokenId = AccessTokenKit::GetHapTokenID(userId, HAP_BUNDLE_NAME1, instIndex);
    std::string bundleName;
    bool ret = utdServiceImpl.GetHapBundleNameByToken(tokenId, bundleName);
    EXPECT_TRUE(ret);
    EXPECT_EQ(bundleName, HAP_BUNDLE_NAME1);
}

/**
* @tc.name: GetHapBundleNameByToken002
* @tc.desc: Abnormal test of GetHapBundleNameByToken
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UtdServiceImplTest, GetHapBundleNameByToken002, TestSize.Level1)
{
    UtdServiceImpl utdServiceImpl;
    uint32_t tokenId = 0;
    std::string bundleName;
    bool ret = utdServiceImpl.GetHapBundleNameByToken(tokenId, bundleName);
    EXPECT_FALSE(ret);
}

/**
* @tc.name: VerifyPermission001
* @tc.desc: Abnormal test of VerifyPermission
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UtdServiceImplTest, VerifyPermission001, TestSize.Level1)
{
    UtdServiceImpl utdServiceImpl;
    std::string permission = "";
    uint32_t tokenId = 0;
    bool ret = utdServiceImpl.VerifyPermission(permission, tokenId);
    EXPECT_FALSE(ret);
}

/**
* @tc.name: VerifyPermission002
* @tc.desc: Abnormal test of VerifyPermission
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UtdServiceImplTest, VerifyPermission002, TestSize.Level1)
{
    UtdServiceImpl utdServiceImpl;
    uint32_t tokenId = 0;
    bool ret = utdServiceImpl.VerifyPermission(MANAGE_DYNAMIC_UTD_TYPE, tokenId);
    EXPECT_FALSE(ret);
}

}; // namespace DistributedDataTest
}; // namespace OHOS::Test
