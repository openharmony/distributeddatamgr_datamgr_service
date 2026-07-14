/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "dfx/xcollie.h"

#include <utility>

#include "gtest/gtest.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;

namespace OHOS::Test {
namespace DistributedDataXCollieTest {
class XCollieDelegateMock final : public XCollieDelegate {
public:
    int32_t SetTimer(const std::string &tag, uint32_t timeoutSeconds, std::function<void(void *)> func, void *arg,
        uint32_t flag) override
    {
        tag_ = tag;
        timeoutSeconds_ = timeoutSeconds;
        func_ = std::move(func);
        arg_ = arg;
        flag_ = flag;
        ++setTimerCount_;
        return timerId_;
    }

    void CancelTimer(int32_t id) override
    {
        canceledId_ = id;
        ++cancelTimerCount_;
    }

    std::string tag_;
    uint32_t timeoutSeconds_ = 0;
    std::function<void(void *)> func_;
    void *arg_ = nullptr;
    uint32_t flag_ = 0;
    int32_t timerId_ = 100;
    int32_t canceledId_ = -1;
    uint32_t setTimerCount_ = 0;
    uint32_t cancelTimerCount_ = 0;
};

class XCollieTest : public testing::Test {
public:
    void SetUp() override
    {
        XCollieDelegate::instance_ = nullptr;
    }

    void TearDown() override
    {
        XCollieDelegate::instance_ = nullptr;
    }
};

/**
 * @tc.name: RegisterXCollieInstance_FirstAndDuplicateRegistration_ReturnsExpected
 * @tc.desc: Test XCollieDelegate registration keeps the first instance and rejects duplicate registration.
 * @tc.author: agent
 * @tc.type: FUNC
 */
HWTEST_F(XCollieTest, RegisterXCollieInstance_FirstAndDuplicateRegistration_ReturnsExpected, TestSize.Level1)
{
    XCollieDelegateMock first;
    XCollieDelegateMock second;

    EXPECT_TRUE(XCollieDelegate::RegisterXCollieInstance(&first));
    EXPECT_EQ(XCollieDelegate::GetInstance(), &first);
    EXPECT_FALSE(XCollieDelegate::RegisterXCollieInstance(&second));
    EXPECT_EQ(XCollieDelegate::GetInstance(), &first);
}

/**
 * @tc.name: XCollie_ConstructWithoutRegisteredDelegate_KeepsInvalidTimerId
 * @tc.desc: Test XCollie keeps the default invalid timer id when no delegate is registered.
 * @tc.author: agent
 * @tc.type: FUNC
 */
HWTEST_F(XCollieTest, XCollie_ConstructWithoutRegisteredDelegate_KeepsInvalidTimerId, TestSize.Level1)
{
    XCollie xcollie("NoDelegate", XCollie::XCOLLIE_LOG, 10);

    EXPECT_EQ(xcollie.id_, -1);
}

/**
 * @tc.name: XCollie_ConstructAndDestructWithRegisteredDelegate_ForwardsTimerOperations
 * @tc.desc: Test XCollie forwards timer creation and cancellation to the registered delegate.
 * @tc.author: agent
 * @tc.type: FUNC
 */
HWTEST_F(XCollieTest, XCollie_ConstructAndDestructWithRegisteredDelegate_ForwardsTimerOperations, TestSize.Level1)
{
    XCollieDelegateMock delegate;
    int32_t arg = 1;
    delegate.timerId_ = 200;

    EXPECT_TRUE(XCollieDelegate::RegisterXCollieInstance(&delegate));
    {
        XCollie xcollie("AutoCache::GetDBStore", XCollie::XCOLLIE_LOG, 10, nullptr, &arg);
        EXPECT_EQ(delegate.setTimerCount_, 1);
        EXPECT_EQ(delegate.tag_, "AutoCache::GetDBStore");
        EXPECT_EQ(delegate.timeoutSeconds_, 10);
        EXPECT_EQ(delegate.arg_, &arg);
        EXPECT_EQ(delegate.flag_, XCollie::XCOLLIE_LOG);
        EXPECT_EQ(delegate.cancelTimerCount_, 0);
    }

    EXPECT_EQ(delegate.cancelTimerCount_, 1);
    EXPECT_EQ(delegate.canceledId_, 200);
}
} // namespace DistributedDataXCollieTest
} // namespace OHOS::Test
