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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "access_token_mock.h"
#include "account_delegate_mock.h"
#include "device_manager_adapter_mock.h"
#include "meta_data_manager_mock.h"
#include "permit_delegate.h"
#include "metadata/store_meta_data.h"
#include "metadata/appid_meta_data.h"
#include "utils/constant.h"

namespace OHOS::DistributedData {
using namespace OHOS::Security::AccessToken;
using namespace std;
using namespace testing;
using ActiveParam = DistributedDB::ActivationCheckParam;
using CheckParam = DistributedDB::PermissionCheckParam;

static constexpr int32_t AUTH_FORM_SHARE = 3;
static constexpr int32_t AUTH_FORM_OTHER = 0;
static constexpr int32_t TEST_FOREGROUND_USER_ID = 100;
static constexpr int64_t TEST_TOKEN_ID = 12345;
static constexpr const char *META_STORE_ID = "service_meta";
static constexpr const char *TEST_ACCOUNT_ID = "test_account_id";
static constexpr const char *TEST_BUNDLE_NAME = "com.test.srcaccess.app";
static constexpr const char *TEST_LOCAL_NETWORK_ID = "test_local_network_id";
static constexpr const char *TEST_REMOTE_NETWORK_ID = "test_remote_network_id";
static constexpr const char *TEST_DEVICE_ID = "test_device_id";
static constexpr const char *TEST_APP_ID = "test_app_id";
static constexpr const char *TEST_LOCAL_UUID = "test_local_uuid";

class PermitDelegateMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static CheckParam BuildCheckParam(const std::string &storeId)
    {
        return CheckParam{
            .userId = "100", .appId = TEST_APP_ID, .storeId = storeId, .deviceId = TEST_DEVICE_ID, .instanceId = 0
        };
    }
    static DeviceInfo BuildLocalDevice(const std::string &networkId)
    {
        DeviceInfo localDevice{};
        localDevice.uuid = TEST_LOCAL_UUID;
        localDevice.networkId = networkId;
        return localDevice;
    }
    static std::string BuildMetaKey(const CheckParam &param, const DeviceInfo &localDevice)
    {
        StoreMetaData data;
        data.user = param.userId == "default" ? "0" : param.userId;
        data.storeId = param.storeId;
        data.deviceId = localDevice.uuid;
        data.instanceId = param.instanceId;
        return data.GetKeyWithoutPath();
    }
    static void ResetMocks(void)
    {
        Mock::VerifyAndClearExpectations(dmAdapterMock.get());
        Mock::VerifyAndClearExpectations(metaDataMgrMock.get());
        Mock::VerifyAndClearExpectations(accountDelegateMock);
        AccountDelegate::instance_ = nullptr;
        AccountDelegate::RegisterAccountInstance(accountDelegateMock);
    }
public:
    static inline shared_ptr<AccessTokenKitMock> accessTokenKitMock = nullptr;
    static inline shared_ptr<MetaDataManagerMock> metaDataMgrMock = nullptr;
    static inline shared_ptr<DeviceManagerAdapterMock> dmAdapterMock = nullptr;
    static inline AccountDelegateMock *accountDelegateMock = nullptr;
};

void PermitDelegateMockTest::SetUpTestCase(void)
{
    accessTokenKitMock = make_shared<AccessTokenKitMock>();
    BAccessTokenKit::accessTokenkit = accessTokenKitMock;
    metaDataMgrMock = make_shared<MetaDataManagerMock>();
    BMetaDataManager::metaDataManager = metaDataMgrMock;
    dmAdapterMock = make_shared<DeviceManagerAdapterMock>();
    BDeviceManagerAdapter::deviceManagerAdapter = dmAdapterMock;
    accountDelegateMock = new (std::nothrow) AccountDelegateMock();
    ASSERT_NE(accountDelegateMock, nullptr);
}

void PermitDelegateMockTest::TearDownTestCase(void)
{
    BAccessTokenKit::accessTokenkit = nullptr;
    accessTokenKitMock = nullptr;
    BDeviceManagerAdapter::deviceManagerAdapter = nullptr;
    dmAdapterMock = nullptr;
    BMetaDataManager::metaDataManager = nullptr;
    metaDataMgrMock = nullptr;
    AccountDelegate::instance_ = nullptr;
    delete accountDelegateMock;
    accountDelegateMock = nullptr;
}

void PermitDelegateMockTest::SetUp(void)
{
    AccountDelegate::instance_ = nullptr;
    AccountDelegate::RegisterAccountInstance(accountDelegateMock);
}

void PermitDelegateMockTest::TearDown(void)
{
    AccountDelegate::instance_ = nullptr;
    Mock::VerifyAndClearExpectations(accountDelegateMock);
    Mock::VerifyAndClearExpectations(dmAdapterMock.get());
    Mock::VerifyAndClearExpectations(metaDataMgrMock.get());
}

