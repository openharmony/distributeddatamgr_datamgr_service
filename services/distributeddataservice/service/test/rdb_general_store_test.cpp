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
#include "error/general_error.h"
#include "errors.h"
#include "eventcenter/event_center.h"
#include "gtest/gtest.h"
#include "metadata/store_meta_data.h"
#include "mock/general_watcher_mock.h"
#include "rdb_query.h"
#include "rdb_helper.h"
#include "relational_store_delegate_mock.h"
#include "store/general_store.h"
#include "store/general_value.h"
#include "store_observer.h"
#include "store_types.h"
#include "types.h"
#include "value_proxy.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedRdb;
using DBStatus = DistributedDB::DBStatus;
RdbGeneralStore::Values g_RdbValues = { { "0000000" }, { true }, { int64_t(100) }, { double(100) }, { int64_t(1) },
    { Bytes({ 1, 2, 3, 4 }) } };
RdbGeneralStore::VBucket g_RdbVBucket = { { "#gid", { "0000000" } }, { "#flag", { true } },
    { "#value", { int64_t(100) } }, { "#float", { double(100) } } };
namespace OHOS::Test {
namespace DistributedRDBTest {
using StoreMetaData = OHOS::DistributedData::StoreMetaData;
using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
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
        mkdir(metaData_.dataDir.c_str(), 0771);
        store_ = std::make_shared<RdbGeneralStore>(metaData_);
        ASSERT_NE(store_, nullptr);
    };
    void TearDown()
    {
    };

protected:
    void InitMetaData();
    static StoreMetaData GetStoreMeta(const std::string &storeName, int32_t area = 1, int32_t user = 0);
    static int32_t CreateTable(std::shared_ptr<RdbGeneralStore> store, const std::string &tableName = "employee");
    static bool Equal(const VBucket &left, const VBucket &right);
    static bool CorruptDatabaseFile(const std::string &filePath, int32_t offset = 0, int32_t size = 0);
    static void CorruptDatabasePager(std::shared_ptr<RdbGeneralStore> store, const std::string &filePath, const std::string &tableName);
    StoreMetaData metaData_;
    std::shared_ptr<RdbGeneralStore> store_;
};

class RdbGeneralQuery : public GenQuery {
public:
    static constexpr uint64_t TYPE_ID = 0x90000001;
    ~RdbGeneralQuery() override = default;
    bool IsEqual(uint64_t tid) override
    {
        return TYPE_ID == tid;
    }
    std::vector<std::string> GetTables() override
    {
        return { table };
    }
    std::vector<Value> GetArgs() override
    {
        return args;
    }
    std::string GetWhereClause() override
    {
        return whereClause;
    }
    std::string table;
    std::string whereClause;
    std::vector<Value> args;
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
    metaData_.isSearchable = true;
}

StoreMetaData RdbGeneralStoreTest::GetStoreMeta(const std::string &storeName, int32_t area, int32_t user)
{
    StoreMetaData metaData;
    metaData.bundleName = BUNDLE_NAME;
    metaData.appId = BUNDLE_NAME;
    metaData.user = std::to_string(user);
    metaData.area = area;
    metaData.instanceId = 0;
    metaData.isAutoSync = true;
    metaData.storeType = 1;
    metaData.storeId = storeName;
    metaData.dataDir = "/data/service/el" + std::to_string(area) + "/public/database/" + std::string(BUNDLE_NAME) +
                       "/rdb/" + storeName;
    metaData.securityLevel = DistributedKv::SecurityLevel::S2;
    metaData.isSearchable = true;
    return metaData;
}

int32_t RdbGeneralStoreTest::CreateTable(std::shared_ptr<RdbGeneralStore> store, const string &tableName)
{
    if (store == nullptr) {
        return -1;
    }

    std::string sql = "CREATE TABLE IF NOT EXISTS ";
    sql += tableName;
    sql += "("
           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "name TEXT NOT NULL,"
           "age INTEGER,"
           "salary REAL,"
           "data BLOB)";
    DistributedData::Values values;
    auto [code, value] = store->Execute(sql, std::move(values));
    return code;
}

bool RdbGeneralStoreTest::Equal(const VBucket &left, const VBucket &right)
{
    if (left.size() != right.size()) {
        return false;
    }
    for (auto lIt = left.begin(), rIt = right.begin(); lIt != left.end() && rIt != right.end(); lIt++, rIt++) {
        if (lIt->first != rIt->first) {
            return false;
        }
        auto &lValue = lIt->second;
        auto &rValue = rIt->second;
        if (lValue.index() != rValue.index()) {
            return false;
        }
        if (lValue.index() == TYPE_INDEX<std::string>) {
            auto ls = Traits::get_if<std::string>(&lValue);
            auto rs = Traits::get_if<std::string>(&rValue);
            return ls != nullptr && rs != nullptr && *ls == *rs;
        }
        if (lValue.index() == TYPE_INDEX<int64_t>) {
            auto ls = Traits::get_if<int64_t>(&lValue);
            auto rs = Traits::get_if<int64_t>(&rValue);
            if (ls == nullptr || rs == nullptr || *ls != *rs) {
                return false;
            }
            continue;
        }
        if (lValue.index() == TYPE_INDEX<double>) {
            auto ls = Traits::get_if<double>(&lValue);
            auto rs = Traits::get_if<double>(&rValue);
            const double epsilon = 1e-9;
            if (ls == nullptr || rs == nullptr || std::abs(*ls - *rs) > epsilon) {
                return false;
            }
            continue;
        }
        if (lValue.index() == TYPE_INDEX<bool>) {
            auto ls = Traits::get_if<bool>(&lValue);
            auto rs = Traits::get_if<bool>(&rValue);
            if (ls == nullptr || rs == nullptr || *ls != *rs) {
                return false;
            }
            continue;
        }
        if (lValue.index() == TYPE_INDEX<DistributedData::Bytes>) {
            auto ls = Traits::get_if<Bytes>(&lValue);
            auto rs = Traits::get_if<Bytes>(&rValue);
            if (ls == nullptr || rs == nullptr || *ls != *rs) {
                return false;
            }
            continue;
        }
    }
    return true;
}

bool RdbGeneralStoreTest::CorruptDatabaseFile(const string &filePath, int32_t offset, int32_t size)
{
    FILE* file = fopen(filePath.c_str(), "r+b");
    if (file == nullptr) {
        return false;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize <= 0) {
        fclose(file);
        return false;
    }

    fseek(file, offset, SEEK_SET);
    int32_t corruptSize = std::min(size, static_cast<int32_t>(fileSize - offset));
    if (corruptSize <= 0) {
        fclose(file);
        return false;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::vector<char> garbage(corruptSize);
    for (int32_t i = 0; i < corruptSize; i++) {
        garbage[i] = static_cast<char>(dis(gen));
    }

    size_t written = fwrite(garbage.data(), 1, corruptSize, file);
    fclose(file);

    return written == static_cast<size_t>(corruptSize);
}

void RdbGeneralStoreTest::CorruptDatabasePager(std::shared_ptr<RdbGeneralStore> store, const std::string &filePath,
    const std::string &tableName)
{
    std::vector<DistributedData::VBucket> largeDataSet;
    const size_t DATA_SIZE = 2 * 1024 * 1024 + 1024; // 2MB + 1KB to ensure we exceed 2MB
    std::vector<uint8_t> largeData(DATA_SIZE, 'A');  // Create large data vector

    const int32_t recordCount = 5; // Number of records to insert
    DistributedData::VBucket vBucket;
    for (int32_t i = 0; i < recordCount; ++i) {
        vBucket["name"] = "test_user_" + std::to_string(i);
        vBucket["age"] = 25 + i;
        vBucket["salary"] = 5000.0 + i * 100;
        vBucket["data"] = largeData; // Use large data to exceed 2MB in total
        largeDataSet.push_back(vBucket);
    }

    auto [code, res] = store->BatchInsert(tableName, std::move(largeDataSet), GeneralStore::ON_CONFLICT_NONE);
    EXPECT_EQ(code, GeneralError::E_OK);
    EXPECT_EQ(res, recordCount);
//    EXPECT_EQ(store->Close(true), GeneralError::E_OK);

    EXPECT_TRUE(CorruptDatabaseFile(filePath, 4096, 20 * 1024 * 1024));
}

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
*/
HWTEST_F(RdbGeneralStoreTest, BindSnapshots001, TestSize.Level1)
{
    BindAssets bindAssets;
    auto result = store_->BindSnapshots(bindAssets);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: BindSnapshots002
* @tc.desc: RdbGeneralStore BindSnapshots nullptr test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, BindSnapshots002, TestSize.Level1)
{
    DistributedData::StoreMetaData meta;
    meta = metaData_;
    meta.isEncrypt = true;
    auto store = std::make_shared<RdbGeneralStore>(meta);
    ASSERT_NE(store, nullptr);
    store->snapshots_ = nullptr;
    BindAssets bindAssets;
    auto result = store->BindSnapshots(bindAssets);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
@tc.name: BindSnapshots003
@tc.desc: RdbGeneralStore BindSnapshots not nullptr test
@tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, BindSnapshots003, TestSize.Level1)
{
    DistributedData::StoreMetaData meta;
    meta = metaData_;
    meta.isEncrypt = true;
    auto store = std::make_shared<RdbGeneralStore>(meta);
    ASSERT_NE(store, nullptr);
    BindAssets bindAssets = std::make_shared<std::map<std::string, std::shared_ptr<Snapshot>>>();
    store->snapshots_ = bindAssets;
    auto result = store->BindSnapshots(bindAssets);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Bind001
* @tc.desc: RdbGeneralStore Bind bindInfo test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Bind001, TestSize.Level1)
{
    DistributedData::Database database;
    GeneralStore::CloudConfig config;
    std::map<uint32_t, DistributedData::GeneralStore::BindInfo> bindInfos;
    auto result = store_->Bind(database, bindInfos, config);
    EXPECT_TRUE(bindInfos.empty());
    EXPECT_EQ(result, GeneralError::E_OK);

    std::shared_ptr<CloudDB> db;
    std::shared_ptr<AssetLoader> loader;
    GeneralStore::BindInfo bindInfo(db, loader);
    EXPECT_EQ(bindInfo.db_, nullptr);
    EXPECT_EQ(bindInfo.loader_, nullptr);
    uint32_t key = 1;
    bindInfos[key] = bindInfo;
    result = store_->Bind(database, bindInfos, config);
    EXPECT_TRUE(!bindInfos.empty());
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    std::shared_ptr<CloudDB> dbs = std::make_shared<CloudDB>();
    std::shared_ptr<AssetLoader> loaders = std::make_shared<AssetLoader>();
    GeneralStore::BindInfo bindInfo1(dbs, loader);
    EXPECT_NE(bindInfo1.db_, nullptr);
    bindInfos[key] = bindInfo1;
    result = store_->Bind(database, bindInfos, config);
    EXPECT_TRUE(!bindInfos.empty());
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    GeneralStore::BindInfo bindInfo2(db, loaders);
    EXPECT_NE(bindInfo2.loader_, nullptr);
    bindInfos[key] = bindInfo2;
    result = store_->Bind(database, bindInfos, config);
    EXPECT_TRUE(!bindInfos.empty());
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);
}

