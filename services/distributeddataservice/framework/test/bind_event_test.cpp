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

#include "snapshot/bind_event.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::DistributedData;

class BindEventTest : public testing::Test {};

/**
* @tc.name: GetBindInfo
* @tc.desc: get bind info.
* @tc.type: FUNC
*/
HWTEST_F(BindEventTest, GetBindInfo, TestSize.Level1)
{
    BindEvent::BindEventInfo info;
    info.tokenId = 100;
    BindEvent event(BindEvent::BIND_SNAPSHOT, std::move(info));
    auto ret = event.GetBindInfo().tokenId;
    ASSERT_EQ(ret, 100);
}