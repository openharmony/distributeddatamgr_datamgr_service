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

#include <gtest/gtest.h>

#include "accesstoken_kit.h"
#include "auth_delegate.h"
#include "bootstrap.h"
#include "crypto_manager.h"
#include "device_manager_adapter.h"
#include "directory/directory_manager.h"
#include "kvdb_general_store.h"
#include "kvdb_notifier_proxy.h"
#include "kvdb_watcher.h"
#include "kvstore_meta_manager.h"
#include "kvstore_sync_manager.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "query_helper.h"
#include "upgrade.h"
#include "user_delegate.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace OHOS::DistributedData;
using StoreMetaData = OHOS::DistributedData::StoreMetaData;
using DBPassword = DistributedDB::CipherPassword;
using DBStatus = DistributedDB::DBStatus;
using Status = OHOS::DistributedKv::Status;
using KVDBWatcher = OHOS::DistributedKv::KVDBWatcher;
using KVDBNotifierProxy = OHOS::DistributedKv::KVDBNotifierProxy;
using QueryHelper = OHOS::DistributedKv::QueryHelper;
namespace OHOS::Test {
namespace DistributedDataTest {
class UpgradeTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp();
    void TearDown(){};
protected:
    static constexpr const char *bundleName = "test_upgrade";
    static constexpr const char *storeName = "test_upgrade_meta";
    void InitMetaData();
    StoreMetaData metaData_;
};

void UpgradeTest::InitMetaData()
{
    metaData_.bundleName = bundleName;
    metaData_.appId = bundleName;
    metaData_.user = "0";
    metaData_.area = OHOS::DistributedKv::EL1;
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = DistributedKv::KvStoreType::SINGLE_VERSION;
    metaData_.storeId = storeName;
    metaData_.dataDir = "/data/service/el1/public/database/" + std::string(bundleName) + "/kvdb/upgrade";
    metaData_.securityLevel = DistributedKv::SecurityLevel::S2;
}

void UpgradeTest::SetUp()
{
    Bootstrap::GetInstance().LoadDirectory();
    InitMetaData();
}

class KvStoreSyncManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
protected:
};

class KVDBWatcherTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
protected:
};

class UserDelegateTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
protected:
};

class QueryHelperTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
protected:
};

class AuthHandlerTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
protected:
};