/**
* @tc.name: Bind002
* @tc.desc: RdbGeneralStore Bind delegate_ is nullptr test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Bind002, TestSize.Level1)
{
    DistributedData::Database database;
    std::map<uint32_t, DistributedData::GeneralStore::BindInfo> bindInfos;
    std::shared_ptr<CloudDB> db = std::make_shared<CloudDB>();
    std::shared_ptr<AssetLoader> loader = std::make_shared<AssetLoader>();
    GeneralStore::BindInfo bindInfo(db, loader);
    uint32_t key = 1;
    bindInfos[key] = bindInfo;
    GeneralStore::CloudConfig config;
    auto result = store_->Bind(database, bindInfos, config);
    EXPECT_EQ(store_->delegate_, nullptr);
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);

    store_->isBound_ = true;
    result = store_->Bind(database, bindInfos, config);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Bind003
* @tc.desc: RdbGeneralStore Bind delegate_ test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Bind003, TestSize.Level1)
{
    DistributedData::Database database;
    std::map<uint32_t, DistributedData::GeneralStore::BindInfo> bindInfos;

    std::shared_ptr<CloudDB> db = std::make_shared<CloudDB>();
    std::shared_ptr<AssetLoader> loader = std::make_shared<AssetLoader>();
    GeneralStore::BindInfo bindInfo(db, loader);
    uint32_t key = 1;
    bindInfos[key] = bindInfo;
    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    GeneralStore::CloudConfig config;
    auto result = store_->Bind(database, bindInfos, config);
    EXPECT_NE(store_->delegate_, nullptr);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Close
* @tc.desc: RdbGeneralStore Close and isforce is false and GetDeviceSyncTaskCount is 2
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Close, TestSize.Level1)
{
    auto result = store_->IsBound(std::atoi(metaData_.user.c_str()));
    EXPECT_EQ(result, false);
    EXPECT_EQ(store_->delegate_, nullptr);
    auto ret = store_->Close();
    EXPECT_EQ(ret, GeneralError::E_OK);
    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    MockRelationalStoreDelegate::SetCloudSyncTaskCount(0);
    MockRelationalStoreDelegate::SetDownloadingAssetsCount(0);
    MockRelationalStoreDelegate::SetDeviceTaskCount(2);
    ret = store_->Close();
    EXPECT_EQ(ret, GeneralError::E_BUSY);
}

/**
 * @tc.name: Close1
 * @tc.desc: RdbGeneralStore Close and isforce is false and GetCloudSyncTaskCount is 1
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest, Close1, TestSize.Level1)
{
    metaData_.storeId = "mock";
    auto store = std::make_shared<RdbGeneralStore>(metaData_);
    MockRelationalStoreDelegate::SetCloudSyncTaskCount(1);
    auto ret = store->Close();
    EXPECT_EQ(ret, GeneralError::E_BUSY);
}
/**
 * @tc.name: Close2
 * @tc.desc: RdbGeneralStore Close and isforce is false and DownloadingAssetsCount is 1
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest, Close2, TestSize.Level1)
{
    metaData_.storeId = "mock";
    auto store = std::make_shared<RdbGeneralStore>(metaData_);
    MockRelationalStoreDelegate::SetCloudSyncTaskCount(0);
    MockRelationalStoreDelegate::SetDownloadingAssetsCount(1);
    auto ret = store->Close();
    EXPECT_EQ(ret, GeneralError::E_BUSY);
}
/**
 * @tc.name: Close3
 * @tc.desc: RdbGeneralStore Close and isforce is true
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest, Close3, TestSize.Level1)
{
    metaData_.storeId = "mock";
    auto store = std::make_shared<RdbGeneralStore>(metaData_);
    auto ret = store->Close(true);
    EXPECT_EQ(ret, GeneralError::E_OK);
}
/**
 * @tc.name: Close4
 * @tc.desc: RdbGeneralStore Close and there is no task
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest, Close4, TestSize.Level1)
{
    metaData_.storeId = "mock";
    auto store = std::make_shared<RdbGeneralStore>(metaData_);
    MockRelationalStoreDelegate::SetCloudSyncTaskCount(0);
    MockRelationalStoreDelegate::SetDownloadingAssetsCount(0);
    MockRelationalStoreDelegate::SetDeviceTaskCount(0);
    auto ret = store->Close();
    EXPECT_EQ(ret, GeneralError::E_OK);
}

/**
* @tc.name: Close
* @tc.desc: RdbGeneralStore Close test
* @tc.type: FUNC
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
*/
HWTEST_F(RdbGeneralStoreTest, Execute, TestSize.Level1)
{
    std::string table = "table";
    std::string sql = "sql";
    EXPECT_EQ(store_->delegate_, nullptr);
    auto result = store_->Execute(table, sql);
    EXPECT_EQ(result, GeneralError::E_ERROR);
    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    result = store_->Execute(table, sql);
    EXPECT_EQ(result, GeneralError::E_OK);

    std::string null = "";
    result = store_->Execute(table, null);
    EXPECT_EQ(result, GeneralError::E_ERROR);
}

