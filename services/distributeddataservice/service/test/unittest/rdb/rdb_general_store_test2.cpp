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
#define LOG_TAG "RdbGeneralStoreTest2"
 
#include "rdb_general_store.h"
 
#include <random>
#include <thread>
 
#include "bootstrap.h"
#include "cloud_db_mock.h"
#include "cloud/schema_meta.h"
#include "device_matrix.h"
#include "error/general_error.h"
#include "errors.h"
#include "eventcenter/event_center.h"
#include "gtest/gtest.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "mock/db_store_mock.h"
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
 
namespace OHOS::Test {
namespace DistributedRDBTest {
using StoreMetaData = OHOS::DistributedData::StoreMetaData;
using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
 
class RdbGeneralStoreTest2 : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp()
    {
        mockCloudDB2 = std::make_shared<MockCloudDB>();
        Bootstrap::GetInstance().LoadDirectory();
        InitMetaData();
        InitDataBase();
        InitMetaDataManager();
        mkdir(metaData_.dataDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
        store_ = std::make_shared<RdbGeneralStore>(metaData_);
        MockRelationalStoreDelegate::SetCloudSyncTaskCount(0);
        MockRelationalStoreDelegate::SetDownloadingAssetsCount(0);
        MockRelationalStoreDelegate::SetDeviceTaskCount(0);
        MockRelationalStoreDelegate::SetResSetStoreConfig(DBStatus::OK);
        MockRelationalStoreDelegate::SetResSetDbSchema(DBStatus::OK);
        MockRelationalStoreDelegate::SetResSync(DBStatus::OK);
        MockRelationalStoreDelegate::SetResRemove(DBStatus::OK);
    };
    void TearDown()
    {
    };
 
protected:
    void InitMetaData();
    void InitDataBase();
    static void InitMetaDataManager();
    static StoreMetaData GetStoreMeta(const std::string &storeName, int32_t area = 1, int32_t user = 0);
    static std::pair<std::shared_ptr<RdbGeneralStore>, StoreMetaData> InitRdbStore(const std::string &storeId,
        const std::string &tableName, bool encrypt = false);
    StoreMetaData metaData_;
    DistributedData::Database dataBase_;
    std::shared_ptr<RdbGeneralStore> store_;
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static inline std::shared_ptr<MockCloudDB> mockCloudDB2 = nullptr;
};
 
std::shared_ptr<DBStoreMock> RdbGeneralStoreTest2::dbStoreMock_ = std::make_shared<DBStoreMock>();
 
void RdbGeneralStoreTest2::InitMetaDataManager()
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    MetaDataManager::GetInstance().SetSyncer([](const auto &, auto) {});
}
 
void RdbGeneralStoreTest2::InitDataBase()
{
    DistributedData::Field field;
    field.colName = "field_name";
    field.type = 0;
    field.primary = true;
    field.autoIncrement = false;
    std::vector<DistributedData::Field> fields;
    std::vector<std::string> deviceSyncFields;
    deviceSyncFields.push_back("field_name");
    fields.push_back(field);
    DistributedData::Table table;
    table.name = "table_name";
    table.deviceSyncFields = deviceSyncFields;
    table.fields = fields;
    std::vector<DistributedData::Table> tables;
    tables.push_back(table);
    dataBase_.bundleName = "test_rdb_general_store";
    dataBase_.user = "0";
    dataBase_.deviceId = "deviceId";
    dataBase_.name = "test_service_rdb_errorDb";
    dataBase_.tables = tables;
}
 
void RdbGeneralStoreTest2::InitMetaData()
{
    metaData_.bundleName = "test_rdb_general_store";
    metaData_.appId = "test_rdb_general_store";
    metaData_.user = "0";
    metaData_.area = DistributedKv::Area::EL1;
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = 1;
    metaData_.storeId = "test_service_rdb_errorDb";
    metaData_.dataDir = "/data/service/el1/public/database/test_rdb_general_store/rdb";
    mkdir(metaData_.dataDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}
 
StoreMetaData RdbGeneralStoreTest2::GetStoreMeta(const std::string &storeName, int32_t area, int32_t user)
{
    StoreMetaData meta;
    meta.bundleName = "test_rdb_general_store";
    meta.appId = "test_rdb_general_store";
    meta.user = std::to_string(user);
    meta.area = area;
    meta.instanceId = 0;
    meta.isAutoSync = true;
    meta.storeType = 1;
    meta.storeId = storeName;
    meta.dataDir = "/data/service/el1/public/database/test_rdb_general_store/rdb/" + storeName;
    return meta;
}
 
std::pair<std::shared_ptr<RdbGeneralStore>, StoreMetaData> RdbGeneralStoreTest2::InitRdbStore(
    const std::string &storeId, const std::string &tableName, bool encrypt)
{
    StoreMetaData meta = GetStoreMeta(storeId);
    if (encrypt) {
        meta.isEncrypt = encrypt;
    }
    auto store = std::make_shared<RdbGeneralStore>(meta);
    store->Init();
    return {store, meta};
}
 
/**
 * @tc.name: SetBinlogEnabled_DatabaseClosed
 * @tc.desc: Test SetBinlogEnabled returns E_ALREADY_CLOSED when database is closed
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, SetBinlogEnabled_DatabaseClosed, TestSize.Level1)
{
    std::shared_ptr<RdbGeneralStore> store = std::make_shared<RdbGeneralStore>(metaData_);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    auto result = store->SetBinlogEnabled(true);
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);
}
 
/**
 * @tc.name: SetBinlogEnabled_DelegateNull
 * @tc.desc: Test SetBinlogEnabled returns E_ALREADY_CLOSED when delegate is null
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, SetBinlogEnabled_DelegateNull, TestSize.Level1)
{
    std::shared_ptr<RdbGeneralStore> store = std::make_shared<RdbGeneralStore>(metaData_);
    auto result = store->SetBinlogEnabled(true);
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);
}
 
/**
 * @tc.name: SetBinlogEnabled_Success
 * @tc.desc: Test SetBinlogEnabled returns E_OK when all conditions are met
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, SetBinlogEnabled_Success, TestSize.Level1)
{
    std::string storeId = "SetBinlogEnabled_Success.db";
    std::string tableName = "SetBinlogEnabled_Success";
    auto[store, meta] = InitRdbStore(storeId, tableName);
    MockRelationalStoreDelegate::SetResSetBinlogEnabled(DBStatus::OK);
    auto result = store->SetBinlogEnabled(true);
    EXPECT_EQ(result, GeneralError::E_OK);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}
 
/**
 * @tc.name: SetBinlogEnabled_Failed
 * @tc.desc: Test SetBinlogEnabled returns E_ERROR when delegate returns error
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, SetBinlogEnabled_Failed, TestSize.Level1)
{
    std::string storeId = "SetBinlogEnabled_Failed.db";
    std::string tableName = "SetBinlogEnabled_Failed";
    auto[store, meta] = InitRdbStore(storeId, tableName);
    MockRelationalStoreDelegate::SetResSetBinlogEnabled(DBStatus::DB_ERROR);
    auto result = store->SetBinlogEnabled(true);
    EXPECT_EQ(result, GeneralError::E_ERROR);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}
 
/**
 * @tc.name: QuerySubscribeOutput_DatabaseClosed
 * @tc.desc: Test QuerySubscribeOutput returns E_ALREADY_CLOSED when database is closed
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, QuerySubscribeOutput_DatabaseClosed, TestSize.Level1)
{
    std::shared_ptr<RdbGeneralStore> store = std::make_shared<RdbGeneralStore>(metaData_);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    SubscribeCur cursorIn{SubQueryType::GET_ALL, 0};
    SubscribeCur cursorOut{SubQueryType::GET_ALL, 0};
    VBuckets dataOut;
    auto result = store->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);
}
 
/**
 * @tc.name: QuerySubscribeOutput_DelegateNull
 * @tc.desc: Test QuerySubscribeOutput returns E_ALREADY_CLOSED when delegate is null
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, QuerySubscribeOutput_DelegateNull, TestSize.Level1)
{
    std::shared_ptr<RdbGeneralStore> store = std::make_shared<RdbGeneralStore>(metaData_);
    SubscribeCur cursorIn{SubQueryType::GET_ALL, 0};
    SubscribeCur cursorOut{SubQueryType::GET_ALL, 0};
    VBuckets dataOut;
    auto result = store->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);
}
 
/**
 * @tc.name: QuerySubscribeOutput_Success
 * @tc.desc: Test QuerySubscribeOutput returns E_OK when all conditions are met
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, QuerySubscribeOutput_Success, TestSize.Level1)
{
    std::string storeId = "QuerySubscribeOutput_Success.db";
    std::string tableName = "QuerySubscribeOutput_Success";
    auto[store, meta] = InitRdbStore(storeId, tableName);
    MockRelationalStoreDelegate::SetResQuerySubscribeOutput(DBStatus::OK);
    SubscribeCur cursorIn{SubQueryType::GET_ALL, 0};
    SubscribeCur cursorOut{SubQueryType::GET_ALL, 0};
    VBuckets dataOut;
    auto result = store->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
    EXPECT_EQ(result, GeneralError::E_OK);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}
 
/**
 * @tc.name: QuerySubscribeOutput_SubscribeQueryEnd
 * @tc.desc: Test QuerySubscribeOutput returns E_OK when delegate returns SUBSCRIBE_QUERY_END
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, QuerySubscribeOutput_SubscribeQueryEnd, TestSize.Level1)
{
    std::string storeId = "QuerySubscribeOutput_SubscribeQueryEnd.db";
    std::string tableName = "QuerySubscribeOutput_SubscribeQueryEnd";
    auto[store, meta] = InitRdbStore(storeId, tableName);
    MockRelationalStoreDelegate::SetResQuerySubscribeOutput(DBStatus::SUBSCRIBE_QUERY_END);
    SubscribeCur cursorIn{SubQueryType::GET_NEW, 100};
    SubscribeCur cursorOut{SubQueryType::GET_NEW, 100};
    VBuckets dataOut;
    auto result = store->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
    EXPECT_EQ(result, 27328588);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}
 
/**
 * @tc.name: QuerySubscribeOutput_Failed
 * @tc.desc: Test QuerySubscribeOutput returns E_ERROR when delegate returns error
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, QuerySubscribeOutput_Failed, TestSize.Level1)
{
    std::string storeId = "QuerySubscribeOutput_Failed.db";
    std::string tableName = "QuerySubscribeOutput_Failed";
    auto[store, meta] = InitRdbStore(storeId, tableName);
    MockRelationalStoreDelegate::SetResQuerySubscribeOutput(DBStatus::DB_ERROR);
    SubscribeCur cursorIn{SubQueryType::GET_ALL, 0};
    SubscribeCur cursorOut{SubQueryType::GET_ALL, 0};
    VBuckets dataOut;
    auto result = store->QuerySubscribeOutput(cursorIn, cursorOut, dataOut);
    EXPECT_EQ(result, GeneralError::E_ERROR);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}
 
/**
 * @tc.name: SetSubscribeCursor_DatabaseClosed
 * @tc.desc: Test SetSubscribeCursor returns E_ALREADY_CLOSED when database is closed
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, SetSubscribeCursor_DatabaseClosed, TestSize.Level1)
{
    std::shared_ptr<RdbGeneralStore> store = std::make_shared<RdbGeneralStore>(metaData_);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    SubscribeCur cursorIn{SubQueryType::GET_ALL, 0};
    auto result = store->SetSubscribeCursor(cursorIn);
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);
}
 
/**
 * @tc.name: SetSubscribeCursor_DelegateNull
 * @tc.desc: Test SetSubscribeCursor returns E_ALREADY_CLOSED when delegate is null
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, SetSubscribeCursor_DelegateNull, TestSize.Level1)
{
    std::shared_ptr<RdbGeneralStore> store = std::make_shared<RdbGeneralStore>(metaData_);
    SubscribeCur cursorIn{SubQueryType::GET_ALL, 0};
    auto result = store->SetSubscribeCursor(cursorIn);
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);
}
 
/**
 * @tc.name: SetSubscribeCursor_Success
 * @tc.desc: Test SetSubscribeCursor returns E_OK when all conditions are met
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, SetSubscribeCursor_Success, TestSize.Level1)
{
    std::string storeId = "SetSubscribeCursor_Success.db";
    std::string tableName = "SetSubscribeCursor_Success";
    auto[store, meta] = InitRdbStore(storeId, tableName);
    MockRelationalStoreDelegate::SetResSetSubscribeCursor(DBStatus::OK);
    SubscribeCur cursorIn{SubQueryType::GET_ALL, 0};
    auto result = store->SetSubscribeCursor(cursorIn);
    EXPECT_EQ(result, GeneralError::E_OK);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}
 
/**
 * @tc.name: SetSubscribeCursor_Failed
 * @tc.desc: Test SetSubscribeCursor returns E_ERROR when delegate returns error
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, SetSubscribeCursor_Failed, TestSize.Level1)
{
    std::string storeId = "SetSubscribeCursor_Failed.db";
    std::string tableName = "SetSubscribeCursor_Failed";
    auto[store, meta] = InitRdbStore(storeId, tableName);
    MockRelationalStoreDelegate::SetResSetSubscribeCursor(DBStatus::DB_ERROR);
    SubscribeCur cursorIn{SubQueryType::GET_ALL, 0};
    auto result = store->SetSubscribeCursor(cursorIn);
    EXPECT_EQ(result, GeneralError::E_ERROR);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}
 
/**
 * @tc.name: SetSubscribeSchema_DatabaseClosed
 * @tc.desc: Test SetSubscribeSchema returns E_ALREADY_CLOSED when database is closed
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, SetSubscribeSchema_DatabaseClosed, TestSize.Level1)
{
    std::shared_ptr<RdbGeneralStore> store = std::make_shared<RdbGeneralStore>(metaData_);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    auto result = store->SetSubscribeSchema("test_schema");
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);
}
 
/**
 * @tc.name: SetSubscribeSchema_DelegateNull
 * @tc.desc: Test SetSubscribeSchema returns E_ALREADY_CLOSED when delegate is null
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, SetSubscribeSchema_DelegateNull, TestSize.Level1)
{
    std::shared_ptr<RdbGeneralStore> store = std::make_shared<RdbGeneralStore>(metaData_);
    auto result = store->SetSubscribeSchema("test_schema");
    EXPECT_EQ(result, GeneralError::E_ALREADY_CLOSED);
}
 
/**
 * @tc.name: SetSubscribeSchema_Success
 * @tc.desc: Test SetSubscribeSchema returns E_OK when all conditions are met
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, SetSubscribeSchema_Success, TestSize.Level1)
{
    std::string storeId = "SetSubscribeSchema_Success.db";
    std::string tableName = "SetSubscribeSchema_Success";
    auto[store, meta] = InitRdbStore(storeId, tableName);
    MockRelationalStoreDelegate::SetResSetSubscribeSchema(DBStatus::OK);
    auto result = store->SetSubscribeSchema("test_schema");
    EXPECT_EQ(result, GeneralError::E_OK);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}
 
/**
 * @tc.name: SetSubscribeSchema_Failed
 * @tc.desc: Test SetSubscribeSchema returns E_ERROR when delegate returns error
 * @tc.type: FUNC
 */
HWTEST_F(RdbGeneralStoreTest2, SetSubscribeSchema_Failed, TestSize.Level1)
{
    std::string storeId = "SetSubscribeSchema_Failed.db";
    std::string tableName = "SetSubscribeSchema_Failed";
    auto[store, meta] = InitRdbStore(storeId, tableName);
    MockRelationalStoreDelegate::SetResSetSubscribeSchema(DBStatus::DB_ERROR);
    auto result = store->SetSubscribeSchema("test_schema");
    EXPECT_EQ(result, GeneralError::E_ERROR);
    EXPECT_EQ(store->Close(true), GeneralError::E_OK);
    remove(meta.dataDir.c_str());
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test