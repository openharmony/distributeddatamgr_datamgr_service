/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "bootstrap.h"
#include "directory_manager.h"
#include "nativetoken_kit.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedKv;
using namespace OHOS;
using namespace OHOS::Security::AccessToken;

class DirectoryManagerTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        HapInfoParams info = {
            .userID = 100, .bundleName = "ohos.test.demo", .instIndex = 0, .appIDDesc = "ohos.test.demo_09AEF01D"
        };
        HapPolicyParams policy = {
            .apl = APL_NORMAL,
            .domain = "test.domain",
            .permList = {
                {
                    .permissionName = "ohos.permission.test",
                    .bundleName = "ohos.test.demo",
                    .grantMode = 1,
                    .availableLevel = APL_NORMAL,
                    .label = "label",
                    .labelId = 1,
                    .description = "open the door",
                    .descriptionId = 1
                }
            },
            .permStateList = {
                {
                    .permissionName = "ohos.permission.test",
                    .isGeneral = true,
                    .resDeviceID = { "local" },
                    .grantStatus = { PermissionState::PERMISSION_GRANTED },
                    .grantFlags = { 1 }
                }
            }
        };
        AccessTokenKit::AllocHapToken(info, policy);
    }
    static void TearDownTestCase()
    {
        auto tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.test.demo", 0);
        AccessTokenKit::DeleteToken(tokenId);
    }
    void SetUp()
    {
        tokenParam_.processName = "foundation";
        tokenParam_.aplStr = "system_core";

        Bootstrap::GetInstance().LoadDirectory();
    }
    void TearDown() {}

protected:
    NativeTokenInfoParams tokenParam_  {0};
};

/**
* @tc.name: GetBundleStorePath
* @tc.desc: test get kv db dir
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(DirectoryManagerTest, GetBundleStorePath, TestSize.Level0)
{
    StoreMetaData metaData;
    metaData.user = "100";
    metaData.bundleName = "ohos.test.demo";
    metaData.dataDir = "/data/app/el1/100/database/ohos.test.demo/kvdb";
    metaData.securityLevel = SecurityLevel::S2;
    metaData.area = 1;
    metaData.tokenId = AccessTokenKit::GetHapTokenID(100, "ohos.test.demo", 0);
    metaData.storeType = KvStoreType::SINGLE_VERSION;
    HapTokenInfo tokenInfo;
    AccessTokenKit::GetHapTokenInfo(metaData.tokenId, tokenInfo);
    metaData.appId = tokenInfo.appID;
    auto path = DirectoryManager::GetInstance().GetStorePath(metaData);
    EXPECT_EQ(path, metaData.dataDir);
}

/**
* @tc.name: GetHapStorePath
* @tc.desc: test get kv db dir
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(DirectoryManagerTest, GetHapStorePath, TestSize.Level0)
{
    StoreMetaData metaData;
    metaData.user = "100";
    metaData.bundleName = "com.sample.helloworld";
    metaData.hapName = "example.hap";
    metaData.dataDir = "/data/app/el2/100/database/com.sample.helloworld/example.hap/kvdb";
    metaData.securityLevel = SecurityLevel::S2;
    metaData.area = 2;
    metaData.storeType = KvStoreType::SINGLE_VERSION;
    auto path = DirectoryManager::GetInstance().GetStorePath(metaData);
    EXPECT_EQ(path, metaData.dataDir);
}

/**
* @tc.name: GetSaStorePath
* @tc.desc: test get kv db dir
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(DirectoryManagerTest, GetSaStorePath, TestSize.Level0)
{
    StoreMetaData metaData;
    metaData.user = "0";
    metaData.bundleName = "bundle_manager_service";
    metaData.appId = "bundle_manager_service";
    metaData.dataDir = "/data/service/el1/public/database/bundle_manager_service/kvdb";
    metaData.securityLevel = SecurityLevel::S2;
    metaData.area = 1;
    metaData.tokenId = GetAccessTokenId(&tokenParam_);
    metaData.storeType = KvStoreType::SINGLE_VERSION;
    auto path = DirectoryManager::GetInstance().GetStorePath(metaData);
    EXPECT_EQ(path, metaData.dataDir);
}

/**
* @tc.name: GetRdbBackupPath
* @tc.desc: test get rdb backup dir
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(DirectoryManagerTest, GetRDBBackupPath, TestSize.Level0)
{
    StoreMetaData metaData;
    metaData.user = "10";
    metaData.bundleName = "ohos.test.demo";
    metaData.dataDir = "/data/app/el1/10/database/ohos.test.demo";
    metaData.securityLevel = SecurityLevel::S2;
    metaData.area = 1;
    metaData.storeType = 10;
    metaData.tokenId = AccessTokenKit::GetHapTokenID(10, "ohos.test.demo", 0);
    HapTokenInfo tokenInfo;
    AccessTokenKit::GetHapTokenInfo(metaData.tokenId, tokenInfo);
    metaData.appId = tokenInfo.appID;
    metaData.storeId = "testStpre";
    auto path = DirectoryManager::GetInstance().GetStoreBackupPath(metaData);
    EXPECT_EQ(path, metaData.dataDir + "/rdb/backup/testStpre");
}
/**
* @tc.name: GetKVDBBackupPath
* @tc.desc: test get kv db backup dir
* @tc.type: FUNC
* @tc.require:
* @tc.author: baoyayong
*/
HWTEST_F(DirectoryManagerTest, GetKVDBBackupPath, TestSize.Level0)
{
    StoreMetaData metaData;
    metaData.user = "10";
    metaData.bundleName = "ohos.test.demo";
    metaData.dataDir = "/data/app/el1/10/database/ohos.test.demo/kvdb";
    metaData.securityLevel = SecurityLevel::S2;
    metaData.area = 1;
    metaData.storeType = 0;
    metaData.tokenId = AccessTokenKit::GetHapTokenID(10, "ohos.test.demo", 0);
    HapTokenInfo tokenInfo;
    AccessTokenKit::GetHapTokenInfo(metaData.tokenId, tokenInfo);
    metaData.appId = tokenInfo.appID;
    metaData.storeId = "testStpre";
    auto path = DirectoryManager::GetInstance().GetStoreBackupPath(metaData);
    EXPECT_EQ(path, metaData.dataDir + "/backup/testStpre");
}

/**
* @tc.name: GetStorageMetaPath
* @tc.desc: test get meta store dir
* @tc.type: FUNC
* @tc.require:
* @tc.author: Sven Wang
*/
HWTEST_F(DirectoryManagerTest, GetStorageMetaPath, TestSize.Level0)
{
    auto path = DirectoryManager::GetInstance().GetMetaStorePath();
    EXPECT_EQ(path, "/data/service/el1/public/database/distributeddata/meta");
}
