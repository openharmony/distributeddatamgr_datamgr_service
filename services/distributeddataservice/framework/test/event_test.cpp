/*
* Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "eventcenter/event.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::DistributedData;

class EventTest : public testing::Test {};

/**
* @tc.name: Equals
* @tc.desc: equals.
* @tc.type: FUNC
*/
HWTEST_F(EventTest, Equals, TestSize.Level1)
{
    Event event1(1);
    Event event2(2);
    ASSERT_FALSE(event1.Equals(event2));
}