/**
* @tc.name: SqlConcatenate
* @tc.desc: RdbGeneralStore SqlConcatenate function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, SqlConcatenate, TestSize.Level1)
{
    DistributedData::VBucket value;
    std::string strColumnSql = "strColumnSql";
    std::string strRowValueSql = "strRowValueSql";
    auto result = store_->SqlConcatenate(value, strColumnSql, strRowValueSql);
    size_t columnSize = value.size();
    EXPECT_EQ(columnSize, 0);
    EXPECT_EQ(result, columnSize);

    DistributedData::VBucket values = g_RdbVBucket;
    result = store_->SqlConcatenate(values, strColumnSql, strRowValueSql);
    columnSize = values.size();
    EXPECT_NE(columnSize, 0);
    EXPECT_EQ(result, columnSize);
}

/**
* @tc.name: Insert001
* @tc.desc: RdbGeneralStore Insert error test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Insert001, TestSize.Level1)
{
    DistributedData::VBuckets values;
    EXPECT_EQ(values.size(), 0);
    std::string table = "table";
    auto result = store_->Insert("", std::move(values));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);
    result = store_->Insert(table, std::move(values));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    DistributedData::VBuckets extends = { { { "#gid", { "0000000" } }, { "#flag", { true } },
                                              { "#value", { int64_t(100) } }, { "#float", { double(100) } } },
        { { "#gid", { "0000001" } } } };
    result = store_->Insert("", std::move(extends));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    DistributedData::VBucket value;
    DistributedData::VBuckets vbuckets = { value };
    result = store_->Insert(table, std::move(vbuckets));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store_->Insert(table, std::move(extends));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);
}

/**
* @tc.name: Insert002
* @tc.desc: RdbGeneralStore Insert function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Insert002, TestSize.Level1)
{
    std::string table = "table";
    DistributedData::VBuckets extends = { { g_RdbVBucket } };
    auto result = store_->Insert(table, std::move(extends));
    EXPECT_EQ(result, GeneralError::E_ERROR);

    std::string test = "test";
    result = store_->Insert(test, std::move(extends));
    EXPECT_EQ(result, GeneralError::E_ERROR);

    result = store_->Insert(test, std::move(extends));
    EXPECT_EQ(result, GeneralError::E_ERROR);

    for (size_t i = 0; i < PRINT_ERROR_CNT + 1; i++) {
        result = store_->Insert(test, std::move(extends));
        EXPECT_EQ(result, GeneralError::E_ERROR);
    }
    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    result = store_->Insert(table, std::move(extends));
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Update
* @tc.desc: RdbGeneralStore Update function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Update, TestSize.Level1)
{
    std::string table = "table";
    std::string setSql = "setSql";
    RdbGeneralStore::Values values;
    std::string whereSql = "whereSql";
    RdbGeneralStore::Values conditions;
    auto result = store_->Update("", setSql, std::move(values), whereSql, std::move(conditions));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store_->Update(table, "", std::move(values), whereSql, std::move(conditions));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store_->Update(table, setSql, std::move(values), whereSql, std::move(conditions));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store_->Update(table, setSql, std::move(g_RdbValues), "", std::move(conditions));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store_->Update(table, setSql, std::move(g_RdbValues), whereSql, std::move(conditions));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store_->Update(table, setSql, std::move(g_RdbValues), whereSql, std::move(g_RdbValues));
    EXPECT_EQ(result, GeneralError::E_ERROR);
    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    result = store_->Update(table, setSql, std::move(g_RdbValues), whereSql, std::move(g_RdbValues));
    EXPECT_EQ(result, GeneralError::E_OK);

    result = store_->Update("test", setSql, std::move(g_RdbValues), whereSql, std::move(g_RdbValues));
    EXPECT_EQ(result, GeneralError::E_ERROR);
}

/**
* @tc.name: Replace
* @tc.desc: RdbGeneralStore Replace function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Replace, TestSize.Level1)
{
    std::string table = "table";
    RdbGeneralStore::VBucket values;
    auto result = store_->Replace("", std::move(g_RdbVBucket));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store_->Replace(table, std::move(values));
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store_->Replace(table, std::move(g_RdbVBucket));
    EXPECT_EQ(result, GeneralError::E_ERROR);
    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    result = store_->Replace(table, std::move(g_RdbVBucket));
    EXPECT_EQ(result, GeneralError::E_OK);

    result = store_->Replace("test", std::move(g_RdbVBucket));
    EXPECT_EQ(result, GeneralError::E_ERROR);
}

/**
* @tc.name: Delete
* @tc.desc: RdbGeneralStore Delete function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Delete, TestSize.Level1)
{
    std::string table = "table";
    std::string sql = "sql";
    auto result = store_->Delete(table, sql, std::move(g_RdbValues));
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Query001
* @tc.desc: RdbGeneralStore Query function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Query001, TestSize.Level1)
{
    std::string table = "table";
    std::string sql = "sql";
    auto [err1, result1] = store_->Query(table, sql, std::move(g_RdbValues));
    EXPECT_EQ(err1, GeneralError::E_ALREADY_CLOSED);
    EXPECT_EQ(result1, nullptr);
    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    auto [err2, result2] = store_->Query(table, sql, std::move(g_RdbValues));
    EXPECT_EQ(err2, GeneralError::E_OK);
    EXPECT_NE(result2, nullptr);
}

/**
* @tc.name: Query002
* @tc.desc: RdbGeneralStore Query function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Query002, TestSize.Level1)
{
    std::string table = "table";
    std::string sql = "sql";
    MockQuery query;
    auto [err1, result1] = store_->Query(table, query);
    EXPECT_EQ(err1, GeneralError::E_INVALID_ARGS);
    EXPECT_EQ(result1, nullptr);

    query.lastResult = true;
    auto [err2, result2] = store_->Query(table, query);
    EXPECT_EQ(err2, GeneralError::E_ALREADY_CLOSED);
    EXPECT_EQ(result2, nullptr);
}

/**
 * @tc.name: Query003
 * @tc.desc: it is not a remote query return E_ERROR.
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest, Query003, TestSize.Level1)
{
    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    ASSERT_NE(store_, nullptr);

    MockQuery query;
    const std::string devices = "device1";
    const std::string sql;
    Values args;
    query.lastResult = true;
    std::string table = "test_table";
    auto [err, cursor] = store_->Query(table, query);
    EXPECT_EQ(err, GeneralError::E_ERROR);
}

/**
 * @tc.name: Query004
 * @tc.desc: Test successful remote query
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest, Query004, TestSize.Level1)
{
    MockQuery query;
    const std::string devices = "device1";
    const std::string sql;
    Values args;
    query.MakeRemoteQuery(devices, sql, std::move(args));
    query.lastResult = true;

    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);

    std::string table = "test_table";
    auto [err, cursor] = store_->Query(table, query);

    EXPECT_EQ(err, GeneralError::E_OK);
    EXPECT_NE(cursor, nullptr);
}

/**
* @tc.name: MergeMigratedData
* @tc.desc: RdbGeneralStore MergeMigratedData function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, MergeMigratedData, TestSize.Level1)
{
    std::string tableName = "tableName";
    DistributedData::VBuckets extends = { { g_RdbVBucket } };
    auto result = store_->MergeMigratedData(tableName, std::move(extends));
    EXPECT_EQ(result, GeneralError::E_ERROR);

    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    result = store_->MergeMigratedData(tableName, std::move(extends));
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Sync
* @tc.desc: RdbGeneralStore Sync function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Sync, TestSize.Level1)
{
    GeneralStore::Devices devices;
    MockQuery query;
    GeneralStore::DetailAsync async;
    SyncParam syncParam;
    auto result = store_->Sync(devices, query, async, syncParam);
    EXPECT_EQ(result.first, GeneralError::E_ALREADY_CLOSED);

    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    result = store_->Sync(devices, query, async, syncParam);
    EXPECT_EQ(result.first, GeneralError::E_OK);
}

/**
* @tc.name: Sync
* @tc.desc: RdbGeneralStore Sync CLOUD_TIME_FIRST test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Sync001, TestSize.Level1)
{
    GeneralStore::Devices devices;
    MockQuery query;
    GeneralStore::DetailAsync async;
    SyncParam syncParam;
    syncParam.mode = GeneralStore::CLOUD_TIME_FIRST;

    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    auto [result1, result2] = store_->Sync(devices, query, async, syncParam);
    EXPECT_EQ(result1, GeneralError::E_OK);
    syncParam.mode = GeneralStore::NEARBY_END;
    std::tie(result1, result2) = store_->Sync(devices, query, async, syncParam);
    EXPECT_EQ(result1, GeneralError::E_ERROR);
    syncParam.mode = GeneralStore::NEARBY_PULL_PUSH;
    std::tie(result1, result2) = store_->Sync(devices, query, async, syncParam);
    EXPECT_EQ(result1, GeneralError::E_OK);
}

/**
* @tc.name: Sync
* @tc.desc: RdbGeneralStore Sync DistributedTable test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Sync002, TestSize.Level1)
{
    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    ASSERT_NE(store_, nullptr);

    GeneralStore::Devices devices;
    RdbQuery query;
    GeneralStore::DetailAsync async;
    SyncParam syncParam;
    auto [result1, result2] = store_->Sync(devices, query, async, syncParam);
    EXPECT_EQ(result1, GeneralError::E_OK);
}

/**
* @tc.name: PreSharing
* @tc.desc: RdbGeneralStore PreSharing function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, PreSharing, TestSize.Level1)
{
    MockQuery query;
    auto [errCode, result] = store_->PreSharing(query);
    EXPECT_NE(errCode, GeneralError::E_OK);
    EXPECT_EQ(result, nullptr);
}

/**
* @tc.name: PreSharing
* @tc.desc: RdbGeneralStore PreSharing function test, return E_INVALID_ARGS.
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, PreSharing001, TestSize.Level1)
{
    MockQuery query;
    query.lastResult = true;
    auto [errCode, result] = store_->PreSharing(query);
    EXPECT_EQ(errCode, GeneralError::E_INVALID_ARGS);
    EXPECT_EQ(result, nullptr);
}

/**
* @tc.name: PreSharing
* @tc.desc: RdbGeneralStore PreSharing function delegate is nullptr test.
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, PreSharing002, TestSize.Level1)
{
    MockQuery query;
    DistributedRdb::PredicatesMemo predicates;
    predicates.devices_ = { "device1" };
    predicates.tables_ = { "tables1" };
    query.lastResult = true;
    query.MakeQuery(predicates);
    auto [errCode, result] = store_->PreSharing(query);
    EXPECT_EQ(errCode, GeneralError::E_ALREADY_CLOSED);
    EXPECT_EQ(result, nullptr);
}

/**
* @tc.name: PreSharing
* @tc.desc: RdbGeneralStore PreSharing function E_CLOUD_DISABLED test.
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, PreSharing003, TestSize.Level1)
{
    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    ASSERT_NE(store_, nullptr);
    MockQuery query;
    DistributedRdb::PredicatesMemo predicates;
    predicates.devices_ = { "device1" };
    predicates.tables_ = { "tables1" };
    query.lastResult = true;
    query.MakeQuery(predicates);
    auto [errCode, result] = store_->PreSharing(query);
    EXPECT_EQ(errCode, GeneralError::E_CLOUD_DISABLED);
    ASSERT_EQ(result, nullptr);
}

/**
* @tc.name: ExtractExtend
* @tc.desc: RdbGeneralStore ExtractExtend function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, ExtractExtend, TestSize.Level1)
{
    RdbGeneralStore::VBucket extend = { { "#gid", { "0000000" } }, { "#flag", { true } },
        { "#value", { int64_t(100) } }, { "#float", { double(100) } }, { "#cloud_gid", { "cloud_gid" } },
        { "cloud_gid", { "" } } };
    DistributedData::VBuckets extends = { { extend } };
    auto result = store_->ExtractExtend(extends);
    EXPECT_EQ(result.size(), extends.size());
    DistributedData::VBuckets values;
    result = store_->ExtractExtend(values);
    EXPECT_EQ(result.size(), values.size());
}

/**
* @tc.name: Clean
* @tc.desc: RdbGeneralStore Clean function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Clean, TestSize.Level1)
{
    std::string tableName = "tableName";
    std::vector<std::string> devices = { "device1", "device2" };
    auto result = store_->Clean(devices, -1, tableName);
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);
    result = store_->Clean(devices, GeneralStore::CLEAN_MODE_BUTT + 1, tableName);
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);
    result = store_->Clean(devices, GeneralStore::CLOUD_INFO, tableName);
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);

    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    result = store_->Clean(devices, GeneralStore::CLOUD_INFO, tableName);
    EXPECT_EQ(result, GeneralError::E_OK);
    result = store_->Clean(devices, GeneralStore::CLOUD_DATA, tableName);
    EXPECT_EQ(result, GeneralError::E_OK);
    std::vector<std::string> devices1;
    result = store_->Clean(devices1, GeneralStore::NEARBY_DATA, tableName);
    EXPECT_EQ(result, GeneralError::E_OK);

    result = store_->Clean(devices, GeneralStore::CLOUD_INFO, tableName);
    EXPECT_EQ(result, GeneralError::E_ERROR);
    result = store_->Clean(devices, GeneralStore::CLOUD_DATA, tableName);
    EXPECT_EQ(result, GeneralError::E_ERROR);
    result = store_->Clean(devices, GeneralStore::CLEAN_MODE_BUTT, tableName);
    EXPECT_EQ(result, GeneralError::E_ERROR);
    result = store_->Clean(devices, GeneralStore::NEARBY_DATA, tableName);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Watch
* @tc.desc: RdbGeneralStore Watch and Unwatch function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, Watch, TestSize.Level1)
{
    MockGeneralWatcher watcher;
    auto result = store_->Watch(GeneralWatcher::Origin::ORIGIN_CLOUD, watcher);
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);
    result = store_->Unwatch(GeneralWatcher::Origin::ORIGIN_CLOUD, watcher);
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store_->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_OK);
    result = store_->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = store_->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_OK);
    result = store_->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);
}

/**
* @tc.name: OnChange
* @tc.desc: RdbGeneralStore OnChange function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, OnChange, TestSize.Level1)
{
    MockGeneralWatcher watcher;
    MockStoreChangedData data;
    DistributedDB::ChangedData changedData;
    store_->observer_.OnChange(data);
    store_->observer_.OnChange(DistributedDB::Origin::ORIGIN_CLOUD, "originalId", std::move(changedData));
    auto result = store_->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_OK);
    store_->observer_.OnChange(data);
    store_->observer_.OnChange(DistributedDB::Origin::ORIGIN_CLOUD, "originalId", std::move(changedData));
    result = store_->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: OnChange001
* @tc.desc: RdbGeneralStore OnChange function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, OnChange001, TestSize.Level1)
{
    MockGeneralWatcher watcher;
    MockStoreChangedData data;
    DistributedDB::ChangedData changedData;
    changedData.primaryData[0] = { { std::monostate{}, 42, 3.14, "hello", true },
        { Bytes{ 1, 2, 3, 4 },
            DistributedDB::Asset{ 1, "zhangsan", "123", "/data/test", "file://xxx", "123", "100", "100", "999",
                static_cast<uint32_t>(AssetOpType::NO_CHANGE), static_cast<uint32_t>(AssetStatus::NORMAL), 0 },
            Bytes{ 5, 6, 7, 8 } },
        { int64_t(-123), 2.718, 100, 0.001 } };
    changedData.primaryData[1] = { { std::monostate{}, 42, 3.14, "hello", true },
        { Bytes{ 1, 2, 3, 4 },
            DistributedDB::Asset{ 1, "zhangsan", "123", "/data/test", "file://xxx", "123", "100", "100", "999",
                static_cast<uint32_t>(AssetOpType::NO_CHANGE), static_cast<uint32_t>(AssetStatus::NORMAL), 0 },
            Bytes{ 5, 6, 7, 8 } },
        { int64_t(-123), 2.718, 100, 0.001 } };
    changedData.primaryData[2] = { { "DELETE#ALL_CLOUDDATA", std::monostate{}, 42, 3.14, "hello", true },
        { Bytes{ 1, 2, 3, 4 },
            DistributedDB::Asset{ 1, "zhangsan", "123", "/data/test", "file://xxx", "123", "100", "100", "999",
                static_cast<uint32_t>(AssetOpType::NO_CHANGE), static_cast<uint32_t>(AssetStatus::NORMAL), 0 },
            Bytes{ 5, 6, 7, 8 } },
        { int64_t(-123), 2.718, 100, 0.001 } };
    changedData.field = { "name", "age" };
    changedData.tableName = "test";
    DistributedDB::ChangedData changedDataTmp;
    changedDataTmp = changedData;
    auto result = store_->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_OK);
    store_->observer_.OnChange(data);
    store_->observer_.OnChange(DistributedDB::Origin::ORIGIN_CLOUD, "originalId", std::move(changedData));
    EXPECT_EQ(watcher.primaryFields_[changedDataTmp.tableName], *(changedDataTmp.field.begin()));
    store_->observer_.OnChange(DistributedDB::Origin::ORIGIN_LOCAL, "originalId", std::move(changedDataTmp));
    ASSERT_NE(watcher.origin_.id.size(), 0);
    EXPECT_EQ(watcher.origin_.id[0], "originalId");
    result = store_->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Release
* @tc.desc: RdbGeneralStore Release and AddRef function test
* @tc.type: FUNC
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
*/
HWTEST_F(RdbGeneralStoreTest, SetDistributedTables, TestSize.Level1)
{
    std::vector<std::string> tables = { "table1", "table2" };
    int32_t type = DistributedTableType::DISTRIBUTED_DEVICE;
    std::vector<DistributedData::Reference> references;
    auto result = store_->SetDistributedTables(tables, type, references);
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);

    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    result = store_->SetDistributedTables(tables, type, references);
    EXPECT_EQ(result, GeneralError::E_OK);

    std::vector<std::string> test = { "test" };
    result = store_->SetDistributedTables(test, type, references);
    EXPECT_EQ(result, GeneralError::E_ERROR);
    result = store_->SetDistributedTables(tables, type, references);
    EXPECT_EQ(result, GeneralError::E_OK);
    type = DistributedTableType::DISTRIBUTED_CLOUD;
    result = store_->SetDistributedTables(tables, type, references);
    EXPECT_EQ(result, GeneralError::E_ERROR);
}

