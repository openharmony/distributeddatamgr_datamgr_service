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

#include <gtest/gtest.h>
#include "accesstoken_kit.h"
#include "checker/bundle_checker.h"
#include "bootstrap.h"
#include "hap_token_info.h"
#include "nativetoken_kit.h"
#include "utils/crypto.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;
class BundleCheckerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp();
    void TearDown();
    NativeTokenInfoParams infoInstance{0};
};

void BundleCheckerTest::SetUp(void)
{
    infoInstance.dcapsNum = 0;
    infoInstance.permsNum = 0;
    infoInstance.aclsNum = 0;
    infoInstance.dcaps = nullptr;
    infoInstance.perms = nullptr;
    infoInstance.acls = nullptr;
    infoInstance.processName = "BundleCheckerTest";
    infoInstance.aplStr = "system_core";

    HapInfoParams info = {
        .userID = 100,
        .bundleName = "com.ohos.dlpmanager",
        .instIndex = 0,
        .appIDDesc = "com.ohos.dlpmanager"
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

void BundleCheckerTest::TearDown()
{
    auto tokenId = AccessTokenKit::GetHapTokenID(100, "com.ohos.dlpmanager", 0);
    AccessTokenKit::DeleteToken(tokenId);
}

/**
* @tc.name: GetAppIdTest001
* @tc.desc: get appId from cache.
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(BundleCheckerTest, GetAppIdTest001, TestSize.Level0)
{
    CheckerManager::StoreInfo info;
    auto tokenId = AccessTokenKit::GetHapTokenID(100, "com.ohos.dlpmanager", 0);
    info.uid = 100 * 200000;
    info.tokenId = tokenId;
    info.bundleName = "com.ohos.dlpmanager";
    BundleChecker checker;
    std::string appIdInCache;
    ASSERT_TRUE(!checker.GetAppId(info).empty());
    checker.appIds_.Get("com.ohos.dlpmanager###100", appIdInCache);
    ASSERT_TRUE(!appIdInCache.empty());
    ASSERT_EQ(Crypto::Sha256(appIdInCache), checker.GetAppId(info));

    checker.DeleteCache(info.bundleName, 100, 0);
    std::string appIdInCache1;
    checker.appIds_.Get("com.ohos.dlpmanager###100", appIdInCache1);
    ASSERT_TRUE(appIdInCache1.empty());

    checker.GetAppId(info);
    ASSERT_GE(checker.appIds_.Size(), 1);
    checker.ClearCache();
    ASSERT_EQ(checker.appIds_.Size(), 0);
}