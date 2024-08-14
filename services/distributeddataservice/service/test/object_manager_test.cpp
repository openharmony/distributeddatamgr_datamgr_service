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
using RestoreStatus = OHOS::DistributedObject::ObjectStoreManager::RestoreStatus;
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
* @tc.desc: DeleteNotifier test.
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
* @tc.desc: Process test.
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
* @tc.desc: DeleteNotifierNoLock test.
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
* @tc.name: Clear001
* @tc.desc: Clear test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, Clear001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    auto result = manager->Clear();
    ASSERT_EQ(result, OHOS::DistributedObject::OBJECT_STORE_NOT_FOUND);
}

/**
* @tc.name: registerAndUnregisterRemoteCallback001
* @tc.desc: test RegisterRemoteCallback and UnregisterRemoteCallback.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, registerAndUnregisterRemoteCallback001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    sptr<IRemoteObject> callback;
    manager->RegisterRemoteCallback(bundleName_, sessionId_, pid_, tokenId_, callback);
    ObjectStoreManager::CallbackInfo callbackInfo = manager->callbacks_.Find(tokenId_).second;
    std::string prefix = bundleName_ + sessionId_;
    ASSERT_NE(callbackInfo.observers_.find(prefix), callbackInfo.observers_.end());
    manager->UnregisterRemoteCallback(bundleName_, pid_, tokenId_, sessionId_);
    callbackInfo = manager->callbacks_.Find(tokenId_).second;
    ASSERT_EQ(callbackInfo.observers_.find(prefix), callbackInfo.observers_.end());
}

/**
* @tc.name: registerAndUnregisterRemoteCallback002
* @tc.desc: abnormal use cases.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, registerAndUnregisterRemoteCallback002, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    sptr<IRemoteObject> callback;
    uint32_t tokenId = 101;
    manager->RegisterRemoteCallback("", sessionId_, pid_, tokenId, callback);
    manager->RegisterRemoteCallback(bundleName_, "", pid_, tokenId, callback);
    manager->RegisterRemoteCallback("", "", pid_, tokenId, callback);
    ASSERT_EQ(manager->callbacks_.Find(tokenId).first, false);
    manager->UnregisterRemoteCallback("", pid_, tokenId, sessionId_);
}

/**
* @tc.name: NotifyDataChanged001
* @tc.desc: NotifyDataChanged test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, NotifyDataChanged001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::string bundleName1_ = "com.examples.ophm.notepad";
    std::string objectKey = bundleName1_ + sessionId_;
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> data;
    std::map<std::string, std::vector<uint8_t>> data1;
    std::vector<uint8_t> data1_;
    data1_.push_back(RestoreStatus::DATA_READY);
    data1_.push_back(RestoreStatus::ASSETS_READY);
    data1_.push_back(RestoreStatus::ALL_READY);
    data1 = {{ "objectKey", data1_ }};
    data = {{ objectKey, data1 }};
    std::shared_ptr<ExecutorPool> executors = std::make_shared<ExecutorPool>(5, 3); // executor pool
    manager->SetThreadPool(executors);
    ASSERT_EQ(manager->restoreStatus_.Find(objectKey).first, false);
    manager->NotifyDataChanged(data);
    ASSERT_EQ(manager->restoreStatus_.Find(objectKey).second, RestoreStatus::DATA_READY);
}

/**
* @tc.name: NotifyAssetsReady001
* @tc.desc: NotifyAssetsReady test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, NotifyAssetsReady001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::string objectKey = bundleName_ + sessionId_;
    std::string srcNetworkId = "1";
    ASSERT_EQ(manager->restoreStatus_.Find(objectKey).first, false);
    manager->NotifyAssetsReady(objectKey, srcNetworkId);
    ASSERT_EQ(manager->restoreStatus_.Find(objectKey).second, RestoreStatus::ASSETS_READY);
    manager->restoreStatus_.Clear();
    manager->restoreStatus_.Insert(objectKey, RestoreStatus::DATA_READY);
    manager->NotifyAssetsReady(objectKey, srcNetworkId);
    ASSERT_EQ(manager->restoreStatus_.Find(objectKey).second, RestoreStatus::ALL_READY);
}

/**
 * @tc.name: NotifyAssetsReady002
 * @tc.desc: NotifyAssetsReady test.
 * @tc.type: FUNC
 */
