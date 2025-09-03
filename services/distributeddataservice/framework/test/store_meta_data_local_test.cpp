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
#include "metadata/store_meta_data_local.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using OHOS::DistributedData::StoreMetaDataLocal;
using OHOS::DistributedData::PolicyValue;
namespace OHOS::Test {
class StoreMetaDataLocalTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: EqualityOperatorTest And InequalityOperatorTest
* @tc.desc: equalityOperator and inequalityOperator
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(StoreMetaDataLocalTest, EqualityOperatorTest, TestSize.Level1)
{
    StoreMetaDataLocal metaData1;
    StoreMetaDataLocal metaData2;
    StoreMetaDataLocal metaData3;

    ASSERT_TRUE(metaData1 == metaData2);
    metaData3.isAutoSync = true;
    ASSERT_FALSE(metaData1 == metaData3);
    metaData3.isBackup = true;
    ASSERT_FALSE(metaData1 == metaData3);
    ASSERT_TRUE(metaData1 != metaData3);
    ASSERT_FALSE(metaData1 != metaData2);
    metaData3.isDirty = true;
    ASSERT_FALSE(metaData1 == metaData3);
    metaData3.isEncrypt = true;
    ASSERT_FALSE(metaData1 == metaData3);
    metaData3.isPublic = true;
    ASSERT_FALSE(metaData1 == metaData3);

    metaData2.isAutoSync = true;
    ASSERT_FALSE(metaData2 == metaData3);
    metaData2.isBackup = true;
    ASSERT_FALSE(metaData2 == metaData3);
    metaData2.isDirty = true;
    ASSERT_FALSE(metaData2 == metaData3);
    metaData2.isEncrypt = true;
    ASSERT_FALSE(metaData2 == metaData3);
    metaData2.isPublic = true;
    ASSERT_TRUE(metaData2 == metaData3);

    metaData2.isBackup = false;
    ASSERT_FALSE(metaData2 == metaData3);
    metaData2.isBackup = true;
    metaData2.isDirty = false;
    ASSERT_FALSE(metaData2 == metaData3);
    metaData2.isBackup = true;
    metaData2.isEncrypt = false;
    ASSERT_FALSE(metaData2 == metaData3);
    metaData2.isBackup = true;
    metaData2.isPublic = false;
    ASSERT_FALSE(metaData2 == metaData3);
}

/**
* @tc.name: GetPrefixTest
* @tc.desc: getPrefix
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(StoreMetaDataLocalTest, GetPrefixTest, TestSize.Level1)
{
    StoreMetaDataLocal metaData;
    std::string expectedPrefix = "KvStoreMetaDataLocal###field1###field2###";
    std::initializer_list<std::string> fields{ "field1", "field2" };

    std::string prefix = metaData.GetPrefix(fields);

    ASSERT_EQ(prefix, expectedPrefix);
}

/**
* @tc.name: GetPolicy
* @tc.desc: test GetPolicy function
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(StoreMetaDataLocalTest, GetPolicy, TestSize.Level1)
{
    StoreMetaDataLocal metaData;
    uint32_t type = UINT32_MAX;
    auto policy = metaData.GetPolicy(type);
    EXPECT_EQ(policy.type, type);
    metaData.policies.push_back(policy);
    EXPECT_TRUE(metaData.HasPolicy(type));
    EXPECT_FALSE(policy.IsValueEffect());
    type = 1;
    policy = metaData.GetPolicy(type);
    EXPECT_NE(policy.type, type);
    policy.index = 1;
    EXPECT_FALSE(metaData.HasPolicy(type));
    EXPECT_TRUE(policy.IsValueEffect());
    type = UINT32_MAX;
    policy = metaData.GetPolicy(type);
    EXPECT_EQ(policy.type, type);
}

/**
* @tc.name: GetPolicy
* @tc.desc: test GetPolicy function
* @tc.type: FUNC
*/
HWTEST_F(StoreMetaDataLocalTest, PolicyValue, TestSize.Level1)
{
    PolicyValue pValue;
    pValue.type = UINT32_MAX;
    pValue.index = 0;
    pValue.valueUint = 1;
    
    PolicyValue pValue1;
    pValue1.type = UINT32_MAX;
    pValue1.index = 1;
    pValue1.valueUint = 1;
    
    PolicyValue pValue2;
    pValue2.type = UINT32_MAX;
    pValue2.index = 0;
    pValue2.valueUint = 1;
    EXPECT_NE(pValue, pValue1);
    EXPECT_EQ(pValue, pValue2);
    
    std::vector pValues = { pValue, pValue1, pValue2 };
    std::vector pValues1 = { pValue, pValue1, pValue1 };
    std::vector pValues2 = { pValue, pValue1, pValue2 };;
    EXPECT_NE(pValues, pValues1);
    EXPECT_EQ(pValues, pValues2);
}
} // namespace OHOS::Test