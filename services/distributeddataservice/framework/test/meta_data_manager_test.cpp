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
#define LOG_TAG "MetaDataManagerTest"

#include <gtest/gtest.h>
#include "log_print.h"
#include "metadata/meta_data_manager.h"
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedKv;
namespace OHOS::Test {
class MetaDataManagerTest : public testing::Test {
public:
    static const std::string INVALID_DEVICE_ID;
    static const std::string EMPTY_DEVICE_ID;
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};
const std::string MetaDataManagerTest::INVALID_DEVICE_ID = "1234567890";
const std::string MetaDataManagerTest::EMPTY_DEVICE_ID = "";

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

/**
* @tc.name: SyncTest001
* @tc.desc: devices is empty.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(MetaDataManagerTest, SyncTest001, TestSize.Level1)
{
    std::vector<std::string> devices;
    devices.emplace_back(EMPTY_DEVICE_ID);
    MetaDataManager::OnComplete complete;
    auto result = MetaDataManager::GetInstance().Sync(devices, complete);
    EXPECT_FALSE(result);
}

/**
* @tc.name: SyncTest002
* @tc.desc: devices is invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(MetaDataManagerTest, SyncTest002, TestSize.Level1)
{
    std::vector<std::string> devices;
    devices.emplace_back(INVALID_DEVICE_ID);
    MetaDataManager::OnComplete complete;
    auto result = MetaDataManager::GetInstance().Sync(devices, complete);
    EXPECT_FALSE(result);
}
} // namespace OHOS::Test