HWTEST_F(ObjectManagerTest, NotifyAssetsReady002, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::string objectKey="com.example.myapplicaiton123456";
    std::string srcNetworkId = "654321";

    manager->restoreStatus_.Clear();
    manager->NotifyAssetsStart(objectKey, srcNetworkId);
    auto [has0, value0] = manager->restoreStatus_.Find(objectKey);
    EXPECT_TRUE(has0);
    EXPECT_EQ(value0, RestoreStatus::NONE);

    manager->restoreStatus_.Clear();
    manager->NotifyAssetsReady(objectKey, srcNetworkId);
    auto [has1, value1] = manager->restoreStatus_.Find(objectKey);
    EXPECT_TRUE(has1);
    EXPECT_EQ(value1, RestoreStatus::ASSETS_READY);

    manager->restoreStatus_.Clear();
    manager->restoreStatus_.Insert(objectKey, RestoreStatus::DATA_NOTIFIED);
    manager->NotifyAssetsReady(objectKey, srcNetworkId);
    auto [has2, value2] = manager->restoreStatus_.Find(objectKey);
    EXPECT_TRUE(has2);
    EXPECT_EQ(value2, RestoreStatus::ALL_READY);

    manager->restoreStatus_.Clear();
    manager->restoreStatus_.Insert(objectKey, RestoreStatus::DATA_READY);
    manager->NotifyAssetsReady(objectKey, srcNetworkId);
    auto [has3, value3] = manager->restoreStatus_.Find(objectKey);
    EXPECT_TRUE(has3);
    EXPECT_EQ(value3, RestoreStatus::ALL_READY);
    manager->restoreStatus_.Clear();
}

/**
* @tc.name: NotifyChange001
* @tc.desc: NotifyChange test.
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
    data1_.push_back(RestoreStatus::DATA_READY);
    data_.push_back(RestoreStatus::ALL_READY);
    data = {{ "test_cloud", data_ }};
    data1 = {{ "p_###SAVEINFO###001", data1_ }};
    manager->NotifyChange(data1);
    manager->NotifyChange(data);
}

/**
 * @tc.name: NotifyChange002
 * @tc.desc: NotifyChange test.
 * @tc.type: FUNC
 */
HWTEST_F(ObjectManagerTest, NotifyChange002, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(1, 0);
    manager->SetThreadPool(executor);
    std::map<std::string, std::vector<uint8_t>> data{};
    std::vector<uint8_t> value{0};
    std::string bundleName = "com.example.myapplication";
    std::string sessionId = "123456";
    std::string source = "source";
    std::string target = "target";
    std::string timestamp = "1234567890";
    ObjectStoreManager::SaveInfo saveInfo(bundleName, sessionId, source, target, timestamp);
    std::string saveInfoStr = DistributedData::Serializable::Marshall(saveInfo);
    auto saveInfoValue = std::vector<uint8_t>(saveInfoStr.begin(), saveInfoStr.end());
    std::string prefix = saveInfo.ToPropertyPrefix();
    std::string assetPrefix = prefix + "p_asset";
    data.insert_or_assign(prefix + "p_###SAVEINFO###", saveInfoValue);
    data.insert_or_assign(prefix + "p_data", value);
    data.insert_or_assign(assetPrefix + ObjectStore::NAME_SUFFIX, value);
    data.insert_or_assign(assetPrefix + ObjectStore::URI_SUFFIX, value);
    data.insert_or_assign(assetPrefix + ObjectStore::PATH_SUFFIX, value);
    data.insert_or_assign(assetPrefix + ObjectStore::CREATE_TIME_SUFFIX, value);
    data.insert_or_assign(assetPrefix + ObjectStore::MODIFY_TIME_SUFFIX, value);
    data.insert_or_assign(assetPrefix + ObjectStore::SIZE_SUFFIX, value);
    data.insert_or_assign("testkey", value);
    manager->NotifyChange(data);
    EXPECT_TRUE(manager->restoreStatus_.Contains(bundleName+sessionId));
    auto [has, taskId] = manager->objectTimer_.Find(bundleName+sessionId);
    EXPECT_TRUE(has);
    manager->restoreStatus_.Clear();
    manager->executors_->Remove(taskId);
    manager->objectTimer_.Clear();
}

