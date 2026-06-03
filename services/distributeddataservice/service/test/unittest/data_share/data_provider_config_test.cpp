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
 * @tc.desc: Verify GetDataShareSAConfigInfo behavior with different URI configurations (no system app check now)
 * @tc.type: FUNC
 * @tc.precon: DataProviderConfig is initialized
 * @tc.step:
 *   1. Set from system app to false, authority and pathSegments empty, expect E_URI_NOT_EXIST
 *   2. Set from system app to false, authority empty, pathSegments not empty, expect E_ERROR
 *   3. Set from system app to false, authority not empty, expect E_ERROR
 * @tc.expect:
 *   1. Return E_URI_NOT_EXIST, bundleName empty
 *   2. Return E_ERROR (no system app check, LoadConfigInfo failed)
 *   3. Return E_ERROR (no system app check, LoadConfigInfo failed)
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
    EXPECT_EQ(errCode2, E_ERROR);
    EXPECT_EQ(providerConfig.providerInfo_.bundleName, "bundleName");

    // uriConfig_.authority is not empty
    providerConfig.uriConfig_.authority = "test";
    auto [errCode3, configInfo3] = providerConfig.GetDataShareSAConfigInfo();
    EXPECT_EQ(errCode3, E_ERROR);
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

/**
 * @tc.name: GetFromDataShareConfig_NormalAppAccessible001
 * @tc.desc: Verify GetFromDataShareConfig behavior with normalAppAccessible=false and non-system app
 * @tc.type: FUNC
 * @tc.precon: DataProviderConfig is initialized with cached config info
 * @tc.step:
 *   1. Set from system app to false
 *   2. Insert config cache with proxyData (normalAppAccessible=false)
 *   3. Call GetFromDataShareConfig and verify E_NOT_SYSTEM_APP is returned
 * @tc.expect:
 *   1. Return E_NOT_SYSTEM_APP when non-system app accesses normalAppAccessible=false proxyData
 */
HWTEST_F(DataProviderConfigTest, GetFromDataShareConfig_NormalAppAccessible001, TestSize.Level1)
{
    ZLOGI("DataProviderConfigTest GetFromDataShareConfig_NormalAppAccessible001 start");
    DataShareThreadLocal::SetFromSystemApp(false);
    std::string testUri = "datashareproxy://com.acts.datasharetest/SAID=12321/test";
    DataProviderConfig providerConfig(testUri, CALLERTOKENID);
    providerConfig.providerInfo_.systemAbilityId = 12321;

    SAConfigProxyData configProxyData;
    configProxyData.uri = "datashareproxy://com.acts.datasharetest/SAID=12321/t";
    configProxyData.normalAppAccessible = false;
    DataShareSAConfigInfo cacheInfo;
    cacheInfo.proxyData.push_back(configProxyData);

    auto configInfoMgr = DataShareSAConfigInfoManager::GetInstance();
    std::string cacheKey = "com.acts.datasharetest12321";
    configInfoMgr->configCache_.Insert(cacheKey, cacheInfo);

    auto errCode = providerConfig.GetFromDataShareConfig();
    EXPECT_EQ(errCode, E_NOT_SYSTEM_APP);

    configInfoMgr->configCache_.Erase(cacheKey);
    ZLOGI("DataProviderConfigTest GetFromDataShareConfig_NormalAppAccessible001 end");
}

/**
 * @tc.name: GetFromDataShareConfig_NormalAppAccessible002
 * @tc.desc: Verify GetFromDataShareConfig behavior with normalAppAccessible=true and non-system app
 * @tc.type: FUNC
 * @tc.precon: DataProviderConfig is initialized with cached config info
 * @tc.step:
 *   1. Set from system app to false
 *   2. Insert config cache with proxyData (normalAppAccessible=true)
 *   3. Call GetFromDataShareConfig and verify E_OK is returned
 * @tc.expect:
 *   1. Return E_OK when non-system app accesses normalAppAccessible=true proxyData
 */
HWTEST_F(DataProviderConfigTest, GetFromDataShareConfig_NormalAppAccessible002, TestSize.Level1)
{
    ZLOGI("DataProviderConfigTest GetFromDataShareConfig_NormalAppAccessible002 start");
    DataShareThreadLocal::SetFromSystemApp(false);
    std::string testUri = "datashareproxy://com.acts.datasharetest/SAID=12321/test_access";
    DataProviderConfig providerConfig(testUri, CALLERTOKENID);
    providerConfig.providerInfo_.systemAbilityId = 12321;

    SAConfigProxyData configProxyData;
    configProxyData.uri = "datashareproxy://com.acts.datasharetest/SAID=12321/test_access";
    configProxyData.normalAppAccessible = true;
    DataShareSAConfigInfo cacheInfo;
    cacheInfo.proxyData.push_back(configProxyData);

    auto configInfoMgr = DataShareSAConfigInfoManager::GetInstance();
    std::string cacheKey = "com.acts.datasharetest12321";
    configInfoMgr->configCache_.Insert(cacheKey, cacheInfo);

    auto errCode = providerConfig.GetFromDataShareConfig();
    EXPECT_EQ(errCode, E_OK);

    configInfoMgr->configCache_.Erase(cacheKey);
    ZLOGI("DataProviderConfigTest GetFromDataShareConfig_NormalAppAccessible002 end");
}

