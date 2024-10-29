/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "RdbAssetLoaderTest"

#include "rdb_asset_loader.h"

#include "gtest/gtest.h"
#include "log_print.h"
#include "rdb_notifier_proxy.h"
#include "rdb_watcher.h"
#include "store/cursor.h"
#include "store/general_value.h"
#include "store/general_watcher.h"
using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
using Type = DistributedDB::Type;
using AssetsRecord = DistributedData::AssetRecord;
const DistributedDB::Asset g_rdbAsset = { .version = 1,
    .name = "Phone",
    .assetId = "0",
    .subpath = "/local/sync",
    .uri = "file://rdbtest/local/sync",
    .modifyTime = "123456",
    .createTime = "createTime",
    .size = "256",
    .hash = "ASE" };
namespace OHOS::Test {
namespace DistributedRDBTest {
class RdbAssetLoaderTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class MockAssetLoader : public DistributedData::AssetLoader {
public:
    int32_t Download(const std::string &tableName, const std::string &gid,
        const DistributedData::Value &prefix, VBucket &assets) override
    {
        return GeneralError::E_OK;
    }

    int32_t RemoveLocalAssets(const std::string &tableName, const std::string &gid,
        const DistributedData::Value &prefix, VBucket &assets) override
    {
        return GeneralError::E_OK;
    }
};

class RdbWatcherTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: Download
* @tc.desc: RdbAssetLoader download test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, Download, TestSize.Level0)
{
    BindAssets bindAssets;
    std::shared_ptr<MockAssetLoader> cloudAssetLoader = std::make_shared<MockAssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(cloudAssetLoader, &bindAssets);
    std::string tableName = "testTable";
    std::string groupId = "testGroup";
    Type prefix;
    std::map<std::string, DistributedDB::Assets> assets;
    assets["asset1"].push_back(g_rdbAsset);
    auto result = rdbAssetLoader.Download(tableName, groupId, prefix, assets);
    EXPECT_EQ(result, DistributedDB::DBStatus::OK);
}

/**
* @tc.name: BatchDownload
* @tc.desc: RdbAssetLoader batch download test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RdbAssetLoaderTest, BatchDownload, TestSize.Level0)
{
    BindAssets bindAssets;
    auto cloudAssetLoader = std::make_shared<AssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(cloudAssetLoader, &bindAssets);
    std::string tableName = "testTable";
    Type prefix;
    std::map<std::string, DistributedDB::Assets> assets;
    assets["asset1"].push_back(g_rdbAsset);
    std::vector<DistributedDB::IAssetLoader::AssetRecord> assetRecords;
    DistributedDB::IAssetLoader::AssetRecord assetRecord { .gid = "gid", .prefix = prefix, .assets = assets };
    assetRecords.emplace_back(assetRecord);
    rdbAssetLoader.BatchDownload(tableName, assetRecords);
    ASSERT_TRUE(!assetRecords.empty());
    EXPECT_EQ(assetRecords[0].status, DistributedDB::DBStatus::OK);
}

/**
* @tc.name: Convert001
* @tc.desc: Convert test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RdbAssetLoaderTest, Convert001, TestSize.Level0)
{
    BindAssets bindAssets;
    auto cloudAssetLoader = std::make_shared<AssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(cloudAssetLoader, &bindAssets);
    Type prefix;
    std::map<std::string, DistributedDB::Assets> assets;
    assets["asset1"].push_back(g_rdbAsset);
    std::vector<DistributedDB::IAssetLoader::AssetRecord> assetRecords;
    DistributedDB::IAssetLoader::AssetRecord assetRecord { .gid = "gid", .prefix = prefix, .assets = assets };
    assetRecords.emplace_back(assetRecord);
    assetRecords.emplace_back(assetRecord);
    auto assetsRecords = rdbAssetLoader.Convert(std::move(assetRecords));
    EXPECT_TRUE(!assetsRecords.empty());
}

/**
* @tc.name: Convert002
* @tc.desc: Convert test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RdbAssetLoaderTest, Convert002, TestSize.Level0)
{
    BindAssets bindAssets;
    auto cloudAssetLoader = std::make_shared<AssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(cloudAssetLoader, &bindAssets);
    Value prefix;
    VBucket assets;
    std::vector<AssetsRecord> assetsRecords;
    AssetRecord assetsRecord { .gid = "gid", .prefix = prefix, .assets = assets };
    assetsRecords.emplace_back(assetsRecord);
    auto assetRecords = rdbAssetLoader.Convert(std::move(assetsRecords));
    EXPECT_TRUE(!assetRecords.empty());
}

/**
* @tc.name: RemoveLocalAssets
* @tc.desc: RdbAssetLoader RemoveLocalAssets test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, RemoveLocalAssets, TestSize.Level0)
{
    BindAssets bindAssets;
    std::shared_ptr<MockAssetLoader> cloudAssetLoader = std::make_shared<MockAssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(cloudAssetLoader, &bindAssets);
    std::vector<DistributedDB::Asset> assets;
    assets.push_back(g_rdbAsset);
    auto result = rdbAssetLoader.RemoveLocalAssets(assets);
    EXPECT_EQ(result, DistributedDB::DBStatus::OK);
}

/**
* @tc.name: PostEvent
* @tc.desc: RdbAssetLoader PostEvent abnormal
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, PostEvent, TestSize.Level0)
{
    BindAssets bindAssets;
    bindAssets.bindAssets = nullptr;
    std::shared_ptr<AssetLoader> assetLoader = std::make_shared<AssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(assetLoader, &bindAssets);
    std::string tableName = "testTable";
    std::string groupId = "testGroup";
    Type prefix;
    std::map<std::string, DistributedDB::Assets> assets;
    assets["asset1"].push_back(g_rdbAsset);
    auto result = rdbAssetLoader.Download(tableName, groupId, prefix, assets);
    EXPECT_EQ(result, DistributedDB::DBStatus::CLOUD_ERROR);
}

/**
* @tc.name: RdbWatcher
* @tc.desc: RdbWatcher function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbWatcherTest, RdbWatcher, TestSize.Level0)
{
    GeneralWatcher::Origin origin;
    GeneralWatcher::PRIFields primaryFields = {{"primaryFields1", "primaryFields2"}};
    GeneralWatcher::ChangeInfo values;
    std::shared_ptr<RdbWatcher> watcher = std::make_shared<RdbWatcher>();
    sptr<RdbNotifierProxy> notifier;
    watcher->SetNotifier(notifier);
    EXPECT_EQ(watcher->notifier_, nullptr);
    auto result = watcher->OnChange(origin, primaryFields, std::move(values));
    EXPECT_EQ(result, GeneralError::E_NOT_INIT);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test