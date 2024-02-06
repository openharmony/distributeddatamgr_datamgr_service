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

class StoreMetaDataLocalTest : public testing::Test {
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
    metaData1.isAutoSync = true;
    metaData1.isBackup = true;
    StoreMetaDataLocal metaData2;
    metaData2.isAutoSync = true;
    metaData2.isBackup = false;
    StoreMetaDataLocal metaData3;
    metaData3.isAutoSync = true;
    metaData3.isBackup = true;

    ASSERT_FALSE(metaData1 == metaData2);
    ASSERT_TRUE(metaData1 == metaData3);
    ASSERT_FALSE(metaData2 == metaData3);

    ASSERT_TRUE(metaData1 != metaData2);
    ASSERT_FALSE(metaData1 != metaData3);
    ASSERT_TRUE(metaData2 != metaData3);
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