/*
* Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include "checker/checker_manager.h"
#include "accesstoken_kit.h"
#include "bootstrap.h"
#include "hap_token_info.h"
#include "nativetoken_kit.h"
#include "utils/crypto.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
class CheckerManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp();
    void TearDown();
    NativeTokenInfoParams infoInstance{0};
};

void CheckerManagerTest::SetUp(void)
{
    infoInstance.dcapsNum = 0;
    infoInstance.permsNum = 0;
    infoInstance.aclsNum = 0;
    infoInstance.dcaps = nullptr;
    infoInstance.perms = nullptr;
    infoInstance.acls = nullptr;
    infoInstance.processName = "foundation";
    infoInstance.aplStr = "system_core";

    HapInfoParams info = {
        .userID = 100,
        .bundleName = "ohos.test.demo",
        .instIndex = 0,
        .appIDDesc = "ohos.test.demo"
    };
    PermissionDef infoManagerTestPermDef = {
        .permissionName = "ohos.permission.test",
        .bundleName = "ohos.test.demo",
        .grantMode = 1,
        .availableLevel = APL_NORMAL,
        .label = "label",
        .labelId = 1,
        .description = "open the door",
        .descriptionId = 1
    };
    PermissionStateFull infoManagerTestState = {
        .permissionName = "ohos.permission.test",
        .isGeneral = true,
        .resDeviceID = {"local"},
        .grantStatus = {PermissionState::PERMISSION_GRANTED},
        .grantFlags = {1}
    };
    HapPolicyParams policy = {
        .apl = APL_NORMAL,
        .domain = "test.domain",
        .permList = {infoManagerTestPermDef},
        .permStateList = {infoManagerTestState}
    };
    AccessTokenKit::AllocHapToken(info, policy);

    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCheckers();
    Bootstrap::GetInstance().LoadNetworks();
}

void CheckerManagerTest::TearDown()
{
    auto tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.test.demo", 0);
    AccessTokenKit::DeleteToken(tokenId);
}
/**
* @tc.name: checkers
* @tc.desc: checker the bundle name of the system abilities.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(CheckerManagerTest, Checkers, TestSize.Level0)
{
    auto *checker = CheckerManager::GetInstance().GetChecker("SystemChecker");
    ASSERT_NE(checker, nullptr);
    checker = CheckerManager::GetInstance().GetChecker("BundleChecker");
    ASSERT_NE(checker, nullptr);
    checker = CheckerManager::GetInstance().GetChecker("OtherChecker");
    ASSERT_EQ(checker, nullptr);
}

/**
* @tc.name: SystemChecker bms
* @tc.desc: checker the bundle name of the system abilities.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(CheckerManagerTest, SystemCheckerBMS, TestSize.Level0)
{
    CheckerManager::StoreInfo info;
    info.uid = 1000;
    info.tokenId = GetAccessTokenId(&infoInstance);
    info.bundleName = "bundle_manager_service";
    ASSERT_EQ("bundle_manager_service", CheckerManager::GetInstance().GetAppId(info));
    ASSERT_TRUE(CheckerManager::GetInstance().IsValid(info));
}

/**
* @tc.name: SystemChecker form
* @tc.desc: checker the bundle name of the system abilities.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(CheckerManagerTest, SystemCheckerForm, TestSize.Level0)
{
    CheckerManager::StoreInfo info;
    info.uid = 1000;
    info.tokenId = GetAccessTokenId(&infoInstance);
    info.bundleName = "form_storage";
    ASSERT_EQ("form_storage", CheckerManager::GetInstance().GetAppId(info));
    ASSERT_TRUE(CheckerManager::GetInstance().IsValid(info));
}

/**
* @tc.name: SystemChecker ivi
* @tc.desc: checker the bundle name of the system abilities.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(CheckerManagerTest, SystemCheckerIVI, TestSize.Level0)
{
    CheckerManager::StoreInfo info;
    info.uid = 1000;
    info.tokenId = GetAccessTokenId(&infoInstance);
    info.bundleName = "ivi_config_manager";
    ASSERT_EQ("ivi_config_manager", CheckerManager::GetInstance().GetAppId(info));
    ASSERT_TRUE(CheckerManager::GetInstance().IsValid(info));
}

/**
* @tc.name: BundleChecker
* @tc.desc: checker the bundle name of the bundle abilities.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(CheckerManagerTest, BundleChecker, TestSize.Level0)
{
    CheckerManager::StoreInfo storeInfo;
    storeInfo.uid = 2000000;
    storeInfo.tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.test.demo", 0);
    storeInfo.bundleName = "ohos.test.demo";
    HapTokenInfo tokenInfo;
    AccessTokenKit::GetHapTokenInfo(storeInfo.tokenId, tokenInfo);
    ASSERT_EQ(Crypto::Sha256(tokenInfo.appID), CheckerManager::GetInstance().GetAppId(storeInfo));
    ASSERT_TRUE(CheckerManager::GetInstance().IsValid(storeInfo));
}