/**
  * @tc.name: SyncActivate001
  * @tc.desc: sync Activate.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(PermitDelegateMockTest, SyncActivate001, testing::ext::TestSize.Level0)
{
    ActiveParam activeParam = {
        .userId = "activeparam",
        .appId = "appid",
        .storeId = "storeid",
        .subUserId = "subactiveparam",
        .instanceId = 1
    };
    PermitDelegate::GetInstance().Init();
    bool result = PermitDelegate::GetInstance().SyncActivate(activeParam);
    EXPECT_FALSE(result);
}


/**
  * @tc.name: SyncActivate002
  * @tc.desc: sync Activate.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(PermitDelegateMockTest, SyncActivate002, testing::ext::TestSize.Level0)
{
    ActiveParam activeParam = {
        .userId = "1",
        .appId = "",
        .storeId = "",
        .subUserId = "subuserid",
        .instanceId = 0
    };
    EXPECT_CALL(*accessTokenKitMock, GetTokenTypeFlag(_)).Times(AnyNumber());
    bool result = PermitDelegate::GetInstance().SyncActivate(activeParam);
    EXPECT_FALSE(result);
}

/**
  * @tc.name: VerifyPermission_001
  * @tc.desc: verify permission.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(PermitDelegateMockTest, VerifyPermission_001, testing::ext::TestSize.Level0)
{
    std::string permission = "";
    uint32_t tokenId = 1;
    bool result = PermitDelegate::GetInstance().VerifyPermission(permission, tokenId);
    EXPECT_TRUE(result);
}

/**
  * @tc.name: VerifyPermission_002
  * @tc.desc: verify permission.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(PermitDelegateMockTest, VerifyPermission_002, testing::ext::TestSize.Level0)
{
    std::string permission = "premmit002";
    uint32_t tokenId = 1;
    EXPECT_CALL(*accessTokenKitMock, VerifyAccessToken(_, _))
        .WillOnce(Return(PermissionState::PERMISSION_GRANTED));
    bool result = PermitDelegate::GetInstance().VerifyPermission(permission, tokenId);
    EXPECT_TRUE(result);
}

/**
  * @tc.name: VerifyPermission_003
  * @tc.desc: verify permission.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(PermitDelegateMockTest, VerifyPermission_003, testing::ext::TestSize.Level0)
{
    std::string permission = "premmit003";
    uint32_t tokenId = 0;
    EXPECT_CALL(*accessTokenKitMock, VerifyAccessToken(_, _))
        .WillOnce(Return(PermissionState::PERMISSION_DENIED));
    bool result = PermitDelegate::GetInstance().VerifyPermission(permission, tokenId);
    EXPECT_FALSE(result);
}

/**
  * @tc.name: VerifyPermission001
  * @tc.desc: verify permission.
  * @tc.type: OVERRIDE FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(PermitDelegateMockTest, VerifyPermission001, testing::ext::TestSize.Level0)
{
    bool result = PermitDelegate::GetInstance().appId2BundleNameMap_.Insert("Permission001", "");
    ASSERT_TRUE(result);
    AppIDMetaData appMeta("permitdelegatemocktestId", "com.permitdelegatetest.app");
    EXPECT_CALL(*metaDataMgrMock, LoadMeta(_, _, _)).WillOnce(DoAll(SetArgReferee<1>(appMeta), Return(true)));
    CheckParam checkParam = {
        .userId = "userid",
        .appId = "permitdelegatemocktestId",
        .storeId = "storeid",
        .deviceId = "deviceid",
        .instanceId = 1
    };
    uint8_t flag = 1;
    EXPECT_CALL(*metaDataMgrMock, LoadMeta(_, _, false)).WillOnce(Return(false));
    result = PermitDelegate::GetInstance().VerifyPermission(checkParam, flag);
    EXPECT_FALSE(result);
}

/**
  * @tc.name: VerifyPermission002
  * @tc.desc: verify permission.
  * @tc.type: OVERRIDE FUNC
  * @tc.require:
  * @tc.author: caozhijun
  */
