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
#define LOG_TAG "DataShareSysEventSubscriberTest"

#include "sys_event_subscriber.h"

#include <gtest/gtest.h>

#include "data_share_service_impl.h"
#include "common_event_manager.h"
#include "common_event_support.h"
#include "log_print.h"
#include "mock/meta_data_manager_mock.h"
#include "proxy_data_manager.h"

namespace OHOS::Test {
using namespace OHOS::DataShare;
using namespace testing::ext;
class DataShareSysEventSubscriberTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: OnBMSReady001
* @tc.desc: test OnBMSReady func
* @tc.type: FUNC
*/
HWTEST_F(DataShareSysEventSubscriberTest, OnBMSReady001, TestSize.Level1)
{
    ZLOGI("DataShareSysEventSubscriberTest OnBMSReady001 start");
    EventFwk::MatchingSkills testMatchingSkills;
    testMatchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_BUNDLE_SCAN_FINISHED);
    testMatchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_ADDED);
    testMatchingSkills.AddEvent(EventFwk::CommonEventSupport::COMMON_EVENT_PACKAGE_REMOVED);
    EventFwk::CommonEventSubscribeInfo subscribeInfo(testMatchingSkills);
    subscribeInfo.SetThreadMode(EventFwk::CommonEventSubscribeInfo::COMMON);
    // executors not null
    auto executors = std::make_shared<ExecutorPool>(1, 0);
    // make sysEventSubscriber not null
    auto sysEventSubscriber = std::make_shared<SysEventSubscriber>(subscribeInfo, executors);
    ASSERT_NE(sysEventSubscriber, nullptr);

    // OnBMSReady is void-typed and const; test both normal/error cases.
    sysEventSubscriber->OnBMSReady();
    ASSERT_NE(sysEventSubscriber->executors_, nullptr);
    // cover executors == nullptr branch
    sysEventSubscriber->executors_ = nullptr;
    sysEventSubscriber->OnBMSReady();
    ZLOGI("DataShareSysEventSubscriberTest OnBMSReady001 end");
}
} // namespace OHOS::Test