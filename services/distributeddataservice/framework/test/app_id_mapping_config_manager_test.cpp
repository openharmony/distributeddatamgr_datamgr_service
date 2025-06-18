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

class AppIdMappingConfigManagerTest : public testing::Test {};

/**
* @tc.name: Convert
* @tc.desc: Convert.
* @tc.type: FUNC
*/
HWTEST_F(AppIdMappingConfigManagerTest, Convert01, TestSize.Level1)
{
    auto result = AppIdMappingConfigManager::GetInstance().Convert("123", "123456789");
    ASSERT_EQ(result.first, "123");
    ASSERT_EQ(result.second, "123456789");
    result = AppIdMappingConfigManager::GetInstance().Convert("123", "987654321");
    ASSERT_EQ(result.first, "123456789");
    ASSERT_EQ(result.second, "default");
}

/**
* @tc.name: Convert
* @tc.desc: Convert.
* @tc.type: FUNC
*/
HWTEST_F(AppIdMappingConfigManagerTest, Convert02, TestSize.Level1)
{
    auto result = AppIdMappingConfigManager::GetInstance().Convert("321");
    ASSERT_EQ(result, "321");;
    result = AppIdMappingConfigManager::GetInstance().Convert("123");
    ASSERT_EQ(result, "123456789");
}