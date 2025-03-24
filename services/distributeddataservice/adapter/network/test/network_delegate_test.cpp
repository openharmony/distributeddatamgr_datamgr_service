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
#define LOG_TAG "NetworkDelegateTest"
#include "network_delegate.h"

#include <gtest/gtest.h>
#include <unistd.h>

using namespace testing::ext;
using namespace OHOS::DistributedData;

namespace OHOS::Test {
namespace DistributedDataTest {
class NetworkDelegateTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void NetworkDelegateTest::SetUpTestCase(void)
{
    auto delegate = NetworkDelegate::GetInstance();
    if (delegate == nullptr) {
        return;
    }
    delegate->RegOnNetworkChange();
}

void NetworkDelegateTest::TearDownTestCase()
{
}

void NetworkDelegateTest::SetUp()
{
}

void NetworkDelegateTest::TearDown()
{
}

/**
* @tc.name: IsNetworkAvailableTest
* @tc.desc: IsNetworkAvailable test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(NetworkDelegateTest, IsNetworkAvailableTest, TestSize.Level0)
{
    NetworkDelegate *instance = NetworkDelegate::GetInstance();
    ASSERT_NE(instance, nullptr);
    EXPECT_FALSE(instance->IsNetworkAvailable());
}

/**
* @tc.name: GetNetworkTypeTest
* @tc.desc: GetNetworkType test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(NetworkDelegateTest, GetNetworkTypeTest, TestSize.Level0)
{
    NetworkDelegate *instance = NetworkDelegate::GetInstance();
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(instance->GetNetworkType(), NetworkDelegate::NetworkType::NONE);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test