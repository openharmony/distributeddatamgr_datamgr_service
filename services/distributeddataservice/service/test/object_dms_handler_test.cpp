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

#include "device_manager_adapter.h"
#include "dms_listener_stub.h"

using namespace testing::ext;
using namespace OHOS::DistributedObject;
namespace OHOS::Test {
constexpr const char *PKG_NAME = "ohos.distributeddata.service";
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
    DistributedHardware::DmDeviceInfo localDeviceInfo;
    DistributedHardware::DeviceManager::GetInstance().GetLocalDeviceInfo(PKG_NAME, localDeviceInfo);
    std::string localNetworkId = localDeviceInfo.networkId;
    DistributedSchedule::EventNotify notify;
    notify.dSchedEventType_ = DistributedSchedule::DSchedEventType::DMS_CONTINUE;
    notify.srcNetworkId_ = localNetworkId;
    notify.dstNetworkId_ = "networkId2";
    notify.srcBundleName_ = "bundleName1";
    notify.destBundleName_ = "bundleName2";
    DmsEventListener listener;
    listener.DSchedEventNotify(notify);
    auto res = ObjectDmsHandler::GetInstance().IsContinue("bundleName1");
    EXPECT_TRUE(res);
    res = ObjectDmsHandler::GetInstance().IsContinue("bundleName2");
    EXPECT_FALSE(res);
    ObjectDmsHandler::GetInstance().dmsEvents_.clear();
}

/**
* @tc.name: IsContinueTest_002
* @tc.desc: IsContinue test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectDmsHandlerTest, IsContinue_002, TestSize.Level0)
{
    ObjectDmsHandler::GetInstance().RegisterDmsEvent();
    DistributedHardware::DmDeviceInfo localDeviceInfo;
    DistributedHardware::DeviceManager::GetInstance().GetLocalDeviceInfo(PKG_NAME, localDeviceInfo);
    std::string localNetworkId = localDeviceInfo.networkId;
    DistributedSchedule::EventNotify notify;
    notify.dSchedEventType_ = DistributedSchedule::DSchedEventType::DMS_CONTINUE;
    notify.srcNetworkId_ = "networkId1";
    notify.dstNetworkId_ = localNetworkId;
    notify.srcBundleName_ = "bundleName1";
    notify.destBundleName_ = "bundleName2";
    DmsEventListener listener;
    listener.DSchedEventNotify(notify);
    auto res = ObjectDmsHandler::GetInstance().IsContinue("bundleName1");
    EXPECT_FALSE(res);
    res = ObjectDmsHandler::GetInstance().IsContinue("bundleName2");
    EXPECT_TRUE(res);
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

/**
* @tc.name: GetDstBundleName_001
* @tc.desc: IsContinue test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectDmsHandlerTest, GetDstBundleName_001, TestSize.Level0)
{
    DistributedHardware::DmDeviceInfo localDeviceInfo;
    DistributedHardware::DeviceManager::GetInstance().GetLocalDeviceInfo(PKG_NAME, localDeviceInfo);
    std::string localNetworkId = localDeviceInfo.networkId;
    
    std::string srcNetworkId = localNetworkId;
    std::string srcBundleName = "bundleName1";
    std::string dstNetworkId = "networkId2";
    std::string destBundleName = "bundleName2";

    auto res = ObjectDmsHandler::GetInstance().GetDstBundleName(srcBundleName, dstNetworkId);
    EXPECT_EQ(res, srcBundleName);

    DistributedSchedule::EventNotify notify;
    notify.dSchedEventType_ = DistributedSchedule::DSchedEventType::DMS_CONTINUE;
    notify.srcNetworkId_ = srcNetworkId;
    notify.dstNetworkId_ = dstNetworkId;
    notify.srcBundleName_ = srcBundleName;
    notify.destBundleName_ = destBundleName;
    ObjectDmsHandler::GetInstance().ReceiveDmsEvent(notify);

    res = ObjectDmsHandler::GetInstance().GetDstBundleName(srcBundleName, dstNetworkId);
    EXPECT_EQ(res, destBundleName);
    
    ObjectDmsHandler::GetInstance().dmsEvents_.clear();

    auto timestamp = std::chrono::steady_clock::now() - std::chrono::seconds(20);
    ObjectDmsHandler::GetInstance().dmsEvents_.push_back({notify, timestamp});
    
    res = ObjectDmsHandler::GetInstance().GetDstBundleName(srcBundleName, dstNetworkId);
    EXPECT_EQ(res, srcBundleName);

    ObjectDmsHandler::GetInstance().dmsEvents_.clear();
}
} // namespace OHOS::Test
