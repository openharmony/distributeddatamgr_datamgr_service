/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#define LOG_TAG "EventCenterTest"
#include "eventcenter/event_center.h"
#include "gtest/gtest.h"
#include "log_print.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
class EventCenterTest : public testing::Test {
public:
    enum TestEventId {
        TEST_EVT_UNKNOWN = Event::EVT_CUSTOM,
        TEST_EVT_BEGIN = Event::EVT_CUSTOM + 1,
        TEST_EVT_MIDDLE,
        TEST_EVT_END,
    };
    class TestBegin : public Event {
    public:
        TestBegin(): Event(TEST_EVT_BEGIN) {};
    };
    class TestMiddle : public Event {
    public:
        TestMiddle(): Event(TEST_EVT_MIDDLE) {};
    };
    class TestEnd : public Event {
    public:
        TestEnd(): Event(TEST_EVT_END) {};
    };
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp()
    {
        waitEvent_ = TEST_EVT_UNKNOWN;
        currEvent_ = TEST_EVT_UNKNOWN;
        EventCenter::GetInstance().Subscribe(TEST_EVT_BEGIN, [this](const Event &evt) {
            ASSERT_EQ(waitEvent_, TEST_EVT_BEGIN);
            EventCenter::Defer defer;
            EventCenter::GetInstance().PostEvent(std::make_unique<TestMiddle>());
            currEvent_ = TEST_EVT_BEGIN;
            waitEvent_ = TEST_EVT_MIDDLE;
        });
        EventCenter::GetInstance().Subscribe(TEST_EVT_MIDDLE, [this](const Event &evt) {
            ASSERT_EQ(waitEvent_, TEST_EVT_MIDDLE);
            EventCenter::Defer defer;
            EventCenter::GetInstance().PostEvent(std::make_unique<TestEnd>());
            currEvent_ = TEST_EVT_MIDDLE;
            waitEvent_ = TEST_EVT_END;
        });
        EventCenter::GetInstance().Subscribe(TEST_EVT_END, [this](const Event &evt) {
            ASSERT_EQ(waitEvent_, TEST_EVT_END);
            currEvent_ = TEST_EVT_END;
            waitEvent_ = TEST_EVT_UNKNOWN;
        });
    }

    void TearDown()
    {
        EventCenter::GetInstance().Unsubscribe(TEST_EVT_BEGIN);
        EventCenter::GetInstance().Unsubscribe(TEST_EVT_MIDDLE);
        EventCenter::GetInstance().Unsubscribe(TEST_EVT_END);
        waitEvent_ = TEST_EVT_UNKNOWN;
        currEvent_ = TEST_EVT_UNKNOWN;
    }

protected:
    int32_t waitEvent_ = TEST_EVT_UNKNOWN;
    int32_t currEvent_ = TEST_EVT_UNKNOWN;
};

/**
* @tc.name: TopLayerASyncEvent
* @tc.desc: the async event on the top layer will dispatch, until the function completed.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(EventCenterTest, TopLayerASyncEvent, TestSize.Level2)
{
    ZLOGI("EventCenterSuite ASyncEvent begin.");
    auto test = [this]() {
        EventCenter::Defer defer;
        EventCenter::GetInstance().PostEvent(std::make_unique<TestBegin>());
        waitEvent_ = TEST_EVT_BEGIN;
    };
    test();
    ASSERT_EQ(currEvent_, TEST_EVT_END);
    ASSERT_EQ(waitEvent_, TEST_EVT_UNKNOWN);
}

/**
* @tc.name: SubLayerASyncEvent
* @tc.desc: the async event on sub layer will defer to dispatch, until the top layer function completed.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(EventCenterTest, SubLayerASyncEvent, TestSize.Level2)
{
    ZLOGI("EventCenterSuite ASyncEvent begin.");
    EventCenter::Defer defer;
    EventCenter::GetInstance().PostEvent(std::make_unique<TestBegin>());
    waitEvent_ = TEST_EVT_BEGIN;
    ASSERT_EQ(currEvent_, TEST_EVT_UNKNOWN);
    ASSERT_EQ(waitEvent_, TEST_EVT_BEGIN);
}

/**
* @tc.name: ASyncEventWithoutDefer
* @tc.desc: async event without defer may call or not
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(EventCenterTest, ASyncEventWithoutDefer, TestSize.Level2)
{
    ZLOGI("EventCenterSuite ASyncEvent begin.");
    EventCenter::Defer defer;
    waitEvent_ = TEST_EVT_BEGIN;
    auto test = [this]() {
        EventCenter::GetInstance().PostEvent(std::make_unique<TestBegin>());
    };
    test();
    ASSERT_EQ(currEvent_, TEST_EVT_UNKNOWN);
    ASSERT_EQ(waitEvent_, TEST_EVT_BEGIN);
}


/**
* @tc.name: ImmediatelyASyncEvent
* @tc.desc: post the async event, there is top layer and no defer; we will dispatch the async event Immediately.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(EventCenterTest, ImmediatelyASyncEvent, TestSize.Level2)
{
    ZLOGI("EventCenterSuite ASyncEvent begin.");
    waitEvent_ = TEST_EVT_BEGIN;
    EventCenter::GetInstance().PostEvent(std::make_unique<TestBegin>());
    ASSERT_EQ(currEvent_, TEST_EVT_END);
    ASSERT_EQ(waitEvent_, TEST_EVT_UNKNOWN);
}