/**
* @tc.name: SetTrackerTable
* @tc.desc: RdbGeneralStore SetTrackerTable function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, SetTrackerTable, TestSize.Level1)
{
    std::string tableName = "tableName";
    std::set<std::string> trackerColNames = { "col1", "col2" };
    std::set<std::string> extendColNames = { "extendColName1", "extendColName2" };
    auto result = store_->SetTrackerTable(tableName, trackerColNames, extendColNames);
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);

    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    result = store_->SetTrackerTable(tableName, trackerColNames, extendColNames);
    EXPECT_EQ(result, GeneralError::E_OK);
    result = store_->SetTrackerTable("WITH_INVENTORY_DATA", trackerColNames, extendColNames);
    EXPECT_EQ(result, GeneralError::E_WITH_INVENTORY_DATA);
    result = store_->SetTrackerTable("test", trackerColNames, extendColNames);
    EXPECT_EQ(result, GeneralError::E_ERROR);
}

///**
//* @tc.name: RemoteQuery
//* @tc.desc: RdbGeneralStore RemoteQuery function test//todo:using public interface
//* @tc.type: FUNC
//*/
//HWTEST_F(RdbGeneralStoreTest, RemoteQuery, TestSize.Level1)
//{
//    std::string device = "device";
//    DistributedDB::RemoteCondition remoteCondition;
//    metaData_.storeId = "mock";
//    store = std::make_shared<RdbGeneralStore>(metaData_);
//    auto result = store->RemoteQuery("test", remoteCondition);
//    EXPECT_EQ(result, nullptr);
//    result = store->RemoteQuery(device, remoteCondition);
//    EXPECT_NE(result, nullptr);
//}

