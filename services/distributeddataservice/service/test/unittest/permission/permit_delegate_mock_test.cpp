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
#include "meta_data_manager_mock.h"
#include "permit_delegate.h"
#include "metadata/store_meta_data.h"
#include "metadata/appid_meta_data.h"

namespace OHOS::DistributedData {
using namespace OHOS::Security::AccessToken;
using namespace std;
using namespace testing;
using ActiveParam = DistributedDB::ActivationCheckParam;
using CheckParam = DistributedDB::PermissionCheckParam;
class PermitDelegateMockTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
public:
    static inline shared_ptr<AccessTokenKitMock> accessTokenKitMock = nullptr;
    static inline shared_ptr<MetaDataManagerMock> metaDataMgrMock = nullptr;
};

void PermitDelegateMockTest::SetUpTestCase(void)
{
    accessTokenKitMock = make_shared<AccessTokenKitMock>();
    BAccessTokenKit::accessTokenkit = accessTokenKitMock;
    metaDataMgrMock = make_shared<MetaDataManagerMock>();
    BMetaDataManager::metaDataManager = metaDataMgrMock;
}

void PermitDelegateMockTest::TearDownTestCase(void)
{
    BAccessTokenKit::accessTokenkit = nullptr;
    accessTokenKitMock = nullptr;
    BMetaDataManager::metaDataManager = nullptr;
    metaDataMgrMock = nullptr;
}

void PermitDelegateMockTest::SetUp(void)
{}

void PermitDelegateMockTest::TearDown(void)
{}

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
}