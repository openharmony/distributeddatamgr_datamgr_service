/*

Copyright (c) 2023 Huawei Device Co., Ltd.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include "utils/verification_utils.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
    class VerificationUtilsTest : public testing::Test {
    public:
        const std::map<std::string, std::string> testRelation = { { "testUserId", "testBundleName" } };
        const std::map<std::string, uint64_t> testExpiresTime = { { "1h", 3600 } };
        static void SetUpTestCase(void) {};
        static void TearDownTestCase(void) {};
        void SetUp() {};
        void TearDown() {};
    };

/**

@tc.name: IfContainIllegalField001
@tc.desc: IfContainIllegalField function test.
@tc.type: FUNC
*/
    HWTEST_F(VerificationUtilsTest, IfContainIllegalField001, TestSize.Level0)
{
    EXPECT_TRUE(VerificationUtils::IfContainIllegalField("validpath"));
    EXPECT_TRUE(VerificationUtils::IfContainIllegalField("another_valid_path"));
    EXPECT_TRUE(VerificationUtils::IfContainIllegalField("file123"));
}
/**

@tc.name: IfContainIllegalField002
@tc.desc: IfContainIllegalField function test.
@tc.type: FUNC
*/
HWTEST_F(VerificationUtilsTest, IfContainIllegalField002, TestSize.Level0)
{
    EXPECT_FALSE(VerificationUtils::IfContainIllegalField("path/with/forward/slash"));
    EXPECT_FALSE(VerificationUtils::IfContainIllegalField("/starting/slash"));
    EXPECT_FALSE(VerificationUtils::IfContainIllegalField("ending/slash/"));
    EXPECT_FALSE(VerificationUtils::IfContainIllegalField("path\with\backslash"));
    EXPECT_FALSE(VerificationUtils::IfContainIllegalField("\starting\backslash"));
    EXPECT_FALSE(VerificationUtils::IfContainIllegalField("ending\backslash\"));
    EXPECT_FALSE(VerificationUtils::IfContainIllegalField(".."));
    EXPECT_FALSE(VerificationUtils::IfContainIllegalField("path/with\mixed/slashes"));
    EXPECT_FALSE(VerificationUtils::IfContainIllegalField("path\with/mixed\slashes"));
}
} // namespace OHOS::Test