/**
* @tc.name: ConvertStatus
* @tc.desc: RdbGeneralStore ConvertStatus function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, ConvertStatus, TestSize.Level1)
{
    auto result = store_->ConvertStatus(DBStatus::OK);
    EXPECT_EQ(result, GeneralError::E_OK);
    result = store_->ConvertStatus(DBStatus::CLOUD_NETWORK_ERROR);
    EXPECT_EQ(result, GeneralError::E_NETWORK_ERROR);
    result = store_->ConvertStatus(DBStatus::CLOUD_LOCK_ERROR);
    EXPECT_EQ(result, GeneralError::E_LOCKED_BY_OTHERS);
    result = store_->ConvertStatus(DBStatus::CLOUD_FULL_RECORDS);
    EXPECT_EQ(result, GeneralError::E_RECODE_LIMIT_EXCEEDED);
    result = store_->ConvertStatus(DBStatus::CLOUD_ASSET_SPACE_INSUFFICIENT);
    EXPECT_EQ(result, GeneralError::E_NO_SPACE_FOR_ASSET);
    result = store_->ConvertStatus(DBStatus::BUSY);
    EXPECT_EQ(result, GeneralError::E_BUSY);
    result = store_->ConvertStatus(DBStatus::DB_ERROR);
    EXPECT_EQ(result, GeneralError::E_ERROR);
    result = store_->ConvertStatus(DBStatus::CLOUD_DISABLED);
    EXPECT_EQ(result, GeneralError::E_CLOUD_DISABLED);
    result = store_->ConvertStatus(DBStatus::CLOUD_SYNC_TASK_MERGED);
    EXPECT_EQ(result, GeneralError::E_SYNC_TASK_MERGED);
}

/**
* @tc.name: QuerySql
* @tc.desc: RdbGeneralStore QuerySql function test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, QuerySql, TestSize.Level1)
{
    metaData_.storeId = "mock";
    auto store = std::make_shared<RdbGeneralStore>(metaData_);
    ASSERT_NE(store, nullptr);
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
*/
HWTEST_F(RdbGeneralStoreTest, BuildSqlWhenCloumnEmpty, TestSize.Level1)
{
    std::string table = "mock_table";
    std::string statement = "mock_statement";
    std::vector<std::string> columns;
    std::string expectSql = "select cloud_gid from naturalbase_rdb_aux_mock_table_log, (select rowid from "
                            "mock_tablemock_statement) where data_key = rowid";
    std::string resultSql = store_->BuildSql(table, statement, columns);
    EXPECT_EQ(resultSql, expectSql);
}

