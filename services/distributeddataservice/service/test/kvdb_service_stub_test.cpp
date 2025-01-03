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

#include "accesstoken_kit.h"
#include "auth_delegate.h"
#include "bootstrap.h"
#include "crypto_manager.h"
#include "device_manager_adapter.h"
#include "directory/directory_manager.h"
#include "kvdb_general_store.h"
#include "kvdb_notifier_proxy.h"
#include "kvdb_watcher.h"
#include "kvstore_sync_manager.h"
#include "kvstore_meta_manager.h"
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
using DBPassvalues = DistributedDB::CipherPassvalues;
using DBStatus = DistributedDB::DBStatus;
using KVDBWatcher = OHOS::DistributedKv::KVDBWatcher;
using KVDBNotifierProxy = OHOS::DistributedKv::KVDBNotifierProxy;
using QueryHelper = OHOS::DistributedKv::QueryHelper;
using Status = OHOS::DistributedKv::Status;

namespace OHOS::Test {
namespace DistributedDataTest {
class KvUpgradeTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp();
    void TearDown(){};
protected:
    static constexpr const char *bundleName = "upgrade";
    static constexpr const char *storeName = "upgrade_meta";
    void InitMetaData();
    StoreMetaData metaData;
};

void KvUpgradeTest::InitMetaData()
{
    metaData.bundleName = bundleName;
    metaData.appId = bundleName;
    metaData.user = "0";
    metaData.area = OHOS::DistributedKv::EL1;
    metaData.instanceId = 0;
    metaData.isAutoSync = true;
    metaData.storeType = DistributedKv::KvStoreType::SINGLE_VERSION;
    metaData.storeId = storeName;
    metaData.dataDir = "/data/service/el1/public/database/" + std::string(bundleName) + "/kvdb/upgrade";
    metaData.securityLevel = DistributedKv::SecurityLevel::S2;
}

void KvUpgradeTest::SetUp()
{
    Bootstrap::GetInstance().LoadDirectory();
    InitMetaData();
}

class KvStoreSyncTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
protected:
};

class KVWatcherTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
protected:
};

class DelegateTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
protected:
};

class QueryHelpTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
protected:
};