/**
* @tc.name: UpdateStore
* @tc.desc: UpdateStore test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(UpgradeTest, UpdateStore, TestSize.Level0)
{
    DistributedKv::Upgrade upgrade;
    StoreMetaData oldMeta = metaData_;
    oldMeta.version = 1;
    oldMeta.storeType = DistributedKv::KvStoreType::DEVICE_COLLABORATION;
    oldMeta.dataDir = "/data/service/el1/public/database/" + std::string(bundleName) + "/kvdb/upgrade/old";
    std::vector<uint8_t> password = {0x01, 0x02, 0x03};
    auto dbStatus = upgrade.UpdateStore(oldMeta, metaData_, password);
    EXPECT_EQ(dbStatus, DBStatus::DB_ERROR);

    oldMeta.version = StoreMetaData::CURRENT_VERSION;
    dbStatus = upgrade.UpdateStore(oldMeta, metaData_, password);
    EXPECT_EQ(dbStatus, DBStatus::NOT_SUPPORT);

    oldMeta.storeType = DistributedKv::KvStoreType::SINGLE_VERSION;
    DistributedKv::Upgrade::Exporter exporter = [](const StoreMetaData &, DBPassword &) {
        return "testexporter";
    };
    upgrade.exporter_ = exporter;
    dbStatus = upgrade.UpdateStore(oldMeta, metaData_, password);
    EXPECT_EQ(dbStatus, DBStatus::NOT_SUPPORT);

    oldMeta.version = 1;
    DistributedKv::Upgrade::Cleaner cleaner = [](const StoreMetaData &meta) -> DistributedKv::Status {
        return DistributedKv::Status::SUCCESS;
    };
    upgrade.cleaner_ = cleaner;
    upgrade.exporter_ = nullptr;
    upgrade.UpdatePassword(metaData_, password);
    dbStatus = upgrade.UpdateStore(oldMeta, metaData_, password);
    EXPECT_EQ(dbStatus, DBStatus::NOT_SUPPORT);

    metaData_.isEncrypt = true;
    upgrade.UpdatePassword(metaData_, password);
    EXPECT_TRUE(upgrade.RegisterExporter(oldMeta.version, exporter));
    EXPECT_TRUE(upgrade.RegisterCleaner(oldMeta.version, cleaner));
    dbStatus = upgrade.UpdateStore(oldMeta, metaData_, password);
    EXPECT_EQ(dbStatus, DBStatus::DB_ERROR);

    StoreMetaData oldMetas = metaData_;
    dbStatus = upgrade.UpdateStore(oldMetas, metaData_, password);
    EXPECT_EQ(dbStatus, DBStatus::OK);
}

/**
* @tc.name: ExportStore
* @tc.desc: ExportStore test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(UpgradeTest, ExportStore, TestSize.Level0)
{
    DistributedKv::Upgrade upgrade;
    StoreMetaData oldMeta = metaData_;
    auto dbStatus = upgrade.ExportStore(oldMeta, metaData_);
    EXPECT_EQ(dbStatus, DBStatus::OK);

    oldMeta.dataDir = "/data/service/el1/public/database/" + std::string(bundleName) + "/kvdb/upgrade/old";
    dbStatus = upgrade.ExportStore(oldMeta, metaData_);
    EXPECT_EQ(dbStatus, DBStatus::NOT_SUPPORT);

    DistributedKv::Upgrade::Exporter exporter = [](const StoreMetaData &, DBPassword &) {
        return "testexporter";
    };
    EXPECT_TRUE(upgrade.RegisterExporter(oldMeta.version, exporter));
    dbStatus = upgrade.ExportStore(oldMeta, metaData_);
    EXPECT_EQ(dbStatus, DBStatus::OK);

    DistributedKv::Upgrade::Exporter test = [](const StoreMetaData &, DBPassword &) {
        return "";
    };
    EXPECT_TRUE(upgrade.RegisterExporter(oldMeta.version, test));
    dbStatus = upgrade.ExportStore(oldMeta, metaData_);
    EXPECT_EQ(dbStatus, DBStatus::NOT_FOUND);
}

/**
* @tc.name: GetEncryptedUuidByMeta
* @tc.desc: GetEncryptedUuidByMeta test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(UpgradeTest, GetEncryptedUuidByMeta, TestSize.Level0)
{
    DistributedKv::Upgrade upgrade;
    auto dbStatus = upgrade.GetEncryptedUuidByMeta(metaData_);
    EXPECT_EQ(dbStatus, metaData_.deviceId);
    metaData_.appId = "";
    dbStatus = upgrade.GetEncryptedUuidByMeta(metaData_);
    EXPECT_EQ(dbStatus, metaData_.appId);
}

/**
* @tc.name: AddSyncOperation
* @tc.desc: AddSyncOperation test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvStoreSyncManagerTest, AddSyncOperation, TestSize.Level0)
{
    DistributedKv::KvStoreSyncManager syncManager;
    uintptr_t syncId = 0;
    DistributedKv::KvStoreSyncManager::SyncFunc syncFunc = nullptr;
    DistributedKv::KvStoreSyncManager::SyncEnd syncEnd = nullptr;
    auto kvStatus = syncManager.AddSyncOperation(syncId, 0, syncFunc, syncEnd);
    EXPECT_EQ(kvStatus, Status::INVALID_ARGUMENT);
    syncId = 1;
    kvStatus = syncManager.AddSyncOperation(syncId, 0, syncFunc, syncEnd);
    EXPECT_EQ(kvStatus, Status::INVALID_ARGUMENT);
    syncFunc = [](const DistributedKv::KvStoreSyncManager::SyncEnd &callback) -> Status {
        std::map<std::string, DBStatus> status_map =
            {{"key1", DBStatus::OK}, {"key2", DBStatus::DB_ERROR}};
        callback(status_map);
        return Status::SUCCESS;
    };
    kvStatus = syncManager.AddSyncOperation(0, 0, syncFunc, syncEnd);
    EXPECT_EQ(kvStatus, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: RemoveSyncOperation
* @tc.desc: RemoveSyncOperation test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvStoreSyncManagerTest, RemoveSyncOperation, TestSize.Level0)
{
    DistributedKv::KvStoreSyncManager syncManager;
    uintptr_t syncId = 0;
    auto kvStatus = syncManager.RemoveSyncOperation(syncId);
    EXPECT_EQ(kvStatus, Status::ERROR);
}

/**
* @tc.name: DoRemoveSyncingOp
* @tc.desc: DoRemoveSyncingOp test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvStoreSyncManagerTest, GetTimeoutSyncOps, TestSize.Level0)
{
    DistributedKv::KvStoreSyncManager syncManager;
    DistributedKv::KvStoreSyncManager::TimePoint currentTime = std::chrono::steady_clock::now();
    DistributedKv::KvStoreSyncManager::KvSyncOperation syncOp;
    syncOp.syncId = 1;
    syncOp.beginTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(1);
    std::list<DistributedKv::KvStoreSyncManager::KvSyncOperation> syncOps;

    EXPECT_TRUE(syncManager.realtimeSyncingOps_.empty());
    EXPECT_TRUE(syncManager.scheduleSyncOps_.empty());
    auto kvStatus = syncManager.GetTimeoutSyncOps(currentTime, syncOps);
    EXPECT_EQ(kvStatus, false);
    syncManager.realtimeSyncingOps_.push_back(syncOp);
    kvStatus = syncManager.GetTimeoutSyncOps(currentTime, syncOps);
    EXPECT_EQ(kvStatus, false);
    syncManager.realtimeSyncingOps_ = syncOps;
    syncManager.scheduleSyncOps_.insert(std::make_pair(syncOp.beginTime, syncOp));
    kvStatus = syncManager.GetTimeoutSyncOps(currentTime, syncOps);
    EXPECT_EQ(kvStatus, false);

    syncManager.realtimeSyncingOps_.push_back(syncOp);
    syncManager.scheduleSyncOps_.insert(std::make_pair(syncOp.beginTime, syncOp));
    EXPECT_TRUE(!syncManager.realtimeSyncingOps_.empty());
    EXPECT_TRUE(!syncManager.scheduleSyncOps_.empty());
    kvStatus = syncManager.GetTimeoutSyncOps(currentTime, syncOps);
    EXPECT_EQ(kvStatus, true);
}

/**
* @tc.name: KVDBWatcher
* @tc.desc: KVDBWatcher test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KVDBWatcherTest, KVDBWatcher, TestSize.Level0)
{
    GeneralWatcher::Origin origin;
    GeneralWatcher::PRIFields primaryFields = {{"primaryFields1", "primaryFields2"}};
    GeneralWatcher::ChangeInfo values;
    std::shared_ptr<KVDBWatcher> watcher = std::make_shared<KVDBWatcher>();
    sptr<OHOS::DistributedKv::IKvStoreObserver> observer;
    watcher->SetObserver(observer);
    EXPECT_EQ(watcher->observer_, nullptr);
    auto result = watcher->OnChange(origin, primaryFields, std::move(values));
    EXPECT_EQ(result, GeneralError::E_OK);
    GeneralWatcher::Fields fields;
    GeneralWatcher::ChangeData data;
    result = watcher->OnChange(origin, fields, std::move(data));
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: ConvertToEntries
* @tc.desc: ConvertToEntries test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KVDBWatcherTest, ConvertToEntries, TestSize.Level0)
{
    std::vector<Values> values;
    Values info1;
    info1.emplace_back(Bytes({1, 2, 3}));
    info1.emplace_back(Bytes({4, 5, 6}));
    values.emplace_back(info1);
    Values info2;
    info2.emplace_back(Bytes({7, 8, 9}));
    info2.emplace_back(Bytes({10, 11, 12}));
    values.emplace_back(info2);
    Values info3;
    info3.emplace_back(Bytes({16, 17, 18}));
    info3.emplace_back(int64_t(1));
    values.emplace_back(info3);
    Values info4;
    info4.emplace_back(int64_t(1));
    info4.emplace_back(Bytes({19, 20, 21}));
    values.emplace_back(info4);
    Values info5;
    info5.emplace_back(int64_t(1));
    info5.emplace_back(int64_t(1));
    values.emplace_back(info5);
    std::shared_ptr<KVDBWatcher> watcher = std::make_shared<KVDBWatcher>();
    auto result = watcher->ConvertToEntries(values);
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].key, Bytes({1, 2, 3}));
    EXPECT_EQ(result[0].value, Bytes({4, 5, 6}));
    EXPECT_EQ(result[1].key, Bytes({7, 8, 9}));
    EXPECT_EQ(result[1].value, Bytes({10, 11, 12}));
}

/**
* @tc.name: ConvertToKeys
* @tc.desc: ConvertToKeys test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KVDBWatcherTest, ConvertToKeys, TestSize.Level0)
{
    std::vector<GeneralWatcher::PRIValue> values = { "key1", 123, "key3", 456, "key5" };
    std::shared_ptr<KVDBWatcher> watcher = std::make_shared<KVDBWatcher>();
    auto result = watcher->ConvertToKeys(values);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "key1");
    EXPECT_EQ(result[1], "key3");
    EXPECT_EQ(result[2], "key5");
}

/**
* @tc.name: UserDelegate
* @tc.desc: UserDelegate test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(UserDelegateTest, UserDelegate, TestSize.Level0)
{
    std::shared_ptr<UserDelegate> userDelegate = std::make_shared<UserDelegate>();
    auto result = userDelegate->GetLocalUserStatus();
    EXPECT_EQ(result.size(), 0);
    std::string deviceId = "";
    result = userDelegate->GetRemoteUserStatus(deviceId);
    EXPECT_TRUE(deviceId.empty());
    EXPECT_TRUE(result.empty());
    deviceId = DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    result = userDelegate->GetRemoteUserStatus(deviceId);
    EXPECT_EQ(result.size(), 0);
}

/**
* @tc.name: StringToDbQuery
* @tc.desc: StringToDbQuery test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(QueryHelperTest, StringToDbQuery, TestSize.Level0)
{
    std::shared_ptr<QueryHelper> queryHelper = std::make_shared<QueryHelper>();
    bool isSuccess = false;
    std::string query = "";
    auto result = queryHelper->StringToDbQuery(query, isSuccess);
    EXPECT_TRUE(isSuccess);
    std::string querys(5 * 1024, 'a');
    query = "querys" + querys;
    result = queryHelper->StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
    query = "query";
    result = queryHelper->StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
}

/**
* @tc.name: Handle001
* @tc.desc: Handle test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(QueryHelperTest, Handle001, TestSize.Level0)
{
    std::shared_ptr<QueryHelper> queryHelper = std::make_shared<QueryHelper>();
    std::vector<std::string> words = {"query0", "query1", "query2", "query3", "query4"};
    int pointer = 0;
    int end = 1;
    int ends = 4;
    DistributedDB::Query dbQuery;
    EXPECT_FALSE(queryHelper->Handle(words, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelper->HandleExtra(words, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelper->HandleEqualTo(words, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelper->HandleNotEqualTo(words, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelper->HandleNotEqualTo(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleGreaterThan(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleGreaterThan(words, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelper->HandleLessThan(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleLessThan(words, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelper->HandleGreaterThanOrEqualTo(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleGreaterThanOrEqualTo(words, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelper->HandleLessThanOrEqualTo(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleLessThanOrEqualTo(words, pointer, end, dbQuery));

    pointer = 0;
    words = {"INTEGER", "LONG", "DOUBLE", "STRING"};
    EXPECT_FALSE(queryHelper->Handle(words, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelper->HandleExtra(words, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelper->HandleEqualTo(words, pointer, end, dbQuery));
    EXPECT_TRUE(queryHelper->HandleNotEqualTo(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleNotEqualTo(words, pointer, end, dbQuery));
    pointer = 0;
    EXPECT_TRUE(queryHelper->HandleGreaterThan(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleGreaterThan(words, pointer, end, dbQuery));
    pointer = 0;
    EXPECT_TRUE(queryHelper->HandleLessThan(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleLessThan(words, pointer, end, dbQuery));
    pointer = 0;
    EXPECT_TRUE(queryHelper->HandleGreaterThanOrEqualTo(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleGreaterThanOrEqualTo(words, pointer, end, dbQuery));
    pointer = 0;
    EXPECT_TRUE(queryHelper->HandleLessThanOrEqualTo(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleLessThanOrEqualTo(words, pointer, end, dbQuery));
}

/**
* @tc.name: Handle002
* @tc.desc: Handle test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(QueryHelperTest, Handle002, TestSize.Level0)
{
    std::shared_ptr<QueryHelper> queryHelper = std::make_shared<QueryHelper>();
    std::vector<std::string> words = {"query0", "query1", "query2", "query3", "query4"};
    int pointer = 1;
    int end = 1;
    int ends = 4;
    DistributedDB::Query dbQuery;
    EXPECT_TRUE(queryHelper->HandleIsNull(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleIsNull(words, pointer, end, dbQuery));
    pointer = 0;
    EXPECT_TRUE(queryHelper->HandleIsNotNull(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleIsNotNull(words, pointer, end, dbQuery));
    pointer = 0;
    EXPECT_TRUE(queryHelper->HandleLike(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleLike(words, pointer, end, dbQuery));
    pointer = 0;
    EXPECT_TRUE(queryHelper->HandleNotLike(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleNotLike(words, pointer, end, dbQuery));
    pointer = 0;
    EXPECT_TRUE(queryHelper->HandleOrderByAsc(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleOrderByAsc(words, pointer, end, dbQuery));
    pointer = 0;
    EXPECT_TRUE(queryHelper->HandleOrderByDesc(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleOrderByDesc(words, pointer, end, dbQuery));
    pointer = 0;
    EXPECT_TRUE(queryHelper->HandleOrderByWriteTime(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleOrderByWriteTime(words, pointer, end, dbQuery));
    pointer = 0;
    EXPECT_TRUE(queryHelper->HandleLimit(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleLimit(words, pointer, end, dbQuery));
    pointer = 0;
    EXPECT_TRUE(queryHelper->HandleKeyPrefix(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleKeyPrefix(words, pointer, end, dbQuery));
}

/**
* @tc.name: Handle003
* @tc.desc: Handle test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(QueryHelperTest, Handle003, TestSize.Level0)
{
    std::shared_ptr<QueryHelper> queryHelper = std::make_shared<QueryHelper>();
    std::vector<std::string> words = {"query0", "query1", "query2", "query3", "query4", "query5"};
    std::vector<std::string> wordss = {"^NOT_IN", "INTEGER", "LONG", "^START", "STRING", "^END"};
    int pointer = 0;
    int end = 1;
    int ends = 5;
    DistributedDB::Query dbQuery;
    EXPECT_FALSE(queryHelper->HandleIn(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleIn(words, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelper->HandleIn(wordss, pointer, end, dbQuery));
    EXPECT_TRUE(queryHelper->HandleIn(wordss, pointer, ends, dbQuery));
    pointer = 0;
    EXPECT_FALSE(queryHelper->HandleNotIn(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleNotIn(words, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelper->HandleNotIn(wordss, pointer, end, dbQuery));
    EXPECT_TRUE(queryHelper->HandleNotIn(wordss, pointer, ends, dbQuery));
}

/**
* @tc.name: Handle004
* @tc.desc: Handle test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(QueryHelperTest, Handle004, TestSize.Level0)
{
    std::shared_ptr<QueryHelper> queryHelper = std::make_shared<QueryHelper>();
    std::vector<std::string> words = {"query0", "query1", "query2", "query3", "query4", "query5"};
    std::vector<std::string> wordss = {"^NOT_IN", "INTEGER", "LONG", "^START", "STRING", "^END"};
    int pointer = 2;
    int end = 3;
    int ends = 5;
    DistributedDB::Query dbQuery;
    EXPECT_FALSE(queryHelper->HandleInKeys(words, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelper->HandleInKeys(words, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelper->HandleInKeys(wordss, pointer, end, dbQuery));
    EXPECT_TRUE(queryHelper->HandleInKeys(wordss, pointer, ends, dbQuery));
    pointer = 3;
    EXPECT_FALSE(queryHelper->HandleSetSuggestIndex(wordss, pointer, end, dbQuery));
    EXPECT_TRUE(queryHelper->HandleSetSuggestIndex(wordss, pointer, ends, dbQuery));
    pointer = 3;
    EXPECT_FALSE(queryHelper->HandleDeviceId(wordss, pointer, end, dbQuery));
    EXPECT_TRUE(queryHelper->HandleDeviceId(wordss, pointer, ends, dbQuery));
    queryHelper->hasPrefixKey_ = true;
    pointer = 3;
    EXPECT_TRUE(queryHelper->HandleDeviceId(wordss, pointer, ends, dbQuery));
}

/**
* @tc.name: StringTo
* @tc.desc: StringTo test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(QueryHelperTest, StringTo, TestSize.Level0)
{
    std::shared_ptr<QueryHelper> queryHelper = std::make_shared<QueryHelper>();
    std::string word = "true";
    EXPECT_TRUE(queryHelper->StringToBoolean(word));
    word = "false";
    EXPECT_FALSE(queryHelper->StringToBoolean(word));
    word = "BOOL";
    EXPECT_FALSE(queryHelper->StringToBoolean(word));

    word = "^EMPTY_STRING";
    auto result = queryHelper->StringToString(word);
    EXPECT_EQ(result, "");
    word = "START";
    result = queryHelper->StringToString(word);
    EXPECT_EQ(result, "START");
    word = "START^^START";
    result = queryHelper->StringToString(word);
    EXPECT_EQ(result, "START START");
    word = "START(^)START";
    result = queryHelper->StringToString(word);
    EXPECT_EQ(result, "START^START");
}

/**
* @tc.name: Get
* @tc.desc: Get test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(QueryHelperTest, Get, TestSize.Level0)
{
    std::shared_ptr<QueryHelper> queryHelper = std::make_shared<QueryHelper>();
    std::vector<std::string> words = {"1", "2", "3", "4", "5", "^END"};
    int elementPointer = 0;
    int end = 5;
    std::vector<int> ret = {1, 2, 3, 4, 5};
    auto result = queryHelper->GetIntegerList(words, elementPointer, end);
    EXPECT_EQ(result, ret);
    elementPointer = 6;
    ret = {};
    result = queryHelper->GetIntegerList(words, elementPointer, end);
    EXPECT_EQ(result, ret);

    elementPointer = 0;
    std::vector<int64_t> ret1 = {1, 2, 3, 4, 5};
    auto result1 = queryHelper->GetLongList(words, elementPointer, end);
    EXPECT_EQ(result1, ret1);
    elementPointer = 6;
    ret1 = {};
    result1 = queryHelper->GetLongList(words, elementPointer, end);
    EXPECT_EQ(result1, ret1);

    elementPointer = 0;
    std::vector<double> ret2 = {1, 2, 3, 4, 5};
    auto result2 = queryHelper->GetDoubleList(words, elementPointer, end);
    EXPECT_EQ(result2, ret2);
    elementPointer = 6;
    ret2 = {};
    result2 = queryHelper->GetDoubleList(words, elementPointer, end);
    EXPECT_EQ(result2, ret2);

    std::vector<std::string> words1 = {"^NOT_IN", "INTEGER", "LONG", "^START", "STRING", "^END"};
    elementPointer = 0;
    std::vector<std::string> ret3 = { "^NOT_IN", "INTEGER", "LONG", "^START", "STRING" };
    auto result3 = queryHelper->GetStringList(words1, elementPointer, end);
    EXPECT_EQ(result3, ret3);
    elementPointer = 6;
    ret3 = {};
    result3 = queryHelper->GetStringList(words1, elementPointer, end);
    EXPECT_EQ(result3, ret3);
}

/**
* @tc.name: AuthHandler
* @tc.desc: AuthHandler test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(AuthHandlerTest, AuthHandler, TestSize.Level0)
{
    int localUserId = 0;
    int peerUserId = 0;
    std::string peerDeviceId = "";
    AclParams aclParams;
    aclParams.isSendStatus = false;
    aclParams.authType = static_cast<int32_t>(DistributedKv::AuthType::IDENTICAL_ACCOUNT);
    bool isSend = false;
    auto result = AuthDelegate::GetInstance()->CheckAccess(localUserId, peerUserId, peerDeviceId, aclParams);
    EXPECT_TRUE(result);
    
    aclParams.authType = static_cast<int32_t>(DistributedKv::AuthType::DEFAULT);
    result = AuthDelegate::GetInstance()->CheckAccess(localUserId, peerUserId, peerDeviceId, aclParams);
    EXPECT_TRUE(result);

    aclParams.authType = static_cast<int32_t>(DistributedKv::AuthType::IDENTICAL_ACCOUNT);
    peerDeviceId = "peerDeviceId";
    result = AuthDelegate::GetInstance()->CheckAccess(localUserId, peerUserId, peerDeviceId, aclParams);
    EXPECT_TRUE(result);

    aclParams.authType = static_cast<int32_t>(DistributedKv::AuthType::DEFAULT);
    result = AuthDelegate::GetInstance()->CheckAccess(localUserId, peerUserId, peerDeviceId, aclParams);
    EXPECT_TRUE(result);

    localUserId = 1;
    result = AuthDelegate::GetInstance()->CheckAccess(localUserId, peerUserId, peerDeviceId, aclParams);
    EXPECT_FALSE(result);

    peerUserId = 1;
    result = AuthDelegate::GetInstance()->CheckAccess(localUserId, peerUserId, peerDeviceId, aclParams);
    EXPECT_FALSE(result);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test