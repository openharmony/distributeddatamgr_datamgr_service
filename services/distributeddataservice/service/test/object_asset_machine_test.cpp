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

#include <gtest/gtest.h>
#include "log_print.h"
#include "snapshot/machine_status.h"
#include "object_asset_machine.h"

using namespace testing::ext;
using namespace OHOS::DistributedObject;
using namespace OHOS::DistributedData;
class ObjectAssetMachineTest  : public testing::Test {
public:
    void SetUp();
    void TearDown();

protected:
    RdbBindInfo rdbBindInfo_;
    Asset asset_;
    std::string uri_;
    std::string bundleName_ = "test_bundleName";
    std::map<std::string, ChangedAssetInfo> changedAssets_;
    std::string sessionId = "123";
    StoreInfo storeInfo_;
};

void ObjectAssetMachineTest::SetUp()
{
    uri_ =  "file:://com.huawei.hmos.notepad/data/storage/el2/distributedfiles/dir/asset1.jpg";
    Asset asset {
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime",
        .size = "size",
        .hash = "modifyTime_size",
        };
    asset_ = asset;

    VBucket vBucket {{"11", 111}};
    RdbBindInfo rdbBindInfo {
        .storeName = "store_test",
        .tableName = "table_test",
        .primaryKey = vBucket,
        .field = "attachment",
        .assetName = "asset1.jpg",
        };
    rdbBindInfo_ = rdbBindInfo;
    StoreInfo storeInfo {
        .tokenId = 0,
        .instanceId = 1,
        .user = "100",
        .bundleName = "bundleName_test",
        .storeName = "store_test",
        };
    storeInfo_ = storeInfo;
    ChangedAssetInfo changedAssetInfo(asset, rdbBindInfo, storeInfo);
    changedAssets_[uri_] = changedAssetInfo;
    std::cout<<"uri:"<<uri_<<"asset.name:"<<asset.name<<std::endl;
}

void ObjectAssetMachineTest::TearDown() {}

/**
* @tc.name: StatusTransfer
* @tc.desc: .
* @tc.type: FUNC
* @tc.require:
* @tc.author: whj
*/

HWTEST_F(ObjectAssetMachineTest, StatusTransfer, TestSize.Level0)
{
    auto machine = std::make_shared<ObjectAssetMachine>();
    Asset asset {
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
        };
    std::pair<std::string, Asset> changedAsset {"device_1", asset};
    machine->DFAPostEvent(REMOTE_CHANGED, changedAssets_[uri_].status, (void*)&changedAssets_[uri_], (void*)&changedAsset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_TRANSFERRING);

    changedAsset.first = "device_2";
    changedAsset.second.hash = "modifyTime2_size2";
    machine->DFAPostEvent(REMOTE_CHANGED, changedAssets_[uri_].status, (void*)&changedAssets_[uri_], (void*)&changedAsset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_WAIT_TRANSFER);
    ASSERT_EQ(changedAssets_[uri_].asset.hash, "modifyTime2_size2");
    ASSERT_EQ(changedAssets_[uri_].deviceId, "device_2");

    machine->DFAPostEvent(TRANSFER_FINISHED, changedAssets_[uri_].status,(void*)&changedAssets_[uri_], (void*)&asset);

    ASSERT_EQ(changedAssets_[uri_].status, STATUS_TRANSFERRING);

    machine->DFAPostEvent(TRANSFER_FINISHED, changedAssets_[uri_].status,(void*)&changedAssets_[uri_], (void*)&changedAssets_[uri_].asset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_STABLE);
}

/**
* @tc.name: StatusTransfer001
* @tc.desc: No conflict scenarios. Normal object Migration and cloud synchronization processes.
* @tc.type: FUNC
* @tc.require:
* @tc.author: whj
*/

HWTEST_F(ObjectAssetMachineTest, StatusTransfer001, TestSize.Level0)
{
    auto machine = std::make_shared<ObjectAssetMachine>();
    Asset asset {
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
        };
    std::pair<std::string, Asset> changedAsset {"device_1", asset};
    machine->DFAPostEvent(REMOTE_CHANGED, changedAssets_[uri_].status, (void*)&changedAssets_[uri_], (void*)&changedAsset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_TRANSFERRING);
    ASSERT_EQ(changedAssets_[uri_].asset.hash, asset.hash);
    ASSERT_EQ(changedAssets_[uri_].deviceId, "device_1");
    machine->DFAPostEvent(TRANSFER_FINISHED, changedAssets_[uri_].status,(void*)&changedAssets_[uri_], (void*)&changedAssets_[uri_].asset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_STABLE);
}

/**
* @tc.name: StatusTransfer002
* @tc.desc: No conflict scenarios: normal cloud sync.
* @tc.type: FUNC
* @tc.require:
* @tc.author: whj
*/

HWTEST_F(ObjectAssetMachineTest, StatusTransfer002, TestSize.Level0)
{
    auto machine = std::make_shared<ObjectAssetMachine>();
    Asset asset {
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime1",
        .size = "size1",
        .hash = "modifyTime1_size1",
        };
    std::pair<std::string, Asset> changedAsset {"device_1", asset};
    machine->DFAPostEvent(UPLOAD, changedAssets_[uri_].status, (void*)&changedAssets_[uri_].asset, nullptr);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_UPLOADING);

    machine->DFAPostEvent(UPLOAD_FINISHED, changedAssets_[uri_].status,(void*)&changedAssets_[uri_], (void*)&changedAssets_[uri_].asset);
    ASSERT_EQ(changedAssets_[uri_].status, STATUS_STABLE);
}