class AuthHandleTest : public testing::Test {
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
HWTEST_F(KvUpgradeTest, UpdateStoreTest, TestSize.Level0)
{
    DistributedKv::Upgrade upgrade01;
    StoreMetaData storeMeta = metaData;
    storeMeta.version = 1;
    storeMeta.storeType = DistributedKv::KvStoreType::DEVICE_COLLABORATION;
    storeMeta.dataDir = "/data/service/el1/public/database/" + std::string(bundleName) + "/kvdb/upgrade/old";
    std::vector<uint8_t> pssd = {0x01, 0x02, 0x03};
    auto status = upgrade01.UpdateStore(storeMeta, metaData, pssd);
    ASSERT_EQ(status, DBStatus::DB_ERROR);

    storeMeta.version = StoreMetaData::CURRENT_VERSION;
    status = upgrade01.UpdateStore(storeMeta, metaData, pssd);
    ASSERT_EQ(status, DBStatus::NOT_SUPPORT);

    storeMeta.storeType = DistributedKv::KvStoreType::SINGLE_VERSION;
    DistributedKv::Upgrade::Exporter exporter = [](const StoreMetaData &, DBPassvalues &) {
        return "testexporter";
    };
    upgrade01.exporter_ = exporter;
    status = upgrade01.UpdateStore(storeMeta, metaData, pssd);
    ASSERT_EQ(status, DBStatus::NOT_SUPPORT);

    storeMeta.version = 1;
    DistributedKv::Upgrade::Cleaner cleaner = [](const StoreMetaData &meta) -> DistributedKv::Status {
        return DistributedKv::Status::SUCCESS;
    };
    upgrade01.cleaner_ = cleaner;
    upgrade01.exporter_ = nullptr;
    upgrade01.UpdatePassvalues(metaData, pssd);
    status = upgrade01.UpdateStore(storeMeta, metaData, pssd);
    ASSERT_EQ(status, DBStatus::NOT_SUPPORT);

    metaData.isEncrypt = true;
    upgrade01.UpdatePassvalues(metaData, pssd);
    ASSERT_TRUE(upgrade01.RegisterExporter(storeMeta.version, exporter));
    ASSERT_TRUE(upgrade01.RegisterCleaner(storeMeta.version, cleaner));
    status = upgrade01.UpdateStore(storeMeta, metaData, pssd);
    ASSERT_EQ(status, DBStatus::DB_ERROR);

    StoreMetaData storeMetas = metaData;
    status = upgrade01.UpdateStore(storeMetas, metaData, pssd);
    ASSERT_EQ(status, DBStatus::OK);
}

/**
* @tc.name: ExportStore
* @tc.desc: ExportStore test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvUpgradeTest, ExportStoreTest, TestSize.Level0)
{
    DistributedKv::Upgrade upgrade02;
    StoreMetaData storeMeta = metaData;
    auto status = upgrade02.ExportStore(storeMeta, metaData);
    ASSERT_EQ(status, DBStatus::OK);

    storeMeta.dataDir = "/data/service/el1/public/database/" + std::string(bundleName) + "/kvdb/upgrade/old";
    status = upgrade02.ExportStore(storeMeta, metaData);
    ASSERT_EQ(status, DBStatus::NOT_SUPPORT);

    DistributedKv::Upgrade::Exporter exporter = [](const StoreMetaData &, DBPassvalues &) {
        return "testexporter";
    };
    ASSERT_TRUE(upgrade02.RegisterExporter(storeMeta.version, exporter));
    status = upgrade02.ExportStore(storeMeta, metaData);
    ASSERT_EQ(status, DBStatus::OK);

    DistributedKv::Upgrade::Exporter test = [](const StoreMetaData &, DBPassvalues &) {
        return "";
    };
    ASSERT_TRUE(upgrade02.RegisterExporter(storeMeta.version, test));
    status = upgrade02.ExportStore(storeMeta, metaData);
    ASSERT_EQ(status, DBStatus::NOT_FOUND);
}

/**
* @tc.name: GetEncryptedUuidByMeta
* @tc.desc: GetEncryptedUuidByMeta test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvUpgradeTest, GetEncryptedUuidByMetaTest, TestSize.Level0)
{
    DistributedKv::Upgrade upgrade;
    auto status = upgrade.GetEncryptedUuidByMeta(metaData);
    ASSERT_EQ(status, metaData.deviceId);
    metaData.appId = "";
    status = upgrade.GetEncryptedUuidByMeta(metaData);
    ASSERT_EQ(status, metaData.appId);
}

/**
* @tc.name: AddSyncOperation
* @tc.desc: AddSyncOperation test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvStoreSyncTest, AddSyncOperationTest, TestSize.Level0)
{
    DistributedKv::KvStoreSyncManager kvSyncManager;
    uintptr_t syncId = 0;
    DistributedKv::KvStoreSyncManager::SyncFunc syncFunc = nullptr;
    DistributedKv::KvStoreSyncManager::SyncEnd syncEnd = nullptr;
    auto kvdbStatus = kvSyncManager.AddSyncOperation(syncId, 0, syncFunc, syncEnd);
    ASSERT_EQ(kvdbStatus, Status::INVALID_ARGUMENT);
    syncId = 1;
    kvdbStatus = kvSyncManager.AddSyncOperation(syncId, 0, syncFunc, syncEnd);
    ASSERT_EQ(kvdbStatus, Status::INVALID_ARGUMENT);
    syncFunc = [](const DistributedKv::KvStoreSyncManager::SyncEnd &callback) -> Status {
        std::map<std::string, DBStatus> status_map =
            {{"key1", DBStatus::OK}, {"key2", DBStatus::DB_ERROR}};
        callback(status_map);
        return Status::SUCCESS;
    };
    kvdbStatus = kvSyncManager.AddSyncOperation(0, 0, syncFunc, syncEnd);
    ASSERT_EQ(kvdbStatus, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: RemoveSyncOperationTest
* @tc.desc: RemoveSyncOperation test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvStoreSyncTest, RemoveSyncOperationTest, TestSize.Level0)
{
    DistributedKv::KvStoreSyncManager kvSyncManager;
    uintptr_t syncId = 0;
    auto kvdbStatus = kvSyncManager.RemoveSyncOperation(syncId);
    ASSERT_EQ(kvdbStatus, Status::ERROR);
}

/**
* @tc.name: DoRemoveSyncingOp
* @tc.desc: DoRemoveSyncingOp test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KvStoreSyncTest, GetTimeoutSyncOpsTest, TestSize.Level0)
{
    DistributedKv::KvStoreSyncManager kvSyncManager;
    DistributedKv::KvStoreSyncManager::TimePoint currentTime01 = std::chrono::steady_clock::now();
    DistributedKv::KvStoreSyncManager::KvSyncOperation syncOp;
    syncOp.syncId = 1;
    syncOp.beginTime = std::chrono::steady_clock::now() + std::chrono::milliseconds(1);
    std::list<DistributedKv::KvStoreSyncManager::KvSyncOperation> syncOps;

    ASSERT_TRUE(kvSyncManager.realtimeSyncingOps_.empty());
    ASSERT_TRUE(kvSyncManager.scheduleSyncOps_.empty());
    auto kvdbStatus = kvSyncManager.GetTimeoutSyncOps(currentTime01, syncOps);
    ASSERT_EQ(kvdbStatus, false);
    kvSyncManager.realtimeSyncingOps_.push_back(syncOp);
    kvdbStatus = kvSyncManager.GetTimeoutSyncOps(currentTime01, syncOps);
    ASSERT_EQ(kvdbStatus, false);
    kvSyncManager.realtimeSyncingOps_ = syncOps;
    kvSyncManager.scheduleSyncOps_.insert(std::make_pair(syncOp.beginTime, syncOp));
    kvdbStatus = kvSyncManager.GetTimeoutSyncOps(currentTime01, syncOps);
    ASSERT_EQ(kvdbStatus, false);

    kvSyncManager.realtimeSyncingOps_.push_back(syncOp);
    kvSyncManager.scheduleSyncOps_.insert(std::make_pair(syncOp.beginTime, syncOp));
    ASSERT_TRUE(!kvSyncManager.realtimeSyncingOps_.empty());
    ASSERT_TRUE(!kvSyncManager.scheduleSyncOps_.empty());
    kvdbStatus = kvSyncManager.GetTimeoutSyncOps(currentTime01, syncOps);
    ASSERT_EQ(kvdbStatus, true);
}

/**
* @tc.name: KVDBWatcher
* @tc.desc: KVDBWatcher test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KVWatcherTest, KVWatcherTest, TestSize.Level0)
{
    GeneralWatcher::Origin origin;
    GeneralWatcher::PRIFields primaryFields = {{"primaryFields1", "primaryFields2"}};
    GeneralWatcher::ChangeInfo values;
    std::shared_ptr<KVDBWatcher> kvWatcher = std::make_shared<KVDBWatcher>();
    sptr<OHOS::DistributedKv::IKvStoreObserver> observer;
    kvWatcher->SetObserver(observer);
    ASSERT_EQ(kvWatcher->observer_, nullptr);
    auto result = kvWatcher->OnChange(origin, primaryFields, std::move(values));
    ASSERT_EQ(result, GeneralError::E_OK);
    GeneralWatcher::Fields fields;
    GeneralWatcher::ChangeData data;
    result = kvWatcher->OnChange(origin, fields, std::move(data));
    ASSERT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: ConvertToEntries
* @tc.desc: ConvertToEntries test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KVWatcherTest, ConvertToEntriesTest, TestSize.Level0)
{
    std::vector<Values> values;
    Values info001;
    info1.emplace_back(Bytes({1, 2, 3}));
    info1.emplace_back(Bytes({4, 5, 6}));
    values.emplace_back(info001);
    Values info002;
    info2.emplace_back(Bytes({7, 8, 9}));
    info2.emplace_back(Bytes({10, 11, 12}));
    values.emplace_back(info002);
    Values info003;
    info3.emplace_back(Bytes({16, 17, 18}));
    info3.emplace_back(int64_t(1));
    values.emplace_back(info003);
    Values info004;
    info4.emplace_back(int64_t(1));
    info4.emplace_back(Bytes({19, 20, 21}));
    values.emplace_back(info004);
    Values info005;
    info5.emplace_back(int64_t(1));
    info5.emplace_back(int64_t(1));
    values.emplace_back(info005);
    std::shared_ptr<KVDBWatcher> kvWatcher = std::make_shared<KVDBWatcher>();
    auto result = kvWatcher->ConvertToEntries(values);
    ASSERT_EQ(result.size(), 2);
    ASSERT_EQ(result[0].key, Bytes({1, 2, 3}));
    ASSERT_EQ(result[0].value, Bytes({4, 5, 6}));
    ASSERT_EQ(result[1].key, Bytes({7, 8, 9}));
    ASSERT_EQ(result[1].value, Bytes({10, 11, 12}));
}

/**
* @tc.name: ConvertToKeys
* @tc.desc: ConvertToKeys test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(KVWatcherTest, ConvertToKeysTest, TestSize.Level0)
{
    std::vector<GeneralWatcher::PRIValue> values = { "key1", 123, "key3", 456, "key5" };
    std::shared_ptr<KVDBWatcher> kvWatcher = std::make_shared<KVDBWatcher>();
    auto result = kvWatcher->ConvertToKeys(values);
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0], "key1");
    ASSERT_EQ(result[1], "key3");
    ASSERT_EQ(result[2], "key5");
}

/**
* @tc.name: UserDelegate
* @tc.desc: UserDelegate test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(DelegateTest, DelegateTest, TestSize.Level0)
{
    std::shared_ptr<UserDelegate> userDelegate = std::make_shared<UserDelegate>();
    auto result = userDelegate->GetLocalUserStatus();
    ASSERT_EQ(result.size(), 0);
    std::string deviceId001 = "";
    result = userDelegate->GetRemoteUserStatus(deviceId001);
    ASSERT_TRUE(deviceId001.empty());
    ASSERT_TRUE(result.empty());
    deviceId001 = DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    result = userDelegate->GetRemoteUserStatus(deviceId001);
    ASSERT_EQ(result.size(), 0);
}

/**
* @tc.name: StringToDbQuery
* @tc.desc: StringToDbQuery test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(QueryHelpTest, StringToDbQueryTest, TestSize.Level0)
{
    std::shared_ptr<QueryHelper> queryHelperImpl = std::make_shared<QueryHelper>();
    bool isSuccess = false;
    std::string query = "";
    auto result = queryHelperImpl->StringToDbQuery(query, isSuccess);
    ASSERT_TRUE(isSuccess);
    std::string querys(5 * 1024, 'a');
    query = "querys" + querys;
    result = queryHelperImpl->StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
    query = "query";
    result = queryHelperImpl->StringToDbQuery(query, isSuccess);
    EXPECT_FALSE(isSuccess);
}

/**
* @tc.name: Handle001
* @tc.desc: Handle test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(QueryHelpTest, Handle001Test, TestSize.Level0)
{
    std::shared_ptr<QueryHelper> queryHelperImpl = std::make_shared<QueryHelper>();
    std::vector<std::string> values = {"query0", "query1", "query2", "query3", "query4"};
    int pointer = 0;
    int end = 1;
    int ends = 4;
    DistributedDB::Query dbQuery;
    EXPECT_FALSE(queryHelperImpl->Handle(values, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleExtra(values, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleEqualTo(values, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleNotEqualTo(values, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleNotEqualTo(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleGreaterThan(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleGreaterThan(values, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleLessThan(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleLessThan(values, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleGreaterThanOrEqualTo(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleGreaterThanOrEqualTo(values, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleLessThanOrEqualTo(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleLessThanOrEqualTo(values, pointer, end, dbQuery));

    pointer = 0;
    values = {"INTEGER", "LONG", "DOUBLE", "STRING"};
    EXPECT_FALSE(queryHelperImpl->Handle(values, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleExtra(values, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleEqualTo(values, pointer, end, dbQuery));
    ASSERT_TRUE(queryHelperImpl->HandleNotEqualTo(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleNotEqualTo(values, pointer, end, dbQuery));
    pointer = 0;
    ASSERT_TRUE(queryHelperImpl->HandleGreaterThan(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleGreaterThan(values, pointer, end, dbQuery));
    pointer = 0;
    ASSERT_TRUE(queryHelperImpl->HandleLessThan(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleLessThan(values, pointer, end, dbQuery));
    pointer = 0;
    ASSERT_TRUE(queryHelperImpl->HandleGreaterThanOrEqualTo(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleGreaterThanOrEqualTo(values, pointer, end, dbQuery));
    pointer = 0;
    ASSERT_TRUE(queryHelperImpl->HandleLessThanOrEqualTo(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleLessThanOrEqualTo(values, pointer, end, dbQuery));
}

/**
* @tc.name: Handle002
* @tc.desc: Handle test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(QueryHelpTest, Handle002Test, TestSize.Level0)
{
    std::shared_ptr<QueryHelper> queryHelperImpl = std::make_shared<QueryHelper>();
    std::vector<std::string> values = {"query0", "query1", "query2", "query3", "query4"};
    int pointer = 1;
    int end = 1;
    int ends = 4;
    DistributedDB::Query dbQuery;
    ASSERT_TRUE(queryHelperImpl->HandleIsNull(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleIsNull(values, pointer, end, dbQuery));
    pointer = 0;
    ASSERT_TRUE(queryHelperImpl->HandleIsNotNull(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleIsNotNull(values, pointer, end, dbQuery));
    pointer = 0;
    ASSERT_TRUE(queryHelperImpl->HandleLike(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleLike(values, pointer, end, dbQuery));
    pointer = 0;
    ASSERT_TRUE(queryHelperImpl->HandleNotLike(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleNotLike(values, pointer, end, dbQuery));
    pointer = 0;
    ASSERT_TRUE(queryHelperImpl->HandleOrderByAsc(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleOrderByAsc(values, pointer, end, dbQuery));
    pointer = 0;
    ASSERT_TRUE(queryHelperImpl->HandleOrderByDesc(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleOrderByDesc(values, pointer, end, dbQuery));
    pointer = 0;
    ASSERT_TRUE(queryHelperImpl->HandleOrderByWriteTime(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleOrderByWriteTime(values, pointer, end, dbQuery));
    pointer = 0;
    ASSERT_TRUE(queryHelperImpl->HandleLimit(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleLimit(values, pointer, end, dbQuery));
    pointer = 0;
    ASSERT_TRUE(queryHelperImpl->HandleKeyPrefix(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleKeyPrefix(values, pointer, end, dbQuery));
}

/**
* @tc.name: Handle003
* @tc.desc: Handle test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(QueryHelpTest, Handle003Test, TestSize.Level0)
{
    std::shared_ptr<QueryHelper> queryHelperImpl = std::make_shared<QueryHelper>();
    std::vector<std::string> values = {"query0", "query1", "query2", "query3", "query4", "query5"};
    std::vector<std::string> values = {"^NOT_IN", "INTEGER", "LONG", "^START", "STRING", "^END"};
    int pointer = 0;
    int end = 1;
    int ends = 5;
    DistributedDB::Query dbQuery;
    EXPECT_FALSE(queryHelperImpl->HandleIn(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleIn(values, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleIn(values, pointer, end, dbQuery));
    ASSERT_TRUE(queryHelperImpl->HandleIn(values, pointer, ends, dbQuery));
    pointer = 0;
    EXPECT_FALSE(queryHelperImpl->HandleNotIn(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleNotIn(values, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleNotIn(values, pointer, end, dbQuery));
    ASSERT_TRUE(queryHelperImpl->HandleNotIn(values, pointer, ends, dbQuery));
}

/**
* @tc.name: Handle004
* @tc.desc: Handle test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(QueryHelpTest, Handle004Test, TestSize.Level0)
{
    std::shared_ptr<QueryHelper> queryHelperImpl = std::make_shared<QueryHelper>();
    std::vector<std::string> values = {"query0", "query1", "query2", "query3", "query4", "query5"};
    std::vector<std::string> values = {"^NOT_IN", "INTEGER", "LONG", "^START", "STRING", "^END"};
    int pointer = 2;
    int end = 3;
    int ends = 5;
    DistributedDB::Query dbQuery;
    EXPECT_FALSE(queryHelperImpl->HandleInKeys(values, pointer, ends, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleInKeys(values, pointer, end, dbQuery));
    EXPECT_FALSE(queryHelperImpl->HandleInKeys(values, pointer, end, dbQuery));
    ASSERT_TRUE(queryHelperImpl->HandleInKeys(values, pointer, ends, dbQuery));
    pointer = 3;
    EXPECT_FALSE(queryHelperImpl->HandleSetSuggestIndex(values, pointer, end, dbQuery));
    ASSERT_TRUE(queryHelperImpl->HandleSetSuggestIndex(values, pointer, ends, dbQuery));
    pointer = 3;
    EXPECT_FALSE(queryHelperImpl->HandleDeviceId(values, pointer, end, dbQuery));
    ASSERT_TRUE(queryHelperImpl->HandleDeviceId(values, pointer, ends, dbQuery));
    queryHelperImpl->hasPrefixKey_ = true;
    pointer = 3;
    ASSERT_TRUE(queryHelperImpl->HandleDeviceId(values, pointer, ends, dbQuery));
}

/**
* @tc.name: StringTo
* @tc.desc: StringTo test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(QueryHelpTest, StringToTest, TestSize.Level0)
{
    std::shared_ptr<QueryHelper> queryHelperImpl = std::make_shared<QueryHelper>();
    std::string values = "true";
    ASSERT_TRUE(queryHelperImpl->StringToBoolean(values));
    values = "false";
    EXPECT_FALSE(queryHelperImpl->StringToBoolean(values));
    values = "BOOL";
    EXPECT_FALSE(queryHelperImpl->StringToBoolean(values));

    values = "^EMPTY_STRING";
    auto result = queryHelperImpl->StringToString(values);
    ASSERT_EQ(result, "");
    values = "START";
    result = queryHelperImpl->StringToString(values);
    ASSERT_EQ(result, "START");
    values = "START^^START";
    result = queryHelperImpl->StringToString(values);
    ASSERT_EQ(result, "START START");
    values = "START(^)START";
    result = queryHelperImpl->StringToString(values);
    ASSERT_EQ(result, "START^START");
}

/**
* @tc.name: Get
* @tc.desc: Get test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(QueryHelpTest, GetTest, TestSize.Level0)
{
    std::shared_ptr<QueryHelper> queryHelperImpl = std::make_shared<QueryHelper>();
    std::vector<std::string> values = {"1", "2", "3", "4", "5", "^END"};
    int elePointer = 0;
    int end = 5;
    std::vector<int> ret = {1, 2, 3, 4, 5};
    auto result = queryHelperImpl->GetIntegerList(values, elePointer, end);
    ASSERT_EQ(result, ret);
    elePointer = 6;
    ret = {};
    result = queryHelperImpl->GetIntegerList(values, elePointer, end);
    ASSERT_EQ(result, ret);

    elePointer = 0;
    std::vector<int64_t> ret1 = {1, 2, 3, 4, 5};
    auto result1 = queryHelperImpl->GetLongList(values, elePointer, end);
    ASSERT_EQ(result1, ret1);
    elePointer = 6;
    ret1 = {};
    result1 = queryHelperImpl->GetLongList(values, elePointer, end);
    ASSERT_EQ(result1, ret1);

    elePointer = 0;
    std::vector<double> ret2 = {1, 2, 3, 4, 5};
    auto result2 = queryHelperImpl->GetDoubleList(values, elePointer, end);
    ASSERT_EQ(result2, ret2);
    elePointer = 6;
    ret2 = {};
    result2 = queryHelperImpl->GetDoubleList(values, elePointer, end);
    ASSERT_EQ(result2, ret2);

    std::vector<std::string> values1 = {"^NOT_IN", "INTEGER", "LONG", "^START", "STRING", "^END"};
    elePointer = 0;
    std::vector<std::string> ret3 = { "^NOT_IN", "INTEGER", "LONG", "^START", "STRING" };
    auto result3 = queryHelperImpl->GetStringList(values1, elePointer, end);
    ASSERT_EQ(result3, ret3);
    elePointer = 6;
    ret3 = {};
    result3 = queryHelperImpl->GetStringList(values1, elePointer, end);
    ASSERT_EQ(result3, ret3);
}

/**
* @tc.name: AuthHandler
* @tc.desc: AuthHandler test the return result of input with different values.
* @tc.type: FUNC
* @tc.author: SQL
*/
HWTEST_F(AuthHandleTest, AuthHandleTest, TestSize.Level0)
{
    int userId = 0;
    int peerUserId = 0;
    std::string peerDeviceId = "";
    AclParams aclParams;
    aclParams.authType = static_cast<int32_t>(DistributedKv::AuthType::IDENTICAL_ACCOUNT);
    auto result = AuthDelegate::GetInstance()->CheckAccess(userId, peerUserId, peerDeviceId, aclParams);
    ASSERT_TRUE(result.first);
    
    aclParams.authType = static_cast<int32_t>(DistributedKv::AuthType::DEFAULT);
    result = AuthDelegate::GetInstance()->CheckAccess(userId, peerUserId, peerDeviceId, aclParams);
    ASSERT_TRUE(result.first);

    aclParams.authType = static_cast<int32_t>(DistributedKv::AuthType::IDENTICAL_ACCOUNT);
    peerDeviceId = "peerDeviceId";
    result = AuthDelegate::GetInstance()->CheckAccess(userId, peerUserId, peerDeviceId, aclParams);
    ASSERT_TRUE(result.first);

    aclParams.authType = static_cast<int32_t>(DistributedKv::AuthType::DEFAULT);
    result = AuthDelegate::GetInstance()->CheckAccess(userId, peerUserId, peerDeviceId, aclParams);
    ASSERT_TRUE(result.first);

    userId = 1;
    result = AuthDelegate::GetInstance()->CheckAccess(userId, peerUserId, peerDeviceId, aclParams);
    EXPECT_FALSE(result.first);

    peerUserId = 1;
    result = AuthDelegate::GetInstance()->CheckAccess(userId, peerUserId, peerDeviceId, aclParams);
    EXPECT_FALSE(result.first);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test