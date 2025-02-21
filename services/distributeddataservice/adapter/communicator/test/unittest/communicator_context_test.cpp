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
#include <cstdint>
#include <gtest/gtest.h>
#include <iostream>
#include <unistd.h>
#include <vector>
#include "communicator_context.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::AppDistributedKv;
using namespace OHOS::DistributedData;
constexpr int32_t SOFTBUS_OK = 0;
using OnCloseAble = std::function<void(const std::string &deviceId)>;

class DevChangeListener final : public AppDeviceChangeListener {
public:
    void OnDeviceChanged(const DeviceInfo &info, const DeviceChangeType &type) const override
    {
    }
    ChangeLevelType GetChangeLevelType() const override
    {
        return ChangeLevelType::MIN;
    }
};

class CommunicatorContextTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

/**
* @tc.name: RegSessionListener
* @tc.desc: RegSessionListener test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(CommunicatorContextTest, RegSessionListener, TestSize.Level0)
{
    DevChangeListener *devChangeListener = nullptr;
    auto status = CommunicatorContext::GetInstance().RegSessionListener(devChangeListener);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    DevChangeListener listener;
    devChangeListener = &listener;
    CommunicatorContext::GetInstance().observers_.emplace_back(devChangeListener);
    status = CommunicatorContext::GetInstance().RegSessionListener(devChangeListener);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: UnRegSessionListener
* @tc.desc: UnRegSessionListener test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(CommunicatorContextTest, UnRegSessionListener, TestSize.Level0)
{
    DevChangeListener *devChangeListener = nullptr;
    auto status = CommunicatorContext::GetInstance().UnRegSessionListener(devChangeListener);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    DevChangeListener listener;
    devChangeListener = &listener;
    CommunicatorContext::GetInstance().observers_.clear();
    EXPECT_EQ(CommunicatorContext::GetInstance().observers_.empty(), true);
    status = CommunicatorContext::GetInstance().UnRegSessionListener(devChangeListener);
    EXPECT_EQ(status, Status::SUCCESS);
    CommunicatorContext::GetInstance().observers_.emplace_back(devChangeListener);
    EXPECT_EQ(CommunicatorContext::GetInstance().observers_.empty(), false);
    status = CommunicatorContext::GetInstance().UnRegSessionListener(devChangeListener);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: NotifySessionReady
* @tc.desc: NotifySessionReady test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(CommunicatorContextTest, NotifySessionReady, TestSize.Level0)
{
    std::string deviceId[2] = {"CommunicatorContextTest", "CommunicatorContextTest"};
    EXPECT_EQ(CommunicatorContext::GetInstance().devices_.Empty(), true);

    DevChangeListener *devChangeListener = nullptr;
    DevChangeListener *devChangeListener1 = nullptr;
    DevChangeListener listener;
    devChangeListener1 = &listener;
    CommunicatorContext::GetInstance().observers_.clear();
    CommunicatorContext::GetInstance().observers_.emplace_back(devChangeListener);
    CommunicatorContext::GetInstance().observers_.emplace_back(devChangeListener1);

    CommunicatorContext::GetInstance().closeListener_ = nullptr;
    CommunicatorContext::GetInstance().NotifySessionReady(deviceId[0], 1);
    EXPECT_EQ(CommunicatorContext::GetInstance().devices_.Empty(), true);
    CommunicatorContext::GetInstance().NotifySessionReady(deviceId[0], SOFTBUS_OK);
    EXPECT_EQ(CommunicatorContext::GetInstance().devices_.Contains(deviceId[0]), true);
    auto myCloseHandler = [](const std::string &deviceId) {
        const_cast<std::string &>(deviceId) = "CommunicatorContextTest01";
    };
    OnCloseAble closeAbleCallback;
    closeAbleCallback = myCloseHandler;
    CommunicatorContext::GetInstance().SetSessionListener(closeAbleCallback);
    CommunicatorContext::GetInstance().NotifySessionReady(deviceId[0], SOFTBUS_OK);
    ASSERT_NE(deviceId[0], deviceId[1]);
}

/**
* @tc.name: NotifySessionClose
* @tc.desc: NotifySessionClose test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(CommunicatorContextTest, NotifySessionClose, TestSize.Level0)
{
    const std::string deviceId = "NotifySessionCloseTest";
    CommunicatorContext::GetInstance().devices_.Clear();
    EXPECT_EQ(CommunicatorContext::GetInstance().devices_.Empty(), true);
    CommunicatorContext::GetInstance().devices_.Insert(deviceId, deviceId);
    EXPECT_EQ(CommunicatorContext::GetInstance().devices_.Empty(), false);
    CommunicatorContext::GetInstance().NotifySessionClose(deviceId);
    EXPECT_EQ(CommunicatorContext::GetInstance().devices_.Empty(), true);
}

/**
* @tc.name: IsSessionReady
* @tc.desc: IsSessionReady test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(CommunicatorContextTest, IsSessionReady, TestSize.Level0)
{
    const std::string deviceId = "NotifySessionCloseTest";
    const std::string deviceId1 = "";
    auto result = CommunicatorContext::GetInstance().IsSessionReady(deviceId1);
    EXPECT_EQ(result, false);
    result = CommunicatorContext::GetInstance().IsSessionReady(deviceId);
    EXPECT_EQ(result, false);
    CommunicatorContext::GetInstance().devices_.Insert(deviceId, deviceId);
    result = CommunicatorContext::GetInstance().IsSessionReady(deviceId);
    EXPECT_EQ(result, true);
}
} // namespace OHOS::Test