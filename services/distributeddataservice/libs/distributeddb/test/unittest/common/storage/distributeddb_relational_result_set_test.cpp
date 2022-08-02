/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifdef RELATIONAL_STORE
#include <gtest/gtest.h>

#include "db_errno.h"
#include "distributeddb/result_set.h"
#include "distributeddb_tools_unit_test.h"
#include "log_print.h"
#include "relational_result_set_impl.h"
#include "relational_row_data.h"
#include "relational_row_data_impl.h"
#include "relational_row_data_set.h"
#include "relational_store_sqlite_ext.h"
#include "types_export.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace DistributedDBUnitTest;
using namespace std;

namespace {
string g_testDir;
string g_storePath;
string g_tableName { "data" };

const vector<uint8_t> BLOB_VALUE { 'a', 'b', 'l', 'o', 'b', '\0', 'e', 'n', 'd' };
const double DOUBLE_VALUE = 1.123456;  // 1.123456 for test
const int64_t INT64_VALUE = 123456;  // 123456 for test
const std::string STR_VALUE = "I'm a string.";

DataValue g_blobValue;
DataValue g_doubleValue;
DataValue g_int64Value;
DataValue g_nullValue;
DataValue g_strValue;

void InitGlobalValue()
{
    Blob *blob = new (std::nothrow) Blob();
    blob->WriteBlob(BLOB_VALUE.data(), BLOB_VALUE.size());
    g_blobValue.Set(blob);

    g_doubleValue = DOUBLE_VALUE;
    g_int64Value = INT64_VALUE;
    g_strValue = STR_VALUE;
}

void CreateDBAndTable()
{
    sqlite3 *db = nullptr;
    int errCode = sqlite3_open(g_storePath.c_str(), &db);
    if (errCode != SQLITE_OK) {
        LOGE("open db failed:%d", errCode);
        sqlite3_close(db);
        return;
    }

    const string sql =
        "PRAGMA journal_mode=WAL;"
        "CREATE TABLE " + g_tableName + "(key INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, value INTEGER);";
    char *zErrMsg = nullptr;
    errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &zErrMsg);
    if (errCode != SQLITE_OK) {
        LOGE("sql error:%s", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sqlite3_close(db);
}
}

class DistributedDBRelationalResultSetTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void DistributedDBRelationalResultSetTest::SetUpTestCase(void)
{
    DistributedDBToolsUnitTest::TestDirInit(g_testDir);
    InitGlobalValue();
    g_storePath = g_testDir + "/relationalResultSetTest.db";
    LOGI("The test db is:%s", g_testDir.c_str());
}

void DistributedDBRelationalResultSetTest::TearDownTestCase(void) {}

void DistributedDBRelationalResultSetTest::SetUp(void)
{
    DistributedDBToolsUnitTest::PrintTestCaseInfo();
    CreateDBAndTable();
}

void DistributedDBRelationalResultSetTest::TearDown(void)
{
    if (DistributedDBToolsUnitTest::RemoveTestDbFiles(g_testDir) != 0) {
        LOGE("rm test db files error.");
    }
}

/**
 * @tc.name: SerializeAndDeserialize
 * @tc.desc: Serialize and deserialize.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalResultSetTest, SerializeAndDeserialize, TestSize.Level1)
{
    RowData data1 {g_strValue, g_int64Value, g_doubleValue, g_blobValue, g_nullValue};
    RowData data2 {g_nullValue, g_blobValue, g_doubleValue, g_int64Value, g_strValue};  // data1.reverse()

    auto rowDataImpl1 = new (std::nothrow) RelationalRowDataImpl(std::move(data1));
    auto rowDataImpl2 = new (std::nothrow) RelationalRowDataImpl(std::move(data2));

    /**
     * @tc.steps: step1. Create a row data set which contains two row data;
     * @tc.expected: OK.
     */
    RelationalRowDataSet rowDataSet1;
    rowDataSet1.SetColNames({"column 1", "column 2", "column 3", "column 4", "column 5"});
    rowDataSet1.Insert(rowDataImpl1);
    rowDataSet1.Insert(rowDataImpl2);

    /**
     * @tc.steps: step2. Serialize the row data set;
     * @tc.expected: Serialize OK.
     */
    auto bufferLength = rowDataSet1.CalcLength();
    vector<uint8_t> buffer1(bufferLength);
    Parcel parcel1(buffer1.data(), buffer1.size());
    ASSERT_EQ(rowDataSet1.Serialize(parcel1), E_OK);
    ASSERT_FALSE(parcel1.IsError());

