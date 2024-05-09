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
#define LOG_TAG "RdbCacheCursorTest"

#include "cache_cursor.h"
#include "gtest/gtest.h"
#include "log_print.h"
#include "store/cursor.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
namespace DistributedDataTest {
class RdbCacheCursorTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: GetCount
* @tc.desc: RdbCacheCursor GetCount test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbCacheCursorTest, GetCount, TestSize.Level0)
{
    std::vector<VBucket> records;
    auto cursor = std::make_shared<CacheCursor>(std::move(records));
    EXPECT_EQ(cursor->GetCount(), 0);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test