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

#include "executor_pool.h"
#include "object_asset_loader.h"
#include "snapshot/machine_status.h"

using namespace testing::ext;
using namespace OHOS::DistributedObject;
using namespace OHOS::DistributedData;
namespace OHOS::Test {

class ObjectAssetMachineTest : public testing::Test {
public:
    void SetUp();
    void TearDown();
    static void SetUpTestCase(void);

protected:
    AssetBindInfo AssetBindInfo_;
    Asset asset_;
    std::string uri_;
    std::string bundleName_ = "test_bundleName";
    std::map<std::string, ChangedAssetInfo> changedAssets_;
    std::string sessionId_ = "123";
    StoreInfo storeInfo_;
    std::shared_ptr<ObjectAssetMachine> machine_;
};

void ObjectAssetMachineTest::SetUpTestCase(void)
{
    auto executors = std::make_shared<ExecutorPool>(2, 1);
    ObjectAssetLoader::GetInstance().SetThreadPool(executors);
}

void ObjectAssetMachineTest::SetUp()
{
    uri_ = "file:://com.examples.notepad/data/storage/el2/distributedfiles/dir/asset1.jpg";
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
    changedAssets_[uri_].status = STATUS_STABLE;
    if (machine_ == nullptr) {
        machine_ = std::make_shared<ObjectAssetMachine>();
    }
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
    Asset asset{
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime2",
        .size = "size2",
        .hash = "modifyTime2_size2",
    };
    std::pair<std::string, Asset> changedAsset{ "device_2", asset };
    changedAssets_[uri_].status = STATUS_TRANSFERRING;
    machine_->DFAPostEvent(REMOTE_CHANGED, changedAssets_[uri_], asset, changedAsset);
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
    Asset asset{
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime2",
        .size = "size2",
        .hash = "modifyTime2_size2",
    };
    std::pair<std::string, Asset> changedAsset{ "device_2", asset };
    changedAssets_[uri_].status = STATUS_TRANSFERRING;
    machine_->DFAPostEvent(TRANSFER_FINISHED, changedAssets_[uri_], asset, changedAsset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_STABLE);
}

/**
* @tc.name: StatusTransfer003
* @tc.desc: Transfer event
* @tc.type: FUNC
* @tc.require:
* @tc.author: whj
*/
HWTEST_F(ObjectAssetMachineTest, StatusTransfer003, TestSize.Level0)
{
    Asset asset{
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
    };
    std::pair<std::string, Asset> changedAsset{ "device_2", asset };
    changedAssets_[uri_].status = STATUS_UPLOADING;
    machine_->DFAPostEvent(REMOTE_CHANGED, changedAssets_[uri_], asset, changedAsset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_WAIT_TRANSFER);
    ASSERT_EQ(changedAssets_[uri_].deviceId, changedAsset.first);
    ASSERT_EQ(changedAssets_[uri_].asset.hash, asset.hash);
}

/**
* @tc.name: DFAPostEvent001
* @tc.desc: DFAPostEvent invalid eventId test
* @tc.type: FUNC
*/
HWTEST_F(ObjectAssetMachineTest, DFAPostEvent001, TestSize.Level0)
{
    Asset asset{
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
    };
    std::pair<std::string, Asset> changedAsset{ "device_2", asset };
    changedAssets_[uri_].status = STATUS_UPLOADING;
    auto ret = machine_->DFAPostEvent(EVENT_BUTT, changedAssets_[uri_], asset, changedAsset);
    ASSERT_EQ(ret, GeneralError::E_ERROR);
}

/**
* @tc.name: DFAPostEvent001
* @tc.desc: DFAPostEvent invalid status test
* @tc.type: FUNC
*/
HWTEST_F(ObjectAssetMachineTest, DFAPostEvent002, TestSize.Level0)
{
    Asset asset{
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
    };
    std::pair<std::string, Asset> changedAsset{ "device_2", asset };
    changedAssets_[uri_].status = DistributedData::STATUS_NO_CHANGE;
    auto ret = machine_->DFAPostEvent(UPLOAD, changedAssets_[uri_], asset, changedAsset);
    ASSERT_EQ(ret, GeneralError::E_ERROR);
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
    Asset asset{
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
    };
    std::pair<std::string, Asset> changedAsset{ "device_1", asset };
    machine_->DFAPostEvent(UPLOAD, changedAssets_[uri_], asset, changedAsset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_UPLOADING);

    machine_->DFAPostEvent(UPLOAD_FINISHED, changedAssets_[uri_], asset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_STABLE);
    // dotransfer
    machine_->DFAPostEvent(REMOTE_CHANGED, changedAssets_[uri_], asset, changedAsset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_TRANSFERRING);
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
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto timestamp = std::to_string(time);
    Asset asset{
        .name = "name_" + timestamp,
        .uri = "uri_" + timestamp,
        .modifyTime = "modifyTime_" + timestamp,
        .size = "size_" + timestamp,
        .hash = "modifyTime_size_" + timestamp,
    };
    AssetBindInfo bindInfo{
        .storeName = "store_" + timestamp,
        .tableName = "table_" + timestamp,
        .primaryKey = {{ "key_" + timestamp, "value_" + timestamp }},
        .field = "attachment_" + timestamp,
        .assetName = "asset_" + timestamp + ".jpg",
    };
    StoreInfo storeInfo {
        .tokenId = time,
        .bundleName = "bundleName_" + timestamp,
        .storeName = "store_" + timestamp,
        .instanceId = time,
        .user = time,
    };
    ChangedAssetInfo changedAssetInfo(asset, bindInfo, storeInfo);
    std::pair<std::string, Asset> changedAsset{ "device_" + timestamp, asset };

    machine_->DFAPostEvent(UPLOAD, changedAssetInfo, asset);
    ASSERT_EQ(changedAssetInfo.status, STATUS_UPLOADING);

    machine_->DFAPostEvent(REMOTE_CHANGED, changedAssetInfo, asset, changedAsset);
    ASSERT_EQ(changedAssetInfo.status, STATUS_WAIT_TRANSFER);
    ASSERT_EQ(changedAssetInfo.asset.hash, asset.hash);

    machine_->DFAPostEvent(UPLOAD_FINISHED, changedAssetInfo, asset);
    ASSERT_EQ(changedAssetInfo.status, STATUS_TRANSFERRING);
}

/**
* @tc.name: StatusDownload001
* @tc.desc: No conflict scenarios: normal cloud sync.
* @tc.type: FUNC
* @tc.require:
* @tc.author: whj
*/
HWTEST_F(ObjectAssetMachineTest, StatusDownload001, TestSize.Level0)
{
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

    machine_->DFAPostEvent(DOWNLOAD, changedAssetInfo, asset, changedAsset);
    ASSERT_EQ(changedAssetInfo.status, STATUS_DOWNLOADING);

    machine_->DFAPostEvent(DOWNLOAD_FINISHED, changedAssetInfo, asset);
    ASSERT_EQ(changedAssetInfo.status, STATUS_STABLE);
}
} // namespace OHOS::Test