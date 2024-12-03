/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "FeatureStubImplTest"

#include "auto_launch_export.h"
#include "bootstrap.h"
#include "error/general_error.h"
#include "executor_pool.h"
#include "feature_stub_impl.h"
#include "feature/feature_system.h"
#include "gtest/gtest.h"
#include "ipc_skeleton.h"
#include "iremote_stub.h"

using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
namespace DistributedDataTest {

class FeatureStubImplTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class MockFeature : public FeatureSystem::Feature {
public:
    int OnRemoteRequest(uint32_t code, OHOS::MessageParcel& data, OHOS::MessageParcel& reply) override
    {
        return E_OK;
    }
    int32_t OnInitialize() override
    {
        return E_OK;
    }
    int32_t OnBind(const BindInfo &bindInfo) override
    {
        return E_OK;
    }
    int32_t OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &bundleName) override
    {
        return E_OK;
    }
    int32_t OnAppUninstall(const std::string &bundleName, int32_t user, int32_t index) override
    {
        return E_OK;
    }
    int32_t OnAppUpdate(const std::string &bundleName, int32_t user, int32_t index) override
    {
        return E_OK;
    }
    int32_t OnAppInstall(const std::string &bundleName, int32_t user, int32_t index) override
    {
        return E_OK;
    }
    int32_t ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param) override
    {
        return E_OK;
    }
    int32_t OnUserChange(uint32_t code, const std::string &user, const std::string &account) override
    {
        return E_OK;
    }
    int32_t Online(const std::string &device) override
    {
        return E_OK;
    }
    int32_t Offline(const std::string &device) override
    {
        return E_OK;
    }
    int32_t OnReady(const std::string &device) override
    {
        return E_OK;
    }
    int32_t OnSessionReady(const std::string &device) override
    {
        return E_OK;
    }
    int32_t OnScreenUnlocked(int32_t user) override
    {
        return E_OK;
    }
};

