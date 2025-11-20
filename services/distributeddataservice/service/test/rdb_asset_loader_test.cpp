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

#include "log_print.h"
#include "object_snapshot.h"
#include "rdb_notifier_proxy.h"
#include "rdb_watcher.h"
#include "store/cursor.h"
#include "store/general_value.h"
#include "store/general_watcher.h"
#include "gtest/gtest.h"

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

    int32_t CancelDownload() override
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
    DistributedRdb::RdbAssetLoader rdbAssetLoader(cloudAssetLoader, bindAssets);
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
    DistributedRdb::RdbAssetLoader rdbAssetLoader(cloudAssetLoader, bindAssets);
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
    DistributedRdb::RdbAssetLoader rdbAssetLoader(cloudAssetLoader, bindAssets);
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
    DistributedRdb::RdbAssetLoader rdbAssetLoader(cloudAssetLoader, bindAssets);
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
    DistributedRdb::RdbAssetLoader rdbAssetLoader(cloudAssetLoader, bindAssets);
    std::vector<DistributedDB::Asset> assets;
    assets.push_back(g_rdbAsset);
    auto result = rdbAssetLoader.RemoveLocalAssets(assets);
    EXPECT_EQ(result, DistributedDB::DBStatus::OK);
}

/**
* @tc.name: CancelDownloadTest
* @tc.desc: RdbAssetLoader CancelDownload test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(RdbAssetLoaderTest, CancelDownloadTest, TestSize.Level0)
{
    BindAssets bindAssets;
    DistributedRdb::RdbAssetLoader rdbAssetLoader1(nullptr, bindAssets);
    auto result = rdbAssetLoader1.CancelDownload();
    EXPECT_EQ(result, DistributedDB::DBStatus::DB_ERROR);

    std::shared_ptr<MockAssetLoader> cloudAssetLoader = std::make_shared<MockAssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader2(cloudAssetLoader, bindAssets);
    result = rdbAssetLoader2.CancelDownload();
    EXPECT_EQ(result, DistributedDB::DBStatus::OK);
}

/**
* @tc.name: PostEvent001
* @tc.desc: RdbAssetLoader PostEvent001 test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, PostEvent001, TestSize.Level0)
{
    BindAssets bindAssets = nullptr;
    std::shared_ptr<AssetLoader> assetLoader = std::make_shared<AssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(assetLoader, bindAssets);
    std::string tableName = "testTable";
    std::string groupId = "testGroup";
    Type prefix;
    std::map<std::string, DistributedDB::Assets> assets;
    assets["asset1"].push_back(g_rdbAsset);
    auto result = rdbAssetLoader.Download(tableName, groupId, prefix, assets);
    EXPECT_EQ(result, DistributedDB::DBStatus::CLOUD_ERROR);
}

/**
* @tc.name: PostEvent002
* @tc.desc: RdbAssetLoader PostEvent002 test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, PostEvent002, TestSize.Level0)
{
    DistributedData::Asset asset = {
        .status = DistributedData::Asset::STATUS_DELETE,
        .id = "",
        .name = "",
        .uri = "",
        .createTime = "",
        .modifyTime = "",
        .size = "",
        .hash = "",
        .path = "",
    };

    DistributedData::Assets assets;
    assets.push_back(asset);
    BindAssets bindAssets = nullptr;
    std::shared_ptr<AssetLoader> assetLoader = std::make_shared<AssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(assetLoader, bindAssets);
    std::set<std::string> skipAssets;
    std::set<std::string> deleteAssets;
    rdbAssetLoader.PostEvent(DistributedData::AssetEvent::DOWNLOAD, assets, skipAssets, deleteAssets);
    EXPECT_EQ(deleteAssets.size(), 1);
}

/**
@tc.name: PostEvent003
@tc.desc: RdbAssetLoader PostEvent003 test
@tc.type: FUNC
@tc.require:
@tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, PostEvent003, TestSize.Level0) {
    DistributedData::Asset asset = {
        .status = DistributedData::Asset::STATUS_NORMAL,
        .id = "",
        .name = "",
        .uri = "",
        .createTime = "",
        .modifyTime = "",
        .size = "",
        .hash = "",
        .path = "",
    };
    DistributedData::Assets assets;
    assets.push_back(asset);
    BindAssets bindAssets = nullptr;
    std::shared_ptr assetLoader = std::make_shared<AssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(assetLoader, bindAssets);
    std::set<std::string> skipAssets;
    std::set<std::string> deleteAssets;
    rdbAssetLoader.PostEvent(DistributedData::AssetEvent::DOWNLOAD, assets, skipAssets, deleteAssets);
    EXPECT_EQ(deleteAssets.size(), 0);
    EXPECT_EQ(skipAssets.size(), 0);
}

/**
@tc.name: PostEvent004
@tc.desc: RdbAssetLoader PostEvent004 test，snapshots_ can find data with a value of null
@tc.type: FUNC
@tc.require:
@tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, PostEvent004, TestSize.Level0) {
    DistributedData::Asset asset = {
        .status = DistributedData::Asset::STATUS_NORMAL,
        .id = "",
        .name = "",
        .uri = "PostEvent004",
        .createTime = "",
        .modifyTime = "",
        .size = "",
        .hash = "",
        .path = "",
    };
    DistributedData::Assets assets;
    assets.push_back(asset);
    BindAssets bindAssets = std::make_shared<std::map<std::string, std::shared_ptr<Snapshot>>>();
    bindAssets->insert({"PostEvent004", nullptr});
    std::shared_ptr assetLoader = std::make_shared<AssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(assetLoader, bindAssets);
    std::set<std::string> skipAssets;
    std::set<std::string> deleteAssets;
    rdbAssetLoader.PostEvent(DistributedData::AssetEvent::DOWNLOAD, assets, skipAssets, deleteAssets);
    EXPECT_EQ(deleteAssets.size(), 0);
    EXPECT_EQ(skipAssets.size(), 0);
}

/**
@tc.name: PostEvent005
@tc.desc: RdbAssetLoader PostEvent005 test，snapshots_ can find data and the value is not null
@tc.type: FUNC
@tc.require:
@tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, PostEvent005, TestSize.Level0) {
    class ObjectSnapshotMock : public DistributedObject::ObjectSnapshot {
    public:
        int32_t Download(Asset &asset) override {
        hasDownload = true;
        return 0;
        }
        bool hasDownload = false;
    };
    DistributedData::Asset asset = {
        .status = DistributedData::Asset::STATUS_NORMAL,
        .id = "",
        .name = "",
        .uri = "PostEvent005",
        .createTime = "",
        .modifyTime = "",
        .size = "",
        .hash = "",
        .path = "",
    };
    DistributedData::Assets assets;
    assets.push_back(asset);
    BindAssets bindAssets = std::make_shared<std::map<std::string, std::shared_ptr<Snapshot>>>();
    auto objectSnapshotMock = std::make_shared<ObjectSnapshotMock>();
    bindAssets->insert({"PostEvent005", objectSnapshotMock});
    std::shared_ptr assetLoader = std::make_shared<AssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(assetLoader, bindAssets);
    std::set<std::string> skipAssets;
    std::set<std::string> deleteAssets;
    rdbAssetLoader.PostEvent(DistributedData::AssetEvent::DOWNLOAD, assets,
                            skipAssets, deleteAssets);
    EXPECT_EQ(objectSnapshotMock->hasDownload, true);
}

/**
@tc.name: PostEvent006
@tc.desc: RdbAssetLoader PostEvent006 test，snapshots_ not found
@tc.type: FUNC
@tc.require:
@tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, PostEvent006, TestSize.Level0) {
    DistributedData::Asset asset = {
        .status = DistributedData::Asset::STATUS_NORMAL,
        .id = "",
        .name = "",
        .uri = "",
        .createTime = "",
        .modifyTime = "",
        .size = "",
        .hash = "",
        .path = "",
    };
    DistributedData::Assets assets;
    assets.push_back(asset);
    BindAssets bindAssets = std::make_shared<std::map<std::string, std::shared_ptr<Snapshot>>>();
    bindAssets->insert({"PostEvent006", std::make_shared<DistributedObject::ObjectSnapshot>()});
    std::shared_ptr assetLoader = std::make_shared<AssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(assetLoader, bindAssets);
    std::set<std::string> skipAssets;
    std::set<std::string> deleteAssets;
    rdbAssetLoader.PostEvent(DistributedData::AssetEvent::DOWNLOAD, assets, skipAssets, deleteAssets);
    EXPECT_EQ(deleteAssets.size(), 0);
    EXPECT_EQ(skipAssets.size(), 0);
}

/**
@tc.name: PostEvent007
@tc.desc: RdbAssetLoader PostEvent007 test, Both deleteAssets and skipAssets can be found.
@tc.type: FUNC
@tc.require:
@tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, PostEvent007, TestSize.Level0) {
    class ObjectSnapshotMock : public DistributedObject::ObjectSnapshot {
    public:
        int32_t Downloaded(Asset &asset) override {
        hasDownloaded = true;
        return 0;
        }
        bool hasDownloaded = false;
    };
    DistributedData::Asset asset = {
        .status = DistributedData::Asset::STATUS_NORMAL,
        .id = "",
        .name = "",
        .uri = "PostEvent007",
        .createTime = "",
        .modifyTime = "",
        .size = "",
        .hash = "",
        .path = "",
    };
    DistributedData::Assets assets;
    assets.push_back(asset);
    BindAssets bindAssets = std::make_shared<std::map<std::string, std::shared_ptr<Snapshot>>>();
    auto objectSnapshotMock = std::make_shared<ObjectSnapshotMock>();
    bindAssets->insert({"PostEvent007", objectSnapshotMock});
    std::shared_ptr assetLoader = std::make_shared<AssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(assetLoader, bindAssets);
    std::set<std::string> skipAssets;
    skipAssets.insert("PostEvent007");
    std::set<std::string> deleteAssets;
    deleteAssets.insert("PostEvent007");
    rdbAssetLoader.PostEvent(DistributedData::AssetEvent::UPLOAD, assets, skipAssets, deleteAssets);
    EXPECT_EQ(objectSnapshotMock->hasDownloaded, false);
}

/**
@tc.name: PostEvent008
@tc.desc: RdbAssetLoader PostEvent008 test, Neither deleteAssets nor skipAssets can be found.
@tc.type: FUNC
@tc.require:
@tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, PostEvent008, TestSize.Level0) {
    class ObjectSnapshotMock : public DistributedObject::ObjectSnapshot {
    public:
        int32_t Downloaded(Asset &asset) override {
        hasDownloaded = true;
        return 0;
        }
        bool hasDownloaded = false;
    };
    DistributedData::Asset asset = {
        .status = DistributedData::Asset::STATUS_NORMAL,
        .id = "",
        .name = "",
        .uri = "PostEvent008",
        .createTime = "",
        .modifyTime = "",
        .size = "",
        .hash = "",
        .path = "",
    };
    DistributedData::Assets assets;
    assets.push_back(asset);
    BindAssets bindAssets = std::make_shared<std::map<std::string, std::shared_ptr<Snapshot>>>();
    auto objectSnapshotMock = std::make_shared<ObjectSnapshotMock>();
    bindAssets->insert({"PostEvent008", objectSnapshotMock});
    std::shared_ptr assetLoader = std::make_shared<AssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(assetLoader, bindAssets);
    std::set<std::string> skipAssets;
    std::set<std::string> deleteAssets;
    rdbAssetLoader.PostEvent(DistributedData::AssetEvent::UPLOAD, assets, skipAssets, deleteAssets);
    EXPECT_EQ(objectSnapshotMock->hasDownloaded, true);
}
/**

@tc.name: PostEvent009
@tc.desc: RdbAssetLoader PostEvent009 test, deleteAssets can be found, skipAssets cannot be found.
@tc.type: FUNC
@tc.require:
@tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, PostEvent009, TestSize.Level0) {
    class ObjectSnapshotMock : public DistributedObject::ObjectSnapshot {
    public:
        int32_t Downloaded(Asset &asset) override {
        hasDownloaded = true;
        return 0;
        }
        bool hasDownloaded = false;
    };
    DistributedData::Asset asset = {
        .status = DistributedData::Asset::STATUS_NORMAL,
        .id = "",
        .name = "",
        .uri = "PostEvent009",
        .createTime = "",
        .modifyTime = "",
        .size = "",
        .hash = "",
        .path = "",
    };
    DistributedData::Assets assets;
    assets.push_back(asset);
    BindAssets bindAssets = std::make_shared<std::map<std::string, std::shared_ptr<Snapshot>>>();
    auto objectSnapshotMock = std::make_shared<ObjectSnapshotMock>();
    bindAssets->insert({"PostEvent009", objectSnapshotMock});
    std::shared_ptr assetLoader = std::make_shared<AssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(assetLoader, bindAssets);
    std::set<std::string> skipAssets;
    std::set<std::string> deleteAssets;
    deleteAssets.insert("PostEvent009");
    rdbAssetLoader.PostEvent(DistributedData::AssetEvent::UPLOAD, assets, skipAssets, deleteAssets);
    EXPECT_EQ(objectSnapshotMock->hasDownloaded, false);
}

/**
@tc.name: PostEvent0010
@tc.desc: RdbAssetLoader PostEvent0010 test, skipAssets can be found, deleteAssets cannot be found.
@tc.type: FUNC
@tc.require:
@tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, PostEvent0010, TestSize.Level0) {
    class ObjectSnapshotMock : public DistributedObject::ObjectSnapshot {
    public:
        int32_t Downloaded(Asset &asset) override {
        hasDownloaded = true;
        return 0;
        }
        bool hasDownloaded = false;
    };
    DistributedData::Asset asset = {
        .status = DistributedData::Asset::STATUS_NORMAL,
        .id = "",
        .name = "",
        .uri = "PostEvent0010",
        .createTime = "",
        .modifyTime = "",
        .size = "",
        .hash = "",
        .path = "",
    };
    DistributedData::Assets assets;
    assets.push_back(asset);
    BindAssets bindAssets = std::make_shared<std::map<std::string, std::shared_ptr<Snapshot>>>();
    auto objectSnapshotMock = std::make_shared<ObjectSnapshotMock>();
    bindAssets->insert({"PostEvent0010", objectSnapshotMock});
    std::shared_ptr assetLoader = std::make_shared<AssetLoader>();
    DistributedRdb::RdbAssetLoader rdbAssetLoader(assetLoader, bindAssets);
    std::set<std::string> skipAssets;
    skipAssets.insert("PostEvent0010");
    std::set<std::string> deleteAssets;
    rdbAssetLoader.PostEvent(DistributedData::AssetEvent::UPLOAD, assets, skipAssets, deleteAssets);
    EXPECT_EQ(objectSnapshotMock->hasDownloaded, false);
}

/**
* @tc.name: ConvertStatus
* @tc.desc: RdbAssetLoader ConvertStatus abnormal
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbAssetLoaderTest, ConvertStatus, TestSize.Level0)
{
    auto status = RdbAssetLoader::ConvertStatus(DistributedRdb::RdbAssetLoader::AssetStatus::STATUS_DOWNLOADING);
    EXPECT_EQ(status, DistributedDB::DBStatus::OK);
    status = RdbAssetLoader::ConvertStatus(DistributedRdb::RdbAssetLoader::AssetStatus::STATUS_BUTT);
    EXPECT_EQ(status, DistributedDB::DBStatus::CLOUD_ERROR);
    status = RdbAssetLoader::ConvertStatus(DistributedRdb::RdbAssetLoader::AssetStatus::STATUS_SKIP_ASSET);
    EXPECT_EQ(status, DistributedDB::DBStatus::SKIP_ASSET);
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

/**
* @tc.name: OnChange
* @tc.desc: RdbWatcher OnChange function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RdbWatcherTest, OnChange, TestSize.Level0)
{
    std::string storeId = "testStoreId";
    int32_t triggerMode = 1;
    std::shared_ptr<RdbWatcher> watcher = std::make_shared<RdbWatcher>();
    sptr<RdbNotifierProxy> notifier;
    watcher->SetNotifier(notifier);
    EXPECT_EQ(watcher->notifier_, nullptr);
    auto result = watcher->OnChange(storeId, triggerMode);
    EXPECT_EQ(result, GeneralError::E_NOT_INIT);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test