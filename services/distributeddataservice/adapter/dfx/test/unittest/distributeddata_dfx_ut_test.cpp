/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include <string>
#include "dfx/reporter.h"
#include "fake_hiview.h"
#include "value_hash.h"

using namespace testing::ext;
using namespace OHOS::DistributedDataDfx;

class DistributeddataDfxUTTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp();

    void TearDown();
    static Reporter* reporter_;
};

Reporter* DistributeddataDfxUTTest::reporter_ = nullptr;

void DistributeddataDfxUTTest::SetUpTestCase()
{
    FakeHivew::Clear();
    size_t max = 12;
    size_t min = 5;
    reporter_ = Reporter::GetInstance();
    ASSERT_NE(nullptr, reporter_);
    reporter_->SetThreadPool(std::make_shared<OHOS::ExecutorPool>(max, min));
}

void DistributeddataDfxUTTest::TearDownTestCase()
{
    reporter_->SetThreadPool(nullptr);
    reporter_ = nullptr;
    FakeHivew::Clear();
}

void DistributeddataDfxUTTest::SetUp() {}

void DistributeddataDfxUTTest::TearDown() {}

/**
  * @tc.name: Dfx001
  * @tc.desc: send data to 1 device, then check reporter message.
  * @tc.type: send data
  * @tc.author: nhj
  */
HWTEST_F(DistributeddataDfxUTTest, Dfx001, TestSize.Level0)
{
     /**
     * @tc.steps: step1. get database fault report instance
     * @tc.expected: step1. Expect get instance success.
     */
    auto behavior = reporter_->GetBehaviourReporter();
    EXPECT_NE(nullptr, behavior);
    struct BehaviourMsg msg{.userId = "user008", .appId = "myApp08", .storeId = "storeTest08",
                            .behaviourType = BehaviourType::DATABASE_BACKUP, .extensionInfo="test111"};

    auto repStatus = behavior->Report(msg);
    EXPECT_TRUE(repStatus == ReportStatus::SUCCESS);
    /**
    * @tc.steps:step2. check dfx reporter.
    * @tc.expected: step2. Expect report message success.
    */
    std::string val = FakeHivew::GetString("ANONYMOUS_UID");
    if (!val.empty()) {
        EXPECT_STREQ(val.c_str(), string("user008").c_str());
    }
    val = FakeHivew::GetString("APP_ID");
    if (!val.empty()) {
        EXPECT_STREQ(val.c_str(), string("myApp08").c_str());
    }
    val = FakeHivew::GetString("STORE_ID");
    if (!val.empty()) {
        EXPECT_STREQ(val.c_str(), string("storeTest08").c_str());
    }
    FakeHivew::Clear();
}

/**
  * @tc.name: Dfx002
  * @tc.desc: Send UdmfBehaviourMsg
  * @tc.type: Send data
  * @tc.author: nhj
  */
HWTEST_F(DistributeddataDfxUTTest, Dfx002, TestSize.Level0)
{
     /**
     * @tc.steps: step1. get database fault report instance
     * @tc.expected: step1. Expect get instance success.
     */
    auto UdmfBehavior = reporter_->GetBehaviourReporter();
    EXPECT_NE(nullptr, UdmfBehavior);
    struct UdmfBehaviourMsg UdMsg{"myApp", "channel", 200, "dataType", "operation", "result"};
    auto repStatus = UdmfBehavior->UDMFReport(UdMsg);
    EXPECT_TRUE(repStatus == ReportStatus::SUCCESS);
    /**
    * @tc.steps:step2. check dfx reporter.
    * @tc.expected: step2. Expect report message success.
    */
    auto val = FakeHivew::GetString("APP_ID");
    if (!val.empty()) {
        EXPECT_STREQ(val.c_str(), string("myApp").c_str());
    }
    auto val01 = FakeHivew::GetString("CHANNEL");
    if (!val01.empty()) {
        EXPECT_STREQ(val01.c_str(), string("channel").c_str());
    }
    auto val02 = FakeHivew::GetString("OPERATION");
    if (!val02.empty()) {
        EXPECT_STREQ(val02.c_str(), string("operation").c_str());
    }
    auto val03 = FakeHivew::GetString("RESULT");
    if (!val03.empty()) {
        EXPECT_STREQ(val03.c_str(), string("result").c_str());
    }
    auto val04 = FakeHivew::GetInt("DB_SIZE");
    if (val04 > 0) {
        EXPECT_EQ(200, val04);
    }
    FakeHivew::Clear();
}