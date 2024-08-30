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

#define LOG_TAG "ObjectDmsHandlerTest"

#include "object_dms_handler.h"

#include <gtest/gtest.h>

#include "dms_listener_stub.h"

using namespace testing::ext;
using namespace OHOS::DistributedObject;
namespace OHOS::Test {
class ObjectDmsHandlerTest : public testing::Test {
public:
    void SetUp() {}
    void TearDown() {}
};

/**
* @tc.name: IsContinueTest_001
* @tc.desc: IsContinue test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectDmsHandlerTest, IsContinue_001, TestSize.Level0)
{
    ObjectDmsHandler::GetInstance().RegisterDmsEvent();
    DmsEventListener listener;
    DistributedSchedule::EventNotify notify;
    notify.dSchedEventType_ = DistributedSchedule::DSchedEventType::DMS_CONTINUE;
    notify.srcNetworkId_ = "srcNetworkId";
    notify.dstNetworkId_ = "dstNetworkId";
    notify.srcBundleName_ = "srcBundleName";
    notify.destBundleName_ = "destBundleName";
    listener.DSchedEventNotify(notify);
    auto res = ObjectDmsHandler::GetInstance().IsContinue("srcNetworkId", "srcBundleName");
    EXPECT_TRUE(res);
    res = ObjectDmsHandler::GetInstance().IsContinue("dstNetworkId", "destBundleName");
    EXPECT_TRUE(res);
    res = ObjectDmsHandler::GetInstance().IsContinue("srcNetworkId", "destBundleName");
    EXPECT_FALSE(res);
    ObjectDmsHandler::GetInstance().dmsEvents_.clear();
}

/**
* @tc.name: ReceiveDmsEvent_001
* @tc.desc: IsContinue test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectDmsHandlerTest, ReceiveDmsEvent_001, TestSize.Level0)
{
    ObjectDmsHandler::GetInstance().RegisterDmsEvent();
    DmsEventListener listener;
    for (int i = 0; i <= 20; i++) {
        DistributedSchedule::EventNotify notify;
        notify.dSchedEventType_ = DistributedSchedule::DSchedEventType::DMS_CONTINUE;
        notify.srcNetworkId_ = "srcNetworkId" + std::to_string(i);
        notify.dstNetworkId_ = "dstNetworkId" + std::to_string(i);
        notify.srcBundleName_ = "srcBundleName" + std::to_string(i);
        notify.destBundleName_ = "destBundleName" + std::to_string(i);
        ObjectDmsHandler::GetInstance().ReceiveDmsEvent(notify);
    }
    EXPECT_EQ(ObjectDmsHandler::GetInstance().dmsEvents_.size(), 20);
    EXPECT_EQ(ObjectDmsHandler::GetInstance().dmsEvents_.front().first.srcNetworkId_, "srcNetworkId1");
    EXPECT_EQ(ObjectDmsHandler::GetInstance().dmsEvents_.back().first.srcNetworkId_, "srcNetworkId20");
    ObjectDmsHandler::GetInstance().dmsEvents_.clear();
}
} // namespace OHOS::Test