    /**
     * @tc.steps: step3. Deserialize the row data set;
     * @tc.expected: Deserialize OK.
     */
    vector<uint8_t> buffer2 = buffer1;
    Parcel parcel2(buffer2.data(), buffer2.size());
    RelationalRowDataSet rowDataSet2;
    ASSERT_EQ(rowDataSet2.DeSerialize(parcel2), E_OK);
    ASSERT_FALSE(parcel2.IsError());

    /**
     * @tc.steps: step4. Compare the deserialized row data set with the original;
     * @tc.expected: Compare OK. They are the same.
     */
    std::vector<std::string> colNames {"column 1", "column 2", "column 3", "column 4", "column 5"};
    EXPECT_EQ(rowDataSet2.GetColNames(), colNames);

    // test the second row. if the second row ok, the first row is likely ok
    StorageType type = StorageType::STORAGE_TYPE_NONE;
    EXPECT_EQ(rowDataSet2.Get(1)->GetType(0, type), DBStatus::OK);
    EXPECT_EQ(type, StorageType::STORAGE_TYPE_NULL);

    vector<uint8_t> desBlob;
    EXPECT_EQ(rowDataSet2.Get(1)->Get(1, desBlob), DBStatus::OK);
    EXPECT_EQ(desBlob, BLOB_VALUE);

    double desDoub;
    EXPECT_EQ(rowDataSet2.Get(1)->Get(2, desDoub), DBStatus::OK);
    EXPECT_EQ(desDoub, DOUBLE_VALUE);

    int64_t desInt64;
    EXPECT_EQ(rowDataSet2.Get(1)->Get(3, desInt64), DBStatus::OK);
    EXPECT_EQ(desInt64, INT64_VALUE);

    std::string desStr;
    EXPECT_EQ(rowDataSet2.Get(1)->Get(4, desStr), DBStatus::OK);
    EXPECT_EQ(desStr, STR_VALUE);
}

/**
 * @tc.name: Put
 * @tc.desc: Test put into result set
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalResultSetTest, Put, TestSize.Level1)
{
    RowData data1 {g_strValue, g_int64Value, g_doubleValue, g_blobValue, g_nullValue};
    RowData data2 {g_nullValue, g_blobValue, g_doubleValue, g_int64Value, g_strValue};  // data1.reverse()
    RowData data3 {g_strValue, g_int64Value, g_doubleValue, g_blobValue, g_nullValue};

    auto rowDataImpl1 = new (std::nothrow) RelationalRowDataImpl(std::move(data1));
    auto rowDataImpl2 = new (std::nothrow) RelationalRowDataImpl(std::move(data2));
    auto rowDataImpl3 = new (std::nothrow) RelationalRowDataImpl(std::move(data3));
    /**
     * @tc.steps: step1. Create 2 row data set which contains 3 row data totally;
     * @tc.expected: OK.
     */
    RelationalRowDataSet rowDataSet1;
    rowDataSet1.Insert(rowDataImpl1);

    RelationalRowDataSet rowDataSet2;
    rowDataSet2.SetRowData({ rowDataImpl2, rowDataImpl3 });

    /**
     * @tc.steps: step2. Put row data set;
     * @tc.expected: The count is in expect.
     */
    RelationalResultSetImpl resultSet;
    resultSet.Put("", 1, std::move(rowDataSet2));
    resultSet.Put("", 2, std::move(rowDataSet1));
    EXPECT_EQ(resultSet.GetCount(), 3);
}

/**
 * @tc.name: EmptyResultSet
 * @tc.desc: Empty result set.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalResultSetTest, EmptyResultSet, TestSize.Level1)
{
    ResultSet *resultSet = new (std::nothrow) RelationalResultSetImpl;
    ASSERT_NE(resultSet, nullptr);

    EXPECT_EQ(resultSet->GetCount(), 0);        // 0 row data
    EXPECT_EQ(resultSet->GetPosition(), -1);    // the init position=-1
    EXPECT_FALSE(resultSet->MoveToFirst());     // move fail. position=-1
    EXPECT_FALSE(resultSet->MoveToLast());      // move fail. position=-1
    EXPECT_FALSE(resultSet->MoveToNext());      // move fail. position=-1
    EXPECT_FALSE(resultSet->MoveToPrevious());  // move fail. position=-1
    EXPECT_FALSE(resultSet->Move(1));           // move fail. position=-1
    EXPECT_FALSE(resultSet->MoveToPosition(0)); // move fail. position=1
    EXPECT_FALSE(resultSet->IsFirst());         // position=1, not the first one
    EXPECT_FALSE(resultSet->IsLast());          // position=1, not the last one
    EXPECT_FALSE(resultSet->IsBeforeFirst());   // position=1, not before the first
    EXPECT_TRUE(resultSet->IsAfterLast());      // position=1, after the last
    EXPECT_FALSE(resultSet->IsClosed());        // not closed
    Entry entry;
    EXPECT_EQ(resultSet->GetEntry(entry), DBStatus::NOT_SUPPORT);   // for relational result set, not support get entry.
    ResultSet::ColumnType columnType;
    EXPECT_EQ(resultSet->GetColumnType(0, columnType), DBStatus::NOT_FOUND);       // the invalid position
    int columnIndex = -1;
    EXPECT_EQ(resultSet->GetColumnIndex("", columnIndex), DBStatus::NONEXISTENT);  // the invalid column name
    int64_t value = 0;
    EXPECT_EQ(resultSet->Get(0, value), DBStatus::NOT_FOUND);  // the invalid position
    std::map<std::string, VariantData> data;
    EXPECT_EQ(resultSet->GetRow(data), DBStatus::NOT_FOUND);   // the invalid position
    delete resultSet;
}

/**
 * @tc.name: NormalResultSet
 * @tc.desc: Normal result set.
 * @tc.type: FUNC
 * @tc.require: AR000GK58G
 * @tc.author: lidongwei
 */
