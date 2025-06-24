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
#include "device_sync_app/device_sync_app_manager.h"
#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::DistributedData;

namespace OHOS::Test {
class DeviceSyncAppManagerTest : public testing::Test {
public:
    void SetUp()
    {
        whiteList1 = {"appId1", "bundleName1", 1};
        whiteList2 = {"appId2", "bundleName2", 2};
        whiteList3 = {"appId3", "bundleName3", 3};

        std::vector<DeviceSyncAppManager::WhiteList> lists = {whiteList1, whiteList2, whiteList3};
        DeviceSyncAppManager::GetInstance().Initialize(lists);
    }

    DeviceSyncAppManager::WhiteList whiteList1;
    DeviceSyncAppManager::WhiteList whiteList2;
    DeviceSyncAppManager::WhiteList whiteList3;
};

/**
* @tc.name: Check001
* @tc.desc: Checks that the given WhiteList object exists in whiteLists_ list.
* @tc.type: FUNC
*/
HWTEST_F(DeviceSyncAppManagerTest, Check001, TestSize.Level1)
{
    DeviceSyncAppManager::WhiteList whiteList = {"appId1", "bundleName1", 1};
    bool result = DeviceSyncAppManager::GetInstance().Check(whiteList);
    EXPECT_TRUE(result);
}

/**
* @tc.name: Check002
* @tc.desc: Check that the given appId object does not match.
* @tc.type: FUNC
*/
HWTEST_F(DeviceSyncAppManagerTest, Check002, TestSize.Level1)
{
    DeviceSyncAppManager::WhiteList testList = {"appId2", "bundleName1", 1};
    bool result = DeviceSyncAppManager::GetInstance().Check(testList);
    EXPECT_FALSE(result);
}

/**
* @tc.name: Check003
* @tc.desc: Check that the given bundleName object does not match.
* @tc.type: FUNC
*/
HWTEST_F(DeviceSyncAppManagerTest, Check003, TestSize.Level1)
{
    DeviceSyncAppManager::WhiteList testList = {"appId1", "bundleName2", 1};
    bool result = DeviceSyncAppManager::GetInstance().Check(testList);
    EXPECT_FALSE(result);
}

/**
* @tc.name: Check004
* @tc.desc: Check that the given version object does not match.
* @tc.type: FUNC
*/
HWTEST_F(DeviceSyncAppManagerTest, Check004, TestSize.Level1)
{
    DeviceSyncAppManager::WhiteList testList = {"appId1", "bundleName1", 2};
    bool result = DeviceSyncAppManager::GetInstance().Check(testList);
    EXPECT_FALSE(result);
}

/**
* @tc.name: Check005
* @tc.desc: Checks that the given WhiteList object does not exist in whiteLists_ list.
* @tc.type: FUNC
*/
HWTEST_F(DeviceSyncAppManagerTest, Check005, TestSize.Level1)
{
    DeviceSyncAppManager::WhiteList whiteList = {"appId4", "bundleName4", 4};
    bool result = DeviceSyncAppManager::GetInstance().Check(whiteList);
    EXPECT_FALSE(result);
}
} // namespace OHOS::Test