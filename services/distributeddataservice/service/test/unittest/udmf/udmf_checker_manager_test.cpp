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

#define LOG_TAG "UdmfCheckerManagerTest"

#include "checker_manager.h"
#include "concurrent_map.h"
#include "error_code.h"
#include "gtest/gtest.h"
#include "unified_types.h"
#include "uri.h"

using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::UDMF;
namespace OHOS::Test {
namespace DistributedDataTest {

class UdmfCheckerManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: IsValid001
* @tc.desc: CheckerManager is IsValid function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(UdmfCheckerManagerTest, IsValid001, TestSize.Level1)
{
    std::vector<UDMF::Privilege> privileges;
    UDMF::CheckerManager::CheckInfo info;
    bool result = UDMF::CheckerManager::GetInstance().IsValid(privileges, info);
    EXPECT_EQ(result, true);
}

/**
* @tc.name: IsValid002
* @tc.desc: CheckerManager is IsValid function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(UdmfCheckerManagerTest, IsValid002, TestSize.Level1)
{
    UDMF::CheckerManager checkerManager;
    std::vector<UDMF::Privilege> privileges;
    UDMF::CheckerManager::CheckInfo info;
    UDMF::CheckerManager::Checker *checker = nullptr;
    checkerManager.checkers_["DataChecker"] = checker;
    bool result = checkerManager.IsValid(privileges, info);
    EXPECT_EQ(result, false);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test