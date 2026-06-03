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

#define LOG_TAG "LifeCycleManagerTest"

#include <gtest/gtest.h>
#include "lifecycle_manager.h"

namespace OHOS::UDMF {
using namespace testing::ext;

constexpr size_t NUM_MIN = 1;
constexpr size_t NUM_MAX = 3;
class LifeCycleManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

/**
* @tc.name: OnGot001
* @tc.desc: Abnormal test of OnGot, executors_ is nullptr
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(LifeCycleManagerTest, OnGot001, TestSize.Level1)
{
    std::shared_ptr<ExecutorPool> executors = nullptr;
    LifeCycleManager lifeCycleManager;
    lifeCycleManager.SetThreadPool(executors);
    UnifiedKey key;
    key.key = "123456";
    key.intention = UD_INTENTION_MAP.at(UD_INTENTION_DRAG);
    key.bundleName = "com.xxx";
    Status status = lifeCycleManager.OnGot(key, 0, false);
    EXPECT_EQ(E_ERROR, status);
    EXPECT_EQ(0, lifeCycleManager.udKeys_.Size());
}

/**
* @tc.name: OnGot002
* @tc.desc: Abnormal test of OnGot, intention is invalid
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(LifeCycleManagerTest, OnGot002, TestSize.Level1)
{
    LifeCycleManager::GetInstance().SetThreadPool(std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN));
    UnifiedKey key;
    key.key = "123456";
    key.intention = "BUTT";
    key.bundleName = "com.xxx";
    Status status = LifeCycleManager::GetInstance().OnGot(key, 0, false);
    EXPECT_EQ(E_INVALID_PARAMETERS, status);
    EXPECT_EQ(0, LifeCycleManager::GetInstance().udKeys_.Size());
}

/**
* @tc.name: OnGot003
* @tc.desc: Normal test of OnGot, intention is DataHub
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(LifeCycleManagerTest, OnGot003, TestSize.Level1)
{
    LifeCycleManager::GetInstance().SetThreadPool(std::make_shared<ExecutorPool>(1, 1));
    bool result = LifeCycleManager::GetInstance().InsertUdKey(0, "udmf://");
    EXPECT_TRUE(result);
    EXPECT_EQ(1, LifeCycleManager::GetInstance().udKeys_.Size());
    UnifiedKey key;
    key.key = "udmf://";
    key.intention = "DataHub";
    key.bundleName = "com.xxx";
    Status status = LifeCycleManager::GetInstance().OnGot(key, 0, true);
    EXPECT_EQ(E_OK, status);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(1, LifeCycleManager::GetInstance().udKeys_.Size());
    LifeCycleManager::GetInstance().ClearUdKeys();
    EXPECT_EQ(0, LifeCycleManager::GetInstance().udKeys_.Size());
}

/**
* @tc.name: OnGot004
* @tc.desc: Normal test of OnGot, intention is drag
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(LifeCycleManagerTest, OnGot004, TestSize.Level1)
{
    LifeCycleManager::GetInstance().SetThreadPool(std::make_shared<ExecutorPool>(1, 1));
    EXPECT_TRUE(LifeCycleManager::GetInstance().InsertUdKey(0, "udmf://1"));
    EXPECT_TRUE(LifeCycleManager::GetInstance().InsertUdKey(0, "udmf://2"));
    EXPECT_EQ(1, LifeCycleManager::GetInstance().udKeys_.Size());
    UnifiedKey key;
    key.key = "udmf://";
    key.intention = "drag";
    key.bundleName = "com.xxx";
    Status status = LifeCycleManager::GetInstance().OnGot(key, 0, false);
    EXPECT_EQ(E_OK, status);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(1, LifeCycleManager::GetInstance().udKeys_.Size());
    LifeCycleManager::GetInstance().ClearUdKeys();
    EXPECT_EQ(0, LifeCycleManager::GetInstance().udKeys_.Size());
}

/**
* @tc.name: OnAppUninstall001
* @tc.desc: Abnormal test of OnAppUninstall
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(LifeCycleManagerTest, OnAppUninstall001, TestSize.Level1)
{
    UnifiedKey key1("drag", "com.demo.test", "123");
    EXPECT_TRUE(LifeCycleManager::GetInstance().InsertUdKey(0, key1.GetUnifiedKey()));
    UnifiedKey key2("drag", "com.demo.test", "456");
    EXPECT_TRUE(LifeCycleManager::GetInstance().InsertUdKey(1, key2.GetUnifiedKey()));
    EXPECT_EQ(2, LifeCycleManager::GetInstance().udKeys_.Size());
    Status status = LifeCycleManager::GetInstance().OnAppUninstall(0);
    EXPECT_EQ(E_OK, status);
    EXPECT_EQ(2, LifeCycleManager::GetInstance().udKeys_.Size());
    LifeCycleManager::GetInstance().ClearUdKeys();
    EXPECT_EQ(0, LifeCycleManager::GetInstance().udKeys_.Size());
}
}; // namespace UDMF