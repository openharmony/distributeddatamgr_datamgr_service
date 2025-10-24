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
    Status status = lifeCycleManager.OnGot(key);
    EXPECT_EQ(E_ERROR, status);
}
}; // namespace UDMF