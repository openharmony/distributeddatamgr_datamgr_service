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
#define LOG_TAG "DataProviderConfigTest"

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
class DataProviderConfigTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
    std::string DATA_SHARE_SA_URI = "datashareproxy://com.acts.datasharetest/SAID=12321/test";
    static constexpr uint32_t CALLERTOKENID = 100;
    static constexpr int64_t USER_TEST = 100;
};

/**
 * @tc.name: GetDataShareSAConfigInfo001
 * @tc.desc: Verify GetDataShareSAConfigInfo behavior with different URI configurations
 * @tc.type: FUNC
 * @tc.precon: DataProviderConfig is initialized
 * @tc.step:
 *   1. Set from system app to false, authority and pathSegments empty, expect E_URI_NOT_EXIST
 *   2. Set from system app to false, authority empty, pathSegments not empty, expect E_NOT_SYSTEM_APP
 *   3. Set from system app to false, authority not empty, expect E_NOT_SYSTEM_APP
 * @tc.expect:
 *   1. Return E_URI_NOT_EXIST, bundleName empty
 *   2. Return E_NOT_SYSTEM_APP, bundleName set to pathSegments
 *   3. Return E_NOT_SYSTEM_APP, bundleName set to authority
 */
HWTEST_F(DataProviderConfigTest, GetDataShareSAConfigInfo001, TestSize.Level1)
{
    ZLOGI("DataProviderConfigTest GetDataShareSAConfigInfo001 start");
    // set from system app false
    DataShareThreadLocal::SetFromSystemApp(false);
    DataProviderConfig providerConfig(DATA_SHARE_SA_URI, CALLERTOKENID);
    // uriConfig_.authority and uriConfig_.pathSegments are both empty
    providerConfig.uriConfig_.authority = "";
    providerConfig.uriConfig_.pathSegments = {};
    auto [errCode1, configInfo1] = providerConfig.GetDataShareSAConfigInfo();
    EXPECT_EQ(errCode1, E_URI_NOT_EXIST);
    EXPECT_TRUE(providerConfig.providerInfo_.bundleName.empty());

    // uriConfig_.authority is empty but uriConfig_.pathSegments is not empty
    providerConfig.uriConfig_.pathSegments.push_back("bundleName");
    auto [errCode2, configInfo2] = providerConfig.GetDataShareSAConfigInfo();
    EXPECT_EQ(errCode2, E_NOT_SYSTEM_APP);
    EXPECT_EQ(providerConfig.providerInfo_.bundleName, "bundleName");

    // uriConfig_.authority is not empty
    providerConfig.uriConfig_.authority = "test";
    auto [errCode3, configInfo3] = providerConfig.GetDataShareSAConfigInfo();
    EXPECT_EQ(errCode3, E_NOT_SYSTEM_APP);
    EXPECT_EQ(providerConfig.providerInfo_.bundleName, "test");
    ZLOGI("DataProviderConfigTest GetDataShareSAConfigInfo001 end");
}

/**
 * @tc.name: GetFromDataShareConfig001
 * @tc.desc: Verify GetFromDataShareConfig behavior with empty URI configuration
 * @tc.type: FUNC
 * @tc.precon: DataProviderConfig is initialized
 * @tc.step:
 *   1. Set from system app to false
 *   2. Create DataProviderConfig with empty authority and pathSegments
 *   3. Call GetFromDataShareConfig and verify results
 * @tc.expect:
 *   1. Return E_URI_NOT_EXIST
 *   2. bundleName is empty
 */
HWTEST_F(DataProviderConfigTest, GetFromDataShareConfig001, TestSize.Level1)
{
    ZLOGI("DataProviderConfigTest GetFromDataShareConfig001 start");
    // set from system app false
    DataShareThreadLocal::SetFromSystemApp(false);
    DataProviderConfig providerConfig(DATA_SHARE_SA_URI, CALLERTOKENID);
    // uriConfig_.authority and uriConfig_.pathSegments are both empty
    providerConfig.uriConfig_.authority = "";
    providerConfig.uriConfig_.pathSegments = {};
    auto errCode = providerConfig.GetFromDataShareConfig();
    EXPECT_EQ(errCode, E_URI_NOT_EXIST);
    EXPECT_TRUE(providerConfig.providerInfo_.bundleName.empty());
    ZLOGI("DataProviderConfigTest GetFromDataShareConfig001 end");
}

/**
 * @tc.name: GetProviderInfo001
 * @tc.desc: Verify GetProviderInfo behavior with empty URI configuration
 * @tc.type: FUNC
 * @tc.precon: DataProviderConfig is initialized
 * @tc.step:
 *   1. Set from system app to true
 *   2. Create DataProviderConfig with valid URI and set visitedUserId
 *   3. Set authority and pathSegments to empty
 *   4. Call GetProviderInfo and verify results
 * @tc.expect:
 *   1. Return E_ERROR
 *   2. systemAbilityId is URIUtils::INVALID_SA_ID
 */
HWTEST_F(DataProviderConfigTest, GetProviderInfo001, TestSize.Level1)
{
    ZLOGI("DataProviderConfigTest GetProviderInfo001 start");
    // set from system app false
    DataShareThreadLocal::SetFromSystemApp(true);
    DataProviderConfig providerConfig(DATA_SHARE_SA_URI, CALLERTOKENID);
    // set visitedUserId = USER_TEST
    providerConfig.providerInfo_.visitedUserId = USER_TEST;
    // uriConfig_.authority and uriConfig_.pathSegments are both empty
    auto [errCode, providerInfo] = providerConfig.GetProviderInfo();
    EXPECT_EQ(errCode, E_ERROR);
    EXPECT_EQ(providerInfo.systemAbilityId, URIUtils::INVALID_SA_ID);
    ZLOGI("DataProviderConfigTest GetProviderInfo001 end");
}
}  // namespace OHOS::Test