/**
 * @tc.name: ComputeStatus001
 * @tc.desc: ComputeStatus.test
 * @tc.type: FUNC
 */
HWTEST_F(ObjectManagerTest, ComputeStatus001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(1, 0);
    manager->SetThreadPool(executor);
    std::string objectKey="com.example.myapplicaiton123456";
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> data{};
    manager->restoreStatus_.Clear();
    manager->ComputeStatus(objectKey, data);
    auto [has0, value0] = manager->restoreStatus_.Find(objectKey);
    EXPECT_TRUE(has0);
    EXPECT_EQ(value0, RestoreStatus::DATA_READY);
    auto [has1, taskId1] = manager->objectTimer_.Find(objectKey);
    EXPECT_TRUE(has1);
    manager->executors_->Remove(taskId1);
    manager->objectTimer_.Clear();
    manager->restoreStatus_.Clear();

    manager->restoreStatus_.Insert(objectKey, RestoreStatus::ASSETS_READY);
    manager->ComputeStatus(objectKey, data);
    auto [has2, value2] = manager->restoreStatus_.Find(objectKey);
    EXPECT_TRUE(has2);
    EXPECT_EQ(value2, RestoreStatus::ALL_READY);
    auto [has3, taskId3] = manager->objectTimer_.Find(objectKey);
    EXPECT_FALSE(has3);
    manager->restoreStatus_.Clear();
}

