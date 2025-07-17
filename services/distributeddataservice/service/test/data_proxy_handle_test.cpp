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
#define LOG_TAG "DataShareProxyHandleTest"

#include "data_share_service_impl.h"

#include <gtest/gtest.h>

#include "accesstoken_kit.h"
#include "dataproxy_handle_common.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "log_print.h"
#include "proxy_data_manager.h"
#include "system_ability_definition.h"
#include "token_setproc.h"

using namespace testing::ext;
using namespace OHOS::DataShare;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;

namespace OHOS::Test {
using OHOS::DataShare::LogLabel;
class DataShareProxyHandleTest : public testing::Test {
public:
    static constexpr int64_t USER_TEST = 100;
    std::string TEST_URI1 = "datashareproxy://com.test.dataprxoyhandle/test1";
    std::string TEST_URI2 = "datashareproxy://com.test.dataprxoyhandle/test2";
    std::string TEST_URI3 = "datashareproxy://com.test.dataprxoyhandle/test3";
    std::string TEST_URI_OTHER = "datashareproxy://com.test.dataprxoyhandle.other/test";
    // value1, value2, value3 are the values in profile
    DataProxyValue VALUE1 = "value1";
    DataProxyValue VALUE2 = "value2";
    DataProxyValue VALUE3 = "value3";
    std::string APPIDENTIFIER1 = "appIdentifier1";
    std::string APPIDENTIFIER2 = "appIdentifier2";
    std::string APPIDENTIFIER3 = "appIdentifier3";
    std::string BUNDLE_NAME = "com.test.dataprxoyhandle";
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetSelfTokenInfo(int32_t user);
    void SetUp();
    void TearDown();
};

void DataShareProxyHandleTest::SetUp(void)
{
    HapInfoParams info = {
        .userID = USER_TEST,
        .bundleName = BUNDLE_NAME,
        .instIndex = 0,
        .appIDDesc = BUNDLE_NAME
    };
    HapPolicyParams policy = {
        .apl = APL_NORMAL,
        .domain = "test.domain",
        .permList = {
            {
                .permissionName = "ohos.permission.GET_BUNDLE_INFO_PRIVILEGED",
                .bundleName = BUNDLE_NAME,
                .grantMode = 1,
                .availableLevel = APL_NORMAL,
                .label = "label",
                .labelId = 1,
                .description = BUNDLE_NAME,
                .descriptionId = 1
            }
        },
        .permStateList = {
            {
                .permissionName = "ohos.permission.GET_BUNDLE_INFO_PRIVILEGED",
                .isGeneral = true,
                .resDeviceID = { "local" },
                .grantStatus = { PermissionState::PERMISSION_GRANTED },
                .grantFlags = { 1 }
            }
        }
    };
    AccessTokenKit::AllocHapToken(info, policy);
    auto testTokenId = Security::AccessToken::AccessTokenKit::GetHapTokenID(
        info.userID, info.bundleName, info.instIndex);
    SetSelfTokenID(testTokenId);
}

void DataShareProxyHandleTest::TearDown(void)
{
    auto tokenId = AccessTokenKit::GetHapTokenID(USER_TEST, "com.test.dataprxoyhandle", 0);
    AccessTokenKit::DeleteToken(tokenId);
}

/**
* @tc.name: OnAppInstallTest001
* @tc.desc: test receive app install event
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProxyHandleTest, OnAppInstallTest001, TestSize.Level1)
{
    ZLOGI("DataShareProxyHandleTest OnAppInstallTest001 start");
    auto testTokenId = GetSelfTokenID();
    ProxyDataManager::GetInstance().OnAppInstall(BUNDLE_NAME, USER_TEST, 0, testTokenId);
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<std::string> uris = { TEST_URI1, TEST_URI2 };
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    auto getResults = dataShareServiceImpl.GetProxyData(uris, config);
    ASSERT_EQ(getResults.size(), uris.size());
    for (auto const &result : getResults) {
        EXPECT_EQ(result.result_, OHOS::DataShare::SUCCESS);
    }
    // see details in profile
    EXPECT_EQ(getResults[0].uri_, TEST_URI1);
    EXPECT_EQ(getResults[0].value_, VALUE1);
    EXPECT_EQ(getResults[0].allowList_.size(), 1);

    EXPECT_EQ(getResults[1].uri_, TEST_URI2);
    EXPECT_EQ(getResults[1].value_, VALUE2);
    EXPECT_EQ(getResults[1].allowList_.size(), 1);

    auto results = dataShareServiceImpl.DeleteProxyData(uris, config);
    ASSERT_EQ(results.size(), uris.size());
    EXPECT_EQ(results[0].uri_, TEST_URI1);
    EXPECT_EQ(results[1].uri_, TEST_URI2);
    for (auto const &result : results) {
        EXPECT_EQ(result.result_, OHOS::DataShare::SUCCESS);
    }
    ZLOGI("DataShareProxyHandleTest OnAppInstallTest001 end");
}

/**
* @tc.name: OnAppInstallTest001
* @tc.desc: test receive app uninstall event
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProxyHandleTest, OnAppUninstallTest001, TestSize.Level1)
{
    ZLOGI("DataShareProxyHandleTest OnAppUninstallTest001 start");
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<DataShareProxyData> proxyDatas;
    std::vector<std::string> allowList = { APPIDENTIFIER1 };
    proxyDatas.emplace_back(TEST_URI1, VALUE1, allowList);
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    auto results = dataShareServiceImpl.PublishProxyData(proxyDatas, config);
    ASSERT_EQ(results.size(), proxyDatas.size());
    for (auto const &result : results) {
        EXPECT_EQ(result.result_, OHOS::DataShare::SUCCESS);
    }
    EXPECT_EQ(results[0].uri_, TEST_URI1);

    std::vector<std::string> uris = { TEST_URI1 };
    auto getResults = dataShareServiceImpl.GetProxyData(uris, config);
    ASSERT_EQ(getResults.size(), uris.size());
    for (auto const &result : getResults) {
        EXPECT_EQ(result.result_, OHOS::DataShare::SUCCESS);
    }
    EXPECT_EQ(getResults[0].uri_, TEST_URI1);
    EXPECT_EQ(getResults[0].value_, VALUE1);
    EXPECT_EQ(getResults[0].allowList_.size(), 1);

    auto testTokenId = GetSelfTokenID();
    ProxyDataManager::GetInstance().OnAppUninstall(BUNDLE_NAME, USER_TEST, 0, testTokenId);

    getResults = dataShareServiceImpl.GetProxyData(uris, config);
    ASSERT_EQ(results.size(), uris.size());
    for (auto const &result : getResults) {
        EXPECT_EQ(result.result_, OHOS::DataShare::URI_NOT_EXIST);
    }
    EXPECT_EQ(getResults[0].uri_, TEST_URI1);
    ZLOGI("DataShareProxyHandleTest OnAppUninstallTest001 end");
}

/**
* @tc.name: OnAppUpdate001
* @tc.desc: test receive app update event
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProxyHandleTest, OnAppUpdateTest001, TestSize.Level1)
{
    ZLOGI("DataShareProxyHandleTest OnAppUpdateTest001 start");
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<DataShareProxyData> proxyDatas;
    std::vector<std::string> allowList = { APPIDENTIFIER3 };
    proxyDatas.emplace_back(TEST_URI3, VALUE3, allowList);
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    auto results = dataShareServiceImpl.PublishProxyData(proxyDatas, config);
    ASSERT_EQ(results.size(), proxyDatas.size());
    for (auto const &result : results) {
        EXPECT_EQ(result.result_, OHOS::DataShare::SUCCESS);
    }
    EXPECT_EQ(results[0].uri_, TEST_URI3);

    std::vector<std::string> uris = { TEST_URI3 };
    auto getResults = dataShareServiceImpl.GetProxyData(uris, config);
    ASSERT_EQ(getResults.size(), uris.size());
    for (auto const &result : getResults) {
        EXPECT_EQ(result.result_, OHOS::DataShare::SUCCESS);
    }
    EXPECT_EQ(getResults[0].uri_, TEST_URI3);
    EXPECT_EQ(getResults[0].value_, VALUE3);
    EXPECT_EQ(getResults[0].allowList_.size(), 1);

    auto testTokenId = GetSelfTokenID();
    ProxyDataManager::GetInstance().OnAppUpdate(BUNDLE_NAME, USER_TEST, 0, testTokenId);

    std::vector<std::string> urisAfterUpdate = { TEST_URI1, TEST_URI2, TEST_URI3 };
    getResults = dataShareServiceImpl.GetProxyData(urisAfterUpdate, config);
    ASSERT_EQ(getResults.size(), urisAfterUpdate.size());
    EXPECT_EQ(getResults[0].uri_, TEST_URI1);
    EXPECT_EQ(getResults[0].result_, OHOS::DataShare::SUCCESS);
    EXPECT_EQ(getResults[1].uri_, TEST_URI2);
    EXPECT_EQ(getResults[1].result_, OHOS::DataShare::SUCCESS);
    EXPECT_EQ(getResults[2].uri_, TEST_URI3);
    EXPECT_EQ(getResults[2].result_, OHOS::DataShare::URI_NOT_EXIST);

    std::vector<std::string> urisToDelete = { TEST_URI1, TEST_URI2 };
    results = dataShareServiceImpl.DeleteProxyData(uris, config);
    ASSERT_EQ(results.size(), uris.size());
    EXPECT_EQ(results[0].uri_, TEST_URI1);
    EXPECT_EQ(results[1].uri_, TEST_URI2);
    for (auto const &result : results) {
        EXPECT_EQ(result.result_, OHOS::DataShare::SUCCESS);
    }
    ZLOGI("DataShareProxyHandleTest OnAppUpdateTest001 end");
}

/**
* @tc.name: PublishProxyData001
* @tc.desc: test publish proxyData with string value function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProxyHandleTest, PublishProxyData001, TestSize.Level1)
{
    ZLOGI("DataShareProxyHandleTest PublishProxyData001 start");
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<DataShareProxyData> proxyDatas;
    DataProxyValue strValue = "strValue";
    std::vector<std::string> allowList = { APPIDENTIFIER1 };
    proxyDatas.emplace_back(TEST_URI1, strValue, allowList);
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    auto results = dataShareServiceImpl.PublishProxyData(proxyDatas, config);
    ASSERT_EQ(results.size(), proxyDatas.size());
    EXPECT_EQ(results[0].uri_, TEST_URI1);
    EXPECT_EQ(results[0].result_, OHOS::DataShare::SUCCESS);

    std::vector<std::string> uris = { TEST_URI1 };
    auto getResults = dataShareServiceImpl.GetProxyData(uris, config);
    ASSERT_EQ(getResults.size(), uris.size());
    EXPECT_EQ(getResults[0].uri_, TEST_URI1);
    EXPECT_EQ(getResults[0].result_, OHOS::DataShare::SUCCESS);
    EXPECT_EQ(getResults[0].value_, strValue);
    EXPECT_EQ(getResults[0].allowList_.size(), 1);

    results = dataShareServiceImpl.DeleteProxyData(uris, config);
    ASSERT_EQ(results.size(), uris.size());
    EXPECT_EQ(results[0].uri_, TEST_URI1);
    EXPECT_EQ(results[0].result_, OHOS::DataShare::SUCCESS);
    ZLOGI("DataShareProxyHandleTest PublishProxyData001 end");
}

/**
* @tc.name: PublishProxyData002
* @tc.desc: test publish proxyData with int value function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProxyHandleTest, PublishProxyData002, TestSize.Level1)
{
    ZLOGI("DataShareProxyHandleTest PublishProxyData002 start");
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<DataShareProxyData> proxyDatas;
    DataProxyValue intValue = 100;
    std::vector<std::string> allowList = { APPIDENTIFIER1 };
    proxyDatas.emplace_back(TEST_URI1, intValue, allowList);
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    auto results = dataShareServiceImpl.PublishProxyData(proxyDatas, config);
    ASSERT_EQ(results.size(), proxyDatas.size());
    EXPECT_EQ(results[0].uri_, TEST_URI1);
    EXPECT_EQ(results[0].result_, OHOS::DataShare::SUCCESS);

    std::vector<std::string> uris = { TEST_URI1 };
    auto getResults = dataShareServiceImpl.GetProxyData(uris, config);
    ASSERT_EQ(getResults.size(), uris.size());
    EXPECT_EQ(getResults[0].uri_, TEST_URI1);
    EXPECT_EQ(getResults[0].result_, OHOS::DataShare::SUCCESS);
    EXPECT_EQ(getResults[0].value_, intValue);
    EXPECT_EQ(getResults[0].allowList_.size(), 1);

    results = dataShareServiceImpl.DeleteProxyData(uris, config);
    ASSERT_EQ(results.size(), uris.size());
    EXPECT_EQ(results[0].uri_, TEST_URI1);
    EXPECT_EQ(results[0].result_, OHOS::DataShare::SUCCESS);
    ZLOGI("DataShareProxyHandleTest PublishProxyData002 end");
}

/**
* @tc.name: PublishProxyData003
* @tc.desc: test publish proxyData with double value function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProxyHandleTest, PublishProxyData003, TestSize.Level1)
{
    ZLOGI("DataShareProxyHandleTest PublishProxyData003 start");
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<DataShareProxyData> proxyDatas;
    DataProxyValue fValue = 100.00;
    std::vector<std::string> allowList = { APPIDENTIFIER1 };
    proxyDatas.emplace_back(TEST_URI1, fValue, allowList);
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    auto results = dataShareServiceImpl.PublishProxyData(proxyDatas, config);
    ASSERT_EQ(results.size(), proxyDatas.size());
    EXPECT_EQ(results[0].uri_, TEST_URI1);
    EXPECT_EQ(results[0].result_, OHOS::DataShare::SUCCESS);

    std::vector<std::string> uris = { TEST_URI1 };
    auto getResults = dataShareServiceImpl.GetProxyData(uris, config);
    ASSERT_EQ(getResults.size(), uris.size());
    EXPECT_EQ(getResults[0].uri_, TEST_URI1);
    EXPECT_EQ(getResults[0].result_, OHOS::DataShare::SUCCESS);
    EXPECT_EQ(getResults[0].value_, fValue);
    EXPECT_EQ(getResults[0].allowList_.size(), 1);

    results = dataShareServiceImpl.DeleteProxyData(uris, config);
    ASSERT_EQ(results.size(), uris.size());
    EXPECT_EQ(results[0].uri_, TEST_URI1);
    EXPECT_EQ(results[0].result_, OHOS::DataShare::SUCCESS);
    ZLOGI("DataShareProxyHandleTest PublishProxyData003 end");
}

/**
* @tc.name: PublishProxyData004
* @tc.desc: test publish proxyData with bool value function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProxyHandleTest, PublishProxyData004, TestSize.Level1)
{
    ZLOGI("DataShareProxyHandleTest PublishProxyData004 start");
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<DataShareProxyData> proxyDatas;
    DataProxyValue boolValue = true;
    std::vector<std::string> allowList = { APPIDENTIFIER1 };
    proxyDatas.emplace_back(TEST_URI1, boolValue, allowList);
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    auto results = dataShareServiceImpl.PublishProxyData(proxyDatas, config);
    ASSERT_EQ(results.size(), proxyDatas.size());
    EXPECT_EQ(results[0].uri_, TEST_URI1);
    EXPECT_EQ(results[0].result_, OHOS::DataShare::SUCCESS);

    std::vector<std::string> uris = { TEST_URI1 };
    auto getResults = dataShareServiceImpl.GetProxyData(uris, config);
    ASSERT_EQ(getResults.size(), uris.size());
    EXPECT_EQ(getResults[0].uri_, TEST_URI1);
    EXPECT_EQ(getResults[0].result_, OHOS::DataShare::SUCCESS);
    EXPECT_EQ(getResults[0].value_, boolValue);
    EXPECT_EQ(getResults[0].allowList_.size(), 1);

    results = dataShareServiceImpl.DeleteProxyData(uris, config);
    ASSERT_EQ(results.size(), uris.size());
    EXPECT_EQ(results[0].uri_, TEST_URI1);
    EXPECT_EQ(results[0].result_, OHOS::DataShare::SUCCESS);
    ZLOGI("DataShareProxyHandleTest PublishProxyData004 end");
}

/**
* @tc.name: PublishProxyData005
* @tc.desc: test publish proxyData of other bundle function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProxyHandleTest, PublishProxyData005, TestSize.Level1)
{
    ZLOGI("DataShareProxyHandleTest PublishProxyData005 start");
    DataShareServiceImpl dataShareServiceImpl;
    std::vector<DataShareProxyData> proxyDatas;
    DataProxyValue boolValue = true;
    std::vector<std::string> allowList = { APPIDENTIFIER1 };
    proxyDatas.emplace_back(TEST_URI_OTHER, boolValue, allowList);
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    std::vector<std::string> uris = { TEST_URI_OTHER };
    auto results = dataShareServiceImpl.PublishProxyData(proxyDatas, config);
    ASSERT_EQ(results.size(), proxyDatas.size());
    EXPECT_EQ(results[0].uri_, TEST_URI_OTHER);
    EXPECT_EQ(results[0].result_, OHOS::DataShare::NO_PERMISSION);
    ZLOGI("DataShareProxyHandleTest PublishProxyData005 end");
}

/**
* @tc.name: DeleteProxyData001
* @tc.desc: test delete proxyData which is not published function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProxyHandleTest, DeleteProxyData001, TestSize.Level1)
{
    ZLOGI("DataShareProxyHandleTest DeleteProxyData001 start");
    DataShareServiceImpl dataShareServiceImpl;
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    std::vector<std::string> uris = { TEST_URI1 };
    auto results = dataShareServiceImpl.DeleteProxyData(uris, config);
    ASSERT_EQ(results.size(), uris.size());
    EXPECT_EQ(results[0].uri_, TEST_URI1);
    EXPECT_EQ(results[0].result_, OHOS::DataShare::URI_NOT_EXIST);
    ZLOGI("DataShareProxyHandleTest DeleteProxyData001 end");
}

/**
* @tc.name: DeleteProxyData002
* @tc.desc: test delete proxyData without permission function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProxyHandleTest, DeleteProxyData002, TestSize.Level1)
{
    ZLOGI("DataShareProxyHandleTest DeleteProxyData002 start");
    DataShareServiceImpl dataShareServiceImpl;
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    std::vector<std::string> uris = { TEST_URI_OTHER };
    auto results = dataShareServiceImpl.DeleteProxyData(uris, config);
    ASSERT_EQ(results.size(), uris.size());
    EXPECT_EQ(results[0].uri_, TEST_URI_OTHER);
    EXPECT_EQ(results[0].result_, OHOS::DataShare::NO_PERMISSION);
    ZLOGI("DataShareProxyHandleTest DeleteProxyData002 end");
}

/**
* @tc.name: GetProxyData001
* @tc.desc: test get proxyData without permission function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProxyHandleTest, GetProxyData001, TestSize.Level1)
{
    ZLOGI("DataShareProxyHandleTest GetProxyData001 start");
    DataShareServiceImpl dataShareServiceImpl;
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    std::vector<std::string> uris = { TEST_URI1 };
    auto getResults = dataShareServiceImpl.GetProxyData(uris, config);
    ASSERT_EQ(getResults.size(), uris.size());
    EXPECT_EQ(getResults[0].uri_, TEST_URI1);
    EXPECT_EQ(getResults[0].result_, OHOS::DataShare::URI_NOT_EXIST);
    ZLOGI("DataShareProxyHandleTest GetProxyData001 end");
}

/**
* @tc.name: GetProxyData002
* @tc.desc: test get proxyData without permission function
* @tc.type: FUNC
* @tc.require:SQL
*/
HWTEST_F(DataShareProxyHandleTest, GetProxyData002, TestSize.Level1)
{
    ZLOGI("DataShareProxyHandleTest GetProxyData002 start");
    auto tokenId = AccessTokenKit::GetHapTokenID(USER_TEST, BUNDLE_NAME, 0);
    AccessTokenKit::DeleteToken(tokenId);
    DataShareServiceImpl dataShareServiceImpl;
    DataProxyConfig config;
    config.type_ = DataProxyType::SHARED_CONFIG;
    std::vector<std::string> uris = { TEST_URI1 };
    auto getResults = dataShareServiceImpl.GetProxyData(uris, config);
    ASSERT_EQ(getResults.size(), uris.size());
    EXPECT_EQ(getResults[0].uri_, TEST_URI1);
    EXPECT_EQ(getResults[0].result_, OHOS::DataShare::NO_PERMISSION);
    SetSelfTokenID(tokenId);
    ZLOGI("DataShareProxyHandleTest GetProxyData002 end");
}
} // namespace OHOS::Test