/**
* @tc.name: BuildSqlWhenParamValid
* @tc.desc: test buildsql method when param valid
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, BuildSqlWhenParamValid, TestSize.Level1)
{
    std::string table = "mock_table";
    std::string statement = "mock_statement";
    std::vector<std::string> columns;
    columns.push_back("mock_column_1");
    columns.push_back("mock_column_2");
    std::string expectSql = "select cloud_gid, mock_column_1, mock_column_2 from naturalbase_rdb_aux_mock_table_log, "
                            "(select rowid, mock_column_1, mock_column_2 from mock_tablemock_statement) where "
                            "data_key = rowid";
    std::string resultSql = store_->BuildSql(table, statement, columns);
    EXPECT_EQ(resultSql, expectSql);
}

/**
* @tc.name: LockAndUnLockCloudDBTest
* @tc.desc: lock and unlock cloudDB test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, LockAndUnLockCloudDBTest, TestSize.Level1)
{
    auto result = store_->LockCloudDB();
    EXPECT_EQ(result.first, 1);
    EXPECT_EQ(result.second, 0);
    auto unlockResult = store_->UnLockCloudDB();
    EXPECT_EQ(unlockResult, 1);
}

/**
* @tc.name: InFinishedTest
* @tc.desc: isFinished test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, InFinishedTest, TestSize.Level1)
{
    DistributedRdb::RdbGeneralStore::SyncId syncId = 1;
    bool isFinished = store_->IsFinished(syncId);
    EXPECT_TRUE(isFinished);
}

/**
* @tc.name: GetRdbCloudTest
* @tc.desc: getRdbCloud test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, GetRdbCloudTest, TestSize.Level1)
{
    auto rdbCloud = store_->GetRdbCloud();
    EXPECT_EQ(rdbCloud, nullptr);
}

/**
* @tc.name: RegisterDetailProgressObserverTest
* @tc.desc: RegisterDetailProgressObserver test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, RegisterDetailProgressObserverTest, TestSize.Level1)
{
    DistributedData::GeneralStore::DetailAsync async;
    auto result = store_->RegisterDetailProgressObserver(async);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: GetFinishTaskTest
* @tc.desc: GetFinishTask test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, GetFinishTaskTest, TestSize.Level1)
{
    DistributedRdb::RdbGeneralStore::SyncId syncId = 1;
    auto result = store_->GetFinishTask(syncId);
    ASSERT_NE(result, nullptr);
}

/**
* @tc.name: GetCBTest
* @tc.desc: GetCB test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, GetCBTest, TestSize.Level1)
{
    DistributedRdb::RdbGeneralStore::SyncId syncId = 1;
    auto result = store_->GetCB(syncId);
    ASSERT_NE(result, nullptr);
}

/**
* @tc.name: UpdateDBStatus
* @tc.desc: UpdateDBStatus test
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, UpdateDBStatus, TestSize.Level1)
{
    auto result = store_->UpdateDBStatus();
    EXPECT_EQ(result, E_ALREADY_CLOSED);
    metaData_.storeId = "mock";
    store_ = std::make_shared<RdbGeneralStore>(metaData_);
    result = store_->UpdateDBStatus();
    EXPECT_EQ(result, E_OK);
}

/**
* @tc.name: RdbGeneralStore_InitWithCreateRequired
* @tc.desc: Test RdbGeneralStore initialization with createRequired parameter
* Test scenarios:
* 1. Initialize store with createRequired=false (should fail)
* 2. Initialize store with createRequired=true (should succeed)
* 3. Verify database directory creation
* 4. Test proper store closure and cleanup
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, RdbGeneralStore_InitWithCreateRequired, TestSize.Level1)
{
    // Setup test environment with unique database identifier
    std::string storeId = "RdbGeneralStore_InitWithCreateRequired.db";
    auto meta = GetStoreMeta(storeId);
    remove(meta.dataDir.c_str());

    // Test scenario 1: createRequired set to false
    auto store = std::make_shared<RdbGeneralStore>(meta, false);
    auto code = store->Init();
    EXPECT_EQ(code, GeneralError::E_ERROR);

    // Test scenario 2: createRequired set to true
    store = std::make_shared<RdbGeneralStore>(meta, true);
    code = store->Init();
    EXPECT_EQ(code, GeneralError::E_OK);

    // Verify database directory was actually created
    EXPECT_EQ(access(meta.dataDir.c_str(), F_OK), 0);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}

/**
* @tc.name: RdbGeneralStore_InsertSingleRecord
* @tc.desc: Test inserting a single record into RdbGeneralStore
* Test scenarios:
* 1. Initialize database and create test table
* 2. Construct test data with multiple data types
* 3. Execute insert operation and verify results
* 4. Query inserted data and verify consistency
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, RdbGeneralStore_InsertSingleRecord, TestSize.Level1)
{
    std::string storeId = "RdbGeneralStore_InsertSingleRecord.db";
    auto meta = GetStoreMeta(storeId);
    remove(meta.dataDir.c_str());
    auto store = std::make_shared<RdbGeneralStore>(meta, true);
    auto code = store->Init();
    EXPECT_EQ(code, GeneralError::E_OK);

    // Create test table
    std::string tableName = "RdbGeneralStore_InsertSingleRecord";
    EXPECT_EQ(CreateTable(store, tableName), GeneralError::E_OK);

    // Construct test data with various data types
    DistributedData::VBucket vBucket;
    vBucket["id"] = 1;
    vBucket["name"] = "lisi";
    vBucket["age"] = 18;
    vBucket["salary"] = 249.9;
    vBucket["data"] = std::vector<uint8_t>{ 1, 2, 3 };

    // Execute insert operation
    int64_t res = -1;
    std::tie(code, res) = store->Insert(tableName, VBucket(vBucket), GeneralStore::ON_CONFLICT_IGNORE);
    EXPECT_EQ(code, GeneralError::E_OK);
    EXPECT_EQ(res, 1);

    // Query and verify inserted data
    std::shared_ptr<Cursor> cursor = nullptr;
    std::tie(code, cursor) = store->Query(std::string("select * from ") + tableName);
    EXPECT_EQ(code, GeneralError::E_OK);
    ASSERT_NE(cursor, nullptr);

    // Navigate and verify query results
    EXPECT_EQ(cursor->MoveToFirst(), GeneralError::E_OK);
    DistributedData::VBucket row;
    EXPECT_EQ(cursor->GetRow(row), GeneralError::E_OK);
    EXPECT_TRUE(Equal(vBucket, row));
    EXPECT_EQ(cursor->GetCount(), 1);
    EXPECT_EQ(cursor->Close(), GeneralError::E_OK);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}

/**
* @tc.name: RdbGeneralStore_UpdateRecord
* @tc.desc: Test updating a record in RdbGeneralStore
* Test scenarios:
* 1. Initialize database and create table
* 2. Insert initial test data
* 3. Construct update query and execute update
* 4. Verify update results and data correctness
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, RdbGeneralStore_UpdateRecord, TestSize.Level1)
{
    std::string storeId = "RdbGeneralStore_UpdateRecord.db";
    auto meta = GetStoreMeta(storeId);
    remove(meta.dataDir.c_str());
    auto store = std::make_shared<RdbGeneralStore>(meta, true);
    auto code = store->Init();
    EXPECT_EQ(code, GeneralError::E_OK);

    // Create test table
    std::string tableName = "RdbGeneralStore_UpdateRecord";
    EXPECT_EQ(CreateTable(store, tableName), GeneralError::E_OK);

    // Insert initial test data
    DistributedData::VBucket vBucket;
    vBucket["name"] = "bili";
    vBucket["age"] = 80;
    vBucket["salary"] = 11112;

    int64_t res = -1;
    std::tie(code, res) = store->Insert(tableName, VBucket(vBucket), GeneralStore::ON_CONFLICT_NONE);
    EXPECT_EQ(code, GeneralError::E_OK);
    EXPECT_EQ(res, 1);

    // Update data: modify salary field
    vBucket["salary"] = 22221;
    RdbGeneralQuery query;
    query.table = tableName;
    NativeRdb::AbsRdbPredicates predicates(tableName);
    predicates.EqualTo("name", "bili");
    query.whereClause = predicates.GetWhereClause();
    query.args = ValueProxy::Convert(predicates.GetBindArgs());
    std::tie(code, res) = store->Update(query, VBucket(vBucket), GeneralStore::ON_CONFLICT_IGNORE);
    EXPECT_EQ(code, GeneralError::E_OK);
    EXPECT_EQ(res, 1);

    // Query and verify update results
    std::shared_ptr<Cursor> cursor = nullptr;
    std::tie(code, cursor) = store->Query(std::string("select * from ") + tableName);
    EXPECT_EQ(code, GeneralError::E_OK);
    ASSERT_NE(cursor, nullptr);

    EXPECT_EQ(cursor->MoveToFirst(), GeneralError::E_OK);
    // Verify name field remains unchanged
    Value name;
    EXPECT_EQ(cursor->Get("name", name), GeneralError::E_OK);
    auto val = Traits::get_if<std::string>(&name);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, "bili");

    // Verify salary field was updated
    Value salary;
    EXPECT_EQ(cursor->Get("salary", salary), GeneralError::E_OK);
    auto salaryVal = Traits::get_if<double>(&salary);
    ASSERT_NE(salaryVal, nullptr);
    EXPECT_DOUBLE_EQ(*salaryVal, 22221);
    EXPECT_EQ(cursor->GetCount(), 1);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}

/**
* @tc.name: RdbGeneralStore_DeleteWithInClause
* @tc.desc: Test deleting records with IN clause condition in RdbGeneralStore
* Test Scenario:
* 1. Initialize RDB store and create test table with specified schema
* 2. Insert a single test record with fields: name="tom", age=15, salary=300, binary data
* 3. Execute delete operation using IN predicate: name IN ("bili", "tom")
* 4. Verify the record matching the condition is successfully deleted
* 5. Confirm table is empty by querying all records
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, RdbGeneralStore_DeleteWithInClause, TestSize.Level1)
{
    // Step 1: Initialize database environment and create test table
    std::string storeId = "RdbGeneralStore_DeleteWithInClause.db";
    auto meta = GetStoreMeta(storeId);
    remove(meta.dataDir.c_str());
    auto store = std::make_shared<RdbGeneralStore>(meta, true);
    auto code = store->Init();
    EXPECT_EQ(code, GeneralError::E_OK);

    std::string tableName = "RdbGeneralStore_DeleteWithInClause";
    EXPECT_EQ(CreateTable(store, tableName), GeneralError::E_OK);

    // Step 2: Prepare and insert test data record
    DistributedData::VBucket vBucket;
    vBucket["name"] = "tom";
    vBucket["age"] = 15;
    vBucket["salary"] = 300;
    vBucket["data"] = std::vector<uint8_t>{ 2, 2, 2 };

    int64_t res = -1;
    std::tie(code, res) = store->Insert(tableName, VBucket(vBucket), GeneralStore::ON_CONFLICT_ROLLBACK);
    EXPECT_EQ(code, GeneralError::E_OK);
    EXPECT_EQ(res, 1);

    // Step 3: Build delete query with IN condition
    RdbGeneralQuery query;
    query.table = tableName;
    NativeRdb::AbsRdbPredicates predicates(tableName);
    predicates.In("name", std::vector<std::string>{ "bili", "tom" });
    query.whereClause = predicates.GetWhereClause();
    query.args = ValueProxy::Convert(predicates.GetBindArgs());
    std::tie(code, res) = store->Delete(query);
    EXPECT_EQ(code, GeneralError::E_OK);
    EXPECT_EQ(res, 1);

    // Step 4: Verify deletion result by querying all records
    std::shared_ptr<Cursor> cursor = nullptr;
    std::tie(code, cursor) = store->Query(std::string("select * from ") + tableName);
    EXPECT_EQ(code, GeneralError::E_OK);
    ASSERT_NE(cursor, nullptr);

    // Step 5: Confirm table is empty and cleanup resources
    EXPECT_EQ(cursor->MoveToFirst(), GeneralError::E_MOVE_DONE);
    EXPECT_EQ(cursor->GetCount(), 0);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}

/**
* @tc.name: RdbGeneralStore_BatchInsertRecords
* @tc.desc: Test batch inserting records into RdbGeneralStore
* Test scenarios:
* 1. Initialize database and create test table
* 2. Prepare batch data with 10 identical records
* 3. Execute batch insert operation and verify results
* 4. Query all inserted records and verify count
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, RdbGeneralStore_BatchInsertRecords, TestSize.Level1)
{
    // Step 1: Initialize database environment and create test table
    std::string storeId = "RdbGeneralStore_BatchInsertRecords.db";
    auto meta = GetStoreMeta(storeId);
    remove(meta.dataDir.c_str());
    auto store = std::make_shared<RdbGeneralStore>(meta, true);
    auto code = store->Init();
    EXPECT_EQ(code, GeneralError::E_OK);

    std::string tableName = "RdbGeneralStore_BatchInsertRecords";
    EXPECT_EQ(CreateTable(store, tableName), GeneralError::E_OK);

    // Step 2: Prepare batch data with 10 identical records
    std::vector<DistributedData::VBucket> values;
    DistributedData::VBucket vBucket;
    vBucket["name"] = "tom";
    vBucket["age"] = 15;
    vBucket["salary"] = 300;
    vBucket["data"] = std::vector<uint8_t>{ 2, 2, 2 };
    const int32_t size = 10;
    for (int i = 0; i < size; i++) {
        values.push_back(vBucket);
    }

    // Step 3: Execute batch insert operation and verify results
    int64_t res = -1;
    std::tie(code, res) = store->BatchInsert(tableName, std::move(values), GeneralStore::ON_CONFLICT_REPLACE);
    EXPECT_EQ(code, GeneralError::E_OK);
    EXPECT_EQ(res, size);

    // Step 4: Query all inserted records and verify count
    RdbGeneralQuery query;
    query.table = tableName;
    NativeRdb::AbsRdbPredicates predicates(tableName);
    query.whereClause = predicates.GetWhereClause();
    query.args = ValueProxy::Convert(predicates.GetBindArgs());
    std::shared_ptr<Cursor> cursor = nullptr;
    std::tie(code, cursor) = store->Query(query);
    EXPECT_EQ(code, GeneralError::E_OK);
    ASSERT_NE(cursor, nullptr);

    EXPECT_EQ(cursor->MoveToFirst(), GeneralError::E_OK);
    EXPECT_EQ(cursor->GetCount(), size);

    // Cleanup test environment
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}

/**
* @tc.name: RdbGeneralStore_CRUDAfterClose
* @tc.desc: Test CRUD operations after closing RdbGeneralStore
* Test scenarios:
* 1. Initialize database and close it properly
* 2. Attempt various CRUD operations on closed database
* 3. Verify that all operations return E_ALREADY_CLOSED error
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, RdbGeneralStore_CRUDAfterClose, TestSize.Level1)
{
    // Step 1: Initialize database environment and immediately close it
    std::string storeId = "RdbGeneralStore_CRUDAfterClose.db";
    auto meta = GetStoreMeta(storeId);
    remove(meta.dataDir.c_str());
    auto store = std::make_shared<RdbGeneralStore>(meta, true);
    auto code = store->Init();
    EXPECT_EQ(code, GeneralError::E_OK);

    std::string tableName = "RdbGeneralStore_CRUDAfterClose";
    EXPECT_EQ(store->Close(), GeneralError::E_OK);

    // Step 2: Prepare test data for CRUD operations
    std::vector<DistributedData::VBucket> values;
    DistributedData::VBucket vBucket;
    vBucket["name"] = "tom";
    vBucket["age"] = 15;
    vBucket["salary"] = 300;
    vBucket["data"] = std::vector<uint8_t>{ 2, 2, 2 };
    const int32_t size = 10;
    for (int i = 0; i < size; i++) {
        values.push_back(vBucket);
    }

    // Step 3: Test BatchInsert operation on closed database
    int64_t res = -1;
    std::tie(code, res) = store->BatchInsert(tableName, std::move(values), GeneralStore::ON_CONFLICT_REPLACE);
    EXPECT_EQ(code, GeneralError::E_ALREADY_CLOSED);
    EXPECT_EQ(res, -1);

    // Step 4: Test Insert operation on closed database
    std::tie(code, res) = store->Insert(tableName, VBucket(vBucket), GeneralStore::ON_CONFLICT_ABORT);
    EXPECT_EQ(code, GeneralError::E_ALREADY_CLOSED);
    EXPECT_EQ(res, -1);

    // Step 5: Test Query operation with query object on closed database
    RdbGeneralQuery query;
    std::shared_ptr<Cursor> cursor = nullptr;
    std::tie(code, cursor) = store->Query(query);
    EXPECT_EQ(code, GeneralError::E_ALREADY_CLOSED);
    ASSERT_EQ(cursor, nullptr);

    // Step 6: Test Query operation with SQL string on closed database
    std::tie(code, cursor) = store->Query("insert");
    EXPECT_EQ(code, GeneralError::E_ALREADY_CLOSED);
    ASSERT_EQ(cursor, nullptr);

    // Step 7: Test Update operation on closed database
    std::tie(code, res) = store->Update(query, VBucket(vBucket), GeneralStore::ON_CONFLICT_FAIL);
    EXPECT_EQ(code, GeneralError::E_ALREADY_CLOSED);
    EXPECT_EQ(res, -1);

    // Step 8: Test Delete operation on closed database
    std::tie(code, res) = store->Delete(query);
    EXPECT_EQ(code, GeneralError::E_ALREADY_CLOSED);
    EXPECT_EQ(res, -1);

    // Cleanup test environment
    remove(meta.dataDir.c_str());
}

/**
* @tc.name: RdbGeneralStore_CRUDWhenBusy
* @tc.desc: Test CRUD operations when RdbGeneralStore is busy
* Test scenarios:
* 1. Initialize database and create test table
* 2. Acquire exclusive lock on the database using native RDB interface
* 3. Attempt various write operations (BatchInsert, Insert, Update, Delete) while database is locked
* 4. Verify that all write operations return E_BUSY error due to lock contention
* 5. Clean up test environment
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, RdbGeneralStore_CRUDWhenBusy, TestSize.Level1)
{
    // Step 1: Initialize database environment and create test table
    std::string storeId = "RdbGeneralStore_CRUDWhenBusy.db";
    auto meta = GetStoreMeta(storeId);
    remove(meta.dataDir.c_str());
    auto store = std::make_shared<RdbGeneralStore>(meta, true);
    auto code = store->Init();
    EXPECT_EQ(code, GeneralError::E_OK);

    std::string tableName = "RdbGeneralStore_CRUDWhenBusy";
    EXPECT_EQ(CreateTable(store, tableName), GeneralError::E_OK);

    // Step 2: Acquire exclusive lock on database using native RDB interface
    class RdbOpenCallbackImpl : public NativeRdb::RdbOpenCallback {
    public:
        int OnCreate(NativeRdb::RdbStore &rdbStore) override
        {
            return 0;
        }
        int OnUpgrade(NativeRdb::RdbStore &rdbStore, int currentVersion, int targetVersion) override
        {
            return 0;
        }
    };
    NativeRdb::RdbStoreConfig config(meta.dataDir);
    RdbOpenCallbackImpl callback;
    auto rdb = NativeRdb::RdbHelper::GetRdbStore(config, 0, callback, code);
    EXPECT_EQ(code, NativeRdb::E_OK);
    ASSERT_NE(rdb, nullptr);
    auto [status, _] = rdb->CreateTransaction(NativeRdb::Transaction::EXCLUSIVE);
    EXPECT_EQ(status, NativeRdb::E_OK);

    // Step 3: Prepare test data for write operations
    std::vector<DistributedData::VBucket> values;
    DistributedData::VBucket vBucket;
    vBucket["name"] = "tom";
    vBucket["age"] = 15;
    vBucket["salary"] = 300;
    vBucket["data"] = std::vector<uint8_t>{ 2, 2, 2 };
    const int32_t size = 10;
    for (int i = 0; i < size; i++) {
        values.push_back(vBucket);
    }

    // Step 4: Test BatchInsert operation under busy condition
    int64_t res = -1;
    std::tie(code, res) = store->BatchInsert(tableName, std::move(values), GeneralStore::ON_CONFLICT_REPLACE);
    EXPECT_EQ(code, GeneralError::E_BUSY);
    EXPECT_EQ(res, -1);

    // Step 5: Test Insert operation under busy condition
    std::tie(code, res) = store->Insert(tableName, VBucket(vBucket), GeneralStore::ON_CONFLICT_ABORT);
    EXPECT_EQ(code, GeneralError::E_BUSY);
    EXPECT_EQ(res, -1);

    // Step 6: Test Update operation under busy condition
    RdbGeneralQuery query;
    query.table = tableName;
    std::tie(code, res) = store->Update(query, VBucket(vBucket), GeneralStore::ON_CONFLICT_FAIL);
    EXPECT_EQ(code, GeneralError::E_BUSY);
    EXPECT_EQ(res, -1);

    // Step 7: Test Delete operation under busy condition
    std::tie(code, res) = store->Delete(query);
    EXPECT_EQ(code, GeneralError::E_BUSY);
    EXPECT_EQ(res, -1);

    // Step 8: Clean up test environment
    remove(meta.dataDir.c_str());
}

/**
* @tc.name: RdbGeneralStore_HandleDatabaseCorruption
* @tc.desc: Test handling of database corruption in RdbGeneralStore
* Test scenarios:
* 1. Initialize a valid database and create a test table
* 2. Close the database and corrupt the database file with random data
* 3. Attempt to reopen the corrupted database
* 4. Verify that the corruption is detected and appropriate error is returned
* @tc.type: FUNC
*/
HWTEST_F(RdbGeneralStoreTest, RdbGeneralStore_HandleDatabaseCorruption, TestSize.Level1)
{
    // Step 1: Initialize database environment and create test table
    std::string storeId = "RdbGeneralStore_HandleDatabaseCorruption.db";
    auto meta = GetStoreMeta(storeId);
    remove(meta.dataDir.c_str());
    auto store = std::make_shared<RdbGeneralStore>(meta, true);
    auto code = store->Init();
    EXPECT_EQ(code, GeneralError::E_OK);

    std::string tableName = "RdbGeneralStore_HandleDatabaseCorruption";
    EXPECT_EQ(CreateTable(store, tableName), GeneralError::E_OK);

    // Close the store before corrupting the database file
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);

    // Step 2: Corrupt the database file by overwriting with random data
    EXPECT_TRUE(CorruptDatabaseFile(meta.dataDir, 0, 512));

    // Step 3: Reopen the store - should detect corruption
    store = std::make_shared<RdbGeneralStore>(meta, true);
    code = store->Init();
    EXPECT_EQ(code, GeneralError::E_DB_CORRUPT);

    // Cleanup test environment
    remove(meta.dataDir.c_str());
}

