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
#define LOG_TAG "MetaDataManagerTest"

#include "metadata/meta_data_manager.h"
#include <gtest/gtest.h>

#include "log_print.h"
#include "metadata/user_meta_data.h"
#include "mock/db_store_mock.h"
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedKv;
namespace OHOS::Test {
class DBStoreMockImpl;
class MetaDataManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
    static std::shared_ptr<DBStoreMockImpl> metaStore;
};

std::shared_ptr<DBStoreMockImpl> MetaDataManagerTest::metaStore = nullptr;

class DBStoreMockImpl : public DBStoreMock {
public:
    using PutLocalBatchFunc = std::function<DBStatus(const std::vector<Entry> &)>;
    PutLocalBatchFunc putLocalBatchFunc;
    DBStatus PutLocalBatch(const std::vector<Entry> &entries) override
    {
        if (putLocalBatchFunc) {
            return putLocalBatchFunc(entries);
        }
        return DBStoreMock::PutLocalBatch(entries);
    }
    using DeleteLocalBatchFunc = std::function<DBStatus(const std::vector<Key> &)>;
    DeleteLocalBatchFunc deleteLocalBatchFunc;
    DBStatus DeleteLocalBatch(const std::vector<Key> &keys) override
    {
        if (deleteLocalBatchFunc) {
            return deleteLocalBatchFunc(keys);
        }
        return DBStoreMock::DeleteLocalBatch(keys);
    }
    using SyncFunc = std::function<DBStatus(const DistributedDB::DeviceSyncOption &,
        const std::function<void(const std::map<std::string, DBStatus> &)> &)>;
    SyncFunc syncFunc;
    DBStatus Sync(const DistributedDB::DeviceSyncOption &option,
        const std::function<void(const std::map<std::string, DBStatus> &)> &onComplete) override
    {
        if (syncFunc) {
            return syncFunc(option, onComplete);
        }
        return DBStoreMock::Sync(option, onComplete);
    }
};

void MetaDataManagerTest::SetUpTestCase(void)
{
    metaStore = std::make_shared<DBStoreMockImpl>();
    MetaDataManager::GetInstance().Initialize(
        metaStore,
        [](const auto &store) {
            return 0;
        },
        "meta");
};
/**
 * @tc.name: FilterConstructorAndGetKeyTest
 * @tc.desc: FilterConstructor and GetKey.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: MengYao
 */
HWTEST_F(MetaDataManagerTest, FilterConstructorAndGetKeyTest, TestSize.Level1)
{
    std::string pattern = "test";
    MetaDataManager::Filter filter(pattern);

    std::vector<uint8_t> key = filter.GetKey();
    ASSERT_EQ(key.size(), 0);
}

/**
 * @tc.name: FilterOperatorTest
 * @tc.desc: FilterOperator.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: MengYao
 */
HWTEST_F(MetaDataManagerTest, FilterOperatorTest, TestSize.Level1)
{
    std::string pattern = "test";
    MetaDataManager::Filter filter(pattern);

    std::string key = "test_key";
    ASSERT_TRUE(filter(key));

    key = "another_key";
    ASSERT_FALSE(filter(key));
}