/**
* @tc.name: Open001
* @tc.desc: Open test.
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
* @tc.desc: OnAssetChanged test.
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
* @tc.desc: DeleteSnapshot test.
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
* @tc.desc: OpenObjectKvStore test.
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
* @tc.desc: FlushClosedStore test.
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
* @tc.desc: Close test.
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
* @tc.desc: SyncOnStore test.
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
* @tc.desc: RetrieveFromStore test.
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
* @tc.desc: SyncCompleted test.
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
* @tc.desc: SplitEntryKey test.
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
* @tc.name: SplitEntryKey002
* @tc.desc: SplitEntryKey test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, SplitEntryKey002, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::string key1 = "com.example.myapplication_sessionId_source_target_1234567890_p_propertyName";
    auto res = manager->SplitEntryKey(key1);
    EXPECT_EQ(res[0], "com.example.myapplication");
    EXPECT_EQ(res[1], "sessionId");
    EXPECT_EQ(res[2], "source");
    EXPECT_EQ(res[3], "target");
    EXPECT_EQ(res[4], "1234567890");
    EXPECT_EQ(res[5], "p_propertyName");

    std::string key2 = "com.example.myapplication_sessionId_source_target_000_p_propertyName";
    res = manager->SplitEntryKey(key2);
    EXPECT_TRUE(res.empty());

    std::string key3 = "com.example.myapplicationsessionIdsourcetarget_1234567890_p_propertyName";
    res = manager->SplitEntryKey(key3);
    EXPECT_TRUE(res.empty());

    std::string key4 = "com.example.myapplicationsessionIdsource_target_1234567890_p_propertyName";
    res = manager->SplitEntryKey(key4);
    EXPECT_TRUE(res.empty());

    std::string key5 = "com.example.myapplicationsessionId_source_target_1234567890_p_propertyName";
    res = manager->SplitEntryKey(key5);
    EXPECT_TRUE(res.empty());
}

/**
* @tc.name: ProcessOldEntry001
* @tc.desc: ProcessOldEntry test.
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
* @tc.desc: ProcessSyncCallback test.
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
* @tc.desc: IsAssetComplete test.
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
* @tc.desc: GetAssetsFromDBRecords test.
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
* @tc.name: GetAssetsFromDBRecords002
* @tc.desc: GetAssetsFromDBRecords test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, GetAssetsFromDBRecords002, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> result;

    std::vector<uint8_t> value0{0};
    std::string data0 = "[STRING]test";
    value0.insert(value0.end(), data0.begin(), data0.end());

    std::vector<uint8_t> value1{0};
    std::string data1 = "(string)test";
    value1.insert(value1.end(), data1.begin(), data1.end());

    std::string prefix = "bundleName_sessionId_source_target_timestamp";
    std::string dataKey = prefix + "_p_data";
    std::string assetPrefix0 = prefix + "_p_asset0";
    std::string assetPrefix1 = prefix + "_p_asset1";

    result.insert({dataKey, value0});
    auto assets = manager->GetAssetsFromDBRecords(result);
    EXPECT_TRUE(assets.empty());
    
    result.clear();
    result.insert({assetPrefix0 + ObjectStore::URI_SUFFIX, value0});
    assets = manager->GetAssetsFromDBRecords(result);
    EXPECT_TRUE(assets.empty());

    result.clear();
    result.insert({assetPrefix1 + ObjectStore::NAME_SUFFIX, value1});
    assets = manager->GetAssetsFromDBRecords(result);
    EXPECT_TRUE(assets.empty());

    result.clear();
    result.insert({assetPrefix0 + ObjectStore::NAME_SUFFIX, value0});
    result.insert({assetPrefix0 + ObjectStore::URI_SUFFIX, value0});
    result.insert({assetPrefix0 + ObjectStore::MODIFY_TIME_SUFFIX, value0});
    result.insert({assetPrefix0 + ObjectStore::SIZE_SUFFIX, value0});
    assets = manager->GetAssetsFromDBRecords(result);
    ASSERT_EQ(assets.size(), 1);
    EXPECT_EQ(assets[0].name, "test");
    EXPECT_EQ(assets[0].uri, "test");
    EXPECT_EQ(assets[0].modifyTime, "test");
    EXPECT_EQ(assets[0].size, "test");
    EXPECT_EQ(assets[0].hash, "test_test");

    result.clear();
    result.insert({assetPrefix1 + ObjectStore::NAME_SUFFIX, value1});
    result.insert({assetPrefix1 + ObjectStore::URI_SUFFIX, value1});
    result.insert({assetPrefix1 + ObjectStore::MODIFY_TIME_SUFFIX, value1});
    result.insert({assetPrefix1 + ObjectStore::SIZE_SUFFIX, value1});
    assets = manager->GetAssetsFromDBRecords(result);
    ASSERT_EQ(assets.size(), 1);
    EXPECT_EQ(assets[0].name, "(string)test");
    EXPECT_EQ(assets[0].uri, "(string)test");
    EXPECT_EQ(assets[0].modifyTime, "(string)test");
    EXPECT_EQ(assets[0].size, "(string)test");
    EXPECT_EQ(assets[0].hash, "(string)test_(string)test");
}

/**
* @tc.name: RegisterAssetsLister001
* @tc.desc: RegisterAssetsLister test.
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
* @tc.desc: PushAssets test.
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

/**
* @tc.name: ConstructorAndDestructor001
* @tc.desc: Constructor and destructor test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, ConstructorAndDestructor001, TestSize.Level0)
{
    auto manager = new ObjectStoreManager();
    EXPECT_TRUE(manager->objectAssetsSendListener_ != nullptr);
    EXPECT_TRUE(manager->objectAssetsRecvListener_ != nullptr);
    delete manager;
}
} // namespace OHOS::Test