/**
 * @tc.name: RdbGeneralStore_HandlePageCorruption
 * @tc.desc: Test RdbGeneralStore behavior when database pages are corrupted
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest, RdbGeneralStore_HandlePageCorruption, TestSize.Level1)
{
    // Step 1: Initialize database environment and create test table
    std::string storeId = "RdbGeneralStore_HandlePageCorruption.db";
    auto meta = GetStoreMeta(storeId);
    remove(meta.dataDir.c_str());
    auto store = std::make_shared<RdbGeneralStore>(meta, true);
    auto code = store->Init();
    EXPECT_EQ(code, GeneralError::E_OK);

    std::string tableName = "RdbGeneralStore_HandlePageCorruption";
    EXPECT_EQ(CreateTable(store, tableName), GeneralError::E_OK);

    // Step 2: Insert initial data exceeding 2MB
    CorruptDatabasePager(store, meta.dataDir.c_str(), tableName);

    // Step 3: Test BatchInsert operation on corrupted database
    DistributedData::VBucket vBucket;
    vBucket["name"] = "test_user";
    vBucket["age"] = 25;
    vBucket["salary"] = 5000.0;
    vBucket["data"] = std::vector<uint8_t>{ 1, 2, 3 }; // Use large data to exceed 2MB in total
    int64_t res = -1;
    std::vector<DistributedData::VBucket> values = { vBucket };
    std::tie(code, res) = store->BatchInsert(tableName, std::move(values), GeneralStore::ON_CONFLICT_REPLACE);
    EXPECT_EQ(code, GeneralError::E_DB_CORRUPT);
    EXPECT_EQ(res, -1);

    // Step 4: Test Insert operation on corrupted database
    std::tie(code, res) = store->Insert(tableName, VBucket(vBucket), GeneralStore::ON_CONFLICT_ABORT);
    EXPECT_EQ(code, GeneralError::E_DB_CORRUPT);
    EXPECT_EQ(res, -1);

    // Step 5: Test Update operation on corrupted database
    RdbGeneralQuery query;
    query.table = tableName;
    NativeRdb::AbsRdbPredicates predicates(tableName);
    predicates.GreaterThan("age", 18);
    query.whereClause = predicates.GetWhereClause();
    query.args = ValueProxy::Convert(predicates.GetBindArgs());

    vBucket["salary"] = 7000.0;
    std::tie(code, res) = store->Update(query, VBucket(vBucket), GeneralStore::ON_CONFLICT_FAIL);
    EXPECT_EQ(code, GeneralError::E_DB_CORRUPT);
    EXPECT_EQ(res, -1);

    // Step 6: Test Delete operation on corrupted database
    std::tie(code, res) = store->Delete(query);
    EXPECT_EQ(code, GeneralError::E_DB_CORRUPT);
    EXPECT_EQ(res, -1);

    // Step 7: Test Query operation on corrupted database
    std::shared_ptr<Cursor> cursor = nullptr;
    std::tie(code, cursor) = store->Query(query);
    EXPECT_EQ(code, GeneralError::E_OK);
    ASSERT_NE(cursor, nullptr);
    do {
        code = cursor->MoveToNext();
    } while (code == GeneralError::E_OK);
    EXPECT_EQ(code, GeneralError::E_DB_CORRUPT);
    // Cleanup test environment
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}

/**
 * @tc.name: RdbGeneralStore_ValidateInvalidParameters
 * @tc.desc: Validate that update and delete operations correctly return E_INVALID_ARGS when provided with invalid parameters
 * Test scenarios:
 * 1. Initialize database environment and create a test table
 * 2. Execute Update operation with invalid parameter configurations
 * 3. Execute Delete operation with invalid parameter configurations
 * 4. Confirm both operations return the E_INVALID_ARGS error code as expected
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest, RdbGeneralStore_ValidateInvalidParameters, TestSize.Level1)
{
    // Step 1: Initialize database environment and create test table
    std::string storeId = "RdbGeneralStore_ValidateInvalidParameters.db";
    auto meta = GetStoreMeta(storeId);
    remove(meta.dataDir.c_str());
    auto store = std::make_shared<RdbGeneralStore>(meta, true);
    auto code = store->Init();
    EXPECT_EQ(code, GeneralError::E_OK);

    std::string tableName = "RdbGeneralStore_ValidateInvalidParameters";
    EXPECT_EQ(CreateTable(store, tableName), GeneralError::E_OK);

    // Insert initial test data
    DistributedData::VBucket vBucket;
    vBucket["name"] = "test_user";
    vBucket["age"] = 25;
    vBucket["salary"] = 5000.0;
    vBucket["data"] = std::vector<uint8_t>{ 1, 2, 3, 4 };

    int64_t res = -1;
    std::tie(code, res) = store->Insert(tableName, VBucket(vBucket), GeneralStore::ON_CONFLICT_NONE);
    EXPECT_EQ(code, GeneralError::E_OK);
    EXPECT_EQ(res, 1);

    class RdbTmpQuery : public RdbGeneralQuery {
    public:
        std::vector<std::string> GetTables() override
        {
            return tables;
        }
        std::vector<std::string> tables;
    };

    // Step 2: Test Update interface with invalid arguments
    // Test case 1: Empty table name
    RdbTmpQuery query;
    NativeRdb::AbsRdbPredicates predicates(tableName);
    predicates.EqualTo("name", "test_user");
    query.whereClause = predicates.GetWhereClause();
    query.args = ValueProxy::Convert(predicates.GetBindArgs());

    std::tie(code, res) = store->Update(query, VBucket(vBucket), GeneralStore::ON_CONFLICT_IGNORE);
    EXPECT_EQ(code, GeneralError::E_INVALID_ARGS);
    EXPECT_EQ(res, -1);

    std::tie(code, res) = store->Delete(query);
    EXPECT_EQ(code, GeneralError::E_INVALID_ARGS);
    EXPECT_EQ(res, -1);

    // Test case 2: Multiple table names
    query.tables = {"table1", "table2"};
    std::tie(code, res) = store->Update(query, VBucket(vBucket), GeneralStore::ON_CONFLICT_IGNORE);
    EXPECT_EQ(code, GeneralError::E_INVALID_ARGS);
    EXPECT_EQ(res, -1);

    std::tie(code, res) = store->Delete(query);
    EXPECT_EQ(code, GeneralError::E_INVALID_ARGS);
    EXPECT_EQ(res, -1);

    // Step 3: Clean up test environment
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}

/**
 * @tc.name: RdbGeneralStore_BatchInsertWithInconsistentFields
 * @tc.desc: Test BatchInsert with inconsistent field counts in each record
 * Test scenarios:
 * 1. Initialize database and create test table
 * 2. Prepare batch data with inconsistent fields:
 *    - First record: name, age
 *    - Second record: name, salary
 *    - Third record: name, age, salary, data
 * 3. Execute batch insert operation and verify it handles inconsistency properly
 * 4. Query data to verify actual inserted count
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest, RdbGeneralStore_BatchInsertWithInconsistentFields, TestSize.Level1)
{
    // Step 1: Initialize database environment and create test table
    std::string storeId = "RdbGeneralStore_BatchInsertWithInconsistentFields.db";
    auto meta = GetStoreMeta(storeId);
    remove(meta.dataDir.c_str());
    auto store = std::make_shared<RdbGeneralStore>(meta, true);
    auto code = store->Init();
    EXPECT_EQ(code, GeneralError::E_OK);

    std::string tableName = "RdbGeneralStore_BatchInsertWithInconsistentFields";
    EXPECT_EQ(CreateTable(store, tableName), GeneralError::E_OK);

    // Step 2: Prepare batch data with inconsistent fields
    std::vector<DistributedData::VBucket> values;

    // First record: name, age
    DistributedData::VBucket record1;
    record1["name"] = "tom";
    record1["age"] = 25;
    values.push_back(record1);

    // Second record: name, salary
    DistributedData::VBucket record2;
    record2["name"] = "jerry";
    record2["salary"] = 5000.0;
    values.push_back(record2);

    // Third record: name, age, salary, data
    DistributedData::VBucket record3;
    record3["name"] = "spike";
    record3["age"] = 30;
    record3["salary"] = 8000.0;
    record3["data"] = std::vector<uint8_t>{ 1, 2, 3, 4 };
    values.push_back(record3);

    // Step 3: Execute batch insert operation and verify result
    int64_t res = -1;
    std::tie(code, res) = store->BatchInsert(tableName, std::move(values), GeneralStore::ON_CONFLICT_REPLACE);
    EXPECT_EQ(code, GeneralError::E_OK);
    EXPECT_EQ(res, 3);

    // Step 4: Query data to verify actual inserted count
    RdbGeneralQuery query;
    query.table = tableName;
    NativeRdb::AbsRdbPredicates predicates(tableName);
    query.whereClause = predicates.GetWhereClause();
    query.args = ValueProxy::Convert(predicates.GetBindArgs());
    std::shared_ptr<Cursor> cursor = nullptr;
    std::tie(code, cursor) = store->Query(query);
    EXPECT_EQ(code, GeneralError::E_OK);
    ASSERT_NE(cursor, nullptr);

    EXPECT_EQ(cursor->MoveToFirst(), GeneralError::E_OK);
    EXPECT_EQ(cursor->GetCount(), res); // Verify that returned count matches actual data count
    EXPECT_EQ(cursor->Close(), GeneralError::E_OK);

    // Cleanup test environment
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}

} // namespace DistributedRDBTest
} // namespace OHOS::Test