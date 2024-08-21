/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#define LOG_TAG "ObjectAssetMachineTest"

#include "object_asset_machine.h"

#include <gtest/gtest.h>

#include "snapshot/machine_status.h"
#include "object_asset_loader.h"
#include "executor_pool.h"

using namespace testing::ext;
using namespace OHOS::DistributedObject;
using namespace OHOS::DistributedData;
namespace OHOS::Test {

class ObjectAssetMachineTest : public testing::Test {
public:
    void SetUp();
    void TearDown();

protected:
    AssetBindInfo AssetBindInfo_;
    Asset asset_;
    std::string uri_;
    std::string bundleName_ = "test_bundleName";
    std::map<std::string, ChangedAssetInfo> changedAssets_;
    std::string sessionId = "123";
    StoreInfo storeInfo_;
};

void ObjectAssetMachineTest::SetUp()
{
    uri_ = "file:://com.huawei.hmos.notepad/data/storage/el2/distributedfiles/dir/asset1.jpg";
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
    changedAssets_[uri_] = changedAssetInfo;
    auto executors = std::make_shared<ExecutorPool>(1, 0);
    ObjectAssetLoader::GetInstance()->SetThreadPool(executors);
}

void ObjectAssetMachineTest::TearDown() {}

/**
* @tc.name: StatusTransfer001
* @tc.desc: Remote changed when transferring.
* @tc.type: FUNC
* @tc.require:
* @tc.author: whj
*/
HWTEST_F(ObjectAssetMachineTest, StatusTransfer001, TestSize.Level0)
{
    auto machine = std::make_shared<ObjectAssetMachine>();
    Asset asset{
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime2",
        .size = "size2",
        .hash = "modifyTime2_size2",
    };
    std::pair<std::string, Asset> changedAsset{ "device_2", asset };
    changedAssets_[uri_].status = STATUS_TRANSFERRING;
    machine->DFAPostEvent(REMOTE_CHANGED, changedAssets_[uri_], asset, changedAsset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_WAIT_TRANSFER);
    ASSERT_EQ(changedAssets_[uri_].deviceId, changedAsset.first);
    ASSERT_EQ(changedAssets_[uri_].asset.hash, asset.hash);
}

/**
* @tc.name: StatusTransfer002
* @tc.desc: Transfer finished.
* @tc.type: FUNC
* @tc.require:
* @tc.author: whj
*/
HWTEST_F(ObjectAssetMachineTest, StatusTransfer002, TestSize.Level0)
{
    auto machine = std::make_shared<ObjectAssetMachine>();
    Asset asset{
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime2",
        .size = "size2",
        .hash = "modifyTime2_size2",
    };
    std::pair<std::string, Asset> changedAsset{ "device_2", asset };
    changedAssets_[uri_].status = STATUS_TRANSFERRING;
    machine->DFAPostEvent(TRANSFER_FINISHED, changedAssets_[uri_], asset, changedAsset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_STABLE);
}

/**
* @tc.name: StatusTransfer003
* @tc.desc: Transfer event
* @tc.type: FUNC
* @tc.require:
* @tc.author: nhj
*/
HWTEST_F(ObjectAssetMachineTest, StatusTransfer003, TestSize.Level0)
{
    auto machine = std::make_shared<ObjectAssetMachine>();
    Asset asset{
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
    };
    std::pair<std::string, Asset> changedAsset{ "device_2", asset };
    changedAssets_[uri_].status = STATUS_UPLOADING;
    machine->DFAPostEvent(REMOTE_CHANGED, changedAssets_[uri_], asset, changedAsset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_WAIT_TRANSFER);
    ASSERT_EQ(changedAssets_[uri_].deviceId, changedAsset.first);
    ASSERT_EQ(changedAssets_[uri_].asset.hash, asset.hash);
}

/**
* @tc.name: StatusUpload001
* @tc.desc: No conflict scenarios: normal cloud sync.
* @tc.type: FUNC
* @tc.require:
* @tc.author: whj
*/
HWTEST_F(ObjectAssetMachineTest, StatusUpload001, TestSize.Level0)
{
    auto machine = std::make_shared<ObjectAssetMachine>();
    Asset asset{
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
    };
    std::pair<std::string, Asset> changedAsset{ "device_1", asset };
    machine->DFAPostEvent(UPLOAD, changedAssets_[uri_], asset, changedAsset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_UPLOADING);

    machine->DFAPostEvent(UPLOAD_FINISHED, changedAssets_[uri_], asset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_STABLE);
}

/**
* @tc.name: StatusUpload002
* @tc.desc: Conflict scenario: Upload before transfer.
* @tc.type: FUNC
* @tc.require:
* @tc.author: whj
*/
HWTEST_F(ObjectAssetMachineTest, StatusUpload002, TestSize.Level0)
{
    auto machine = std::make_shared<ObjectAssetMachine>();
    Asset asset{
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
    };
    std::pair<std::string, Asset> changedAsset{ "device_1", asset };
    machine->DFAPostEvent(UPLOAD, changedAssets_[uri_], asset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_UPLOADING);

    machine->DFAPostEvent(REMOTE_CHANGED, changedAssets_[uri_], asset, changedAsset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_WAIT_TRANSFER);
    ASSERT_EQ(changedAssets_[uri_].asset.hash, asset.hash);

    machine->DFAPostEvent(UPLOAD_FINISHED, changedAssets_[uri_], asset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_TRANSFERRING);
}

/**
* @tc.name: StatusDownload001
* @tc.desc: No conflict scenarios: normal cloud sync.
* @tc.type: FUNC
* @tc.require:
* @tc.author: nhj
*/
HWTEST_F(ObjectAssetMachineTest, StatusDownload001, TestSize.Level0)
{
    auto machine = std::make_shared<ObjectAssetMachine>();
    Asset asset{
        .name = "name_006",
        .uri = "uri_006",
        .modifyTime = "modifyTime_006",
        .size = "size_006",
        .hash = "modifyTime_006_size_006",
    };
    AssetBindInfo AssetBindInfo{
        .storeName = "store_006",
        .tableName = "table_006",
        .primaryKey = {{ "006", "006" }},
        .field = "attachment_006",
        .assetName = "asset_006.jpg",
    };
    StoreInfo storeInfo {
        .tokenId = 600,
        .bundleName = "bundleName_006",
        .storeName = "store_006",
        .instanceId = 600,
        .user = 600,
    };
    ChangedAssetInfo changedAssetInfo(asset, AssetBindInfo, storeInfo);
    std::pair<std::string, Asset> changedAsset{ "device_006", asset };

    machine->DFAPostEvent(DOWNLOAD, changedAssetInfo, asset, changedAsset);
    ASSERT_EQ(changedAssetInfo.status, STATUS_DOWNLOADING);

    machine->DFAPostEvent(DOWNLOAD_FINISHED, changedAssetInfo, asset);
    ASSERT_EQ(changedAssetInfo.status, STATUS_STABLE);
}
} // namespace OHOS::Test