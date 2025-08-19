/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#define LOG_TAG "DatashareURIUtilsTest"
#include "utils.h"

#include <gtest/gtest.h>
#include <string>

namespace OHOS {
namespace DataShare {
using namespace testing::ext;
using namespace OHOS::DataShare;
class DatashareURIUtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

HWTEST_F(DatashareURIUtilsTest, GeneralAnonymous_001, TestSize.Level0)
{
    EXPECT_EQ(StringUtils::GeneralAnonymous("DataShareTable"), "Data***able");
}

HWTEST_F(DatashareURIUtilsTest, GeneralAnonymous_002, TestSize.Level0)
{
    EXPECT_EQ(StringUtils::GeneralAnonymous("1bc"), "******");
}

HWTEST_F(DatashareURIUtilsTest, GeneralAnonymous_003, TestSize.Level0)
{
    EXPECT_EQ(StringUtils::GeneralAnonymous("contact"), "cont***");
}

HWTEST_F(DatashareURIUtilsTest, GeneralAnonymous_004, TestSize.Level0)
{
    EXPECT_EQ(StringUtils::GeneralAnonymous("name"), "******");
}

HWTEST_F(DatashareURIUtilsTest, GeneralAnonymous_005, TestSize.Level0)
{
    EXPECT_EQ(StringUtils::GeneralAnonymous(""), "******");
}

HWTEST_F(DatashareURIUtilsTest, GeneralAnonymous_006, TestSize.Level0)
{
    EXPECT_EQ(StringUtils::GeneralAnonymous("store.db"), "stor***");
}
} // namespace DataShare
} // namespace OHOS