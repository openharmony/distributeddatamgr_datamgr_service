/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#define LOG_TAG "DevManagerTest"
#include <gtest/gtest.h>

#include "dev_manager.h"
#include "types.h"
#include "log_print.h"
using namespace testing::ext;
using namespace OHOS::DistributedKv;

class DevManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

    void SetUp();
    void TearDown();
};

void DevManagerTest::SetUpTestCase(void)
{}

void DevManagerTest::TearDownTestCase(void)
{}

void DevManagerTest::SetUp(void)
{}

void DevManagerTest::TearDown(void)
{}

/**
* @tc.name: GetLocalDevice001
* @tc.desc: Get local device's infomation
* @tc.type: FUNC
* @tc.require:
* @tc.author: taoyuxin
*/
HWTEST_F(DevManagerTest, GetLocalDevice001, TestSize.Level1)
{
    ZLOGI("GetLocalDevice001 begin.");
    DevManager &devManager = OHOS::DistributedKv::DevManager::GetInstance();
    DevManager::DetailInfo devInfo = devManager.GetLocalDevice();

    EXPECT_NE(devInfo.networkId, "");
    EXPECT_NE(devInfo.uuid, "");
    EXPECT_NE(devInfo.udid, "");
}

/**
* @tc.name: ToUUID001
* @tc.desc: Get uuid from networkId
* @tc.type: FUNC
* @tc.require:
* @tc.author: taoyuxin
*/
HWTEST_F(DevManagerTest, ToUUID001, TestSize.Level1)
{
    ZLOGI("ToUUID001 begin.");
    DevManager &devManager = OHOS::DistributedKv::DevManager::GetInstance();
    DevManager::DetailInfo devInfo = devManager.GetLocalDevice();
    EXPECT_NE(devInfo.networkId, "");
    std::string uuid = devManager.ToUUID(devInfo.networkId);
    EXPECT_NE(uuid, "");
    EXPECT_EQ(uuid, devInfo.uuid);
}

/**
* @tc.name: GetRemoteDevices001
* @tc.desc: Get remote devices
* @tc.type: FUNC
* @tc.require:
* @tc.author: taoyuxin
*/
HWTEST_F(DevManagerTest, GetRemoteDevices001, TestSize.Level1)
{
    ZLOGI("GetRemoteDevices001 begin.");
    DevManager &devManager = OHOS::DistributedKv::DevManager::GetInstance();
    vector<DevManager::DetailInfo> devInfo = devManager.GetRemoteDevices();
    EXPECT_EQ(devInfo.size(), 0);
}
