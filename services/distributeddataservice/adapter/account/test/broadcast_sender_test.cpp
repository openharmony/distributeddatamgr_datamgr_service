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
#define LOG_TAG "BroadcastSenderTest"

#include "broadcast_sender.h"
#include "broadcast_sender_impl.h"
#include <gtest/gtest.h>
#include "log_print.h"
#include <memory>

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DistributedKv;
class BroadcastSenderTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class BroadcastSenderImplTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: GetInstance001
* @tc.desc: test GetInstance function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(BroadcastSenderTest, GetInstance001, TestSize.Level0)
{
    std::shared_ptr<BroadcastSender> instance = DistributedKv::BroadcastSender::GetInstance();
    EXPECT_NE(instance, nullptr);
}

/**
* @tc.name: GetInstance002
* @tc.desc: test GetInstance function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(BroadcastSenderTest, GetInstance002, TestSize.Level0)
{
    std::shared_ptr<BroadcastSender> instance1 = DistributedKv::BroadcastSender::GetInstance();
    std::shared_ptr<BroadcastSender> instance2 = DistributedKv::BroadcastSender::GetInstance();
    EXPECT_EQ(instance1, instance2);
}

/**
* @tc.name: SendEvent
* @tc.desc: test SendEvent function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(BroadcastSenderImplTest, SendEvent, TestSize.Level0)
{
    std::shared_ptr<BroadcastSender> instance = DistributedKv::BroadcastSender::GetInstance();
    EXPECT_NE(instance, nullptr);
    EventParams params = {};
    EXPECT_NO_FATAL_FAILURE(instance->SendEvent(params));
}
} // namespace OHOS::Test