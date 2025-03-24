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

#include "bootstrap.h"
#include <gtest/gtest.h>
#include <memory>

namespace OHOS::DistributedData {
using namespace testing;
class BootStrapMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
private:
    static constexpr const char *defaultLabel = "distributeddata";
    static constexpr const char *defaultMeta = "service_meta";
};

void BootStrapMockTest::SetUpTestCase(void)
{}

void BootStrapMockTest::TearDownTestCase(void)
{}

void BootStrapMockTest::SetUp(void)
{}

void BootStrapMockTest::TearDown(void)
{}
 
 /**
   * @tc.name: GetProcessLabel
   * @tc.desc: Get process label.
   * @tc.type: FUNC
   */
HWTEST_F(BootStrapMockTest, GetProcessLabel, testing::ext::TestSize.Level0)
{
    std::string proLabel = Bootstrap::GetInstance().GetProcessLabel();
    EXPECT_TRUE(proLabel == BootStrapMockTest::defaultLabel);
}
 
 /**
   * @tc.name: GetMetaDBNameAndLoadMultiItem
   * @tc.desc: Get meta db name and load other items.
   * @tc.type: FUNC
   */
HWTEST_F(BootStrapMockTest, GetMetaDBName, testing::ext::TestSize.Level0)
{
    std::string metaDbName = Bootstrap::GetInstance().GetMetaDBName();
    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadCheckers();
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCloud();
    Bootstrap::GetInstance().LoadAppIdMappings();
    std::shared_ptr<ExecutorPool> executors = std::make_shared<ExecutorPool>(0, 1);
    Bootstrap::GetInstance().LoadBackup(executors);
    EXPECT_TRUE(metaDbName == BootStrapMockTest::defaultMeta);
}
}