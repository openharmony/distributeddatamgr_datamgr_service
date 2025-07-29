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
#include <ipc_skeleton.h>

#include "bootstrap.h"
#include "device_manager_adapter_mock.h"
#include "executor_pool.h"
#include "kv_store_nb_delegate_mock.h"
#include "kvstore_meta_manager.h"
#include "metadata/object_user_meta_data.h"
#include "object_types.h"
#include "snapshot/machine_status.h"

using namespace testing::ext;
using namespace OHOS::DistributedObject;
using namespace OHOS::DistributedData;
using namespace std;
using namespace testing;
using AssetValue = OHOS::CommonType::AssetValue;
using RestoreStatus = OHOS::DistributedObject::ObjectStoreManager::RestoreStatus;
namespace OHOS::Test {
class IObjectSaveCallback {
public:
    virtual void Completed(const std::map<std::string, int32_t> &results) = 0;
};
class IObjectRevokeSaveCallback {
public:
    virtual void Completed(int32_t status) = 0;
};
class IObjectRetrieveCallback {
public:
    virtual void Completed(const std::map<std::string, std::vector<uint8_t>> &results, bool allReady) = 0;
};
class IObjectChangeCallback {
public:
    virtual void Completed(const std::map<std::string, std::vector<uint8_t>> &results, bool allReady) = 0;
};

class IObjectProgressCallback {
public:
    virtual void Completed(int32_t progress) = 0;
};

class ObjectSaveCallbackBroker : public IObjectSaveCallback, public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedObject.IObjectSaveCallback");
};
class ObjectSaveCallbackStub : public IRemoteStub<ObjectSaveCallbackBroker> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        return 0;
    }
};
class ObjectRevokeSaveCallbackBroker : public IObjectRevokeSaveCallback, public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedObject.IObjectRevokeSaveCallback");
};
class ObjectRevokeSaveCallbackStub : public IRemoteStub<ObjectRevokeSaveCallbackBroker> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        return 0;
    }
};
class ObjectRetrieveCallbackBroker : public IObjectRetrieveCallback, public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedObject.IObjectRetrieveCallback");
};
class ObjectRetrieveCallbackStub : public IRemoteStub<ObjectRetrieveCallbackBroker> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        return 0;
    }
};

class ObjectChangeCallbackBroker : public IObjectChangeCallback, public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedObject.IObjectChangeCallback");
};

class ObjectChangeCallbackStub : public IRemoteStub<ObjectChangeCallbackBroker> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        return 0;
    }
};

class ObjectProgressCallbackBroker : public IObjectProgressCallback, public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedObject.IObjectProgressCallback");
};

class ObjectProgressCallbackStub : public IRemoteStub<ObjectProgressCallbackBroker> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override
    {
        return 0;
    }
};

class ObjectSaveCallback : public ObjectSaveCallbackStub {
public:
    explicit ObjectSaveCallback(const std::function<void(const std::map<std::string, int32_t> &)> &callback)
        : callback_(callback)
    {
    }
    void Completed(const std::map<std::string, int32_t> &results) override
    {
    }

private:
    const std::function<void(const std::map<std::string, int32_t> &)> callback_;
};
class ObjectRevokeSaveCallback : public ObjectRevokeSaveCallbackStub {
public:
    explicit ObjectRevokeSaveCallback(const std::function<void(int32_t)> &callback) : callback_(callback)
    {
    }
    void Completed(int32_t) override
    {
    }

private:
    const std::function<void(int32_t status)> callback_;
};
class ObjectRetrieveCallback : public ObjectRetrieveCallbackStub {
public:
    explicit ObjectRetrieveCallback(
        const std::function<void(const std::map<std::string, std::vector<uint8_t>> &, bool)> &callback)
        : callback_(callback)
    {
    }
    void Completed(const std::map<std::string, std::vector<uint8_t>> &results, bool allReady) override
    {
    }

private:
    const std::function<void(const std::map<std::string, std::vector<uint8_t>> &, bool)> callback_;
};

class ObjectChangeCallback : public ObjectChangeCallbackStub {
public:
    explicit ObjectChangeCallback(
        const std::function<void(const std::map<std::string, std::vector<uint8_t>> &, bool)> &callback)
        : callback_(callback)
    {
    }
    void Completed(const std::map<std::string, std::vector<uint8_t>> &results, bool allReady) override
    {
    }

private:
    const std::function<void(const std::map<std::string, std::vector<uint8_t>> &, bool)> callback_;
};

class ObjectProgressCallback : public ObjectProgressCallbackStub {
public:
    explicit ObjectProgressCallback(const std::function<void(int32_t)> &callback) : callback_(callback)
    {
    }
    void Completed(int32_t progress) override
    {
    }

private:
    const std::function<void(int32_t)> callback_;
};

class ObjectManagerTest : public testing::Test {
public:
    void SetUp();
    void TearDown();
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

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
    static inline std::shared_ptr<DeviceManagerAdapterMock> devMgrAdapterMock = nullptr;
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

void ObjectManagerTest::SetUpTestCase(void)
{
    devMgrAdapterMock = make_shared<DeviceManagerAdapterMock>();
    BDeviceManagerAdapter::deviceManagerAdapter = devMgrAdapterMock;
    std::shared_ptr<ExecutorPool> executors = std::make_shared<ExecutorPool>(1, 0);
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCheckers();
    DistributedKv::KvStoreMetaManager::GetInstance().BindExecutor(executors);
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaParameter();
    DistributedKv::KvStoreMetaManager::GetInstance().InitMetaListener();
}

void ObjectManagerTest::TearDownTestCase(void)
{
    BDeviceManagerAdapter::deviceManagerAdapter = nullptr;
    devMgrAdapterMock = nullptr;
}

void ObjectManagerTest::TearDown()
{
}

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
    results = { { "test_cloud", DistributedDB::DBStatus::OK } };