/**
 * @tc.name: SaveMetaTest001
 * @tc.desc: Batch save empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, SaveMetaTest001, TestSize.Level1)
{
    std::vector<MetaDataManager::Entry> values;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(values, true));
}

/**
 * @tc.name: SaveMetaTest002
 * @tc.desc: Return error code when saving.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, SaveMetaTest002, TestSize.Level1)
{
    std::vector<MetaDataManager::Entry> values;
    values.push_back({ "SaveMetaTest002", "value" });
    metaStore->putLocalBatchFunc = [](auto &entries) {
        return DistributedDB::DBStatus::DB_ERROR;
    };
    EXPECT_FALSE(MetaDataManager::GetInstance().SaveMeta(values, true));
    metaStore->putLocalBatchFunc = nullptr;
}

/**
 * @tc.name: SaveMetaTest004
 * @tc.desc: Trigger sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, SaveMetaTest003, TestSize.Level1)
{
    metaStore->putLocalBatchFunc = nullptr;
    std::vector<MetaDataManager::Entry> values;
    values.push_back({ "SaveMetaTest004", "value" });
    bool flag = false;
    MetaDataManager::GetInstance().SetCloudSyncer([&flag]() {
        flag = true;
        return;
    });
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(values, false));
    EXPECT_TRUE(flag);
    MetaDataManager::GetInstance().SetCloudSyncer(nullptr);
}

/**
 * @tc.name: DelMetaTest001
 * @tc.desc: Return INVALID_PASSWD_OR_CORRUPTED_DB when del meta.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, DelMetaTest001, TestSize.Level1)
{
    metaStore->deleteLocalBatchFunc = nullptr;
    std::vector<std::string> keys{ "DelMetaTest001" };
    metaStore->deleteLocalBatchFunc = [](auto &keys) {
        return DistributedDB::DBStatus::DB_ERROR;
    };
    EXPECT_FALSE(MetaDataManager::GetInstance().DelMeta(keys, true));
    metaStore->deleteLocalBatchFunc = nullptr;
}

/**
 * @tc.name: DelMetaTest003
 * @tc.desc: Trigger sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, DelMetaTest002, TestSize.Level1)
{
    std::vector<std::string> keys{ "DelMetaTest003" };
    bool flag = false;
    MetaDataManager::GetInstance().SetCloudSyncer([&flag]() {
        flag = true;
        return;
    });
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(keys, false));
    EXPECT_TRUE(flag);
    MetaDataManager::GetInstance().SetCloudSyncer(nullptr);
}

/**
 * @tc.name: Subscribe001
 * @tc.desc: Trigger sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, Subscribe001, TestSize.Level1)
{
    std::string prefix = "prefix001";
    MetaDataManager::Observer observer = [](const std::string &key, const std::string &value, int32_t flag) {
        return false;
    };
    auto hold = MetaDataManager::GetInstance().metaStore_;
    MetaDataManager::GetInstance().metaStore_ = nullptr;
    bool res =  MetaDataManager::GetInstance().Subscribe(prefix, observer, false);
    EXPECT_TRUE(res);
    res =  MetaDataManager::GetInstance().Subscribe(prefix, observer, false);
    EXPECT_TRUE(res);
    res = MetaDataManager::GetInstance().Unsubscribe("errPrefix");
    EXPECT_TRUE(res);
    res = MetaDataManager::GetInstance().Unsubscribe(prefix);
    EXPECT_TRUE(res);
    MetaDataManager::GetInstance().metaStore_ = hold;
}

/**
 * @tc.name: SyncWithOptionTest001
 * @tc.desc: devices is empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, SyncWithOptionTest001, TestSize.Level1)
{
    MetaDataManager::DeviceMetaSyncOption option;
    MetaDataManager::OnComplete complete;
    auto result = MetaDataManager::GetInstance().Sync(option, complete);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SyncWithOptionTest002
 * @tc.desc: devices is valid and sync success.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, SyncWithOptionTest002, TestSize.Level1)
{
    UserMetaData userMetaData;
    UserStatus userStatus(100, true);
    userMetaData.users.push_back(userStatus);
    std::string userKey = UserMetaRow::GetKeyFor("device001");
    MetaDataManager::GetInstance().SaveMeta(userKey, userMetaData, false);

    MetaDataManager::DeviceMetaSyncOption option;
    option.devices = { "device001" };
    option.localDevice = "localDevice";
    option.bundleName = "com.test";
    option.storeId = "testStore";
    bool callbackCalled = false;
    MetaDataManager::OnComplete complete = [&callbackCalled](const auto &results) {
        callbackCalled = true;
    };
    int syncCallCount = 0;
    metaStore->syncFunc = [&syncCallCount](const auto &option, const auto &onComplete) {
        syncCallCount++;
        std::map<std::string, DistributedDB::DBStatus> result;
        for (const auto &device : option.devices) {
            result[device] = DistributedDB::DBStatus::OK;
        }
        onComplete(result);
        return DistributedDB::DBStatus::OK;
    };
    auto result = MetaDataManager::GetInstance().Sync(option, complete);
    EXPECT_TRUE(result);
    EXPECT_EQ(syncCallCount, 2);
    EXPECT_TRUE(callbackCalled);
    metaStore->syncFunc = nullptr;
}

/**
 * @tc.name: SyncWithOptionTest003
 * @tc.desc: all devices sync failed, should call complete directly.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, SyncWithOptionTest003, TestSize.Level1)
{
    MetaDataManager::DeviceMetaSyncOption option;
    option.devices = { "device001" };
    option.localDevice = "localDevice";
    option.bundleName = "com.test";
    option.storeId = "testStore";
    bool callbackCalled = false;
    std::map<std::string, int32_t> callbackResults;
    MetaDataManager::OnComplete complete = [&callbackCalled, &callbackResults](const auto &results) {
        callbackCalled = true;
        callbackResults = results;
    };
    metaStore->syncFunc = [](const auto &option, const auto &onComplete) {
        std::map<std::string, DistributedDB::DBStatus> result;
        for (const auto &device : option.devices) {
            result[device] = DistributedDB::DBStatus::DB_ERROR;
        }
        onComplete(result);
        return DistributedDB::DBStatus::OK;
    };
    auto result = MetaDataManager::GetInstance().Sync(option, complete);
    EXPECT_TRUE(result);
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(callbackResults.size(), 1);
    metaStore->syncFunc = nullptr;
}

/**
 * @tc.name: SyncWithOptionTest004
 * @tc.desc: sync return error.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, SyncWithOptionTest004, TestSize.Level1)
{
    MetaDataManager::DeviceMetaSyncOption option;
    option.devices = { "device001" };
    option.localDevice = "localDevice";
    option.bundleName = "com.test";
    option.storeId = "testStore";
    MetaDataManager::OnComplete complete;
    metaStore->syncFunc = [](const auto &option, const auto &onComplete) {
        return DistributedDB::DBStatus::DB_ERROR;
    };
    auto result = MetaDataManager::GetInstance().Sync(option, complete);
    EXPECT_FALSE(result);
    metaStore->syncFunc = nullptr;
}

/**
 * @tc.name: SyncWithOptionTest005
 * @tc.desc: all devices sync failed and complete is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, SyncWithOptionTest005, TestSize.Level1)
{
    MetaDataManager::DeviceMetaSyncOption option;
    option.devices = { "device001" };
    option.localDevice = "localDevice";
    option.bundleName = "com.test";
    option.storeId = "testStore";
    MetaDataManager::OnComplete complete;
    metaStore->syncFunc = [](const auto &option, const auto &onComplete) {
        std::map<std::string, DistributedDB::DBStatus> result;
        for (const auto &device : option.devices) {
            result[device] = DistributedDB::DBStatus::DB_ERROR;
        }
        onComplete(result);
        return DistributedDB::DBStatus::OK;
    };
    auto result = MetaDataManager::GetInstance().Sync(option, complete);
    EXPECT_TRUE(result);
    metaStore->syncFunc = nullptr;
}

/**
 * @tc.name: SyncWithOptionTest006
 * @tc.desc: partial devices sync success, verify two-phase sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, SyncWithOptionTest006, TestSize.Level1)
{
    UserMetaData userMetaData;
    UserStatus userStatus(100, true);
    userMetaData.users.push_back(userStatus);
    std::string userKey = UserMetaRow::GetKeyFor("device001");
    MetaDataManager::GetInstance().SaveMeta(userKey, userMetaData, false);

    MetaDataManager::DeviceMetaSyncOption option;
    option.devices = { "device001", "device002" };
    option.localDevice = "localDevice";
    option.bundleName = "com.test";
    option.storeId = "testStore";
    int completeCallCount = 0;
    MetaDataManager::OnComplete complete = [&completeCallCount](const auto &results) {
        completeCallCount++;
    };
    int syncCallCount = 0;
    metaStore->syncFunc = [&syncCallCount](const auto &option, const auto &onComplete) {
        syncCallCount++;
        std::map<std::string, DistributedDB::DBStatus> result;
        result["device001"] = DistributedDB::DBStatus::OK;
        result["device002"] = DistributedDB::DBStatus::DB_ERROR;
        onComplete(result);
        return DistributedDB::DBStatus::OK;
    };
    auto result = MetaDataManager::GetInstance().Sync(option, complete);
    EXPECT_TRUE(result);
    EXPECT_EQ(syncCallCount, 2);
    metaStore->syncFunc = nullptr;
}

/**
 * @tc.name: SyncWithOptionTest007
 * @tc.desc: SyncWithQueryKeys complete is nullptr.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, SyncWithOptionTest007, TestSize.Level1)
{
    UserMetaData userMetaData;
    UserStatus userStatus(100, true);
    userMetaData.users.push_back(userStatus);
    std::string userKey = UserMetaRow::GetKeyFor("device001");
    MetaDataManager::GetInstance().SaveMeta(userKey, userMetaData, false);

    MetaDataManager::DeviceMetaSyncOption option;
    option.devices = { "device001" };
    option.localDevice = "localDevice";
    option.bundleName = "com.test";
    option.storeId = "testStore";
    MetaDataManager::OnComplete complete;
    int syncCallCount = 0;
    metaStore->syncFunc = [&syncCallCount](const auto &option, const auto &onComplete) {
        syncCallCount++;
        std::map<std::string, DistributedDB::DBStatus> result;
        for (const auto &device : option.devices) {
            result[device] = DistributedDB::DBStatus::OK;
        }
        onComplete(result);
        return DistributedDB::DBStatus::OK;
    };
    auto result = MetaDataManager::GetInstance().Sync(option, complete);
    EXPECT_TRUE(result);
    EXPECT_EQ(syncCallCount, 2);
    metaStore->syncFunc = nullptr;
}
} // namespace OHOS::Test
