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
#define LOG_TAG "RdbGeneralStoreTest"

#include "rdb_general_store.h"

#include <random>
#include <thread>

#include "bootstrap.h"
#include "cloud/schema_meta.h"
#include "gtest/gtest.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "mock/general_watcher_mock.h"
#include "rdb_query.h"
#include "store/general_store.h"
#include "types.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedRdb;
using DBStatus = DistributedDB::DBStatus;
using StoreMetaData = OHOS::DistributedData::StoreMetaData;
RdbGeneralStore::Values g_RdbValues = { { "0000000" }, { true }, { int64_t(100) }, { double(100) }, { int64_t(1) },
    { Bytes({ 1, 2, 3, 4 }) } };
RdbGeneralStore::VBucket g_RdbVBucket = { { "#gid", { "0000000" } }, { "#flag", { true } },
    { "#value", { int64_t(100) } }, { "#float", { double(100) } } };
bool g_testResult = false;
namespace OHOS::Test {
namespace DistributedRDBTest {
static constexpr uint32_t PRINT_ERROR_CNT = 150;
static constexpr const char *BUNDLE_NAME = "test_rdb_general_store";
static constexpr const char *STORE_NAME = "test_service_rdb";
class RdbGeneralStoreTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp()
    {
        Bootstrap::GetInstance().LoadDirectory();
        InitMetaData();
    };
    void TearDown()
    {
        g_testResult = false;
    };

protected:
    void InitMetaData();
    StoreMetaData metaData_;
};

void RdbGeneralStoreTest::InitMetaData()
{
    metaData_.bundleName = BUNDLE_NAME;
    metaData_.appId = BUNDLE_NAME;
    metaData_.user = "0";
    metaData_.area = DistributedKv::Area::EL1;
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = 1;
    metaData_.storeId = STORE_NAME;
    metaData_.dataDir = "/data/service/el1/public/database/" + std::string(BUNDLE_NAME) + "/rdb";
    metaData_.securityLevel = DistributedKv::SecurityLevel::S2;
}

class MockRelationalStoreDelegate : public DistributedDB::RelationalStoreDelegate {
public:
    ~MockRelationalStoreDelegate() = default;

    DBStatus Sync(const std::vector<std::string> &devices, DistributedDB::SyncMode mode, const Query &query,
        const SyncStatusCallback &onComplete, bool wait) override
    {
        return DBStatus::OK;
    }

    int32_t GetCloudSyncTaskCount() override
    {
        static int32_t count = 0;
        count = (count + 1) % 2; // The result of count + 1 is the remainder of 2.
        return count;
    }

    DBStatus RemoveDeviceData(const std::string &device, const std::string &tableName) override
    {
        return DBStatus::OK;
    }

    DBStatus RemoteQuery(const std::string &device, const RemoteCondition &condition, uint64_t timeout,
        std::shared_ptr<ResultSet> &result) override
    {
        if (device == "test") {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus RemoveDeviceData() override
    {
        return DBStatus::OK;
    }

    DBStatus Sync(const std::vector<std::string> &devices, DistributedDB::SyncMode mode, const Query &query,
        const SyncProcessCallback &onProcess, int64_t waitTime) override
    {
        return DBStatus::OK;
    }

    DBStatus SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDb) override
    {
        return DBStatus::OK;
    }

    DBStatus SetCloudDbSchema(const DataBaseSchema &schema) override
    {
        return DBStatus::OK;
    }

    DBStatus RegisterObserver(StoreObserver *observer) override
    {
        return DBStatus::OK;
    }

    DBStatus UnRegisterObserver() override
    {
        return DBStatus::OK;
    }

    DBStatus UnRegisterObserver(StoreObserver *observer) override
    {
        return DBStatus::OK;
    }

    DBStatus SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader) override
    {
        return DBStatus::OK;
    }

    DBStatus Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess) override
    {
        return DBStatus::OK;
    }