    std::function<void(const std::map<std::string, int32_t> &results)> func;
    func = [](const std::map<std::string, int32_t> &results) { return results; };
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
    func = [](const std::map<std::string, int32_t> &results) { return results; };
    syncManager->seqIdCallbackRelations_.emplace(sequenceId_, func);
    std::vector<uint64_t> seqIds = { sequenceId_, sequenceId_2, sequenceId_3 };
    std::string userId = "user_1";
    auto result = syncManager->DeleteNotifierNoLock(sequenceId_, userId_);
    ASSERT_EQ(result, SequenceSyncManager::SUCCESS_USER_HAS_FINISHED);
    syncManager->userIdSeqIdRelations_[userId] = seqIds;
    result = syncManager->DeleteNotifierNoLock(sequenceId_, userId_);
    ASSERT_EQ(result, SequenceSyncManager::SUCCESS_USER_IN_USE);
}

/**
* @tc.name: SaveToStoreTest
* @tc.desc: SaveToStore test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, SaveToStoreTest, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string dataDir = "/data/app/el2/100/database";
    manager.SetData(dataDir, userId_);
    auto result = manager.Open();
    ASSERT_EQ(result, DistributedObject::OBJECT_SUCCESS);
    ASSERT_NE(manager.delegate_, nullptr);
    ObjectRecord data{};
    result = manager.SaveToStore("appId", "sessionId", "toDeviceId", data);
    ASSERT_EQ(result, DistributedObject::OBJECT_SUCCESS);

    manager.ForceClose();
    ASSERT_EQ(manager.delegate_, nullptr);
    result = manager.SaveToStore("appId", "sessionId", "toDeviceId", data);
    ASSERT_NE(result, DistributedObject::OBJECT_SUCCESS);
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
    ObjectUserMetaData userMeta;
    MetaDataManager::GetInstance().SaveMeta(ObjectUserMetaData::GetKey(), userMeta, true);
    auto &manager = ObjectStoreManager::GetInstance();
    std::string dataDir = "/data/app/el2/100/database";
    manager.SetData(dataDir, userId_);
    auto result = manager.Clear();
    ASSERT_EQ(result, OHOS::DistributedObject::OBJECT_SUCCESS);
    auto size = manager.callbacks_.Size();
    ASSERT_EQ(size, 0);
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
    auto &manager = ObjectStoreManager::GetInstance();
    std::function<void(const std::map<std::string, std::vector<uint8_t>> &, bool)> cb =
        [](const std::map<std::string, std::vector<uint8_t>> &, bool) {};
    sptr<ObjectChangeCallbackBroker> objectRemoteResumeCallback = new (std::nothrow) ObjectChangeCallback(cb);
    ASSERT_NE(objectRemoteResumeCallback, nullptr);
    manager.RegisterRemoteCallback(bundleName_, sessionId_, pid_, tokenId_, objectRemoteResumeCallback->AsObject());
    ObjectStoreManager::CallbackInfo callbackInfo = manager.callbacks_.Find(tokenId_).second;
    std::string prefix = bundleName_ + sessionId_;
    ASSERT_NE(callbackInfo.observers_.find(prefix), callbackInfo.observers_.end());
    manager.UnregisterRemoteCallback(bundleName_, pid_, tokenId_, sessionId_);
    callbackInfo = manager.callbacks_.Find(tokenId_).second;
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
    auto &manager = ObjectStoreManager::GetInstance();
    sptr<IRemoteObject> callback;
    uint32_t tokenId = 101;
    manager.RegisterRemoteCallback("", sessionId_, pid_, tokenId, callback);
    manager.RegisterRemoteCallback(bundleName_, "", pid_, tokenId, callback);
    manager.RegisterRemoteCallback("", "", pid_, tokenId, callback);
    manager.RegisterRemoteCallback(bundleName_, sessionId_, pid_, tokenId, nullptr);
    ASSERT_EQ(manager.callbacks_.Find(tokenId).first, false);
    manager.UnregisterRemoteCallback("", pid_, tokenId, sessionId_);
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
    auto &manager = ObjectStoreManager::GetInstance();
    std::string bundleName1_ = "com.examples.ophm.notepad";
    std::string objectKey = bundleName1_ + sessionId_;
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> data;
    std::map<std::string, std::vector<uint8_t>> data1;
    std::vector<uint8_t> data1_;
    data1_.push_back(RestoreStatus::DATA_READY);
    data1_.push_back(RestoreStatus::ASSETS_READY);
    data1_.push_back(RestoreStatus::ALL_READY);
    data1 = { { "objectKey", data1_ } };
    data = { { objectKey, data1 } };
    std::shared_ptr<ExecutorPool> executors = std::make_shared<ExecutorPool>(5, 3); // executor pool
    manager.SetThreadPool(executors);
    ASSERT_EQ(manager.restoreStatus_.Find(objectKey).first, false);
    manager.NotifyDataChanged(data, {});
    ASSERT_EQ(manager.restoreStatus_.Find(objectKey).second, RestoreStatus::DATA_READY);
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
    auto &manager = ObjectStoreManager::GetInstance();
    std::string objectKey = bundleName_ + sessionId_;
    std::string srcNetworkId = "1";
    ASSERT_EQ(manager.restoreStatus_.Find(objectKey).first, false);
    manager.NotifyAssetsReady(objectKey, srcNetworkId);
    ASSERT_EQ(manager.restoreStatus_.Find(objectKey).second, RestoreStatus::ASSETS_READY);
    manager.restoreStatus_.Clear();
    manager.restoreStatus_.Insert(objectKey, RestoreStatus::DATA_READY);
    manager.NotifyAssetsReady(objectKey, srcNetworkId);
    ASSERT_EQ(manager.restoreStatus_.Find(objectKey).second, RestoreStatus::ALL_READY);
}

/**
 * @tc.name: NotifyAssetsReady002
 * @tc.desc: NotifyAssetsReady test.
 * @tc.type: FUNC
 */
