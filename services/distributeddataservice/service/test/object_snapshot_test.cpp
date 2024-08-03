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

#define LOG_TAG "ObjectSnapshotTest"

#include "object_snapshot.h"
#include <gtest/gtest.h>
#include "snapshot/machine_status.h"
#include "executor_pool.h"

using namespace testing::ext;
using namespace OHOS::DistributedObject;
using namespace OHOS::DistributedData;
namespace OHOS::Test {

class ObjectSnapshotTest : public testing::Test {
public:
    void SetUp();
    void TearDown();

protected:
    AssetBindInfo AssetBindInfo_;
    Asset asset_;
    std::string uri_;
    StoreInfo storeInfo_;
};

void ObjectSnapshotTest::SetUp()
{
    uri_ = "file:://com.example.hmos.notepad/data/storage/el2/distributedfiles/dir/asset1.jpg";
    Asset asset{
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime",
        .size = "size",
        .hash = "modifyTime_size",
    };
    asset_ = asset;

    VBucket vBucket{ { "11", 111 } };
    AssetBindInfo AssetBindInfo{
        .storeName = "store_test",
        .tableName = "table_test",
        .primaryKey = vBucket,
        .field = "attachment",
        .assetName = "asset1.jpg",
    };
    AssetBindInfo_ = AssetBindInfo;
    StoreInfo storeInfo {
        .tokenId = 0,
        .bundleName = "bundleName_test",
        .storeName = "store_test",
        .instanceId = 1,
        .user = 100,
    };
    storeInfo_ = storeInfo;
    ChangedAssetInfo changedAssetInfo(asset, AssetBindInfo, storeInfo);
}

void ObjectSnapshotTest::TearDown() {}

/**
* @tc.name: UploadTest001
* @tc.desc: Upload test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectSnapshotTest, UploadTest001, TestSize.Level0)
{
    auto snapshot = std::make_shared<ObjectSnapshot>();
    Asset asset{
        .name = "test_name",
        .uri = "file:://com.example.hmos.notepad/data/storage/el2/distributedfiles/dir/asset2.jpg",
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
    };
    snapshot->BindAsset(asset_, AssetBindInfo_, storeInfo_);
    ASSERT_EQ(snapshot->IsBoundAsset(asset), false);
    auto upload = snapshot->Upload(asset);
    ASSERT_EQ(upload, 0);
}

/**
* @tc.name: UploadTest002
* @tc.desc: Upload test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectSnapshotTest, UploadTest002, TestSize.Level0)
{
    auto snapshot = std::make_shared<ObjectSnapshot>();
    snapshot->BindAsset(asset_, AssetBindInfo_, storeInfo_);
    ASSERT_EQ(snapshot->IsBoundAsset(asset_), true);
    auto upload = snapshot->Upload(asset_);
    ASSERT_EQ(upload, 0);
}

/**
* @tc.name: DownloadTest001
* @tc.desc: Download test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectSnapshotTest, DownloadTest001, TestSize.Level0)
{
    auto snapshot = std::make_shared<ObjectSnapshot>();
    Asset asset{
        .name = "test_name",
        .uri = "file:://com.example.hmos.notepad/data/storage/el2/distributedfiles/dir/asset2.jpg",
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
    };
    snapshot->BindAsset(asset_, AssetBindInfo_, storeInfo_);
    ASSERT_EQ(snapshot->IsBoundAsset(asset), false);
    auto upload = snapshot->Download(asset);
    ASSERT_EQ(upload, 0);
}

/**
* @tc.name: DownloadTest002
* @tc.desc: Download test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectSnapshotTest, DownloadTest002, TestSize.Level0)
{
    auto snapshot = std::make_shared<ObjectSnapshot>();
    snapshot->BindAsset(asset_, AssetBindInfo_, storeInfo_);
    ASSERT_EQ(snapshot->IsBoundAsset(asset_), true);
    auto upload = snapshot->Download(asset_);
    ASSERT_EQ(upload, 0);
}

/**
* @tc.name: GetAssetStatusTest001
* @tc.desc: GetAssetStatus test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectSnapshotTest, GetAssetStatusTest001, TestSize.Level0)
{
    auto snapshot = std::make_shared<ObjectSnapshot>();
    Asset asset{
        .name = "test_name",
        .uri = "file:://com.example.hmos.notepad/data/storage/el2/distributedfiles/dir/asset2.jpg",
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
    };
    snapshot->BindAsset(asset_, AssetBindInfo_, storeInfo_);
    ASSERT_EQ(snapshot->IsBoundAsset(asset), false);
    auto upload = snapshot->GetAssetStatus(asset);
    ASSERT_EQ(upload, STATUS_BUTT);
}

/**
* @tc.name: GetAssetStatusTest002
* @tc.desc: GetAssetStatus test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectSnapshotTest, GetAssetStatusTest002, TestSize.Level0)
{
    auto snapshot = std::make_shared<ObjectSnapshot>();
    snapshot->BindAsset(asset_, AssetBindInfo_, storeInfo_);
    ASSERT_EQ(snapshot->IsBoundAsset(asset_), true);
    auto upload = snapshot->GetAssetStatus(asset_);
    ASSERT_EQ(upload, STATUS_STABLE);
}

/**
* @tc.name: UploadedTest001
* @tc.desc: Uploaded test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectSnapshotTest, UploadedTest001, TestSize.Level0)
{
    auto snapshot = std::make_shared<ObjectSnapshot>();
    Asset asset{
        .name = "test_name",
        .uri = "file:://com.example.hmos.notepad/data/storage/el2/distributedfiles/dir/asset2.jpg",
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
    };
    snapshot->BindAsset(asset_, AssetBindInfo_, storeInfo_);
    ASSERT_EQ(snapshot->IsBoundAsset(asset), false);
    auto upload = snapshot->Uploaded(asset);
    ASSERT_EQ(upload, E_OK);
}

/**
* @tc.name: UploadedTest002
* @tc.desc: Uploaded test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectSnapshotTest, UploadedTest002, TestSize.Level0)
{
    auto snapshot = std::make_shared<ObjectSnapshot>();
    snapshot->BindAsset(asset_, AssetBindInfo_, storeInfo_);
    ASSERT_EQ(snapshot->IsBoundAsset(asset_), true);
    auto upload = snapshot->Uploaded(asset_);
    ASSERT_EQ(upload, E_OK);
}

/**
* @tc.name: DownloadedTest001
* @tc.desc: Downloaded test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectSnapshotTest, DownloadedTest001, TestSize.Level0)
{
    auto snapshot = std::make_shared<ObjectSnapshot>();
    Asset asset{
        .name = "test_name",
        .uri = "file:://com.example.hmos.notepad/data/storage/el2/distributedfiles/dir/asset2.jpg",
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
    };
    snapshot->BindAsset(asset_, AssetBindInfo_, storeInfo_);
    ASSERT_EQ(snapshot->IsBoundAsset(asset), false);
    auto upload = snapshot->Downloaded(asset);
    ASSERT_EQ(upload, E_OK);
}

/**
* @tc.name: DownloadedTest002
* @tc.desc: Downloaded test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectSnapshotTest, DownloadedTest002, TestSize.Level0)
{
    auto snapshot = std::make_shared<ObjectSnapshot>();
    snapshot->BindAsset(asset_, AssetBindInfo_, storeInfo_);
    ASSERT_EQ(snapshot->IsBoundAsset(asset_), true);
    auto upload = snapshot->Downloaded(asset_);
    ASSERT_EQ(upload, E_OK);
}

/**
* @tc.name: TransferredTest001
* @tc.desc: Transferred test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectSnapshotTest, TransferredTest001, TestSize.Level0)
{
    auto snapshot = std::make_shared<ObjectSnapshot>();
    Asset asset{
        .name = "test_name",
        .uri = "file:://com.example.hmos.notepad/data/storage/el2/distributedfiles/dir/asset2.jpg",
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
    };
    snapshot->BindAsset(asset_, AssetBindInfo_, storeInfo_);
    ASSERT_EQ(snapshot->IsBoundAsset(asset), false);
    auto upload = snapshot->Transferred(asset);
    ASSERT_EQ(upload, E_OK);
}

/**
* @tc.name: TransferredTest002
* @tc.desc: Transferred test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectSnapshotTest, TransferredTest002, TestSize.Level0)
{
    auto snapshot = std::make_shared<ObjectSnapshot>();
    snapshot->BindAsset(asset_, AssetBindInfo_, storeInfo_);
    ASSERT_EQ(snapshot->IsBoundAsset(asset_), true);
    auto upload = snapshot->Transferred(asset_);
    ASSERT_EQ(upload, E_OK);
}

/**
* @tc.name: OnDataChangedTest001
* @tc.desc: OnDataChanged test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectSnapshotTest, OnDataChangedTest001, TestSize.Level0)
{
    auto snapshot = std::make_shared<ObjectSnapshot>();
    std::string deviceId = "object_snapshot_test_1";
    Asset asset{
        .name = "test_name",
        .uri = "file:://com.example.hmos.notepad/data/storage/el2/distributedfiles/dir/asset2.jpg",
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
    };
    snapshot->BindAsset(asset_, AssetBindInfo_, storeInfo_);
    ASSERT_EQ(snapshot->IsBoundAsset(asset), false);
    auto upload = snapshot->OnDataChanged(asset, deviceId);
    ASSERT_EQ(upload, E_OK);
}

/**
* @tc.name: OnDataChangedTest002
* @tc.desc: OnDataChanged test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectSnapshotTest, OnDataChangedTest002, TestSize.Level0)
{
    auto snapshot = std::make_shared<ObjectSnapshot>();
    std::string deviceId = "object_snapshot_test_1";
    snapshot->BindAsset(asset_, AssetBindInfo_, storeInfo_);
    ASSERT_EQ(snapshot->IsBoundAsset(asset_), true);
    auto upload = snapshot->OnDataChanged(asset_, deviceId);
    ASSERT_EQ(upload, E_OK);
}

/**
* @tc.name: BindAsset001
* @tc.desc: BindAsset test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectSnapshotTest, BindAsset001, TestSize.Level0)
{
    auto snapshot = std::make_shared<ObjectSnapshot>();
    Asset asset{
        .name = "test_name",
        .uri = "file:://com.example.hmos.notepad/data/storage/el2/distributedfiles/dir/asset2.jpg",
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
    };
    ASSERT_EQ(snapshot->IsBoundAsset(asset), false);
    snapshot->BindAsset(asset, AssetBindInfo_, storeInfo_);
    ASSERT_EQ(snapshot->IsBoundAsset(asset), true);
    snapshot->BindAsset(asset, AssetBindInfo_, storeInfo_);
}
} // namespace OHOS::Test