HWTEST_F(DistributedDBRelationalResultSetTest, NormalResultSet, TestSize.Level1)
{
    auto *resultSet = new (std::nothrow) RelationalResultSetImpl;
    ASSERT_NE(resultSet, nullptr);

    /**
     * @tc.steps: step1. Create a result set which contains two row data;
     * @tc.expected: OK.
     */
    RelationalRowDataSet rowDataSet1;
    EXPECT_EQ(rowDataSet1.Insert(new (std::nothrow) RelationalRowDataImpl({g_strValue, g_int64Value})), E_OK);
    RelationalRowDataSet rowDataSet2;
    rowDataSet2.SetColNames({"column 1", "column 2"});
    EXPECT_EQ(rowDataSet2.Insert(new (std::nothrow) RelationalRowDataImpl({g_nullValue, g_blobValue})), E_OK);

    EXPECT_EQ(resultSet->Put("", 2, std::move(rowDataSet2)), E_OK);  // the second one
    EXPECT_EQ(resultSet->Put("", 1, std::move(rowDataSet1)), E_OK);  // the first one

    /**
     * @tc.steps: step2. Check the result set;
     * @tc.expected: All the interface is running in expect.
     */
    EXPECT_EQ(resultSet->GetCount(), 2);        // two row data
    EXPECT_EQ(resultSet->GetPosition(), -1);    // the init position=-1
    EXPECT_TRUE(resultSet->MoveToFirst());      // move ok. position=0
    EXPECT_TRUE(resultSet->MoveToLast());       // move ok. position=1
    EXPECT_FALSE(resultSet->MoveToNext());      // move fail. position=2
    EXPECT_TRUE(resultSet->MoveToPrevious());   // move ok. position=1
    EXPECT_FALSE(resultSet->Move(1));           // move fail. position=2
    EXPECT_TRUE(resultSet->MoveToPosition(0));  // move ok. position=0
    EXPECT_TRUE(resultSet->IsFirst());          // position=0, the first one
    EXPECT_FALSE(resultSet->IsLast());          // position=0, not the last one
    EXPECT_FALSE(resultSet->IsBeforeFirst());   // position=0, not before the first
    EXPECT_FALSE(resultSet->IsAfterLast());     // position=0, not after the last
    EXPECT_FALSE(resultSet->IsClosed());        // not closed

    Entry entry;
    EXPECT_EQ(resultSet->GetEntry(entry), DBStatus::NOT_SUPPORT);   // for relational result set, not support get entry.

    ResultSet::ColumnType columnType;
    EXPECT_EQ(resultSet->GetColumnType(0, columnType), DBStatus::OK);
    EXPECT_EQ(columnType, ResultSet::ColumnType::STRING);

    std::vector<std::string> expectCols { "column 1", "column 2" };
    std::vector<std::string> colNames;
    resultSet->GetColumnNames(colNames);
    EXPECT_EQ(colNames, expectCols);

    int columnIndex = -1;
    EXPECT_EQ(resultSet->GetColumnIndex("column 1", columnIndex), DBStatus::OK);
    EXPECT_EQ(columnIndex, 0);

    int64_t value = 0;
    EXPECT_EQ(resultSet->Get(1, value), DBStatus::OK);
    EXPECT_EQ(value, INT64_VALUE);

    std::map<std::string, VariantData> data;
    EXPECT_EQ(resultSet->GetRow(data), DBStatus::OK);
    EXPECT_EQ(std::get<std::string>(data["column 1"]), STR_VALUE);
    EXPECT_EQ(std::get<int64_t>(data["column 2"]), INT64_VALUE);
    delete resultSet;
}
#endif
