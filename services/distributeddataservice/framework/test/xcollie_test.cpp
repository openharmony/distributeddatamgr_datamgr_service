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
class XCollieHandlerMock final {
public:
    static int32_t SetTimer(
        const std::string &tag, uint32_t timeoutSeconds, std::function<void(void *)> func, void *arg, uint32_t flag)
    {
        instance_->tag_ = tag;
        instance_->timeoutSeconds_ = timeoutSeconds;
        instance_->func_ = std::move(func);
        instance_->arg_ = arg;
        instance_->flag_ = flag;
        ++instance_->setTimerCount_;
        return instance_->timerId_;
    }

    static void CancelTimer(int32_t id)
    {
        instance_->canceledId_ = id;
        ++instance_->cancelTimerCount_;
    }

    static XCollieHandlerMock *instance_;
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

XCollieHandlerMock *XCollieHandlerMock::instance_ = nullptr;

class XCollieTest : public testing::Test {
public:
    void SetUp() override
    {
        XCollie::setTimer_ = nullptr;
        XCollie::cancelTimer_ = nullptr;
        XCollieHandlerMock::instance_ = nullptr;
    }

    void TearDown() override
    {
        XCollie::setTimer_ = nullptr;
        XCollie::cancelTimer_ = nullptr;
        XCollieHandlerMock::instance_ = nullptr;
    }
};

/**
 * @tc.name: RegisterTimerHandler
 * @tc.desc: Test XCollie keeps the first timer handler and rejects duplicate registration.
 * @tc.author: agent
 * @tc.type: FUNC
 */
HWTEST_F(XCollieTest, RegisterTimerHandler, TestSize.Level1)
{
    XCollieHandlerMock mock;
    XCollieHandlerMock::instance_ = &mock;

    EXPECT_TRUE(XCollie::RegisterTimerHandler(XCollieHandlerMock::SetTimer, XCollieHandlerMock::CancelTimer));
    EXPECT_EQ(XCollie::setTimer_, XCollieHandlerMock::SetTimer);
    EXPECT_EQ(XCollie::cancelTimer_, XCollieHandlerMock::CancelTimer);
    EXPECT_FALSE(XCollie::RegisterTimerHandler(XCollieHandlerMock::SetTimer, XCollieHandlerMock::CancelTimer));
    EXPECT_EQ(XCollie::setTimer_, XCollieHandlerMock::SetTimer);
    EXPECT_EQ(XCollie::cancelTimer_, XCollieHandlerMock::CancelTimer);
}

/**
 * @tc.name: RegisterNullHandler
 * @tc.desc: Test XCollie rejects incomplete timer handlers.
 * @tc.author: agent
 * @tc.type: FUNC
 */
HWTEST_F(XCollieTest, RegisterNullHandler, TestSize.Level1)
{
    EXPECT_FALSE(XCollie::RegisterTimerHandler(nullptr, XCollieHandlerMock::CancelTimer));
    EXPECT_FALSE(XCollie::RegisterTimerHandler(XCollieHandlerMock::SetTimer, nullptr));
    EXPECT_EQ(XCollie::setTimer_, nullptr);
    EXPECT_EQ(XCollie::cancelTimer_, nullptr);
}

/**
 * @tc.name: ConstructWithoutHandler
 * @tc.desc: Test XCollie keeps the default invalid timer id when no timer handler is registered.
 * @tc.author: agent
 * @tc.type: FUNC
 */
HWTEST_F(XCollieTest, ConstructWithoutHandler, TestSize.Level1)
{
    XCollie xcollie("NoHandler", XCollie::XCOLLIE_LOG, 10);

    EXPECT_EQ(xcollie.id_, -1);
}

/**
 * @tc.name: ForwardTimerOperations
 * @tc.desc: Test XCollie forwards timer creation and cancellation to the registered handler.
 * @tc.author: agent
 * @tc.type: FUNC
 */
HWTEST_F(XCollieTest, ForwardTimerOperations, TestSize.Level1)
{
    XCollieHandlerMock handler;
    int32_t arg = 1;
    handler.timerId_ = 200;
    XCollieHandlerMock::instance_ = &handler;

    EXPECT_TRUE(XCollie::RegisterTimerHandler(XCollieHandlerMock::SetTimer, XCollieHandlerMock::CancelTimer));
    {
        XCollie xcollie("AutoCache::GetDBStore", XCollie::XCOLLIE_LOG, 10, nullptr, &arg);
        EXPECT_EQ(handler.setTimerCount_, 1);
        EXPECT_EQ(handler.tag_, "AutoCache::GetDBStore");
        EXPECT_EQ(handler.timeoutSeconds_, 10);
        EXPECT_EQ(handler.arg_, &arg);
        EXPECT_EQ(handler.flag_, XCollie::XCOLLIE_LOG);
        EXPECT_EQ(handler.cancelTimerCount_, 0);
    }

    EXPECT_EQ(handler.cancelTimerCount_, 1);
    EXPECT_EQ(handler.canceledId_, 200);
}
} // namespace DistributedDataXCollieTest
} // namespace OHOS::Test
