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
#define LOG_TAG "NetworkAdapterTest"
#include <gtest/gtest.h>
#include <unistd.h>
#include <iostream>

#include "communicator/device_manager_adapter.h"
#include "log_print.h"
#include "network_adapter.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

namespace OHOS::Test {
namespace DistributedDataTest {
class NetworkAdapterTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void NetworkAdapterTest::SetUpTestCase(void)
{
    NetworkAdapter::GetInstance().RegOnNetworkChange();
}

void NetworkAdapterTest::TearDownTestCase()
{
}

void NetworkAdapterTest::SetUp()
{
}

void NetworkAdapterTest::TearDown()
{
}

/**
* @tc.name: GetInstanceTest
* @tc.desc: GetInstance test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(NetworkAdapterTest, GetInstanceTest, TestSize.Level0)
{
    NetworkAdapter &instance = NetworkAdapter::GetInstance();
    EXPECT_NE(&instance, nullptr);
}

/**
* @tc.name: SetNetTest
* @tc.desc: SetNet test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(NetworkAdapterTest, SetNetTest, TestSize.Level0)
{
    NetworkAdapter::GetInstance().SetNet(NetworkAdapter::NetworkType::NONE);
    auto ret = NetworkAdapter::GetInstance().IsNetworkAvailable();
    EXPECT_EQ(ret, false);
    auto type = NetworkAdapter::GetInstance().GetNetworkType(true);
    EXPECT_EQ(type, NetworkAdapter::NONE);
    type = NetworkAdapter::GetInstance().GetNetworkType();
    EXPECT_EQ(type, NetworkAdapter::NONE);

    NetworkAdapter::GetInstance().SetNet(NetworkAdapter::NetworkType::WIFI);
    type = NetworkAdapter::GetInstance().GetNetworkType();
    EXPECT_EQ(type, NetworkAdapter::WIFI);

    NetworkAdapter::GetInstance().SetNet(NetworkAdapter::NetworkType::CELLULAR);
    type = NetworkAdapter::GetInstance().GetNetworkType();
    EXPECT_EQ(type, NetworkAdapter::CELLULAR);

    NetworkAdapter::GetInstance().SetNet(NetworkAdapter::NetworkType::ETHERNET);
    type = NetworkAdapter::GetInstance().GetNetworkType();
    EXPECT_EQ(type, NetworkAdapter::ETHERNET);

    NetworkAdapter::GetInstance().SetNet(NetworkAdapter::NetworkType::OTHER);
    type = NetworkAdapter::GetInstance().GetNetworkType();
    EXPECT_EQ(type, NetworkAdapter::OTHER);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test