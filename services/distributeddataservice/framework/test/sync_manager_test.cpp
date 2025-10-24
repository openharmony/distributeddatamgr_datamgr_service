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
#include "ipc_skeleton.h"
#include "sync_mgr/sync_mgr.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;

namespace OHOS::Test {
class SyncManagerTest : public testing::Test {
public:
    void SetUp()
    {
        SyncManager::AutoSyncInfo syncInfo1 = { 1, "appId1", "bundleName1" };
        SyncManager::AutoSyncInfo syncInfo2 = { 2, "appId2", "bundleName2" };
        SyncManager::AutoSyncInfo syncInfo3 = { 3, "appId3", "bundleName3" };
        SyncManager::AutoSyncInfo syncInfo4 = { 3, "appId13", "bundleName13", { "storeId1", "storeId2" } };
        SyncManager::GetInstance().Initialize({ syncInfo1, syncInfo2, syncInfo3, syncInfo4 });
    }
};

/**
* @tc.name: NeedForceReplaceSchema001
* @tc.desc: the app need force replace the sync schema.
* @tc.type: FUNC
*/
HWTEST_F(SyncManagerTest, NeedForceReplaceSchema001, TestSize.Level1)
{
    SyncManager::AutoSyncInfo autoSyncInfo = { 1, "appId1", "bundleName1" };
    bool result = SyncManager::GetInstance().NeedForceReplaceSchema(autoSyncInfo);
    EXPECT_TRUE(result);
}

/**
* @tc.name: NeedForceReplaceSchema002
* @tc.desc: the app need not force replace the sync schema.
* @tc.type: FUNC
*/
HWTEST_F(SyncManagerTest, NeedForceReplaceSchema002, TestSize.Level1)
{
    SyncManager::AutoSyncInfo autoSyncInfo = { 1, "appId2", "bundleName1" };
    bool result = SyncManager::GetInstance().NeedForceReplaceSchema(autoSyncInfo);
    EXPECT_FALSE(result);
}

/**
* @tc.name: NeedForceReplaceSchema003
* @tc.desc: the app need not force replace the sync schema.
* @tc.type: FUNC
*/
HWTEST_F(SyncManagerTest, NeedForceReplaceSchema003, TestSize.Level1)
{
    SyncManager::AutoSyncInfo autoSyncInfo = { 1, "appId1", "bundleName2" };
    bool result = SyncManager::GetInstance().NeedForceReplaceSchema(autoSyncInfo);
    EXPECT_FALSE(result);
}

/**
* @tc.name: NeedForceReplaceSchema004
* @tc.desc: the app need not force replace the sync schema.
* @tc.type: FUNC
*/
HWTEST_F(SyncManagerTest, NeedForceReplaceSchema004, TestSize.Level1)
{
    SyncManager::AutoSyncInfo autoSyncInfo = { 2, "appId1", "bundleName2" };
    bool result = SyncManager::GetInstance().NeedForceReplaceSchema(autoSyncInfo);
    EXPECT_FALSE(result);
}

/**
* @tc.name: NeedForceReplaceSchema005
* @tc.desc: the app need not force replace the sync schema.
* @tc.type: FUNC
*/
HWTEST_F(SyncManagerTest, NeedForceReplaceSchema005, TestSize.Level1)
{
    SyncManager::AutoSyncInfo autoSyncInfo = { 4, "appId4", "bundleName4" };
    bool result = SyncManager::GetInstance().NeedForceReplaceSchema(autoSyncInfo);
    EXPECT_FALSE(result);
}

/**
* @tc.name: IsAutoSyncApp001
* @tc.desc: the app is auto sync system app.
* @tc.type: FUNC
*/
HWTEST_F(SyncManagerTest, IsAutoSyncApp001, TestSize.Level1)
{
    // valid bundleName and valid appid
    bool autoSync = SyncManager::GetInstance().IsAutoSyncApp("bundleName2", "appId2");
    EXPECT_TRUE(autoSync);
    // invalid bundleName and invalid appid
    autoSync = SyncManager::GetInstance().IsAutoSyncApp("bundleName4", "appId4");
    EXPECT_FALSE(autoSync);
    // valid bundleName and invalid appid
    autoSync = SyncManager::GetInstance().IsAutoSyncApp("bundleName1", "appId5");
    EXPECT_FALSE(autoSync);
}

/**
* @tc.name: IsAutoSyncStore001
* @tc.desc: the bundleName is equal to the sync system app, but the appId is another app.
* @tc.type: FUNC
*/
HWTEST_F(SyncManagerTest, IsAutoSyncStore001, TestSize.Level1)
{
    // valid bundleName, valid appid, valid storeId,
    bool autoSyncStore = SyncManager::GetInstance().IsAutoSyncStore("bundleName13", "appId13", "storeId1");
    EXPECT_TRUE(autoSyncStore);
    // valid bundleName, valid appid, another valid storeId,
    autoSyncStore = SyncManager::GetInstance().IsAutoSyncStore("bundleName13", "appId13", "storeId2");
    EXPECT_TRUE(autoSyncStore);
    // valid bundleName, valid appid, invalid storeId,
    autoSyncStore = SyncManager::GetInstance().IsAutoSyncStore("bundleName13", "appId13", "storeId3");
    EXPECT_FALSE(autoSyncStore);
    // invalid bundleName, valid appid, invalid storeId,
    autoSyncStore = SyncManager::GetInstance().IsAutoSyncStore("bundleName1", "appId13", "storeId3");
    EXPECT_FALSE(autoSyncStore);
    // invalid bundleName, invalid appid, invalid storeId,
    autoSyncStore = SyncManager::GetInstance().IsAutoSyncStore("bundleName1", "appId1", "storeId3");
    EXPECT_FALSE(autoSyncStore);
    // invalid bundleName, invalid appid, valid storeId,
    autoSyncStore = SyncManager::GetInstance().IsAutoSyncStore("bundleName1", "appId1", "storeId1");
    EXPECT_FALSE(autoSyncStore);
    // invalid bundleName, valid appid, valid storeId,
    autoSyncStore = SyncManager::GetInstance().IsAutoSyncStore("bundleName1", "appId13", "storeId1");
    EXPECT_FALSE(autoSyncStore);
}

/**
@tc.name: SetDoubleSyncInfo001
@tc.desc: set doubleSyncInfo
@tc.type: FUNC
*/
HWTEST_F(SyncManagerTest, SetDoubleSyncInfo001, TestSize.Level1)
{
    SyncManager::DoubleSyncInfo doubleSyncInfo;
    doubleSyncInfo.bundleName = "testName";
    doubleSyncInfo.appId = "testId";
    SyncManager::GetInstance().SetDoubleSyncInfo(doubleSyncInfo);
    EXPECT_TRUE(SyncManager::GetInstance().doubleSyncMap_.find(doubleSyncInfo.bundleName) !=
        SyncManager::GetInstance().doubleSyncMap_.end());
}

/**

@tc.name: IsAccessRestricted001
@tc.desc: IsAccessRestricted test.
@tc.type: FUNC
*/
HWTEST_F(SyncManagerTest, IsAccessRestricted001, TestSize.Level1)
{
    int32_t tokenId = OHOS::IPCSkeleton::GetCallingTokenID();\
    SyncManager::DoubleSyncInfo info;
    info.tokenId = tokenId;
    info.appId = "";
    info.bundleName = "";
    bool res = SyncManager::GetInstance().IsAccessRestricted(tokenId, bundleName);
    EXPECT_TRUE(res);
}
} // namespace OHOS::Test