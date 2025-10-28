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

#include "sensitive.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::DistributedKv;
namespace OHOS::Test {
class SensitiveTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void SensitiveTest::SetUpTestCase(void)
{
}

void SensitiveTest::TearDownTestCase(void)
{
}

void SensitiveTest::SetUp(void)
{
}

void SensitiveTest::TearDown(void)
{
}

/**
 * @tc.name: GetDeviceSecurityLevel001
 * @tc.desc: Test for GetDeviceSecurityLevel deviceId is empty.
 * @tc.type: FUNC
 */
HWTEST_F(SensitiveTest, GetDeviceSecurityLevel001, TestSize.Level1)
{
    auto sensitive = std::make_shared<Sensitive>();
    auto res = sensitive->GetDeviceSecurityLevel();
    EXPECT_EQ(res, DATA_SEC_LEVEL1);
}

/**
 * @tc.name: GetDeviceSecurityLevel002
 * @tc.desc: Test for GetDeviceSecurityLevel secruityLevel > DATA_SEC_LEVEL1.
 * @tc.type: FUNC
 */
HWTEST_F(SensitiveTest, GetDeviceSecurityLevel002, TestSize.Level1)
{
    auto sensitive = std::make_shared<Sensitive>();
    sensitive->securityLevel = DATA_SEC_LEVEL2;
    auto res = sensitive->GetDeviceSecurityLevel();
    EXPECT_EQ(res, DATA_SEC_LEVEL2);
}

/**
 * @tc.name: Operator001
 * @tc.desc: Test for Operator securityLabel is NOT_SET.
 * @tc.type: FUNC
 */
HWTEST_F(SensitiveTest, Operator001, TestSize.Level1)
{
    Sensitive sensitive;
    DistributedDB::SecurityOption option;
    option.securityLabel = DistributedDB::NOT_SET;
    EXPECT_TRUE(sensitive >= option);
}

/**
 * @tc.name: Operator002
 * @tc.desc: Test for Operator securityLabel is S3.
 * @tc.type: FUNC
 */
HWTEST_F(SensitiveTest, Operator002, TestSize.Level1)
{
    Sensitive sensitive;
    DistributedDB::SecurityOption option;
    option.securityLabel = DistributedDB::S3;
    EXPECT_FALSE(sensitive >= option);
}

/**
 * @tc.name: Operator003
 * @tc.desc: Test for Operator securityLabel is S1.
 * @tc.type: FUNC
 */
HWTEST_F(SensitiveTest, Operator003, TestSize.Level1)
{
    Sensitive sensitive;
    DistributedDB::SecurityOption option;
    option.securityLabel = DistributedDB::S1;
    EXPECT_TRUE(sensitive >= option);
}

/**
 * @tc.name: Operator004
 * @tc.desc: Test for Operator securityLabel is S4.
 * @tc.type: FUNC
 */
HWTEST_F(SensitiveTest, Operator004, TestSize.Level1)
{
    Sensitive sensitive;
    sensitive.securityLevel = DATA_SEC_LEVEL2;
    DistributedDB::SecurityOption option;
    option.securityLabel = DistributedDB::S4;
    EXPECT_FALSE(sensitive >= option);
}

/**
 * @tc.name: Operator005
 * @tc.desc: Test for Operator securityLabel is S1.
 * @tc.type: FUNC
 */
HWTEST_F(SensitiveTest, Operator005, TestSize.Level1)
{
    Sensitive sensitive;
    sensitive.securityLevel = DATA_SEC_LEVEL2;
    DistributedDB::SecurityOption option;
    option.securityLabel = DistributedDB::S1;
    EXPECT_TRUE(sensitive >= option);
}
} // namespace OHOS::Test