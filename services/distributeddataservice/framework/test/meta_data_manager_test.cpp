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
#include "metadata/meta_data_manager.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::DistributedData;

class MetaDataManagerTest : public testing::Test {
};

/**
* @tc.name: FilterConstructorAndGetKeyTest
* @tc.desc: FilterConstructor and GetKey.
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(MetaDataManagerTest, FilterConstructorAndGetKeyTest, TestSize.Level1)
{
    std::string pattern = "test";
    MetaDataManager::Filter filter(pattern);

    std::vector<uint8_t> key = filter.GetKey();
    ASSERT_EQ(key.size(), 0);
}

/**
* @tc.name: FilterOperatorTest
* @tc.desc: FilterOperator.
* @tc.type: FUNC
* @tc.require:
* @tc.author: MengYao
*/
HWTEST_F(MetaDataManagerTest, FilterOperatorTest, TestSize.Level1)
{
    std::string pattern = "test";
    MetaDataManager::Filter filter(pattern);

    std::string key = "test_key";
    ASSERT_TRUE(filter(key));

    key = "another_key";
    ASSERT_FALSE(filter(key));
}