HWTEST_F(ObjectManagerTest, NotifyAssetsReady002, TestSize.Level0)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string objectKey = "com.example.myapplicaiton123456";
    std::string srcNetworkId = "654321";

    manager.restoreStatus_.Clear();
    manager.NotifyAssetsStart(objectKey, srcNetworkId);
    auto [has0, value0] = manager.restoreStatus_.Find(objectKey);
    EXPECT_TRUE(has0);
    EXPECT_EQ(value0, RestoreStatus::NONE);

    manager.restoreStatus_.Clear();
    manager.NotifyAssetsReady(objectKey, srcNetworkId);
    auto [has1, value1] = manager.restoreStatus_.Find(objectKey);
    EXPECT_TRUE(has1);
    EXPECT_EQ(value1, RestoreStatus::ASSETS_READY);

    manager.restoreStatus_.Clear();
    manager.restoreStatus_.Insert(objectKey, RestoreStatus::DATA_NOTIFIED);
    manager.NotifyAssetsReady(objectKey, srcNetworkId);
    auto [has2, value2] = manager.restoreStatus_.Find(objectKey);
    EXPECT_TRUE(has2);
    EXPECT_EQ(value2, RestoreStatus::ALL_READY);

    manager.restoreStatus_.Clear();
    manager.restoreStatus_.Insert(objectKey, RestoreStatus::DATA_READY);
    manager.NotifyAssetsReady(objectKey, srcNetworkId);
    auto [has3, value3] = manager.restoreStatus_.Find(objectKey);
    EXPECT_TRUE(has3);
    EXPECT_EQ(value3, RestoreStatus::ALL_READY);
    manager.restoreStatus_.Clear();
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
    auto &manager = ObjectStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> data;
    std::map<std::string, std::vector<uint8_t>> data1;
    std::vector<uint8_t> data1_;
    data1_.push_back(RestoreStatus::DATA_READY);
    data_.push_back(RestoreStatus::ALL_READY);
    data = { { "test_cloud", data_ } };
    data1 = { { "p_###SAVEINFO###001", data1_ } };
    manager.NotifyChange(data1);
    EXPECT_FALSE(manager.restoreStatus_.Find("p_###SAVEINFO###001").first);
    manager.NotifyChange(data);
    EXPECT_FALSE(manager.restoreStatus_.Find("test_cloud").first);
}

/**
 * @tc.name: NotifyChange002
 * @tc.desc: NotifyChange test.
 * @tc.type: FUNC
 */
HWTEST_F(ObjectManagerTest, NotifyChange002, TestSize.Level0)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(1, 0);
    manager.SetThreadPool(executor);
    std::map<std::string, std::vector<uint8_t>> data{};
    std::vector<uint8_t> value{ 0 };
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
    EXPECT_CALL(*devMgrAdapterMock, IsSameAccount(_)).WillOnce(Return(true));
    manager.NotifyChange(data);
    EXPECT_TRUE(manager.restoreStatus_.Contains(bundleName + sessionId));
    auto [has, taskId] = manager.objectTimer_.Find(bundleName + sessionId);
    EXPECT_TRUE(has);
    manager.restoreStatus_.Clear();
    manager.executors_->Remove(taskId);
    manager.objectTimer_.Clear();
}

/**
 * @tc.name: ComputeStatus001
 * @tc.desc: ComputeStatus.test
 * @tc.type: FUNC
 */
HWTEST_F(ObjectManagerTest, ComputeStatus001, TestSize.Level0)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::shared_ptr<ExecutorPool> executor = std::make_shared<ExecutorPool>(1, 0);
    manager.SetThreadPool(executor);
    std::string objectKey = "com.example.myapplicaiton123456";
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> data{};
    manager.restoreStatus_.Clear();
    manager.ComputeStatus(objectKey, {}, data);
    auto [has0, value0] = manager.restoreStatus_.Find(objectKey);
    EXPECT_TRUE(has0);
    EXPECT_EQ(value0, RestoreStatus::DATA_READY);
    auto [has1, taskId1] = manager.objectTimer_.Find(objectKey);
    EXPECT_TRUE(has1);
    manager.executors_->Remove(taskId1);
    manager.objectTimer_.Clear();
    manager.restoreStatus_.Clear();

    manager.restoreStatus_.Insert(objectKey, RestoreStatus::ASSETS_READY);
    manager.ComputeStatus(objectKey, {}, data);
    auto [has2, value2] = manager.restoreStatus_.Find(objectKey);
    EXPECT_TRUE(has2);
    EXPECT_EQ(value2, RestoreStatus::ALL_READY);
    auto [has3, taskId3] = manager.objectTimer_.Find(objectKey);
    EXPECT_FALSE(has3);
    manager.restoreStatus_.Clear();
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
    auto &manager = ObjectStoreManager::GetInstance();
    manager.kvStoreDelegateManager_ = nullptr;
    auto result = manager.Open();
    ASSERT_EQ(result, DistributedObject::OBJECT_INNER_ERROR);
    std::string dataDir = "/data/app/el2/100/database";
    manager.SetData(dataDir, userId_);
    manager.delegate_ = nullptr;
    result = manager.Open();
    ASSERT_EQ(result, DistributedObject::OBJECT_SUCCESS);
    manager.delegate_ = manager.OpenObjectKvStore();
    result = manager.Open();
    ASSERT_EQ(result, DistributedObject::OBJECT_SUCCESS);
    manager.ForceClose();
    ASSERT_EQ(manager.delegate_, nullptr);
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
    auto &manager = ObjectStoreManager::GetInstance();
    std::shared_ptr<Snapshot> snapshot = std::make_shared<ObjectSnapshot>();
    auto snapshotKey = appId_ + "_" + sessionId_;
    auto result = manager.OnAssetChanged(tokenId_, appId_, sessionId_, deviceId_, assetValue_);
    ASSERT_EQ(result, DistributedObject::OBJECT_INNER_ERROR);
    manager.snapshots_.Insert(snapshotKey, snapshot);
    result = manager.OnAssetChanged(tokenId_, appId_, sessionId_, deviceId_, assetValue_);
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
    auto &manager = ObjectStoreManager::GetInstance();
    std::shared_ptr<Snapshot> snapshot = std::make_shared<ObjectSnapshot>();
    auto snapshotKey = bundleName_ + "_" + sessionId_;
    auto snapshots = manager.snapshots_.Find(snapshotKey).second;
    ASSERT_EQ(snapshots, nullptr);
    manager.DeleteSnapshot(bundleName_, sessionId_);

    manager.snapshots_.Insert(snapshotKey, snapshot);
    snapshots = manager.snapshots_.Find(snapshotKey).second;
    ASSERT_NE(snapshots, nullptr);
    manager.DeleteSnapshot(bundleName_, sessionId_);
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
    auto &manager = ObjectStoreManager::GetInstance();
    manager.isSyncing_ = true;
    manager.syncCount_ = 10; // test syncCount_
    manager.delegate_ = nullptr;
    manager.FlushClosedStore();
    manager.isSyncing_ = false;
    manager.FlushClosedStore();
    manager.syncCount_ = 0; // test syncCount_
    manager.FlushClosedStore();
    manager.delegate_ = manager.OpenObjectKvStore();
    ASSERT_NE(manager.delegate_, nullptr);
    manager.FlushClosedStore();
    ASSERT_EQ(manager.delegate_, nullptr);
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
    auto &manager = ObjectStoreManager::GetInstance();
    manager.syncCount_ = 1; // test syncCount_
    manager.Close();
    ASSERT_EQ(manager.syncCount_, 1); // 1 is for testing
    manager.delegate_ = manager.OpenObjectKvStore();
    manager.Close();
    ASSERT_EQ(manager.syncCount_, 0); // 0 is for testing
}