HWTEST_F(PermitDelegateMockTest, VerifyPermission002, testing::ext::TestSize.Level0)
{
    std::string key = "Permission002";
    std::string value = "com.permitDelegateUnitTest.app";
    PermitDelegate::GetInstance().appId2BundleNameMap_.Insert(key, value);
    auto ret = PermitDelegate::GetInstance().appId2BundleNameMap_.Find(key);
    ASSERT_TRUE(ret.second == value);
    CheckParam checkParam = {
        .userId = "userid2",
        .appId = "permitdelegatemocktestId2",
        .storeId = "storeid2",
        .deviceId = "deviceid2",
        .instanceId = 0
    };
    uint8_t flag = 1;
    EXPECT_CALL(*metaDataMgrMock, LoadMeta(_, _, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(*accessTokenKitMock, VerifyAccessToken(_, _))
        .WillOnce(Return(PermissionState::PERMISSION_GRANTED));
    bool result = PermitDelegate::GetInstance().VerifyPermission(checkParam, flag);
    EXPECT_TRUE(result);
}

/**
  * @tc.name: IsSrcTransferAllowed_SkipBranches_ReturnTrue
  * @tc.desc: meta store, kv store and non-share auth form skip src access control.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: agent
  */
HWTEST_F(PermitDelegateMockTest, IsSrcTransferAllowed_SkipBranches_ReturnTrue, testing::ext::TestSize.Level0)
{
    AppIDMetaData appIDMeta(TEST_APP_ID, TEST_BUNDLE_NAME);

    // meta store: device manager is not accessed at all
    auto metaParam = BuildCheckParam(META_STORE_ID);
    EXPECT_CALL(*dmAdapterMock, GetAuthType(_)).Times(0);
    EXPECT_CALL(*dmAdapterMock, GetLocalDevice()).Times(0);
    EXPECT_TRUE(PermitDelegate::GetInstance().IsSrcTransferAllowed(metaParam, appIDMeta, 0));
    ResetMocks();

    // kv store: the cached meta has a kv store type
    auto kvParam = BuildCheckParam("store_kv");
    auto localDevice = BuildLocalDevice(TEST_LOCAL_NETWORK_ID);
    StoreMetaData cachedData;
    cachedData.user = kvParam.userId;
    cachedData.storeId = kvParam.storeId;
    cachedData.deviceId = localDevice.uuid;
    cachedData.instanceId = kvParam.instanceId;
    cachedData.storeType = StoreMetaData::STORE_KV_BEGIN;
    auto kvKey = BuildMetaKey(kvParam, localDevice);
    PermitDelegate::GetInstance().metaDataBucket_.Set(kvKey, cachedData);
    EXPECT_CALL(*dmAdapterMock, GetAuthType(_)).WillOnce(Return(AUTH_FORM_SHARE));
    EXPECT_CALL(*dmAdapterMock, GetLocalDevice()).WillOnce(Return(localDevice));
    EXPECT_CALL(*accountDelegateMock, QueryForegroundUserId(_)).Times(0);
    EXPECT_TRUE(PermitDelegate::GetInstance().IsSrcTransferAllowed(kvParam, appIDMeta, 0));
    PermitDelegate::GetInstance().DelCache(kvKey);
    ResetMocks();

    // remote auth form is not share
    auto otherParam = BuildCheckParam("store_other");
    EXPECT_CALL(*dmAdapterMock, GetAuthType(_)).WillOnce(Return(AUTH_FORM_OTHER));
    EXPECT_CALL(*dmAdapterMock, GetLocalDevice()).WillOnce(Return(BuildLocalDevice("")));
    EXPECT_CALL(*metaDataMgrMock, LoadMeta(_, _, false)).WillOnce(Return(false));
    EXPECT_TRUE(PermitDelegate::GetInstance().IsSrcTransferAllowed(otherParam, appIDMeta, 0));
    ResetMocks();
}

/**
  * @tc.name: IsSrcTransferAllowed_DenyBranches_ReturnFalse
  * @tc.desc: missing account, foreground user, local or remote network id denies transfer.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: agent
  */
HWTEST_F(PermitDelegateMockTest, IsSrcTransferAllowed_DenyBranches_ReturnFalse, testing::ext::TestSize.Level0)
{
    AppIDMetaData appIDMeta(TEST_APP_ID, TEST_BUNDLE_NAME);
    auto localDevice = BuildLocalDevice(TEST_LOCAL_NETWORK_ID);
    auto setup = [](const DeviceInfo &device) {
        EXPECT_CALL(*dmAdapterMock, GetAuthType(_)).WillRepeatedly(Return(AUTH_FORM_SHARE));
        EXPECT_CALL(*metaDataMgrMock, LoadMeta(_, _, false)).WillRepeatedly(Return(false));
        EXPECT_CALL(*dmAdapterMock, GetLocalDevice()).WillRepeatedly(Return(device));
        EXPECT_CALL(*accountDelegateMock, QueryForegroundUserId(_))
            .WillRepeatedly(DoAll(SetArgReferee<0>(TEST_FOREGROUND_USER_ID), Return(true)));
    };
    AccountDelegate::instance_ = nullptr;
    setup(localDevice);
    EXPECT_FALSE(PermitDelegate::GetInstance().IsSrcTransferAllowed(
        BuildCheckParam("no_account"), appIDMeta, 0));
    ResetMocks();
    setup(localDevice);
    EXPECT_CALL(*accountDelegateMock, QueryForegroundUserId(_)).WillRepeatedly(Return(false));
    EXPECT_FALSE(PermitDelegate::GetInstance().IsSrcTransferAllowed(
        BuildCheckParam("fg_fail"), appIDMeta, 0));
    ResetMocks();
    setup(BuildLocalDevice(""));
    EXPECT_FALSE(PermitDelegate::GetInstance().IsSrcTransferAllowed(
        BuildCheckParam("local_empty"), appIDMeta, 0));
    ResetMocks();
    setup(localDevice);
    EXPECT_CALL(*dmAdapterMock, ToNetworkID(_)).WillRepeatedly(Return(std::string("")));
    EXPECT_FALSE(PermitDelegate::GetInstance().IsSrcTransferAllowed(
        BuildCheckParam("remote_empty"), appIDMeta, 0));
    ResetMocks();
    setup(localDevice);
    EXPECT_CALL(*dmAdapterMock, ToNetworkID(_)).WillRepeatedly(
        Return(std::string(TEST_REMOTE_NETWORK_ID)));
    EXPECT_CALL(*accountDelegateMock, GetCurrentAccountId()).WillRepeatedly(
        Return(std::string(TEST_ACCOUNT_ID)));
    EXPECT_CALL(*dmAdapterMock, CheckSrcAccessControl(
        AllOf(Field(&AccessCaller::accountId, TEST_ACCOUNT_ID),
            Field(&AccessCaller::bundleName, TEST_BUNDLE_NAME),
            Field(&AccessCaller::networkId, TEST_LOCAL_NETWORK_ID),
            Field(&AccessCaller::userId, TEST_FOREGROUND_USER_ID),
            Field(&AccessCaller::tokenId, TEST_TOKEN_ID)),
        Field(&AccessCallee::networkId, TEST_REMOTE_NETWORK_ID)))
        .WillRepeatedly(Return(false));
    EXPECT_FALSE(PermitDelegate::GetInstance().IsSrcTransferAllowed(
        BuildCheckParam("denied"), appIDMeta, TEST_TOKEN_ID));
    ResetMocks();
}

/**
  * @tc.name: IsTransferAllowed_TokenIdHandling_ReturnDeniedSend
  * @tc.desc: a denied src check with a valid token id, and an invalid token id type, both reject transfer.
  * @tc.type: FUNC
  * @tc.require:
  * @tc.author: agent
  */
HWTEST_F(PermitDelegateMockTest, IsTransferAllowed_TokenIdHandling_ReturnDeniedSend, testing::ext::TestSize.Level0)
{
    auto param = BuildCheckParam("store_transfer");
    auto localDevice = BuildLocalDevice(TEST_LOCAL_NETWORK_ID);
    DBProperty property;

    // the uint32 token id is taken from the property and forwarded to the src access control
    EXPECT_CALL(*metaDataMgrMock, LoadMeta(_, _, true)).WillOnce(Return(true));
    EXPECT_CALL(*metaDataMgrMock, LoadMeta(_, _, false)).WillOnce(Return(false));
    EXPECT_CALL(*dmAdapterMock, GetAuthType(_)).WillOnce(Return(AUTH_FORM_SHARE));
    EXPECT_CALL(*dmAdapterMock, GetLocalDevice()).Times(2).WillRepeatedly(Return(localDevice));
    EXPECT_CALL(*accountDelegateMock, QueryForegroundUserId(_))
        .WillOnce(DoAll(SetArgReferee<0>(TEST_FOREGROUND_USER_ID), Return(true)));
    EXPECT_CALL(*dmAdapterMock, ToNetworkID(_)).WillOnce(
        Return(std::string(TEST_REMOTE_NETWORK_ID)));
    EXPECT_CALL(*accountDelegateMock, GetCurrentAccountId()).WillOnce(
        Return(std::string(TEST_ACCOUNT_ID)));
    EXPECT_CALL(*dmAdapterMock, CheckSrcAccessControl(
        Field(&AccessCaller::tokenId, TEST_TOKEN_ID), _)).WillOnce(Return(false));
    property[Constant::TOKEN_ID] = static_cast<uint32_t>(TEST_TOKEN_ID);
    EXPECT_EQ(PermitDelegate::GetInstance().IsTransferAllowed(param, property),
        DataFlowCheckRet::DENIED_SEND);
    ResetMocks();

    // the token id in the property has an unexpected type
    EXPECT_CALL(*metaDataMgrMock, LoadMeta(_, _, true)).WillOnce(Return(true));
    EXPECT_CALL(*dmAdapterMock, CheckSrcAccessControl(_, _)).Times(0);
    property[Constant::TOKEN_ID] = std::string("invalid");
    EXPECT_EQ(PermitDelegate::GetInstance().IsTransferAllowed(param, property),
        DataFlowCheckRet::DENIED_SEND);
}
}