    DBStatus SetTrackerTable(const TrackerSchema &schema) override
    {
        if (schema.tableName == "WITH_INVENTORY_DATA") {
            return DBStatus::WITH_INVENTORY_DATA;
        }
        if (schema.tableName == "test") {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus ExecuteSql(const SqlCondition &condition, std::vector<DistributedDB::VBucket> &records) override
    {
        if (condition.sql == "") {
            return DBStatus::DB_ERROR;
        }

        std::string sqls = "INSERT INTO test ( #flag, #float, #gid, #value) VALUES  ( ?, ?, ?, ?)";
        std::string sqlIn = " UPDATE test SET setSql WHERE whereSql";
        std::string sql = "REPLACE INTO test ( #flag, #float, #gid, #value) VALUES  ( ?, ?, ?, ?)";
        if (condition.sql == sqls || condition.sql == sqlIn || condition.sql == sql) {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus SetReference(const std::vector<TableReferenceProperty> &tableReferenceProperty) override
    {
        if (g_testResult) {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus CleanTrackerData(const std::string &tableName, int64_t cursor) override
    {
        return DBStatus::OK;
    }

    DBStatus Pragma(PragmaCmd cmd, PragmaData &pragmaData) override
    {
        return DBStatus::OK;
    }

    DBStatus UpsertData(const std::string &tableName, const std::vector<DistributedDB::VBucket> &records,
        RecordStatus status = RecordStatus::WAIT_COMPENSATED_SYNC) override
    {
        return DBStatus::OK;
    }

    DBStatus SetCloudSyncConfig(const CloudSyncConfig &config) override
    {
        return DBStatus::OK;
    }

protected:
    DBStatus RemoveDeviceDataInner(const std::string &device, ClearMode mode) override
    {
        if (g_testResult) {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus CreateDistributedTableInner(const std::string &tableName, TableSyncType type) override
    {
        if (tableName == "test") {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }
};

class MockStoreChangedData : public DistributedDB::StoreChangedData {
public:
    std::string GetDataChangeDevice() const override
    {
        return "DataChangeDevice";
    }

    void GetStoreProperty(DistributedDB::StoreProperty &storeProperty) const override
    {
        return;
    }
};

/**
* @tc.name: BindSnapshots001
* @tc.desc: RdbGeneralStore BindSnapshots test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, BindSnapshots001, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    BindAssets bindAssets;
    auto result = store->BindSnapshots(bindAssets.bindAssets);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: BindSnapshots002
* @tc.desc: RdbGeneralStore BindSnapshots nullptr test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, BindSnapshots002, TestSize.Level1)
{
    DistributedData::StoreMetaData meta;
    meta = metaData_;
    meta.isEncrypt = true;
    auto store = new (std::nothrow) RdbGeneralStore(meta);
    ASSERT_NE(store, nullptr);
    store->snapshots_.bindAssets = nullptr;
    BindAssets bindAssets;
    auto result = store->BindSnapshots(bindAssets.bindAssets);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Bind001
* @tc.desc: RdbGeneralStore Bind bindInfo test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Bind001, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    DistributedData::Database database;
    GeneralStore::CloudConfig config;
    std::map<uint32_t, DistributedData::GeneralStore::BindInfo> bindInfos;
    auto result = store->Bind(database, bindInfos, config);
    EXPECT_TRUE(bindInfos.empty());
    EXPECT_EQ(result, GeneralError::E_OK);

    std::shared_ptr<CloudDB> db;
    std::shared_ptr<AssetLoader> loader;
    GeneralStore::BindInfo bindInfo(db, loader);
    EXPECT_EQ(bindInfo.db_, nullptr);
    EXPECT_EQ(bindInfo.loader_, nullptr);
    uint32_t key = 1;
    bindInfos[key] = bindInfo;
    result = store->Bind(database, bindInfos, config);
    EXPECT_TRUE(!bindInfos.empty());
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    std::shared_ptr<CloudDB> dbs = std::make_shared<CloudDB>();
    std::shared_ptr<AssetLoader> loaders = std::make_shared<AssetLoader>();
    GeneralStore::BindInfo bindInfo1(dbs, loader);
    EXPECT_NE(bindInfo1.db_, nullptr);
    bindInfos[key] = bindInfo1;
    result = store->Bind(database, bindInfos, config);
    EXPECT_TRUE(!bindInfos.empty());
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    GeneralStore::BindInfo bindInfo2(db, loaders);
    EXPECT_NE(bindInfo2.loader_, nullptr);
    bindInfos[key] = bindInfo2;
    result = store->Bind(database, bindInfos, config);
    EXPECT_TRUE(!bindInfos.empty());
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);
}

/**
* @tc.name: Bind002
* @tc.desc: RdbGeneralStore Bind delegate_ is nullptr test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Bind002, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    DistributedData::Database database;
    std::map<uint32_t, DistributedData::GeneralStore::BindInfo> bindInfos;

    std::shared_ptr<CloudDB> db = std::make_shared<CloudDB>();
    std::shared_ptr<AssetLoader> loader = std::make_shared<AssetLoader>();
    GeneralStore::BindInfo bindInfo(db, loader);
    uint32_t key = 1;
    bindInfos[key] = bindInfo;
    GeneralStore::CloudConfig config;
    auto result = store->Bind(database, bindInfos, config);
    EXPECT_EQ(store->delegate_, nullptr);
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);

    store->isBound_ = true;
    result = store->Bind(database, bindInfos, config);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Bind003
* @tc.desc: RdbGeneralStore Bind delegate_ test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Bind003, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    DistributedData::Database database;
    std::map<uint32_t, DistributedData::GeneralStore::BindInfo> bindInfos;

    std::shared_ptr<CloudDB> db = std::make_shared<CloudDB>();
    std::shared_ptr<AssetLoader> loader = std::make_shared<AssetLoader>();
    GeneralStore::BindInfo bindInfo(db, loader);
    uint32_t key = 1;
    bindInfos[key] = bindInfo;
    MockRelationalStoreDelegate mockDelegate;
    store->delegate_ = &mockDelegate;
    GeneralStore::CloudConfig config;
    auto result = store->Bind(database, bindInfos, config);
    EXPECT_NE(store->delegate_, nullptr);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Close
* @tc.desc: RdbGeneralStore Close and IsBound function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Close, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    auto result = store->IsBound();
    EXPECT_EQ(result, false);
    EXPECT_EQ(store->delegate_, nullptr);
    auto ret = store->Close();
    EXPECT_EQ(ret, GeneralError::E_OK);

    MockRelationalStoreDelegate mockDelegate;
    store->delegate_ = &mockDelegate;
    ret = store->Close();
    EXPECT_EQ(ret, GeneralError::E_BUSY);
}

/**
* @tc.name: Close
* @tc.desc: RdbGeneralStore Close test
* @tc.type: FUNC
* @tc.require:
* @tc.author: shaoyuanzhao
*/
HWTEST_F(RdbGeneralStoreTest, BusyClose, TestSize.Level1)
{
    auto store = std::make_shared<RdbGeneralStore>(metaData_);
    ASSERT_NE(store, nullptr);
    std::thread thread([store]() {
        std::unique_lock<decltype(store->rwMutex_)> lock(store->rwMutex_);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto ret = store->Close();
    EXPECT_EQ(ret, GeneralError::E_BUSY);
    thread.join();
    ret = store->Close();
    EXPECT_EQ(ret, GeneralError::E_OK);
}

/**
* @tc.name: Execute
* @tc.desc: RdbGeneralStore Execute function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Execute, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string table = "table";
    std::string sql = "sql";
    EXPECT_EQ(store->delegate_, nullptr);
    auto result = store->Execute(table, sql);
    EXPECT_EQ(result, GeneralError::E_ERROR);

    MockRelationalStoreDelegate mockDelegate;
    store->delegate_ = &mockDelegate;
    result = store->Execute(table, sql);
    EXPECT_EQ(result, GeneralError::E_OK);

    std::string null = "";
    result = store->Execute(table, null);
    EXPECT_EQ(result, GeneralError::E_ERROR);
}

/**
* @tc.name: SqlConcatenate
* @tc.desc: RdbGeneralStore SqlConcatenate function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, SqlConcatenate, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    DistributedData::VBucket value;
    std::string strColumnSql = "strColumnSql";
    std::string strRowValueSql = "strRowValueSql";
    auto result = store->SqlConcatenate(value, strColumnSql, strRowValueSql);
    size_t columnSize = value.size();
    EXPECT_EQ(columnSize, 0);
    EXPECT_EQ(result, columnSize);

    DistributedData::VBucket values = g_RdbVBucket;
    result = store->SqlConcatenate(values, strColumnSql, strRowValueSql);
    columnSize = values.size();
    EXPECT_NE(columnSize, 0);
    EXPECT_EQ(result, columnSize);
}

/**
* @tc.name: Insert001
* @tc.desc: RdbGeneralStore Insert error test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Insert001, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    DistributedData::VBuckets values;
    EXPECT_EQ(values.size(), 0);
    std::string table = "table";
    auto result = store->Insert("", std::move(values));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);
    result = store->Insert(table, std::move(values));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    DistributedData::VBuckets extends = { { { "#gid", { "0000000" } }, { "#flag", { true } },
                                              { "#value", { int64_t(100) } }, { "#float", { double(100) } } },
        { { "#gid", { "0000001" } } } };
    result = store->Insert("", std::move(extends));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    DistributedData::VBucket value;
    DistributedData::VBuckets vbuckets = { value };
    result = store->Insert(table, std::move(vbuckets));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store->Insert(table, std::move(extends));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);
}

/**
* @tc.name: Insert002
* @tc.desc: RdbGeneralStore Insert function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Insert002, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string table = "table";
    DistributedData::VBuckets extends = { { g_RdbVBucket } };
    auto result = store->Insert(table, std::move(extends));
    EXPECT_EQ(result, GeneralError::E_ERROR);

    MockRelationalStoreDelegate mockDelegate;
    store->delegate_ = &mockDelegate;
    result = store->Insert(table, std::move(extends));
    EXPECT_EQ(result, GeneralError::E_OK);

    std::string test = "test";
    result = store->Insert(test, std::move(extends));
    EXPECT_EQ(result, GeneralError::E_ERROR);

    result = store->Insert(test, std::move(extends));
    EXPECT_EQ(result, GeneralError::E_ERROR);

    for (size_t i = 0; i < PRINT_ERROR_CNT + 1; i++) {
        result = store->Insert(test, std::move(extends));
        EXPECT_EQ(result, GeneralError::E_ERROR);
    }
}

/**
* @tc.name: Update
* @tc.desc: RdbGeneralStore Update function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Update, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string table = "table";
    std::string setSql = "setSql";
    RdbGeneralStore::Values values;
    std::string whereSql = "whereSql";
    RdbGeneralStore::Values conditions;
    auto result = store->Update("", setSql, std::move(values), whereSql, std::move(conditions));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store->Update(table, "", std::move(values), whereSql, std::move(conditions));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store->Update(table, setSql, std::move(values), whereSql, std::move(conditions));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store->Update(table, setSql, std::move(g_RdbValues), "", std::move(conditions));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store->Update(table, setSql, std::move(g_RdbValues), whereSql, std::move(conditions));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store->Update(table, setSql, std::move(g_RdbValues), whereSql, std::move(g_RdbValues));
    EXPECT_EQ(result, GeneralError::E_ERROR);

    MockRelationalStoreDelegate mockDelegate;
    store->delegate_ = &mockDelegate;
    result = store->Update(table, setSql, std::move(g_RdbValues), whereSql, std::move(g_RdbValues));
    EXPECT_EQ(result, GeneralError::E_OK);

    result = store->Update("test", setSql, std::move(g_RdbValues), whereSql, std::move(g_RdbValues));
    EXPECT_EQ(result, GeneralError::E_ERROR);
}

/**
* @tc.name: Replace
* @tc.desc: RdbGeneralStore Replace function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Replace, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string table = "table";
    RdbGeneralStore::VBucket values;
    auto result = store->Replace("", std::move(g_RdbVBucket));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store->Replace(table, std::move(values));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store->Replace(table, std::move(g_RdbVBucket));
    EXPECT_EQ(result, GeneralError::E_ERROR);

    MockRelationalStoreDelegate mockDelegate;
    store->delegate_ = &mockDelegate;
    result = store->Replace(table, std::move(g_RdbVBucket));
    EXPECT_EQ(result, GeneralError::E_OK);

    result = store->Replace("test", std::move(g_RdbVBucket));
    EXPECT_EQ(result, GeneralError::E_ERROR);
}

/**
* @tc.name: Delete
* @tc.desc: RdbGeneralStore Delete function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Delete, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string table = "table";
    std::string sql = "sql";
    auto result = store->Delete(table, sql, std::move(g_RdbValues));
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Query001
* @tc.desc: RdbGeneralStore Query function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Query001, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string table = "table";
    std::string sql = "sql";
    auto [err1, result1] = store->Query(table, sql, std::move(g_RdbValues));
    EXPECT_EQ(err1, GeneralError::E_ALREADY_CLOSED);
    EXPECT_EQ(result1, nullptr);

    MockRelationalStoreDelegate mockDelegate;
    store->delegate_ = &mockDelegate;
    auto [err2, result2] = store->Query(table, sql, std::move(g_RdbValues));
    EXPECT_EQ(err2, GeneralError::E_OK);
    EXPECT_NE(result2, nullptr);
}

/**
* @tc.name: Query002
* @tc.desc: RdbGeneralStore Query function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Query002, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string table = "table";
    std::string sql = "sql";
    MockQuery query;
    auto [err1, result1] = store->Query(table, query);
    EXPECT_EQ(err1, GeneralError::E_INVALID_ARGS);
    EXPECT_EQ(result1, nullptr);

    query.lastResult = true;
    auto [err2, result2] = store->Query(table, query);
    EXPECT_EQ(err2, GeneralError::E_ALREADY_CLOSED);
    EXPECT_EQ(result2, nullptr);
}

/**
* @tc.name: MergeMigratedData
* @tc.desc: RdbGeneralStore MergeMigratedData function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, MergeMigratedData, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string tableName = "tableName";
    DistributedData::VBuckets extends = { { g_RdbVBucket } };
    auto result = store->MergeMigratedData(tableName, std::move(extends));
    EXPECT_EQ(result, GeneralError::E_ERROR);

    MockRelationalStoreDelegate mockDelegate;
    store->delegate_ = &mockDelegate;
    result = store->MergeMigratedData(tableName, std::move(extends));
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Sync
* @tc.desc: RdbGeneralStore Sync function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Sync, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    GeneralStore::Devices devices;
    MockQuery query;
    GeneralStore::DetailAsync async;
    SyncParam syncParam;
    auto result = store->Sync(devices, query, async, syncParam);
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);

    MockRelationalStoreDelegate mockDelegate;
    store->delegate_ = &mockDelegate;
    result = store->Sync(devices, query, async, syncParam);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: PreSharing
* @tc.desc: RdbGeneralStore PreSharing function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, PreSharing, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    MockQuery query;
    auto [errCode, result] = store->PreSharing(query);
    EXPECT_NE(errCode, GeneralError::E_OK);
    EXPECT_EQ(result, nullptr);
}

/**
* @tc.name: ExtractExtend
* @tc.desc: RdbGeneralStore ExtractExtend function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, ExtractExtend, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    RdbGeneralStore::VBucket extend = { { "#gid", { "0000000" } }, { "#flag", { true } },
        { "#value", { int64_t(100) } }, { "#float", { double(100) } }, { "#cloud_gid", { "cloud_gid" } } };
    DistributedData::VBuckets extends = { { extend } };
    auto result = store->ExtractExtend(extends);
    EXPECT_EQ(result.size(), extends.size());
    DistributedData::VBuckets values;
    result = store->ExtractExtend(values);
    EXPECT_EQ(result.size(), values.size());
}

/**
* @tc.name: Clean
* @tc.desc: RdbGeneralStore Clean function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Clean, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string tableName = "tableName";
    std::vector<std::string> devices = { "device1", "device2" };
    auto result = store->Clean(devices, -1, tableName);
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);
    result = store->Clean(devices, GeneralStore::CLEAN_MODE_BUTT + 1, tableName);
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);
    result = store->Clean(devices, GeneralStore::CLOUD_INFO, tableName);
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);

    MockRelationalStoreDelegate mockDelegate;
    store->delegate_ = &mockDelegate;
    result = store->Clean(devices, GeneralStore::CLOUD_INFO, tableName);
    EXPECT_EQ(result, GeneralError::E_OK);
    result = store->Clean(devices, GeneralStore::CLOUD_DATA, tableName);
    EXPECT_EQ(result, GeneralError::E_OK);
    std::vector<std::string> devices1;
    result = store->Clean(devices1, GeneralStore::NEARBY_DATA, tableName);
    EXPECT_EQ(result, GeneralError::E_OK);

    g_testResult = true;
    result = store->Clean(devices, GeneralStore::CLOUD_INFO, tableName);
    EXPECT_EQ(result, GeneralError::E_ERROR);
    result = store->Clean(devices, GeneralStore::CLOUD_DATA, tableName);
    EXPECT_EQ(result, GeneralError::E_ERROR);
    result = store->Clean(devices, GeneralStore::CLEAN_MODE_BUTT, tableName);
    EXPECT_EQ(result, GeneralError::E_ERROR);
    result = store->Clean(devices, GeneralStore::NEARBY_DATA, tableName);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Watch
* @tc.desc: RdbGeneralStore Watch and Unwatch function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Watch, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    MockGeneralWatcher watcher;
    auto result = store->Watch(GeneralWatcher::Origin::ORIGIN_CLOUD, watcher);
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);
    result = store->Unwatch(GeneralWatcher::Origin::ORIGIN_CLOUD, watcher);
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_OK);
    result = store->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_OK);
    result = store->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);
}

/**
* @tc.name: OnChange
* @tc.desc: RdbGeneralStore OnChange function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, OnChange, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    MockGeneralWatcher watcher;
    MockStoreChangedData data;
    DistributedDB::ChangedData changedData;
    store->observer_.OnChange(data);
    store->observer_.OnChange(DistributedDB::Origin::ORIGIN_CLOUD, "originalId", std::move(changedData));
    auto result = store->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_OK);
    store->observer_.OnChange(data);
    store->observer_.OnChange(DistributedDB::Origin::ORIGIN_CLOUD, "originalId", std::move(changedData));
    result = store->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Release
* @tc.desc: RdbGeneralStore Release and AddRef function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, Release, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    auto result = store->Release();
    EXPECT_EQ(result, 0);
    store = new (std::nothrow) RdbGeneralStore(metaData_);
    store->ref_ = 0;
    result = store->Release();
    EXPECT_EQ(result, 0);
    store->ref_ = 2;
    result = store->Release();
    EXPECT_EQ(result, 1);

    result = store->AddRef();
    EXPECT_EQ(result, 2);
    store->ref_ = 0;
    result = store->AddRef();
    EXPECT_EQ(result, 0);
}

/**
* @tc.name: SetDistributedTables
* @tc.desc: RdbGeneralStore SetDistributedTables function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, SetDistributedTables, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::vector<std::string> tables = { "table1", "table2" };
    int32_t type = 0;
    std::vector<DistributedData::Reference> references;
    auto result = store->SetDistributedTables(tables, type, references);
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);

    MockRelationalStoreDelegate mockDelegate;
    store->delegate_ = &mockDelegate;
    result = store->SetDistributedTables(tables, type, references);
    EXPECT_EQ(result, GeneralError::E_OK);

    std::vector<std::string> test = { "test" };
    result = store->SetDistributedTables(test, type, references);
    EXPECT_EQ(result, GeneralError::E_ERROR);
    g_testResult = true;
    result = store->SetDistributedTables(tables, type, references);
    EXPECT_EQ(result, GeneralError::E_ERROR);
}

/**
* @tc.name: SetTrackerTable
* @tc.desc: RdbGeneralStore SetTrackerTable function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, SetTrackerTable, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string tableName = "tableName";
    std::set<std::string> trackerColNames = { "col1", "col2" };
    std::set<std::string> extendColNames = { "extendColName1", "extendColName2" };
    auto result = store->SetTrackerTable(tableName, trackerColNames, extendColNames);
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);

    MockRelationalStoreDelegate mockDelegate;
    store->delegate_ = &mockDelegate;
    result = store->SetTrackerTable(tableName, trackerColNames, extendColNames);
    EXPECT_EQ(result, GeneralError::E_OK);
    result = store->SetTrackerTable("WITH_INVENTORY_DATA", trackerColNames, extendColNames);
    EXPECT_EQ(result, GeneralError::E_WITH_INVENTORY_DATA);
    result = store->SetTrackerTable("test", trackerColNames, extendColNames);
    EXPECT_EQ(result, GeneralError::E_ERROR);
}

/**
* @tc.name: RemoteQuery
* @tc.desc: RdbGeneralStore RemoteQuery function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, RemoteQuery, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string device = "device";
    DistributedDB::RemoteCondition remoteCondition;
    MockRelationalStoreDelegate mockDelegate;
    store->delegate_ = &mockDelegate;
    auto result = store->RemoteQuery("test", remoteCondition);
    EXPECT_EQ(result, nullptr);
    result = store->RemoteQuery(device, remoteCondition);
    EXPECT_NE(result, nullptr);
}

/**
* @tc.name: ConvertStatus
* @tc.desc: RdbGeneralStore ConvertStatus function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, ConvertStatus, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    auto result = store->ConvertStatus(DBStatus::OK);
    EXPECT_EQ(result, GeneralError::E_OK);
    result = store->ConvertStatus(DBStatus::CLOUD_NETWORK_ERROR);
    EXPECT_EQ(result, GeneralError::E_NETWORK_ERROR);
    result = store->ConvertStatus(DBStatus::CLOUD_LOCK_ERROR);
    EXPECT_EQ(result, GeneralError::E_LOCKED_BY_OTHERS);
    result = store->ConvertStatus(DBStatus::CLOUD_FULL_RECORDS);
    EXPECT_EQ(result, GeneralError::E_RECODE_LIMIT_EXCEEDED);
    result = store->ConvertStatus(DBStatus::CLOUD_ASSET_SPACE_INSUFFICIENT);
    EXPECT_EQ(result, GeneralError::E_NO_SPACE_FOR_ASSET);
    result = store->ConvertStatus(DBStatus::BUSY);
    EXPECT_EQ(result, GeneralError::E_BUSY);
    result = store->ConvertStatus(DBStatus::DB_ERROR);
    EXPECT_EQ(result, GeneralError::E_ERROR);
}

/**
* @tc.name: QuerySql
* @tc.desc: RdbGeneralStore QuerySql function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, QuerySql, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    MockRelationalStoreDelegate mockDelegate;
    store->delegate_ = &mockDelegate;
    auto [err1, result1] = store->QuerySql("", std::move(g_RdbValues));
    EXPECT_EQ(err1, GeneralError::E_ERROR);
    EXPECT_TRUE(result1.empty());

    auto [err2, result2] = store->QuerySql("sql", std::move(g_RdbValues));
    EXPECT_EQ(err1, GeneralError::E_ERROR);
    EXPECT_TRUE(result2.empty());
}

/**
* @tc.name: BuildSqlWhenCloumnEmpty
* @tc.desc: test buildsql method when cloumn empty
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, BuildSqlWhenCloumnEmpty, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string table = "mock_table";
    std::string statement = "mock_statement";
    std::vector<std::string> columns;
    std::string expectSql = "select cloud_gid from naturalbase_rdb_aux_mock_table_log, (select rowid from "
                            "mock_tablemock_statement) where data_key = rowid";
    std::string resultSql = store->BuildSql(table, statement, columns);
    EXPECT_EQ(resultSql, expectSql);
}

/**
* @tc.name: BuildSqlWhenParamValid
* @tc.desc: test buildsql method when param valid
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, BuildSqlWhenParamValid, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string table = "mock_table";
    std::string statement = "mock_statement";
    std::vector<std::string> columns;
    columns.push_back("mock_column_1");
    columns.push_back("mock_column_2");
    std::string expectSql = "select cloud_gid, mock_column_1, mock_column_2 from naturalbase_rdb_aux_mock_table_log, "
                            "(select rowid, mock_column_1, mock_column_2 from mock_tablemock_statement) where "
                            "data_key = rowid";
    std::string resultSql = store->BuildSql(table, statement, columns);
    EXPECT_EQ(resultSql, expectSql);
}

/**
* @tc.name: GetWaterVersionTest
* @tc.desc: GetWaterVersion test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, GetWaterVersionTest, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string deviceId = "";
    std::vector<std::string> expected = {};
    std::vector<std::string> actual = store->GetWaterVersion(deviceId);
    EXPECT_EQ(expected, actual);
    deviceId = "mock_deviceId";
    actual = store->GetWaterVersion(deviceId);
    EXPECT_EQ(expected, actual);
}

/**
* @tc.name: LockAndUnLockCloudDBTest
* @tc.desc: lock and unlock cloudDB test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, LockAndUnLockCloudDBTest, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    auto result = store->LockCloudDB();
    EXPECT_EQ(result.first, 1);
    EXPECT_EQ(result.second, 0);
    auto unlockResult = store->UnLockCloudDB();
    EXPECT_EQ(unlockResult, 1);
}

/**
* @tc.name: InFinishedTest
* @tc.desc: isFinished test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, InFinishedTest, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    DistributedRdb::RdbGeneralStore::SyncId syncId = 1;
    bool isFinished = store->IsFinished(syncId);
    EXPECT_TRUE(isFinished);
}

/**
* @tc.name: GetRdbCloudTest
* @tc.desc: getRdbCloud test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, GetRdbCloudTest, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    auto rdbCloud = store->GetRdbCloud();
    EXPECT_EQ(rdbCloud, nullptr);
}

/**
* @tc.name: RegisterDetailProgressObserverTest
* @tc.desc: RegisterDetailProgressObserver test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, RegisterDetailProgressObserverTest, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    DistributedData::GeneralStore::DetailAsync async;
    auto result = store->RegisterDetailProgressObserver(async);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: GetFinishTaskTest
* @tc.desc: GetFinishTask test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, GetFinishTaskTest, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    DistributedRdb::RdbGeneralStore::SyncId syncId = 1;
    auto result = store->GetFinishTask(syncId);
    ASSERT_NE(result, nullptr);
}

/**
* @tc.name: GetCBTest
* @tc.desc: GetCB test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbGeneralStoreTest, GetCBTest, TestSize.Level1)
{
    auto store = new (std::nothrow) RdbGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    DistributedRdb::RdbGeneralStore::SyncId syncId = 1;
    auto result = store->GetCB(syncId);
    ASSERT_NE(result, nullptr);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test