/**
* @tc.name: OnRemoteRequest001
* @tc.desc: OnRemoteRequest function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnRemoteRequest001, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = nullptr;
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    uint32_t code = 0;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    auto result = featureStubImpl->OnRemoteRequest(code, data, reply, option);
    EXPECT_NE(result , E_OK);
}

/**
* @tc.name: OnRemoteRequest002
* @tc.desc: OnRemoteRequest function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnRemoteRequest002, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = std::make_shared<MockFeature>();
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    uint32_t code = 0;
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    auto result = featureStubImpl->OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(result , E_OK);
}

/**
* @tc.name: OnInitialize001
* @tc.desc: OnInitialize function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnInitialize001, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = nullptr;
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(1, 0);
    auto result = featureStubImpl->OnInitialize(executor);
    EXPECT_EQ(result , -1);
}

/**
* @tc.name: OnInitialize002
* @tc.desc: OnInitialize function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnInitialize002, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = std::make_shared<MockFeature>();
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(1, 0);
    auto result = featureStubImpl->OnInitialize(executor);
    EXPECT_EQ(result , E_OK);
}

/**
* @tc.name: OnAppExit001
* @tc.desc: OnAppExit function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnAppExit001, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = nullptr;
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    pid_t uid = 0;
    pid_t pid = 0;
    uint32_t tokenId = 0;
    std::string bundleName = "com.ohos.test";
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(1, 0);
    auto result = featureStubImpl->OnAppExit(uid, pid , tokenId , bundleName);
    EXPECT_EQ(result , -1);
}

/**
* @tc.name: OnAppExit002
* @tc.desc: OnAppExit function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnAppExit002, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = std::make_shared<MockFeature>();
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    pid_t uid = 0;
    pid_t pid = 0;
    uint32_t tokenId = 0;
    std::string bundleName = "com.ohos.test";
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(1, 0);
    auto result = featureStubImpl->OnAppExit(uid, pid , tokenId , bundleName);
    EXPECT_EQ(result , E_OK);
}

/**
* @tc.name: OnAppUninstall001
* @tc.desc: OnAppUninstall function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnAppUninstall001, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = nullptr;
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string bundleName = "com.ohos.test";
    int32_t user = 0;
    int32_t index = 0;
    auto result = featureStubImpl->OnAppUninstall(bundleName, user, index);
    EXPECT_EQ(result , -1);
}

/**
* @tc.name: OnAppUninstall002
* @tc.desc: OnAppUninstall function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnAppUninstall002, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = std::make_shared<MockFeature>();
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string bundleName = "com.ohos.test";
    int32_t user = 0;
    int32_t index = 0;
    auto result = featureStubImpl->OnAppUninstall(bundleName, user, index);
    EXPECT_EQ(result , E_OK);
}

/**
* @tc.name: OnAppUpdate001
* @tc.desc: OnAppUpdate function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnAppUpdate001, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = nullptr;
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string bundleName = "com.ohos.test";
    int32_t user = 0;
    int32_t index = 0;
    auto result = featureStubImpl->OnAppUpdate(bundleName, user, index);
    EXPECT_EQ(result , -1);
}

/**
* @tc.name: OnAppUpdate002
* @tc.desc: OnAppUpdate function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnAppUpdate002, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = std::make_shared<MockFeature>();
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string bundleName = "com.ohos.test";
    int32_t user = 0;
    int32_t index = 0;
    auto result = featureStubImpl->OnAppUpdate(bundleName, user, index);
    EXPECT_EQ(result , E_OK);
}

/**
* @tc.name: OnAppInstall001
* @tc.desc: OnAppInstall function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnAppInstall001, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = nullptr;
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string bundleName = "com.ohos.test";
    int32_t user = 0;
    int32_t index = 0;
    auto result = featureStubImpl->OnAppInstall(bundleName, user, index);
    EXPECT_EQ(result , -1);
}

/**
* @tc.name: OnAppInstall002
* @tc.desc: OnAppInstall function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnAppInstall002, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = std::make_shared<MockFeature>();
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string bundleName = "com.ohos.test";
    int32_t user = 0;
    int32_t index = 0;
    auto result = featureStubImpl->OnAppInstall(bundleName, user, index);
    EXPECT_EQ(result , E_OK);
}

/**
* @tc.name: ResolveAutoLaunch001
* @tc.desc: ResolveAutoLaunch function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, ResolveAutoLaunch001, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = nullptr;
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string identifier = "identifier";
    DistributedDB::AutoLaunchParam param;
    auto result = featureStubImpl->ResolveAutoLaunch(identifier, param);
    EXPECT_EQ(result , -1);
}

/**
* @tc.name: ResolveAutoLaunch002
* @tc.desc: ResolveAutoLaunch function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, ResolveAutoLaunch002, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = std::make_shared<MockFeature>();
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string identifier = "identifier";
    DistributedDB::AutoLaunchParam param;
    auto result = featureStubImpl->ResolveAutoLaunch(identifier, param);
    EXPECT_EQ(result , E_OK);
}

/**
* @tc.name: OnUserChange001
* @tc.desc: OnUserChange function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnUserChange001, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = nullptr;
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    uint32_t code = 0;
    std::string user = "user";
    std::string account = "account";
    auto result = featureStubImpl->OnUserChange(code, user, account);
    EXPECT_EQ(result , -1);
}

/**
* @tc.name: OnUserChange002
* @tc.desc: OnUserChange function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnUserChange002, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = std::make_shared<MockFeature>();
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    uint32_t code = 0;
    std::string user = "user";
    std::string account = "account";
    auto result = featureStubImpl->OnUserChange(code, user, account);
    EXPECT_EQ(result , E_OK);
}

/**
* @tc.name: Online001
* @tc.desc: Online function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, Online001, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = nullptr;
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string device = "device";
    auto result = featureStubImpl->Online(device);
    EXPECT_EQ(result , -1);
}

/**
* @tc.name: Online002
* @tc.desc: Online function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, Online002, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = std::make_shared<MockFeature>();
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string device = "device";
    auto result = featureStubImpl->Online(device);
    EXPECT_EQ(result , E_OK);
}

/**
* @tc.name: Offline001
* @tc.desc: Offline function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, Offline001, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = nullptr;
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string device = "device";
    auto result = featureStubImpl->Offline(device);
    EXPECT_EQ(result , -1);
}

/**
* @tc.name: Offline002
* @tc.desc: Offline function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, Offline002, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = std::make_shared<MockFeature>();
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string device = "device";
    auto result = featureStubImpl->Offline(device);
    EXPECT_EQ(result , E_OK);
}

/**
* @tc.name: OnReady001
* @tc.desc: OnReady function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnReady001, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = nullptr;
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string device = "device";
    auto result = featureStubImpl->OnReady(device);
    EXPECT_EQ(result , -1);
}

/**
* @tc.name: OnReady002
* @tc.desc: OnReady function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnReady002, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = std::make_shared<MockFeature>();
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string device = "device";
    auto result = featureStubImpl->OnReady(device);
    EXPECT_EQ(result , E_OK);
}

/**
* @tc.name: OnSessionReady001
* @tc.desc: OnSessionReady function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnSessionReady001, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = nullptr;
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string device = "device";
    auto result = featureStubImpl->OnSessionReady(device);
    EXPECT_EQ(result , -1);
}

/**
* @tc.name: OnSessionReady002
* @tc.desc: OnSessionReady function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnSessionReady002, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = std::make_shared<MockFeature>();
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    std::string device = "device";
    auto result = featureStubImpl->OnSessionReady(device);
    EXPECT_EQ(result , E_OK);
}

/**
* @tc.name: OnScreenUnlocked001
* @tc.desc: OnScreenUnlocked function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnScreenUnlocked001, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = nullptr;
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    int32_t user = 0;
    auto result = featureStubImpl->OnScreenUnlocked(user);
    EXPECT_EQ(result , -1);
}

/**
* @tc.name: OnScreenUnlocked002
* @tc.desc: OnScreenUnlocked function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(FeatureStubImplTest, OnScreenUnlocked002, TestSize.Level1)
{
    std::shared_ptr<FeatureSystem::Feature> feature = std::make_shared<MockFeature>();
    std::shared_ptr<FeatureStubImpl> featureStubImpl = std::make_shared<FeatureStubImpl>(feature);
    int32_t user = 0;
    auto result = featureStubImpl->OnScreenUnlocked(user);
    EXPECT_EQ(result , E_OK);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test