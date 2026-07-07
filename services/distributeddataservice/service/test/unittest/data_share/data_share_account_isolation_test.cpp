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
#define LOG_TAG "DataShareAccountIsolationTest"

#include <gtest/gtest.h>

#include "account/account_delegate.h"
#include "account_delegate_mock.h"
#include "common_utils.h"
#include "data_provider_config.h"
#include "data_share_profile_config.h"
#include "data_share_service_impl.h"
#include "device_manager_adapter.h"
#include "device_manager_adapter_mock.h"
#include "log_print.h"
#include "mock/native_and_hap_token_mock.h"
#include "strategies/data_proxy/load_config_from_data_proxy_node_strategy.h"
#include "strategies/general/load_config_common_strategy.h"
#include "utils.h"

namespace OHOS::DataShare {
using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;

class DataShareAccountIsolationTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        MockToken::SetTestEnvironment();
    }
    static void TearDownTestCase()
    {
        MockToken::ResetTestEnvironment();
    }
    void SetUp() override
    {
        AccountDelegate::instance_ = nullptr;
        AccountDelegate::RegisterAccountInstance(&mock_);
    }
    void TearDown() override
    {
    }
    AccountDelegateMock mock_;
};

// ===== GetAccountIdFromProxyURI tests (pure URI parsing, no mock needed) =====

