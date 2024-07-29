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

#define LOG_TAG "ObjectManagerTest"

#include "object_manager.h"
#include <gtest/gtest.h>
#include "snapshot/machine_status.h"
#include "executor_pool.h"
#include "object_types.h"
#include "kv_store_nb_delegate_mock.h"

using namespace testing::ext;
using namespace OHOS::DistributedObject;
using AssetValue = OHOS::CommonType::AssetValue;
namespace OHOS::Test {

class ObjectManagerTest : public testing::Test {
public:
    void SetUp();
    void TearDown();

protected:
    Asset asset_;
    std::string uri_;
    std::string appId_ = "objectManagerTest_appid_1";
    std::string sessionId_ = "123";
    std::vector<uint8_t> data_;
    std::string deviceId_ = "7001005458323933328a258f413b3900";
    uint64_t sequenceId_ = 10;
    uint64_t sequenceId_2 = 20;
    uint64_t sequenceId_3 = 30;
    std::string userId_ = "100";
    std::string bundleName_ = "com.examples.hmos.notepad";
    OHOS::ObjectStore::AssetBindInfo assetBindInfo_;
    pid_t pid_ = 10;
    uint32_t tokenId_ = 100;
    AssetValue assetValue_;
};

void ObjectManagerTest::SetUp()
{
    uri_ = "file:://com.examples.hmos.notepad/data/storage/el2/distributedfiles/dir/asset1.jpg";
    Asset asset{
        .name = "test_name",
        .uri = uri_,
        .modifyTime = "modifyTime",
        .size = "size",
        .hash = "modifyTime_size",
    };
    asset_ = asset;

    AssetValue assetValue{
        .id = "test_name",
        .name = uri_,
        .uri = uri_,
        .createTime = "2024.07.23",
        .modifyTime = "modifyTime",
        .size = "size",
        .hash = "modifyTime_size",
        .path = "/data/storage/el2",
    };
    assetValue_ = assetValue;

    data_.push_back(10); // 10 is for testing
    data_.push_back(20); // 20 is for testing
    data_.push_back(30); // 30 is for testing

    OHOS::ObjectStore::AssetBindInfo AssetBindInfo{
        .storeName = "store_test",
        .tableName = "table_test",
        .field = "attachment",
        .assetName = "asset1.jpg",
    };
    assetBindInfo_ = AssetBindInfo;
}

void ObjectManagerTest::TearDown() {}

/**
* @tc.name: DeleteNotifier001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, DeleteNotifier001, TestSize.Level0)
{
    auto syncManager = SequenceSyncManager::GetInstance();
    auto result = syncManager->DeleteNotifier(sequenceId_, userId_);
    ASSERT_EQ(result, SequenceSyncManager::ERR_SID_NOT_EXIST);
}

/**
* @tc.name: Process001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, Process001, TestSize.Level0)
{
    auto syncManager = SequenceSyncManager::GetInstance();
    std::map<std::string, DistributedDB::DBStatus> results;
    results = {{ "test_cloud", DistributedDB::DBStatus::OK }};

    std::function<void(const std::map<std::string, int32_t> &results)> func;
    func = [](const std::map<std::string, int32_t> &results) {
        return results;
    };
    auto result = syncManager->Process(sequenceId_, results, userId_);
    ASSERT_EQ(result, SequenceSyncManager::ERR_SID_NOT_EXIST);
    syncManager->seqIdCallbackRelations_.emplace(sequenceId_, func);
    result = syncManager->Process(sequenceId_, results, userId_);
    ASSERT_EQ(result, SequenceSyncManager::SUCCESS_USER_HAS_FINISHED);
}

/**
* @tc.name: DeleteNotifierNoLock001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, DeleteNotifierNoLock001, TestSize.Level0)
{
    auto syncManager = SequenceSyncManager::GetInstance();
    std::function<void(const std::map<std::string, int32_t> &results)> func;
    func = [](const std::map<std::string, int32_t> &results) {
        return results;
    };
    syncManager->seqIdCallbackRelations_.emplace(sequenceId_, func);
    std::vector<uint64_t> seqIds = {sequenceId_, sequenceId_2, sequenceId_3};
    std::string userId = "user_1";
    auto result = syncManager->DeleteNotifierNoLock(sequenceId_, userId_);
    ASSERT_EQ(result, SequenceSyncManager::SUCCESS_USER_HAS_FINISHED);
    syncManager->userIdSeqIdRelations_[userId] = seqIds;
    result = syncManager->DeleteNotifierNoLock(sequenceId_, userId_);
    ASSERT_EQ(result, SequenceSyncManager::SUCCESS_USER_IN_USE);
}

/**
* @tc.name: Save001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, Save001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    auto result = manager->Clear();
    ASSERT_EQ(result, OHOS::DistributedObject::OBJECT_STORE_NOT_FOUND);
}

/**
* @tc.name: UnregisterRemoteCallback001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, UnregisterRemoteCallback001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    manager->UnregisterRemoteCallback("", pid_, tokenId_, sessionId_);
    manager->UnregisterRemoteCallback(bundleName_, pid_, tokenId_, sessionId_);
}

/**
* @tc.name: NotifyChange001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, NotifyChange001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> data;
    std::map<std::string, std::vector<uint8_t>> data1;
    std::vector<uint8_t> data1_;
    data = {{ "test_cloud", data_ }};
    data1 = {{ "", data1_ }};
    manager->NotifyChange(data1);
    manager->NotifyChange(data);
}

/**
* @tc.name: NotifyAssetsReady001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, NotifyAssetsReady001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::string bundleName_1 = bundleName_ + "1";
    std::string bundleName_2 = bundleName_ + "2";
    std::string bundleName_3 = bundleName_ + "3";
    std::string srcNetworkId = "1";
    manager->restoreStatus_.Insert(bundleName_1, DistributedObject::ObjectStoreManager::RestoreStatus::ALL_READY);
    manager->restoreStatus_.Insert(bundleName_2, DistributedObject::ObjectStoreManager::RestoreStatus::DATA_READY);
    manager->restoreStatus_.Insert(bundleName_3, DistributedObject::ObjectStoreManager::RestoreStatus::ASSETS_READY);
    manager->NotifyAssetsReady(bundleName_1, srcNetworkId);
    manager->NotifyAssetsReady(bundleName_2, srcNetworkId);
    manager->NotifyAssetsReady(bundleName_3, srcNetworkId);
}

/**
* @tc.name: Open001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, Open001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    manager->kvStoreDelegateManager_ = nullptr;
    auto result = manager->Open();
    ASSERT_EQ(result, DistributedObject::OBJECT_INNER_ERROR);
    std::string dataDir = "/data/app/el2/100/database";
    manager->SetData(dataDir, userId_);
    manager->delegate_ = nullptr;
    result = manager->Open();
    ASSERT_EQ(result, DistributedObject::OBJECT_SUCCESS);
    manager->delegate_ = manager->OpenObjectKvStore();
    result = manager->Open();
    ASSERT_EQ(result, DistributedObject::OBJECT_SUCCESS);
}

/**
* @tc.name: OnAssetChanged001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, OnAssetChanged001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::shared_ptr<Snapshot> snapshot = std::make_shared<ObjectSnapshot>();
    auto snapshotKey = appId_ + "_" + sessionId_;
    auto result = manager->OnAssetChanged(tokenId_, appId_, sessionId_, deviceId_, assetValue_);
    ASSERT_EQ(result, DistributedObject::OBJECT_INNER_ERROR);
    manager->snapshots_.Insert(snapshotKey, snapshot);
    result = manager->OnAssetChanged(tokenId_, appId_, sessionId_, deviceId_, assetValue_);
    ASSERT_EQ(result, DistributedObject::OBJECT_SUCCESS);
}

/**
* @tc.name: DeleteSnapshot001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, DeleteSnapshot001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::shared_ptr<Snapshot> snapshot = std::make_shared<ObjectSnapshot>();
    auto snapshotKey = bundleName_ + "_" + sessionId_;
    auto snapshots = manager->snapshots_.Find(snapshotKey).second;
    ASSERT_EQ(snapshots, nullptr);
    manager->DeleteSnapshot(bundleName_, sessionId_);

    manager->snapshots_.Insert(snapshotKey, snapshot);
    snapshots = manager->snapshots_.Find(snapshotKey).second;
    ASSERT_NE(snapshots, nullptr);
    manager->DeleteSnapshot(bundleName_, sessionId_);
}

/**
* @tc.name: OpenObjectKvStore001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, OpenObjectKvStore001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    manager->objectDataListener_ = nullptr;
    ASSERT_EQ(manager->objectDataListener_, nullptr);
    manager->OpenObjectKvStore();
    ASSERT_NE(manager->objectDataListener_, nullptr);
    manager->OpenObjectKvStore();
}

/**
* @tc.name: FlushClosedStore001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, FlushClosedStore001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    manager->isSyncing_ = true;
    manager->syncCount_ = 10; // test syncCount_
    manager->delegate_ = nullptr;
    manager->FlushClosedStore();
    manager->isSyncing_ = false;
    manager->FlushClosedStore();
    manager->syncCount_ = 0; // test syncCount_
    manager->FlushClosedStore();
    manager->delegate_ = manager->OpenObjectKvStore();
    manager->FlushClosedStore();
}

/**
* @tc.name: Close001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, Close001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    manager->syncCount_ = 1; // test syncCount_
    manager->Close();
    ASSERT_EQ(manager->syncCount_, 1); // 1 is for testing
    manager->delegate_ = manager->OpenObjectKvStore();
    manager->Close();
    ASSERT_EQ(manager->syncCount_, 0); // 0 is for testing
}

/**
* @tc.name: SyncOnStore001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, SyncOnStore001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::function<void(const std::map<std::string, int32_t> &results)> func;
    func = [](const std::map<std::string, int32_t> &results) {
        return results;
    };
    std::string prefix = "ObjectManagerTest";
    std::vector<std::string> deviceList;
    deviceList.push_back("local");
    deviceList.push_back("local1");
    auto result = manager->SyncOnStore(prefix, deviceList, func);
    ASSERT_EQ(result, OBJECT_SUCCESS);
}

/**
* @tc.name: RevokeSaveToStore001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, RevokeSaveToStore001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    DistributedDB::KvStoreNbDelegateMock mockDelegate;
    manager->delegate_ = &mockDelegate;
    std::vector<uint8_t> id;
    id.push_back(1);  // for testing
    id.push_back(2);  // for testing
    std::map<std::string, std::vector<uint8_t>> results;
    results = {{ "test_cloud", id }};
    auto result = manager->RetrieveFromStore(appId_, sessionId_, results);
    ASSERT_EQ(result, OBJECT_SUCCESS);
}

/**
* @tc.name: SyncCompleted001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, SyncCompleted001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    auto syncManager = SequenceSyncManager::GetInstance();
    std::map<std::string, DistributedDB::DBStatus> results;
    results = {{ "test_cloud", DistributedDB::DBStatus::OK }};
    std::function<void(const std::map<std::string, int32_t> &results)> func;
    func = [](const std::map<std::string, int32_t> &results) {
        return results;
    };
    manager->userId_ = "99";
    std::vector<uint64_t> userId;
    userId.push_back(99);
    userId.push_back(100);
    manager->SyncCompleted(results, sequenceId_);
    syncManager->userIdSeqIdRelations_ = {{ "test_cloud", userId }};
    manager->SyncCompleted(results, sequenceId_);
    userId.clear();
    syncManager->seqIdCallbackRelations_.emplace(sequenceId_, func);
    manager->SyncCompleted(results, sequenceId_);
    userId.push_back(99);
    userId.push_back(100);
    manager->SyncCompleted(results, sequenceId_);
}

/**
* @tc.name: SplitEntryKey001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, SplitEntryKey001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::string key1 = "";
    std::string key2 = "ObjectManagerTest";
    auto result = manager->SplitEntryKey(key1);
    ASSERT_EQ(result.empty(), true);
    result = manager->SplitEntryKey(key2);
    ASSERT_EQ(result.empty(), true);
}

/**
* @tc.name: ProcessOldEntry001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, ProcessOldEntry001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    manager->delegate_ = manager->OpenObjectKvStore();
    std::vector<DistributedDB::Entry> entries;
    auto status = manager->delegate_->GetEntries(std::vector<uint8_t>(appId_.begin(), appId_.end()), entries);
    ASSERT_EQ(status, DistributedDB::DBStatus::NOT_FOUND);
    manager->ProcessOldEntry(appId_);

    DistributedDB::KvStoreNbDelegateMock mockDelegate;
    manager->delegate_ = &mockDelegate;
    status = manager->delegate_->GetEntries(std::vector<uint8_t>(appId_.begin(), appId_.end()), entries);
    ASSERT_EQ(status, DistributedDB::DBStatus::OK);
    manager->ProcessOldEntry(appId_);
}

/**
* @tc.name: ProcessSyncCallback001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, ProcessSyncCallback001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::map<std::string, int32_t> results;
    manager->ProcessSyncCallback(results, appId_, sessionId_, deviceId_);
    results.insert({"local", 1}); // for testing
    ASSERT_EQ(results.empty(), false);
    ASSERT_NE(results.find("local"), results.end());
    manager->ProcessSyncCallback(results, appId_, sessionId_, deviceId_);
}

/**
* @tc.name: IsAssetComplete001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, IsAssetComplete001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> results;
    std::vector<uint8_t> completes;
    completes.push_back(1); // for testing
    completes.push_back(2); // for testing
    std::string assetPrefix = "IsAssetComplete_test";
    results.insert({assetPrefix, completes});
    auto result = manager->IsAssetComplete(results, assetPrefix);
    ASSERT_EQ(result, false);
    results.insert({assetPrefix + ObjectStore::NAME_SUFFIX, completes});
    result = manager->IsAssetComplete(results, assetPrefix);
    ASSERT_EQ(result, false);
    results.insert({assetPrefix + ObjectStore::URI_SUFFIX, completes});
    result = manager->IsAssetComplete(results, assetPrefix);
    ASSERT_EQ(result, false);
    results.insert({assetPrefix + ObjectStore::PATH_SUFFIX, completes});
    result = manager->IsAssetComplete(results, assetPrefix);
    ASSERT_EQ(result, false);
    results.insert({assetPrefix + ObjectStore::CREATE_TIME_SUFFIX, completes});
    result = manager->IsAssetComplete(results, assetPrefix);
    ASSERT_EQ(result, false);
    results.insert({assetPrefix + ObjectStore::MODIFY_TIME_SUFFIX, completes});
    result = manager->IsAssetComplete(results, assetPrefix);
    ASSERT_EQ(result, false);
    results.insert({assetPrefix + ObjectStore::SIZE_SUFFIX, completes});
    result = manager->IsAssetComplete(results, assetPrefix);
    ASSERT_EQ(result, true);
}

/**
* @tc.name: GetAssetsFromDBRecords001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, GetAssetsFromDBRecords001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> results;
    std::vector<uint8_t> completes;
    completes.push_back(1); // for testing
    completes.push_back(2); // for testing
    std::string assetPrefix = "IsAssetComplete_test";
    results.insert({assetPrefix, completes});
    results.insert({assetPrefix + ObjectStore::NAME_SUFFIX, completes});
    results.insert({assetPrefix + ObjectStore::URI_SUFFIX, completes});
    results.insert({assetPrefix + ObjectStore::MODIFY_TIME_SUFFIX, completes});
    results.insert({assetPrefix + ObjectStore::SIZE_SUFFIX, completes});
    auto result = manager->GetAssetsFromDBRecords(results);
    ASSERT_EQ(result.empty(), false);
}

/**
* @tc.name: RegisterAssetsLister001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, RegisterAssetsLister001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    manager->objectAssetsSendListener_ = nullptr;
    manager->objectAssetsRecvListener_ = nullptr;
    auto result = manager->RegisterAssetsLister();
    ASSERT_EQ(result, true);
    manager->objectAssetsSendListener_ = new ObjectAssetsSendListener();
    manager->objectAssetsRecvListener_ = new ObjectAssetsRecvListener();;
    result = manager->RegisterAssetsLister();
    ASSERT_EQ(result, true);
}

/**
* @tc.name: RegisterAssetsLister001
* @tc.desc: Transfer event.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, PushAssets001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> data;
    std::string assetPrefix = "PushAssets_test";
    std::vector<uint8_t> completes;
    completes.push_back(1); // for testing
    completes.push_back(2); // for testing
    data.insert({assetPrefix, completes});
    auto result = manager->PushAssets(100, appId_, sessionId_, data, deviceId_);
    ASSERT_EQ(result, DistributedObject::OBJECT_SUCCESS);
}
} // namespace OHOS::TestF