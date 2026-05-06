/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#define LOG_TAG "DataProviderConfigMockTest"

#include "data_provider_config.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include "common_utils.h"
#include "data_share_sa_config_info_manager.h"
#include "data_share_profile_config.h"
#include "datashare_errno.h"
#include "log_print.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::DataShare;
class DataProviderConfigMockTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
    std::string DATA_SHARE_SA_URI = "datashare:///com.acts.datasharetest/SAID=12321/test";
    std::string DATA_SHARE_SA_URI_NOT_EXIST = "datashare:///com.acts.datasharetest/SAID=10000/test";
    static constexpr uint32_t CALLERTOKENID = 100;
    static constexpr int64_t USER_TEST = 100;
    static constexpr int32_t TEST_SA_ID = 12321;
};

/**
 * @tc.name: GetFromDataShareConfig001
 * @tc.desc: Verify GetFromDataShareConfig behavior for valid and invalid URIs
 * @tc.type: FUNC
 * @tc.precon: DataProviderConfig is initialized
 * @tc.step:
 *   1. Set from system app to true, use valid URI, expect success
 *   2. Use non-existent URI, expect failure
 * @tc.expect:
 *   1. Return E_OK and correct providerInfo for valid URI
 *   2. Return E_URI_NOT_EXIST for non-existent URI
 */
HWTEST_F(DataProviderConfigMockTest, GetFromDataShareConfig001, TestSize.Level1)
{
    ZLOGI("DataProviderConfigMockTest GetFromDataShareConfig001 start");
    // set from system app true
    DataShareThreadLocal::SetFromSystemApp(true);
    DataProviderConfig providerConfig(DATA_SHARE_SA_URI, CALLERTOKENID);
    auto errCode = providerConfig.GetFromDataShareConfig();
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(providerConfig.providerInfo_.readPermission, "read3");
    EXPECT_TRUE(providerConfig.providerInfo_.allowLists.empty());

    // cannot find uri
    DataProviderConfig providerConfig1(DATA_SHARE_SA_URI_NOT_EXIST, CALLERTOKENID);
    errCode = providerConfig1.GetFromDataShareConfig();
    EXPECT_EQ(errCode, E_URI_NOT_EXIST);
    ZLOGI("DataProviderConfigMockTest GetFromDataShareConfig001 end");
}

/**
 * @tc.name: GetProviderInfo001
 * @tc.desc: Verify GetProviderInfo behavior with valid setup
 * @tc.type: FUNC
 * @tc.precon: DataProviderConfig is initialized
 * @tc.step:
 *   1. Set from system app to true
 *   2. Create DataProviderConfig with valid URI
 *   3. Set visitedUserId to USER_TEST
 *   4. Call GetProviderInfo and verify results
 * @tc.expect:
 *   1. Return E_OK
 *   2. Correct systemAbilityId in providerInfo
 */
HWTEST_F(DataProviderConfigMockTest, GetProviderInfo001, TestSize.Level1)
{
    ZLOGI("DataProviderConfigMockTest GetProviderInfo001 start");
    // set from system app true
    DataShareThreadLocal::SetFromSystemApp(true);
    DataProviderConfig providerConfig(DATA_SHARE_SA_URI, CALLERTOKENID);
    // set visitedUserId = USER_TEST
    providerConfig.providerInfo_.visitedUserId = USER_TEST;
    auto [errCode, providerInfo] = providerConfig.GetProviderInfo();
    EXPECT_EQ(errCode, E_OK);
    EXPECT_EQ(providerInfo.systemAbilityId, TEST_SA_ID);
    ZLOGI("DataProviderConfigMockTest GetProviderInfo001 end");
}
}  // namespace OHOS::Test