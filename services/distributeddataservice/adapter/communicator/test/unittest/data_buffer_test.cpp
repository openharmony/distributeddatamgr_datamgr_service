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
#include "data_buffer.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::AppDistributedKv;

class CommunicatorDataBufferTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

/**
* @tc.name: Init
* @tc.desc: Init Test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(CommunicatorDataBufferTest, Init, TestSize.Level0)
{
    DataBuffer buffer;
    auto result = buffer.Init(0);
    ASSERT_EQ(result, false);
    char buff[] = "DataBuffer Init test";
    buffer.buf_ = buff;
    result = buffer.Init(1);
    ASSERT_EQ(result, false);
    result = buffer.Init(0);
    ASSERT_EQ(result, false);
    buffer.buf_ = nullptr;
    result = buffer.Init(1);
    ASSERT_EQ(result, true);
}
} // namespace OHOS::Test