/**
 * @tc.name: GetFromDataShareConfig_NormalAppAccessible003
 * @tc.desc: Verify GetFromDataShareConfig behavior with normalAppAccessible=false and system app
 * @tc.type: FUNC
 * @tc.precon: DataProviderConfig is initialized with cached config info
 * @tc.step:
 *   1. Set from system app to true
 *   2. Insert config cache with proxyData (normalAppAccessible=false)
 *   3. Call GetFromDataShareConfig and verify E_OK is returned
 * @tc.expect:
 *   1. Return E_OK when system app accesses any proxyData regardless of normalAppAccessible
 */
HWTEST_F(DataProviderConfigTest, GetFromDataShareConfig_NormalAppAccessible003, TestSize.Level1)
{
    ZLOGI("DataProviderConfigTest GetFromDataShareConfig_NormalAppAccessible003 start");
    DataShareThreadLocal::SetFromSystemApp(true);
    std::string testUri = "datashareproxy://com.acts.datasharetest/SAID=12321/test";
    DataProviderConfig providerConfig(testUri, CALLERTOKENID);
    providerConfig.providerInfo_.systemAbilityId = 12321;

    SAConfigProxyData configProxyData;
    configProxyData.uri = "datashareproxy://com.acts.datasharetest/SAID=12321/t";
    configProxyData.normalAppAccessible = false;
    DataShareSAConfigInfo cacheInfo;
    cacheInfo.proxyData.push_back(configProxyData);

    auto configInfoMgr = DataShareSAConfigInfoManager::GetInstance();
    std::string cacheKey = "com.acts.datasharetest12321";
    configInfoMgr->configCache_.Insert(cacheKey, cacheInfo);

    auto errCode = providerConfig.GetFromDataShareConfig();
    EXPECT_EQ(errCode, E_OK);

    configInfoMgr->configCache_.Erase(cacheKey);
    ZLOGI("DataProviderConfigTest GetFromDataShareConfig_NormalAppAccessible003 end");
}

/**
 * @tc.name: VerifyProvider_NormalAppAccessible001
 * @tc.desc: Verify VerifyProvider returns true when normalAppAccessible=true and non-system app
 * @tc.type: FUNC
 * @tc.precon: ProviderInfo has normalAppAccessible=true
 * @tc.step:
 *   1. Set from system app to false
 *   2. Create ProviderInfo with normalAppAccessible=true
 *   3. Call VerifyProvider and verify true is returned
 * @tc.expect:
 *   1. Return true when normalAppAccessible=true regardless of allowlist
 */
HWTEST_F(DataProviderConfigTest, VerifyProvider_NormalAppAccessible001, TestSize.Level1)
{
    ZLOGI("DataProviderConfigTest VerifyProvider_NormalAppAccessible001 start");
    DataShareThreadLocal::SetFromSystemApp(false);
    DataProviderConfig::ProviderInfo providerInfo;
    providerInfo.normalAppAccessible = true;
    providerInfo.bundleName = "testBundle";
    providerInfo.appIdentifier = "";

    bool result = VerifyProvider(providerInfo, 0);
    EXPECT_TRUE(result);
    ZLOGI("DataProviderConfigTest VerifyProvider_NormalAppAccessible001 end");
}

/**
 * @tc.name: VerifyProvider_NormalAppAccessible002
 * @tc.desc: Verify VerifyProvider returns true when IsFromSystemApp=true
 * @tc.type: FUNC
 * @tc.precon: IsFromSystemApp is true
 * @tc.step:
 *   1. Set from system app to true
 *   2. Create ProviderInfo with normalAppAccessible=false
 *   3. Call VerifyProvider and verify true is returned
 * @tc.expect:
 *   1. Return true when system app regardless of normalAppAccessible
 */
HWTEST_F(DataProviderConfigTest, VerifyProvider_NormalAppAccessible002, TestSize.Level1)
{
    ZLOGI("DataProviderConfigTest VerifyProvider_NormalAppAccessible002 start");
    DataShareThreadLocal::SetFromSystemApp(true);
    DataProviderConfig::ProviderInfo providerInfo;
    providerInfo.normalAppAccessible = false;
    providerInfo.bundleName = "testBundle";

    bool result = VerifyProvider(providerInfo, 0);
    EXPECT_TRUE(result);
    ZLOGI("DataProviderConfigTest VerifyProvider_NormalAppAccessible002 end");
}
}  // namespace OHOS::Test