/**
* @tc.name: RetrieveFromStore001
* @tc.desc: RetrieveFromStore test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, RetrieveFromStore001, TestSize.Level0)
{
    auto &manager = ObjectStoreManager::GetInstance();
    DistributedDB::KvStoreNbDelegateMock mockDelegate;
    manager.delegate_ = &mockDelegate;
    std::vector<uint8_t> id;
    id.push_back(1); // for testing
    id.push_back(2); // for testing
    std::map<std::string, std::vector<uint8_t>> results;
    results = { { "test_cloud", id } };
    auto result = manager.RetrieveFromStore(appId_, sessionId_, results);
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
    auto &manager = ObjectStoreManager::GetInstance();
    auto syncManager = SequenceSyncManager::GetInstance();
    std::map<std::string, DistributedDB::DBStatus> results;
    results = { { "test_cloud", DistributedDB::DBStatus::OK } };
    std::function<void(const std::map<std::string, int32_t> &results)> func;
    func = [](const std::map<std::string, int32_t> &results) { return results; };
    manager.userId_ = "99";
    std::vector<uint64_t> userId;
    userId.push_back(99);
    userId.push_back(100);
    manager.SyncCompleted(results, sequenceId_);
    syncManager->userIdSeqIdRelations_ = { { "test_cloud", userId } };
    manager.SyncCompleted(results, sequenceId_);
    userId.clear();
    syncManager->seqIdCallbackRelations_.emplace(sequenceId_, func);
    manager.SyncCompleted(results, sequenceId_);
    userId.push_back(99);
    userId.push_back(100);
    manager.SyncCompleted(results, sequenceId_);
    EXPECT_FALSE(manager.isSyncing_);
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
    auto &manager = ObjectStoreManager::GetInstance();
    std::string key1 = "";
    std::string key2 = "ObjectManagerTest";
    auto result = manager.SplitEntryKey(key1);
    ASSERT_EQ(result.empty(), true);
    result = manager.SplitEntryKey(key2);
    ASSERT_EQ(result.empty(), true);
}

/**
* @tc.name: SplitEntryKey002
* @tc.desc: SplitEntryKey test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, SplitEntryKey002, TestSize.Level0)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string key1 = "com.example.myapplication_sessionId_source_target_1234567890_p_propertyName";
    auto res = manager.SplitEntryKey(key1);
    EXPECT_EQ(res[0], "com.example.myapplication");
    EXPECT_EQ(res[1], "sessionId");
    EXPECT_EQ(res[2], "source");
    EXPECT_EQ(res[3], "target");
    EXPECT_EQ(res[4], "1234567890");
    EXPECT_EQ(res[5], "p_propertyName");

    std::string key2 = "com.example.myapplication_sessionId_source_target_000_p_propertyName";
    res = manager.SplitEntryKey(key2);
    EXPECT_TRUE(res.empty());

    std::string key3 = "com.example.myapplicationsessionIdsourcetarget_1234567890_p_propertyName";
    res = manager.SplitEntryKey(key3);
    EXPECT_TRUE(res.empty());

    std::string key4 = "com.example.myapplicationsessionIdsource_target_1234567890_p_propertyName";
    res = manager.SplitEntryKey(key4);
    EXPECT_TRUE(res.empty());

    std::string key5 = "com.example.myapplicationsessionId_source_target_1234567890_p_propertyName";
    res = manager.SplitEntryKey(key5);
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
    auto &manager = ObjectStoreManager::GetInstance();
    manager.delegate_ = manager.OpenObjectKvStore();
    std::vector<DistributedDB::Entry> entries;
    auto status = manager.delegate_->GetEntries(std::vector<uint8_t>(appId_.begin(), appId_.end()), entries);
    ASSERT_EQ(status, DistributedDB::DBStatus::NOT_FOUND);
    manager.ProcessOldEntry(appId_);

    DistributedDB::KvStoreNbDelegateMock mockDelegate;
    manager.delegate_ = &mockDelegate;
    status = manager.delegate_->GetEntries(std::vector<uint8_t>(appId_.begin(), appId_.end()), entries);
    ASSERT_EQ(status, DistributedDB::DBStatus::OK);
    manager.ProcessOldEntry(appId_);
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
    auto &manager = ObjectStoreManager::GetInstance();
    std::map<std::string, int32_t> results;
    manager.ProcessSyncCallback(results, appId_, sessionId_, deviceId_);
    results.insert({ "local", 1 }); // for testing
    ASSERT_EQ(results.empty(), false);
    ASSERT_NE(results.find("local"), results.end());
    manager.ProcessSyncCallback(results, appId_, sessionId_, deviceId_);
}

/**
* @tc.name: ProcessSyncCallback002
* @tc.desc: ProcessSyncCallback test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, ProcessSyncCallback002, TestSize.Level0)
{
    std::string dataDir = "/data/app/el2/100/database";
    auto &manager = ObjectStoreManager::GetInstance();
    std::map<std::string, int32_t> results;

    results.insert({ "remote", 1 }); // for testing
    ASSERT_EQ(results.empty(), false);
    ASSERT_EQ(results.find("local"), results.end());

    manager.kvStoreDelegateManager_ = nullptr;
    // open store failed -> success
    manager.ProcessSyncCallback(results, appId_, sessionId_, deviceId_);

    // open store success -> success
    manager.SetData(dataDir, userId_);
    ASSERT_NE(manager.kvStoreDelegateManager_, nullptr);
    manager.delegate_ = manager.OpenObjectKvStore();
    ASSERT_NE(manager.delegate_, nullptr);
    manager.ProcessSyncCallback(results, appId_, sessionId_, deviceId_);
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
    auto &manager = ObjectStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> results;
    std::vector<uint8_t> completes;
    completes.push_back(1); // for testing
    completes.push_back(2); // for testing
    std::string assetPrefix = "IsAssetComplete_test";
    results.insert({ assetPrefix, completes });
    auto result = manager.IsAssetComplete(results, assetPrefix);
    ASSERT_EQ(result, false);
    results.insert({ assetPrefix + ObjectStore::NAME_SUFFIX, completes });
    result = manager.IsAssetComplete(results, assetPrefix);
    ASSERT_EQ(result, false);
    results.insert({ assetPrefix + ObjectStore::URI_SUFFIX, completes });
    result = manager.IsAssetComplete(results, assetPrefix);
    ASSERT_EQ(result, false);
    results.insert({ assetPrefix + ObjectStore::PATH_SUFFIX, completes });
    result = manager.IsAssetComplete(results, assetPrefix);
    ASSERT_EQ(result, false);
    results.insert({ assetPrefix + ObjectStore::CREATE_TIME_SUFFIX, completes });
    result = manager.IsAssetComplete(results, assetPrefix);
    ASSERT_EQ(result, false);
    results.insert({ assetPrefix + ObjectStore::MODIFY_TIME_SUFFIX, completes });
    result = manager.IsAssetComplete(results, assetPrefix);
    ASSERT_EQ(result, false);
    results.insert({ assetPrefix + ObjectStore::SIZE_SUFFIX, completes });
    result = manager.IsAssetComplete(results, assetPrefix);
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
    auto &manager = ObjectStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> results;
    std::vector<uint8_t> completes;
    completes.push_back(1); // for testing
    completes.push_back(2); // for testing
    std::string assetPrefix = "IsAssetComplete_test";
    results.insert({ assetPrefix, completes });
    results.insert({ assetPrefix + ObjectStore::NAME_SUFFIX, completes });
    results.insert({ assetPrefix + ObjectStore::URI_SUFFIX, completes });
    results.insert({ assetPrefix + ObjectStore::MODIFY_TIME_SUFFIX, completes });
    results.insert({ assetPrefix + ObjectStore::SIZE_SUFFIX, completes });
    auto result = manager.GetAssetsFromDBRecords(results);
    ASSERT_EQ(result.empty(), false);
}

/**
* @tc.name: GetAssetsFromDBRecords002
* @tc.desc: GetAssetsFromDBRecords test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, GetAssetsFromDBRecords002, TestSize.Level0)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> result;

    std::vector<uint8_t> value0{ 0 };
    std::string data0 = "[STRING]test";
    value0.insert(value0.end(), data0.begin(), data0.end());

    std::vector<uint8_t> value1{ 0 };
    std::string data1 = "(string)test";
    value1.insert(value1.end(), data1.begin(), data1.end());

    std::string prefix = "bundleName_sessionId_source_target_timestamp";
    std::string dataKey = prefix + "_p_data";
    std::string assetPrefix0 = prefix + "_p_asset0";
    std::string assetPrefix1 = prefix + "_p_asset1";

    result.insert({ dataKey, value0 });
    auto assets = manager.GetAssetsFromDBRecords(result);
    EXPECT_TRUE(assets.empty());

    result.clear();
    result.insert({ assetPrefix0 + ObjectStore::URI_SUFFIX, value0 });
    assets = manager.GetAssetsFromDBRecords(result);
    EXPECT_TRUE(assets.empty());

    result.clear();
    result.insert({ assetPrefix1 + ObjectStore::NAME_SUFFIX, value1 });
    assets = manager.GetAssetsFromDBRecords(result);
    EXPECT_TRUE(assets.empty());

    result.clear();
    result.insert({ assetPrefix0 + ObjectStore::NAME_SUFFIX, value0 });
    result.insert({ assetPrefix0 + ObjectStore::URI_SUFFIX, value0 });
    result.insert({ assetPrefix0 + ObjectStore::MODIFY_TIME_SUFFIX, value0 });
    result.insert({ assetPrefix0 + ObjectStore::SIZE_SUFFIX, value0 });
    assets = manager.GetAssetsFromDBRecords(result);
    ASSERT_EQ(assets.size(), 1);
    EXPECT_EQ(assets[0].name, "test");
    EXPECT_EQ(assets[0].uri, "test");
    EXPECT_EQ(assets[0].modifyTime, "test");
    EXPECT_EQ(assets[0].size, "test");
    EXPECT_EQ(assets[0].hash, "test_test");

    result.clear();
    result.insert({ assetPrefix1 + ObjectStore::NAME_SUFFIX, value1 });
    result.insert({ assetPrefix1 + ObjectStore::URI_SUFFIX, value1 });
    result.insert({ assetPrefix1 + ObjectStore::MODIFY_TIME_SUFFIX, value1 });
    result.insert({ assetPrefix1 + ObjectStore::SIZE_SUFFIX, value1 });
    assets = manager.GetAssetsFromDBRecords(result);
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
    auto &manager = ObjectStoreManager::GetInstance();
    manager.objectAssetsSendListener_ = nullptr;
    manager.objectAssetsRecvListener_ = nullptr;
    auto result = manager.RegisterAssetsLister();
    ASSERT_EQ(result, true);
    manager.objectAssetsSendListener_ = new ObjectAssetsSendListener();
    manager.objectAssetsRecvListener_ = new ObjectAssetsRecvListener();
    result = manager.RegisterAssetsLister();
    ASSERT_EQ(result, true);
}

/**
* @tc.name: PushAssets001
* @tc.desc: PushAssets test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(ObjectManagerTest, PushAssets001, TestSize.Level0)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> data;
    std::string assetPrefix = "PushAssets_test";
    std::vector<uint8_t> completes;
    completes.push_back(1); // for testing
    completes.push_back(2); // for testing
    data.insert({ assetPrefix, completes });
    auto result = manager.PushAssets(appId_, appId_, sessionId_, data, deviceId_);
    ASSERT_EQ(result, DistributedObject::OBJECT_SUCCESS);
}

/**
* @tc.name: PushAssets002
* @tc.desc: PushAssets test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, PushAssets002, TestSize.Level0)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::map<std::string, std::vector<uint8_t>> data;
    std::vector<uint8_t> value{ 0 };
    std::string data0 = "[STRING]test";
    value.insert(value.end(), data0.begin(), data0.end());

    std::string prefix = "bundleName_sessionId_source_target_timestamp";
    std::string dataKey = prefix + "_p_data";
    std::string assetPrefix = prefix + "_p_asset0";
    std::string fieldsPrefix = "p_";
    std::string deviceIdKey = "__deviceId";

    data.insert({ assetPrefix + ObjectStore::NAME_SUFFIX, value });
    data.insert({ assetPrefix + ObjectStore::URI_SUFFIX, value });
    data.insert({ assetPrefix + ObjectStore::MODIFY_TIME_SUFFIX, value });
    data.insert({ assetPrefix + ObjectStore::SIZE_SUFFIX, value });
    data.insert({ fieldsPrefix + deviceIdKey, value });

    manager.objectAssetsSendListener_ = nullptr;
    int32_t ret = manager.PushAssets(appId_, appId_, sessionId_, data, deviceId_);
    EXPECT_NE(ret, DistributedObject::OBJECT_SUCCESS);

    manager.objectAssetsSendListener_ = new ObjectAssetsSendListener();
    ASSERT_NE(manager.objectAssetsSendListener_, nullptr);
    ret = manager.PushAssets(appId_, appId_, sessionId_, data, deviceId_);
    EXPECT_NE(ret, DistributedObject::OBJECT_SUCCESS);
}

/**
* @tc.name: AddNotifier001
* @tc.desc: AddNotifie and DeleteNotifier test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, AddNotifier001, TestSize.Level0)
{
    auto syncManager = SequenceSyncManager::GetInstance();
    std::function<void(const std::map<std::string, int32_t> &results)> func;
    func = [](const std::map<std::string, int32_t> &results) { return results; };
    auto sequenceId_ = syncManager->AddNotifier(userId_, func);
    auto result = syncManager->DeleteNotifier(sequenceId_, userId_);
    ASSERT_EQ(result, SequenceSyncManager::SUCCESS_USER_HAS_FINISHED);
}

/**
* @tc.name: AddNotifier002
* @tc.desc: AddNotifie and DeleteNotifier test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, AddNotifier002, TestSize.Level0)
{
    auto syncManager = SequenceSyncManager::GetInstance();
    std::function<void(const std::map<std::string, int32_t> &results)> func;
    func = [](const std::map<std::string, int32_t> &results) { return results; };
    auto sequenceId = syncManager->AddNotifier(userId_, func);
    ASSERT_NE(sequenceId, sequenceId_);
    auto result = syncManager->DeleteNotifier(sequenceId_, userId_);
    ASSERT_EQ(result, SequenceSyncManager::ERR_SID_NOT_EXIST);
}

/**
* @tc.name: BindAsset 001
* @tc.desc: BindAsset test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, BindAsset001, TestSize.Level0)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string bundleName = "BindAsset";
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    auto result = manager.BindAsset(tokenId, bundleName, sessionId_, assetValue_, assetBindInfo_);
    ASSERT_EQ(result, DistributedObject::OBJECT_DBSTATUS_ERROR);
}

/**
* @tc.name: OnFinished001
* @tc.desc: OnFinished test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, OnFinished001, TestSize.Level1)
{
    std::string srcNetworkId = "srcNetworkId";
    sptr<AssetObj> assetObj = nullptr;
    int32_t result = 100;
    ObjectAssetsRecvListener listener;
    int32_t ret = listener.OnFinished(srcNetworkId, assetObj, result);
    EXPECT_NE(ret, DistributedObject::OBJECT_SUCCESS);

    sptr<AssetObj> assetObj_1 = new AssetObj();
    assetObj_1->dstBundleName_ = bundleName_;
    assetObj_1->srcBundleName_ = bundleName_;
    assetObj_1->dstNetworkId_ = "1";
    assetObj_1->sessionId_ = "123";
    ret = listener.OnFinished(srcNetworkId, assetObj_1, result);
    EXPECT_EQ(ret, DistributedObject::OBJECT_SUCCESS);
}

/**
* @tc.name: GetObjectData001
* @tc.desc: GetObjectData test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, GetObjectData001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();

    std::string bundleName = bundleName_;
    std::string sessionId = sessionId_;
    std::string source = "sourceDeviceId";
    std::string target = "targetDeviceId";
    std::string timestamp = "1234567890";
    ObjectStoreManager::SaveInfo saveInfo(bundleName, sessionId, source, target, timestamp);
    std::string prefix = saveInfo.ToPropertyPrefix();
    EXPECT_FALSE(prefix.empty());

    // p_name not asset key
    std::string p_name = "p_namejpg";
    std::string key = bundleName + "_" + sessionId + "_" + source + "_" + target + "_" + timestamp + "_" + p_name;
    std::map<std::string, std::vector<uint8_t>> changedData = { { key, data_ } };
    bool hasAsset = false;
    auto ret = manager.GetObjectData(changedData, saveInfo, hasAsset);
    EXPECT_FALSE(ret.empty());
    EXPECT_FALSE(hasAsset);
}

/**
* @tc.name: GetObjectData002
* @tc.desc: GetObjectData test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, GetObjectData002, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();

    std::string bundleName = "";
    std::string sessionId = "";
    std::string source = "";
    std::string target = "";
    std::string timestamp = "";
    ObjectStoreManager::SaveInfo saveInfo(bundleName, sessionId, source, target, timestamp);
    std::string prefix = saveInfo.ToPropertyPrefix();
    EXPECT_TRUE(prefix.empty());

    // saveInfo.bundleName, sourceDeviceId, targetDeviceId empty
    saveInfo.sessionId = sessionId_;
    saveInfo.timestamp = "1234567890";

    bundleName = bundleName_;
    sessionId = sessionId_;
    source = "sourceDeviceId";
    target = "targetDeviceId";
    timestamp = "1234567890";
    std::string p_name = "p_name.jpg";
    std::string key = bundleName + "_" + sessionId + "_" + source + "_" + target + "_" + timestamp + "_" + p_name;
    std::map<std::string, std::vector<uint8_t>> changedData = { { key, data_ } };
    bool hasAsset = false;
    auto ret = manager.GetObjectData(changedData, saveInfo, hasAsset);
    EXPECT_FALSE(ret.empty());
    EXPECT_EQ(saveInfo.bundleName, bundleName);
    EXPECT_TRUE(hasAsset);

    // only targetDeviceId empty
    saveInfo.bundleName = "test_bundleName";
    saveInfo.sourceDeviceId = "test_source";
    // p_name not asset key
    p_name = "p_namejpg";
    std::string key_1 = bundleName + "_" + sessionId + "_" + source + "_" + target + "_" + timestamp + "_" + p_name;
    std::map<std::string, std::vector<uint8_t>> changedData_1 = { { key_1, data_ } };
    hasAsset = false;
    ret = manager.GetObjectData(changedData_1, saveInfo, hasAsset);
    EXPECT_FALSE(ret.empty());
    EXPECT_NE(saveInfo.bundleName, bundleName);
    EXPECT_FALSE(hasAsset);
}

/**
* @tc.name: InitUserMeta001
* @tc.desc: test clear old user meta.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, InitUserMeta001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    auto status = manager.InitUserMeta();
    ASSERT_EQ(status, DistributedObject::OBJECT_SUCCESS);
}

/**
* @tc.name: registerAndUnregisterProgressObserverCallback001
* @tc.desc: test RegisterProgressObserverCallback and UnregisterProgressObserverCallback.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(ObjectManagerTest, registerAndUnregisterProgressObserverCallback001, TestSize.Level0)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::function<void(int32_t)> cb = [](int32_t progress) {};
    sptr<ObjectProgressCallbackBroker> objectRemoteResumeCallback = new (std::nothrow) ObjectProgressCallback(cb);
    ASSERT_NE(objectRemoteResumeCallback, nullptr);
    manager.RegisterProgressObserverCallback(bundleName_, sessionId_, pid_, tokenId_,
        objectRemoteResumeCallback->AsObject());
    ObjectStoreManager::ProgressCallbackInfo progressCallbackInfo = manager.processCallbacks_.Find(tokenId_).second;
    std::string objectKey = bundleName_ + sessionId_;
    ASSERT_NE(progressCallbackInfo.observers_.find(objectKey), progressCallbackInfo.observers_.end());
    manager.UnregisterProgressObserverCallback(bundleName_, pid_, tokenId_, sessionId_);
    progressCallbackInfo = manager.processCallbacks_.Find(tokenId_).second;
    ASSERT_EQ(progressCallbackInfo.observers_.find(objectKey), progressCallbackInfo.observers_.end());
}

/**
* @tc.name: registerAndUnregisterProgressObserverCallback002
* @tc.desc: abnormal use cases.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(ObjectManagerTest, registerAndUnregisterProgressObserverCallback002, TestSize.Level0)
{
    auto &manager = ObjectStoreManager::GetInstance();
    sptr<IRemoteObject> callback;
    uint32_t tokenId = 101;
    manager.RegisterProgressObserverCallback("", sessionId_, pid_, tokenId, callback);
    manager.RegisterProgressObserverCallback(bundleName_, "", pid_, tokenId, callback);
    manager.RegisterProgressObserverCallback("", "", pid_, tokenId, callback);
    ObjectStoreManager::ProgressCallbackInfo progressCallbackInfo = manager.processCallbacks_.Find(tokenId_).second;
    progressCallbackInfo.pid = pid_;
    manager.RegisterProgressObserverCallback(bundleName_, sessionId_, pid_, tokenId_, callback);
    ASSERT_EQ(manager.processCallbacks_.Find(tokenId).first, false);
    manager.UnregisterProgressObserverCallback("", pid_, tokenId, sessionId_);
    manager.UnregisterProgressObserverCallback("", pid_, tokenId, "");
    manager.UnregisterProgressObserverCallback(bundleName_, pid_, tokenId, "");
}

/**
* @tc.name: NotifyAssetsRecvProgress001
* @tc.desc: NotifyAssetsRecvProgress test.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(ObjectManagerTest, NotifyAssetsRecvProgress001, TestSize.Level0)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string objectKey = bundleName_ + sessionId_;
    std::string errProgress = "errProgress";
    int32_t progress = 100;
    ASSERT_EQ(manager.assetsRecvProgress_.Find(objectKey).first, true);
    ObjectStoreManager::ProgressCallbackInfo progressCallbackInfo = manager.processCallbacks_.Find(tokenId_).second;
    manager.NotifyAssetsRecvProgress(errProgress, progress);
    manager.assetsRecvProgress_.Clear();
    manager.assetsRecvProgress_.Insert(objectKey, progress);
    progressCallbackInfo.observers_.clear();
    manager.NotifyAssetsRecvProgress(errProgress, progress);
}

/**
* @tc.name: OnRecvProgress001
* @tc.desc: OnRecvProgress test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, OnRecvProgress001, TestSize.Level1)
{
    std::string srcNetworkId = "srcNetworkId";
    sptr<AssetObj> assetObj = nullptr;
    uint64_t totalBytes = 100;
    uint64_t processBytes = 100;
    ObjectAssetsRecvListener listener;
    int32_t ret = listener.OnRecvProgress(srcNetworkId, assetObj, totalBytes, processBytes);
    EXPECT_EQ(ret, DistributedObject::OBJECT_INNER_ERROR);
    uint64_t totalBytes_01 = 0;
    ret = listener.OnRecvProgress(srcNetworkId, assetObj, totalBytes_01, processBytes);
    EXPECT_EQ(ret, DistributedObject::OBJECT_INNER_ERROR);
    sptr<AssetObj> assetObj_1 = new AssetObj();
    ret = listener.OnRecvProgress(srcNetworkId, assetObj_1, totalBytes_01, processBytes);
    EXPECT_EQ(ret, DistributedObject::OBJECT_INNER_ERROR);
    ret = listener.OnRecvProgress(srcNetworkId, assetObj_1, totalBytes, processBytes);
    EXPECT_EQ(ret, DistributedObject::OBJECT_SUCCESS);
}

/**
* @tc.name: OnFinished002
* @tc.desc: OnFinished test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, OnFinished002, TestSize.Level1)
{
    std::string srcNetworkId = "srcNetworkId";
    ObjectAssetsRecvListener listener;
    sptr<AssetObj> assetObj_1 = new AssetObj();
    assetObj_1->dstBundleName_ = bundleName_;
    assetObj_1->srcBundleName_ = bundleName_;
    assetObj_1->dstNetworkId_ = "1";
    assetObj_1->sessionId_ = "123";
    int32_t result = 100;
    auto ret = listener.OnFinished(srcNetworkId, assetObj_1, result);
    int32_t result_1 = 0;
    ret = listener.OnFinished(srcNetworkId, assetObj_1, result_1);
    EXPECT_EQ(ret, DistributedObject::OBJECT_SUCCESS);
}

/**
* @tc.name: Save001
* @tc.desc: Save test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, Save001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string appId = "appId";
    std::string sessionId = "sessionId";
    ObjectRecord data;
    std::string deviceId = "deviceId";
    auto ret = manager.Save(appId, sessionId, data, deviceId, nullptr);
    EXPECT_EQ(ret, DistributedKv::INVALID_ARGUMENT);
}

/**
* @tc.name: Save002
* @tc.desc: Save test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, Save002, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string appId = "appId";
    std::string sessionId = "sessionId";
    ObjectRecord data;
    std::string deviceId = "";
    std::function<void(const std::map<std::string, int32_t> &)> cb = [](const std::map<std::string, int32_t> &) {};
    sptr<ObjectSaveCallbackBroker> objectSaveCallback = new (std::nothrow) ObjectSaveCallback(cb);
    ASSERT_NE(objectSaveCallback, nullptr);
    auto ret = manager.Save(appId, sessionId, data, deviceId, objectSaveCallback->AsObject());
    EXPECT_EQ(ret, DistributedKv::INVALID_ARGUMENT);
}

/**
* @tc.name: RevokeSave001
* @tc.desc: RevokeSave test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, RevokeSave001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string appId = "appId";
    std::string sessionId = "sessionId";
    auto ret = manager.RevokeSave(appId, sessionId, nullptr);
    EXPECT_EQ(ret, DistributedKv::INVALID_ARGUMENT);
}

/**
* @tc.name: RevokeSave002
* @tc.desc: RevokeSave test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, RevokeSave002, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string appId = "appId";
    std::string sessionId = "sessionId";
    std::function<void(int32_t status)> cb = [](int32_t) {};
    sptr<ObjectRevokeSaveCallbackBroker> objectRevokeSaveCallback = new (std::nothrow) ObjectRevokeSaveCallback(cb);
    ASSERT_NE(objectRevokeSaveCallback, nullptr);
    auto ret = manager.RevokeSave(appId, sessionId, objectRevokeSaveCallback->AsObject());
    EXPECT_EQ(ret, DistributedObject::OBJECT_SUCCESS);
}

/**
* @tc.name: Retrieve001
* @tc.desc: Retrieve test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, Retrieve001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string bundleName = "bundleName";
    std::string sessionId = "sessionId";
    uint32_t tokenId = 0;
    auto ret = manager.Retrieve(bundleName, sessionId, nullptr, tokenId);
    EXPECT_EQ(ret, DistributedKv::INVALID_ARGUMENT);
}

/**
* @tc.name: Retrieve002
* @tc.desc: Retrieve test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, Retrieve002, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string bundleName = "bundleName";
    std::string sessionId = "sessionId";
    uint32_t tokenId = 0;
    std::function<void(const std::map<std::string, std::vector<uint8_t>> &, bool)> cb =
        [](const std::map<std::string, std::vector<uint8_t>> &, bool) {};
    sptr<ObjectRetrieveCallbackBroker> objectRetrieveCallback = new (std::nothrow) ObjectRetrieveCallback(cb);
    ASSERT_NE(objectRetrieveCallback, nullptr);
    auto ret = manager.Retrieve(bundleName, sessionId, objectRetrieveCallback->AsObject(), tokenId);
    EXPECT_EQ(ret, DistributedKv::KEY_NOT_FOUND);
}

/**
* @tc.name: PullAssets001
* @tc.desc: ObjectStoreManager pull assets test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, PullAssets001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    
    std::map<std::string, ObjectRecord> data;
    ObjectRecord record;
    std::vector<uint8_t> value{0};
    std::string dataStr = "[STRING]test_asset";
    value.insert(value.end(), dataStr.begin(), dataStr.end());
    
    std::string assetPrefix = "test_asset";
    record.insert({assetPrefix + ObjectStore::NAME_SUFFIX, value});
    record.insert({assetPrefix + ObjectStore::URI_SUFFIX, value});
    record.insert({assetPrefix + ObjectStore::MODIFY_TIME_SUFFIX, value});
    record.insert({assetPrefix + ObjectStore::SIZE_SUFFIX, value});
    
    data.insert({"test_object", record});
    
    ObjectStoreManager::SaveInfo saveInfo(bundleName_, sessionId_, deviceId_, "target_device", "1234567890");
    manager.PullAssets(data, saveInfo);
    EXPECT_EQ(data.size(), 1);
    EXPECT_EQ(data["test_object"].size(), 4);
}

/**
* @tc.name: GetSnapShots001
* @tc.desc: ObjectStoreManager get snapShots test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerTest, GetSnapShots001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string bundleName = "bundleA";
    std::string storeName = "storeA";
    std::string bundleName2 = "bundleB";
    std::string storeName2 = "storeB";

    auto ptr1 = manager.GetSnapShots(bundleName, storeName);
    EXPECT_NE(ptr1, nullptr);
    EXPECT_TRUE(ptr1->empty());
    auto snapshot = std::make_shared<ObjectSnapshot>();
    (*ptr1)["snap1"] = snapshot;

    auto ptr2 = manager.GetSnapShots(bundleName, storeName);
    EXPECT_EQ(ptr1, ptr2);
}
} // namespace OHOS::Test
