/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "DumpHelperTest "

#include "gtest/gtest.h"
#include "log_print.h"
#include "dump_helper.h"
#include "dump/dump_manager.h"
#include "types.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
namespace DistributedDataTest {
class DumpHelperTest  : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: GetInstanceTest
* @tc.desc: GetInstance test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(DumpHelperTest, GetInstanceTest, TestSize.Level0)
{
    DumpHelper &dumpHelper = DumpHelper::GetInstance();
    EXPECT_NE(&dumpHelper, nullptr);

    int fd = 1;
    std::vector<std::string> args;
    EXPECT_TRUE(dumpHelper.Dump(fd, args));

    args = {"COMMAND_A", "ARG_1", "ARG_2"};
    EXPECT_TRUE(dumpHelper.Dump(fd, args));
}

/**
* @tc.name: AddErrorInfoTest
* @tc.desc: AddErrorInfo test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(DumpHelperTest, AddErrorInfoTest, TestSize.Level0)
{
   DumpHelper &dumpHelper = DumpHelper::GetInstance();
   EXPECT_NE(&dumpHelper, nullptr);
   int32_t errorCode = 1001;
   std::string errorInfo = "Test error information";

   dumpHelper.AddErrorInfo(errorCode, errorInfo);

   const OHOS::DistributedData::DumpHelper::ErrorInfo& lastError = dumpHelper.errorInfo_.back();
   EXPECT_EQ(lastError.errorCode, errorCode);
   EXPECT_EQ(lastError.errorInfo, errorInfo);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test