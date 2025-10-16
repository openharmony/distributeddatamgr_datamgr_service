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

#include "app_id_mapping/app_id_mapping_config_manager.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class AppIdMappingConfigManagerTest : public testing::Test {};

/**
* @tc.name: Convert
* @tc.desc: Generate a pair based on the input appId and accountId.
* @tc.type: FUNC
*/
HWTEST_F(AppIdMappingConfigManagerTest, Convert01, TestSize.Level1)
{
    std::vector<AppIdMappingConfigManager::AppMappingInfo> mapper = {
        {"src1", "dst1"},
        {"src2", "dst2", false},
        {"src3", "dst3", true}
    };
    AppIdMappingConfigManager::GetInstance().Initialize(mapper);
    auto result = AppIdMappingConfigManager::GetInstance().Convert("123", "123456789");
    EXPECT_EQ(result.first, "123");
    EXPECT_EQ(result.second, "123456789");
    result = AppIdMappingConfigManager::GetInstance().Convert("src1", "987654321");
    EXPECT_EQ(result.first, "dst1");
    EXPECT_EQ(result.second, "default");

    result = AppIdMappingConfigManager::GetInstance().Convert("src2", "987654321");
    EXPECT_EQ(result.first, "dst2");
    EXPECT_EQ(result.second, "default");
    result = AppIdMappingConfigManager::GetInstance().Convert("src3", "123123");
    EXPECT_EQ(result.first, "dst3");
    EXPECT_EQ(result.second, "123123");
}

/**
* @tc.name: Convert
* @tc.desc: Convert the input appId to another id.
* @tc.type: FUNC
*/
HWTEST_F(AppIdMappingConfigManagerTest, Convert02, TestSize.Level1)
{
    std::vector<AppIdMappingConfigManager::AppMappingInfo> mapper = {
        {"src1", "dst1"},
        {"src2", "dst2"},
        {"src3", "dst3"}
    };
    AppIdMappingConfigManager::GetInstance().Initialize(mapper);
    auto result = AppIdMappingConfigManager::GetInstance().Convert("321");
    EXPECT_EQ(result, "321");
    result = AppIdMappingConfigManager::GetInstance().Convert("src1");
    EXPECT_EQ(result, "dst1");
}
} // namespace OHOS::Test