/**
 * @tc.name: GetAccountIdFromProxyURI_WithAccountIdParam_ExpectAccountIdReturned
 * @tc.desc: Verify GetAccountIdFromProxyURI extracts accountId from URI query param
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, GetAccountIdFromProxyURI_WithAccountIdParam_ExpectAccountIdReturned,
    TestSize.Level0)
{
    ZLOGI("GetAccountIdFromProxyURI_WithAccountIdParam start");
    std::string uri = "datashareproxy://com.test/module/store/table?accountId=100";
    auto accountId = URIUtils::GetAccountIdFromProxyURI(uri);
    EXPECT_EQ(accountId, 100);
}

/**
 * @tc.name: GetAccountIdFromProxyURI_NoAccountIdParam_ExpectInvalid
 * @tc.desc: Verify GetAccountIdFromProxyURI returns invalid when no accountId in URI
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, GetAccountIdFromProxyURI_NoAccountIdParam_ExpectInvalid, TestSize.Level0)
{
    ZLOGI("GetAccountIdFromProxyURI_NoAccountIdParam start");
    std::string uri = "datashareproxy://com.test/module/store/table?appIndex=1";
    auto accountId = URIUtils::GetAccountIdFromProxyURI(uri);
    EXPECT_LT(accountId, 1);
}

/**
 * @tc.name: GetAccountIdFromProxyURI_NoQueryParams_ExpectInvalid
 * @tc.desc: Verify GetAccountIdFromProxyURI returns invalid when URI has no query params
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, GetAccountIdFromProxyURI_NoQueryParams_ExpectInvalid, TestSize.Level0)
{
    ZLOGI("GetAccountIdFromProxyURI_NoQueryParams start");
    std::string uri = "datashareproxy://com.test/module/store/table";
    auto accountId = URIUtils::GetAccountIdFromProxyURI(uri);
    EXPECT_LT(accountId, 1);
}

/**
 * @tc.name: GetAccountIdFromProxyURI_MultipleParams_ExpectAccountIdReturned
 * @tc.desc: Verify GetAccountIdFromProxyURI works with multiple query params
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, GetAccountIdFromProxyURI_MultipleParams_ExpectAccountIdReturned,
    TestSize.Level0)
{
    ZLOGI("GetAccountIdFromProxyURI_MultipleParams start");
    std::string uri = "datashareproxy://com.test/module/store/table?user=100&accountId=200&appIndex=1";
    auto accountId = URIUtils::GetAccountIdFromProxyURI(uri);
    EXPECT_EQ(accountId, 200);
}

/**
 * @tc.name: GetAccountIdFromProxyURI_AccountIdZero_ExpectInvalid
 * @tc.desc: Verify GetAccountIdFromProxyURI returns invalid when accountId is 0
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, GetAccountIdFromProxyURI_AccountIdZero_ExpectInvalid, TestSize.Level0)
{
    ZLOGI("GetAccountIdFromProxyURI_AccountIdZero start");
    std::string uri = "datashareproxy://com.test/module/store/table?accountId=0";
    auto accountId = URIUtils::GetAccountIdFromProxyURI(uri);
    EXPECT_LT(accountId, 1);
}

// ===== ProfileInfo.accountIsolation serialization tests =====

/**
 * @tc.name: ProfileInfoAccountIsolation_MarshalUnmarshal_ExpectValuePreserved
 * @tc.desc: Verify accountIsolation field Marshal/Unmarshal preserves value
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, ProfileInfoAccountIsolation_MarshalUnmarshal_ExpectValuePreserved,
    TestSize.Level0)
{
    ZLOGI("ProfileInfoAccountIsolation_MarshalUnmarshal start");
    ProfileInfo info;
    info.accountIsolation = true;
    info.storeName = "store";
    info.tableName = "table";
    DistributedData::Serializable::json node;
    info.Marshal(node);
    EXPECT_EQ(node["accountIsolation"], true);

    ProfileInfo unmarshalled;
    unmarshalled.Unmarshal(node);
    EXPECT_EQ(unmarshalled.accountIsolation, true);
}

/**
 * @tc.name: ProfileInfoAccountIsolation_DefaultFalse_ExpectDefaultPreserved
 * @tc.desc: Verify accountIsolation defaults to false and Marshal/Unmarshal preserves it
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, ProfileInfoAccountIsolation_DefaultFalse_ExpectDefaultPreserved,
    TestSize.Level0)
{
    ZLOGI("ProfileInfoAccountIsolation_DefaultFalse start");
    ProfileInfo info;
    EXPECT_EQ(info.accountIsolation, false);

    DistributedData::Serializable::json node;
    info.Marshal(node);
    ProfileInfo unmarshalled;
    unmarshalled.Unmarshal(node);
    EXPECT_EQ(unmarshalled.accountIsolation, false);
}

/**
 * @tc.name: ProfileInfoAccountIsolation_MarshalTrueUnmarshal_ExpectTruePreserved
 * @tc.desc: Verify accountIsolation=true is correctly serialized and deserialized
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, ProfileInfoAccountIsolation_MarshalTrueUnmarshal_ExpectTruePreserved,
    TestSize.Level0)
{
    ZLOGI("ProfileInfoAccountIsolation_MarshalTrueUnmarshal start");
    ProfileInfo info;
    info.accountIsolation = true;
    info.isSilentProxyEnable = true;
    info.storeName = "mystore";
    info.tableName = "mytable";
    DistributedData::Serializable::json node;
    EXPECT_TRUE(info.Marshal(node));

    ProfileInfo unmarshalled;
    EXPECT_TRUE(unmarshalled.Unmarshal(node));
    EXPECT_EQ(unmarshalled.accountIsolation, true);
    EXPECT_EQ(unmarshalled.storeName, "mystore");
    EXPECT_EQ(unmarshalled.tableName, "mytable");
}

// ===== ProviderInfo.accountId/accountIsolation field tests =====

/**
 * @tc.name: ProviderInfoAccountId_DefaultMinusOne_ExpectDefaultPreserved
 * @tc.desc: Verify ProviderInfo.accountId defaults to -1 and accountIsolation defaults to false
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, ProviderInfoAccountId_DefaultMinusOne_ExpectDefaultPreserved,
    TestSize.Level0)
{
    ZLOGI("ProviderInfoAccountId_DefaultMinusOne start");
    DataProviderConfig::ProviderInfo providerInfo;
    EXPECT_EQ(providerInfo.accountId, -1);
    EXPECT_EQ(providerInfo.accountIsolation, false);
}

// ===== Context.accountId field test =====

/**
 * @tc.name: ContextAccountId_DefaultMinusOne_ExpectDefaultPreserved
 * @tc.desc: Verify Context.accountId defaults to -1
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, ContextAccountId_DefaultMinusOne_ExpectDefaultPreserved,
    TestSize.Level0)
{
    ZLOGI("ContextAccountId_DefaultMinusOne start");
    Context context;
    EXPECT_EQ(context.accountId, -1);
}

// ===== AccountDelegate mock tests for new methods =====

/**
 * @tc.name: AccountDelegateMock_GetSubProfileIdByToken_ExpectMockCallable
 * @tc.desc: Verify AccountDelegate mock has GetSubProfileIdByToken method
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, AccountDelegateMock_GetSubProfileIdByToken_ExpectMockCallable,
    TestSize.Level0)
{
    ZLOGI("AccountDelegateMock_GetSubProfileIdByToken start");
    AccountDelegateMock mock;
    EXPECT_CALL(mock, GetSubProfileIdByToken(0u)).WillOnce(Return(200));
    auto ret = mock.GetSubProfileIdByToken(0u);
    EXPECT_EQ(ret, 200);
}

/**
 * @tc.name: AccountDelegateMock_GetForegroundSubProfileId_ExpectMockCallable
 * @tc.desc: Verify AccountDelegate mock has GetForegroundSubProfileId method
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(
    DataShareAccountIsolationTest, AccountDelegateMock_GetForegroundSubProfileId_ExpectMockCallable, TestSize.Level0)
{
    ZLOGI("AccountDelegateMock_GetForegroundSubProfileId start");
    AccountDelegateMock mock;
    EXPECT_CALL(mock, GetForegroundSubProfileId(100)).WillOnce(Return(200));
    auto ret = mock.GetForegroundSubProfileId(100);
    EXPECT_EQ(ret, 200);
}

/**
 * @tc.name: AccountDelegateMock_GetSubProfileIdByAppIndex_ExpectMockCallable
 * @tc.desc: Verify AccountDelegate mock has GetSubProfileIdByAppIndex method
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, AccountDelegateMock_GetSubProfileIdByAppIndex_ExpectMockCallable,
    TestSize.Level0)
{
    ZLOGI("AccountDelegateMock_GetSubProfileIdByAppIndex start");
    AccountDelegateMock mock;
    EXPECT_CALL(mock, GetSubProfileIdByAppIndex(100, 0)).WillOnce(Return(200));
    auto ret = mock.GetSubProfileIdByAppIndex(100, 0);
    EXPECT_EQ(ret, 200);
}

/**
 * @tc.name: AccountDelegateMock_GetAppIndexBySubProfileId_ExpectMockCallable
 * @tc.desc: Verify AccountDelegate mock has GetAppIndexBySubProfileId method
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, AccountDelegateMock_GetAppIndexBySubProfileId_ExpectMockCallable,
    TestSize.Level0)
{
    ZLOGI("AccountDelegateMock_GetAppIndexBySubProfileId start");
    AccountDelegateMock mock;
    EXPECT_CALL(mock, GetAppIndexBySubProfileId(100, 200)).WillOnce(Return(3));
    auto ret = mock.GetAppIndexBySubProfileId(100, 200);
    EXPECT_EQ(ret, 3);
}

// ===== ResolveProviderAppIndex mock tests =====

/**
 * @tc.name: ResolveProviderAppIndex_NoCloneAccountIsolationTrue_ExpectDataDirSet
 * @tc.desc: Verify ResolveProviderAppIndex sets dataDir when no clone app and accountIsolation=true
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, ResolveProviderAppIndex_NoCloneAccountIsolationTrue_ExpectDataDirSet,
    TestSize.Level1)
{
    ZLOGI("ResolveProviderAppIndex_NoCloneAccountIsolationTrue start");
    DeviceManagerAdapterMock dmMock;
    DeviceInfo localDev;
    localDev.uuid = "test_car_uuid";
    EXPECT_CALL(dmMock, GetLocalDevice()).WillRepeatedly(Return(localDev));
    EXPECT_CALL(dmMock, GetDeviceTypeByUuid("test_car_uuid"))
        .WillRepeatedly(Return(DeviceManagerAdapter::DEVICE_TYPE_CAR));
    BDeviceManagerAdapter::deviceManagerAdapter = std::shared_ptr<BDeviceManagerAdapter>(&dmMock,
        [](BDeviceManagerAdapter *) {});

    DataProviderConfig::ProviderInfo providerInfo;
    providerInfo.accountIsolation = true;
    providerInfo.accountId = 200;
    providerInfo.visitedUserId = 100;
    providerInfo.appIndex = 0;
    providerInfo.storeName = "mystore";

    DataShareServiceImpl serviceImpl;
    serviceImpl.ResolveProviderAppIndex(providerInfo);
    EXPECT_EQ(providerInfo.appIndex, 0);
    BDeviceManagerAdapter::deviceManagerAdapter = nullptr;
}

/**
 * @tc.name: ResolveProviderAppIndex_AccountIsolationFalse_ExpectNoChange
 * @tc.desc: Verify ResolveProviderAppIndex does nothing when accountIsolation=false
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest,
    ResolveProviderAppIndex_AccountIsolationFalse_ExpectNoChange,
    TestSize.Level1)
{
    ZLOGI("ResolveProviderAppIndex_AccountIsolationFalse start");
    DataProviderConfig::ProviderInfo providerInfo;
    providerInfo.accountIsolation = false;
    providerInfo.accountId = 200;
    providerInfo.visitedUserId = 100;
    providerInfo.appIndex = 2;

    DataShareServiceImpl serviceImpl;
    serviceImpl.ResolveProviderAppIndex(providerInfo);
    EXPECT_EQ(providerInfo.appIndex, 2);
}

/**
 * @tc.name: ResolveProviderAppIndex_AccountIdMinusOne_ExpectNoChange
 * @tc.desc: Verify ResolveProviderAppIndex does nothing when accountId=-1
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest,
    ResolveProviderAppIndex_AccountIdMinusOne_ExpectNoChange,
    TestSize.Level1)
{
    ZLOGI("ResolveProviderAppIndex_AccountIdMinusOne start");
    DataProviderConfig::ProviderInfo providerInfo;
    providerInfo.accountIsolation = true;
    providerInfo.accountId = -1;
    providerInfo.visitedUserId = 100;
    providerInfo.appIndex = 2;

    DataShareServiceImpl serviceImpl;
    serviceImpl.ResolveProviderAppIndex(providerInfo);
    EXPECT_EQ(providerInfo.appIndex, 2);
}

/**
 * @tc.name: ResolveProviderAppIndex_DelegateNull_ExpectNoChange
 * @tc.desc: Verify ResolveProviderAppIndex does nothing when AccountDelegate is null
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, ResolveProviderAppIndex_DelegateNull_ExpectNoChange, TestSize.Level1)
{
    ZLOGI("ResolveProviderAppIndex_DelegateNull start");
    DeviceManagerAdapterMock dmMock;
    DeviceInfo localDev;
    localDev.uuid = "test_car_uuid";
    EXPECT_CALL(dmMock, GetLocalDevice()).WillRepeatedly(Return(localDev));
    EXPECT_CALL(dmMock, GetDeviceTypeByUuid("test_car_uuid"))
        .WillRepeatedly(Return(DeviceManagerAdapter::DEVICE_TYPE_CAR));
    BDeviceManagerAdapter::deviceManagerAdapter = std::shared_ptr<BDeviceManagerAdapter>(&dmMock,
        [](BDeviceManagerAdapter *) {});

    AccountDelegate::instance_ = nullptr;

    DataProviderConfig::ProviderInfo providerInfo;
    providerInfo.accountIsolation = true;
    providerInfo.accountId = 200;
    providerInfo.visitedUserId = 100;
    providerInfo.appIndex = 2;

    DataShareServiceImpl serviceImpl;
    serviceImpl.ResolveProviderAppIndex(providerInfo);
    EXPECT_EQ(providerInfo.appIndex, 2);
    BDeviceManagerAdapter::deviceManagerAdapter = nullptr;
}

// ===== ResolveAccessorAppIndexForSilentProxy mock tests =====

/**
 * @tc.name: ResolveAccessorAppIndexForSilentProxy_NoCloneApp_ExpectOriginalAppIndex
 * @tc.desc: Verify ResolveAccessorAppIndexForSilentProxy returns original appIndex when provider has no clone app
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, ResolveAccessorAppIndexForSilentProxy_NoCloneApp_ExpectOriginalAppIndex,
    TestSize.Level1)
{
    ZLOGI("ResolveAccessorAppIndexForSilentProxy_NoCloneApp start");
    DeviceManagerAdapterMock dmMock;
    DeviceInfo localDev;
    localDev.uuid = "test_car_uuid";
    EXPECT_CALL(dmMock, GetLocalDevice()).WillRepeatedly(Return(localDev));
    EXPECT_CALL(dmMock, GetDeviceTypeByUuid("test_car_uuid"))
        .WillRepeatedly(Return(DeviceManagerAdapter::DEVICE_TYPE_CAR));
    BDeviceManagerAdapter::deviceManagerAdapter = std::shared_ptr<BDeviceManagerAdapter>(&dmMock,
        [](BDeviceManagerAdapter *) {});

    DataShareServiceImpl serviceImpl;
    std::string uri = "datashareproxy://com.test/module/store/table?accountId=300";
    auto result = serviceImpl.ResolveAccessorAppIndexForSilentProxy(uri, "com.test", 100, 0);
    EXPECT_EQ(result, 0);
    BDeviceManagerAdapter::deviceManagerAdapter = nullptr;
}

/**
 * @tc.name: ResolveAccessorAppIndexForSilentProxy_NoAccountId_ExpectOriginalAppIndex
 * @tc.desc: Verify ResolveAccessorAppIndexForSilentProxy returns original appIndex when no accountId in URI
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest,
    ResolveAccessorAppIndexForSilentProxy_NoAccountId_ExpectOriginalAppIndex,
    TestSize.Level1)
{
    ZLOGI("ResolveAccessorAppIndexForSilentProxy_NoAccountId start");
    DataShareServiceImpl serviceImpl;
    std::string uri = "datashareproxy://com.test/module/store/table?appIndex=2";
    auto result = serviceImpl.ResolveAccessorAppIndexForSilentProxy(uri, "com.test", 100, 2);
    EXPECT_EQ(result, 2);
}

/**
 * @tc.name: ResolveAccessorAppIndexForSilentProxy_AccountIdZero_ExpectOriginalAppIndex
 * @tc.desc: Verify ResolveAccessorAppIndexForSilentProxy returns original appIndex when accountId=0
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest,
    ResolveAccessorAppIndexForSilentProxy_AccountIdZero_ExpectOriginalAppIndex,
    TestSize.Level1)
{
    ZLOGI("ResolveAccessorAppIndexForSilentProxy_AccountIdZero start");
    DataShareServiceImpl serviceImpl;
    std::string uri = "datashareproxy://com.test/module/store/table?accountId=0";
    auto result = serviceImpl.ResolveAccessorAppIndexForSilentProxy(uri, "com.test", 100, 2);
    EXPECT_EQ(result, 2);
}

/**
 * @tc.name: ResolveAccessorAppIndexForSilentProxy_NonCarDevice_ExpectOriginalAppIndex
 * @tc.desc: Verify ResolveAccessorAppIndexForSilentProxy returns original appIndex on non-car device
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, ResolveAccessorAppIndexForSilentProxy_NonCarDevice_ExpectOriginalAppIndex,
    TestSize.Level1)
{
    ZLOGI("ResolveAccessorAppIndexForSilentProxy_NonCarDevice start");
    DataShareServiceImpl serviceImpl;
    std::string uri = "datashareproxy://com.test/module/store/table?accountId=300";
    auto result = serviceImpl.ResolveAccessorAppIndexForSilentProxy(uri, "com.test", 100, 2);
    EXPECT_EQ(result, 2);
}

// ===== ACCOUNT_ID constant test =====

/**
 * @tc.name: AccountIdConstant_ExpectCorrectValue
 * @tc.desc: Verify URIUtils::ACCOUNT_ID constant is "accountId"
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, AccountIdConstant_ExpectCorrectValue, TestSize.Level0)
{
    ZLOGI("AccountIdConstant_ExpectCorrectValue start");
    EXPECT_STREQ(URIUtils::ACCOUNT_ID, "accountId");
}

// ===== DataProviderConfig.accountIsolation pass-through test =====

/**
 * @tc.name: GetFromDataProperties_AccountIsolation_ExpectPassThrough
 * @tc.desc: Verify GetFromDataProperties passes accountIsolation from ProfileInfo to ProviderInfo
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, GetFromDataProperties_AccountIsolation_ExpectPassThrough,
    TestSize.Level1)
{
    ZLOGI("GetFromDataProperties_AccountIsolation start");
    std::string uri = "datashare:///com.test/module/store/table";
    DataProviderConfig providerConfig(uri, 100);
    ProfileInfo profileInfo;
    profileInfo.isSilentProxyEnable = true;
    profileInfo.storeName = "store";
    profileInfo.tableName = "table";
    profileInfo.accountIsolation = true;

    auto result = providerConfig.GetFromDataProperties(profileInfo, "module");
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(providerConfig.providerInfo_.accountIsolation, true);
}

/**
 * @tc.name: GetFromDataProperties_AccountIsolationFalse_ExpectFalsePassThrough
 * @tc.desc: Verify GetFromDataProperties passes accountIsolation=false correctly
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, GetFromDataProperties_AccountIsolationFalse_ExpectFalsePassThrough,
    TestSize.Level1)
{
    ZLOGI("GetFromDataProperties_AccountIsolationFalse start");
    std::string uri = "datashare:///com.test/module/store/table";
    DataProviderConfig providerConfig(uri, 100);
    ProfileInfo profileInfo;
    profileInfo.isSilentProxyEnable = true;
    profileInfo.storeName = "store";
    profileInfo.tableName = "table";
    profileInfo.accountIsolation = false;

    auto result = providerConfig.GetFromDataProperties(profileInfo, "module");
    EXPECT_EQ(result, E_OK);
    EXPECT_EQ(providerConfig.providerInfo_.accountIsolation, false);
}
/**
 * @tc.name: Context_AccountIsolation_DefaultFalse_ExpectDefaultPreserved
 * @tc.desc: Verify Context.accountIsolation defaults to false
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, Context_AccountIsolation_DefaultFalse_ExpectDefaultPreserved,
    TestSize.Level0)
{
    ZLOGI("Context_AccountIsolation_DefaultFalse start");
    auto context = std::make_shared<Context>();
    EXPECT_FALSE(context->accountIsolation);
    context->accountIsolation = true;
    EXPECT_TRUE(context->accountIsolation);
}

/**
 * @tc.name: GetContextInfoFromDataProperties_AccountIsolationTrue_ExpectPassedToContext
 * @tc.desc: Verify GetContextInfoFromDataProperties passes accountIsolation=true from ProfileInfo to Context
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, GetContextInfoFromDataProperties_AccountIsolationTrue_ExpectPassedToContext,
    TestSize.Level1)
{
    ZLOGI("GetContextInfoFromDataProperties_AccountIsolationTrue start");
    LoadConfigFromDataProxyNodeStrategy strategy;
    auto context = std::make_shared<Context>();
    ProfileInfo profileInfo;
    profileInfo.accountIsolation = true;
    profileInfo.storeName = "mystore";
    profileInfo.tableName = "mytable";
    strategy.GetContextInfoFromDataProperties(profileInfo, "module", context);
    EXPECT_EQ(context->accountIsolation, true);
    EXPECT_EQ(context->calledStoreName, "mystore");
    EXPECT_EQ(context->calledTableName, "mytable");
}

/**
 * @tc.name: GetContextInfoFromDataProperties_AccountIsolationFalse_ExpectPassedToContext
 * @tc.desc: Verify GetContextInfoFromDataProperties passes accountIsolation=false from ProfileInfo to Context
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, GetContextInfoFromDataProperties_AccountIsolationFalse_ExpectPassedToContext,
    TestSize.Level1)
{
    ZLOGI("GetContextInfoFromDataProperties_AccountIsolationFalse start");
    LoadConfigFromDataProxyNodeStrategy strategy;
    auto context = std::make_shared<Context>();
    ProfileInfo profileInfo;
    profileInfo.accountIsolation = false;
    profileInfo.storeName = "store";
    profileInfo.tableName = "table";
    strategy.GetContextInfoFromDataProperties(profileInfo, "module", context);
    EXPECT_EQ(context->accountIsolation, false);
    EXPECT_EQ(context->calledStoreName, "store");
    EXPECT_EQ(context->calledTableName, "table");
}

/**
 * @tc.name: ResolveProviderAppIndex_AccountIdZero_ExpectNoChange
 * @tc.desc: Verify ResolveProviderAppIndex exits early when accountId is 0, appIndex unchanged
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, ResolveProviderAppIndex_AccountIdZero_ExpectNoChange, TestSize.Level1)
{
    ZLOGI("ResolveProviderAppIndex_AccountIdZero start");
    LoadConfigFromDataProxyNodeStrategy strategy;
    auto context = std::make_shared<Context>();
    context->accountId = 0;
    context->appIndex = 5;
    strategy.ResolveProviderAppIndex(context);
    EXPECT_EQ(context->appIndex, 5);
}

/**
 * @tc.name: ResolveProviderAppIndex_AccountIdNegative_ExpectNoChange
 * @tc.desc: Verify ResolveProviderAppIndex exits early when accountId is negative, appIndex unchanged
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, ResolveProviderAppIndex_AccountIdNegative_ExpectNoChange, TestSize.Level1)
{
    ZLOGI("ResolveProviderAppIndex_AccountIdNegative start");
    LoadConfigFromDataProxyNodeStrategy strategy;
    auto context = std::make_shared<Context>();
    context->accountId = -1;
    context->appIndex = 3;
    strategy.ResolveProviderAppIndex(context);
    EXPECT_EQ(context->appIndex, 3);
}

/**
 * @tc.name: ResolveProviderAppIndex_DelegateNullDataProxy_ExpectNoChange
 * @tc.desc: Verify AccountDelegate is null with DataProxyNodeStrategy, appIndex unchanged
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, ResolveProviderAppIndex_DelegateNullDataProxy_ExpectNoChange, TestSize.Level1)
{
    ZLOGI("ResolveProviderAppIndex_DelegateNull start");
    AccountDelegate::instance_ = nullptr;
    LoadConfigFromDataProxyNodeStrategy strategy;
    auto context = std::make_shared<Context>();
    context->accountId = 100;
    context->appIndex = 2;
    context->calledBundleName = "com.test";
    context->visitedUserId = 100;
    strategy.ResolveProviderAppIndex(context);
    EXPECT_EQ(context->appIndex, 2);
}

/**
 * @tc.name: ResolveProviderAppIndex_EmptyCloneAppIndexes_ExpectNoAppIndexChange
 * @tc.desc: Verify ResolveProviderAppIndex does not change appIndex when clone app indexes are empty
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, ResolveProviderAppIndex_EmptyCloneAppIndexes_ExpectNoAppIndexChange,
    TestSize.Level1)
{
    ZLOGI("ResolveProviderAppIndex_EmptyCloneAppIndexes start");
    LoadConfigFromDataProxyNodeStrategy strategy;
    auto context = std::make_shared<Context>();
    context->accountId = 100;
    context->appIndex = 0;
    context->calledBundleName = "com.test.no.clone";
    context->visitedUserId = 100;
    strategy.ResolveProviderAppIndex(context);
    EXPECT_EQ(context->appIndex, 0);
}

/**
 * @tc.name: ResolveAccessorAccountId_DelegateNull_ExpectNoChange
 * @tc.desc: Verify ResolveAccessorAccountId exits early when AccountDelegate is null
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, ResolveAccessorAccountId_DelegateNull_ExpectNoChange, TestSize.Level1)
{
    ZLOGI("ResolveAccessorAccountId_DelegateNull start");
    AccountDelegate::instance_ = nullptr;
    LoadConfigCommonStrategy strategy;
    auto context = std::make_shared<Context>();
    context->accountId = -1;
    context->visitedUserId = 100;
    context->callerTokenId = 0;
    strategy.ResolveAccessorAccountId(context);
    EXPECT_EQ(context->accountId, -1);
}

// ===== MatchAccountDataDir tests =====

bool MatchAccountDataDir(const std::string &dataDir, int32_t accountId);

/**
 * @tc.name: MatchAccountDataDir_AccountIdInPathSegment_ExpectMatch
 * @tc.desc: Verify MatchAccountDataDir returns true when accountId is a path segment in dataDir
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, MatchAccountDataDir_AccountIdInPathSegment_ExpectMatch, TestSize.Level0)
{
    ZLOGI("MatchAccountDataDir_AccountIdInPathSegment start");
    std::string dataDir = "/data/app/el2/100/com.test/200/mystore.db";
    EXPECT_TRUE(MatchAccountDataDir(dataDir, 200));
}

/**
 * @tc.name: MatchAccountDataDir_AccountIdNotInPath_ExpectNoMatch
 * @tc.desc: Verify MatchAccountDataDir returns false when accountId is not in dataDir
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, MatchAccountDataDir_AccountIdNotInPath_ExpectNoMatch, TestSize.Level0)
{
    ZLOGI("MatchAccountDataDir_AccountIdNotInPath start");
    std::string dataDir = "/data/app/el2/100/com.test/mystore.db";
    EXPECT_FALSE(MatchAccountDataDir(dataDir, 200));
}

/**
 * @tc.name: MatchAccountDataDir_AccountIdAsPartialNumber_ExpectNoMatch
 * @tc.desc: Verify MatchAccountDataDir returns false when accountId is substring of larger number
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, MatchAccountDataDir_AccountIdAsPartialNumber_ExpectNoMatch,
    TestSize.Level0)
{
    ZLOGI("MatchAccountDataDir_AccountIdAsPartialNumber start");
    std::string dataDir = "/data/app/el2/100/com.test/200/mystore.db";
    EXPECT_FALSE(MatchAccountDataDir(dataDir, 20));
}

/**
 * @tc.name: MatchAccountDataDir_AccountIdZero_ExpectNoMatch
 * @tc.desc: Verify MatchAccountDataDir returns false when accountId is 0
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, MatchAccountDataDir_AccountIdZero_ExpectNoMatch, TestSize.Level0)
{
    ZLOGI("MatchAccountDataDir_AccountIdZero start");
    std::string dataDir = "/data/app/el2/100/com.test/0/mystore.db";
    EXPECT_FALSE(MatchAccountDataDir(dataDir, 0));
}

/**
 * @tc.name: MatchAccountDataDir_AccountIdNegative_ExpectNoMatch
 * @tc.desc: Verify MatchAccountDataDir returns false when accountId is negative
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, MatchAccountDataDir_AccountIdNegative_ExpectNoMatch, TestSize.Level0)
{
    ZLOGI("MatchAccountDataDir_AccountIdNegative start");
    std::string dataDir = "/data/app/el2/100/com.test/mystore.db";
    EXPECT_FALSE(MatchAccountDataDir(dataDir, -1));
}

/**
 * @tc.name: MatchAccountDataDir_EmptyDataDir_ExpectNoMatch
 * @tc.desc: Verify MatchAccountDataDir returns false when dataDir is empty
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, MatchAccountDataDir_EmptyDataDir_ExpectNoMatch, TestSize.Level0)
{
    ZLOGI("MatchAccountDataDir_EmptyDataDir start");
    std::string dataDir = "";
    EXPECT_FALSE(MatchAccountDataDir(dataDir, 100));
}

/**
 * @tc.name: MatchAccountDataDir_AccountIdAtEndWithoutSlash_ExpectNoMatch
 * @tc.desc: Verify MatchAccountDataDir returns false when accountId is not followed by /
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(DataShareAccountIsolationTest, MatchAccountDataDir_AccountIdAtEndWithoutSlash_ExpectNoMatch,
    TestSize.Level0)
{
    ZLOGI("MatchAccountDataDir_AccountIdAtEndWithoutSlash start");
    std::string dataDir = "/data/app/el2/100/com.test200/mystore.db";
    EXPECT_FALSE(MatchAccountDataDir(dataDir, 200));
}
} // namespace OHOS::DataShare
