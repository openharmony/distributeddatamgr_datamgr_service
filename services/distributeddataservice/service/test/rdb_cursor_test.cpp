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
#define LOG_TAG "RdbCursorTest"

#include "gtest/gtest.h"
#include "log_print.h"
#include "rdb_cursor.h"
#include "result_set.h"
#include "store/general_value.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
using DBStatus = DistributedDB::DBStatus;
namespace OHOS::Test {
namespace DistributedRDBTest {
static constexpr int MAX_DATA_NUM = 100;
class MockResultSet : public DistributedDB::ResultSet {
public:
    MockResultSet() {}
    virtual ~MockResultSet() {}

    void Close() override
    {
    }

    int GetCount() const override
    {
        return MAX_DATA_NUM;
    }

    bool MoveToFirst() override
    {
        return true;
    }

    bool MoveToNext() override
    {
        return true;
    }

    bool MoveToPrevious() override
    {
        return true;
    }

    bool IsAfterLast() const override
    {
        return true;
    }

    int GetPosition() const override
    {
        return MAX_DATA_NUM;
    }

    bool MoveToLast() override
    {
        return true;
    }

    bool Move(int offset) override
    {
        return true;
    }

    bool MoveToPosition(int position) override
    {
        return true;
    }

    bool IsFirst() const override
    {
        return true;
    }

    bool IsLast() const override
    {
        return true;
    }

    bool IsBeforeFirst() const override
    {
        return true;
    }

    bool IsClosed() const override
    {
        return true;
    }

    DBStatus GetEntry(DistributedDB::Entry &entry) const override
    {
        return DBStatus::OK;
    }

    void GetColumnNames(std::vector<std::string> &columnNames) const override
    {
        columnNames = {"age", "identifier", "name", "phoneNumber"};
    }

    DBStatus GetColumnType(int columnIndex, DistributedDB::ResultSet::ColumnType &columnType) const override
    {
        if (columnIndex < 0) {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus GetColumnIndex(const std::string &columnName, int &columnIndex) const override
    {
        if (columnIndex < 0) {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus GetColumnName(int columnIndex, std::string &columnName) const override
    {
        if (columnIndex < 0) {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus Get(int columnIndex, std::vector<uint8_t> &value) const override
    {
        if (columnIndex < 0) {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus Get(int columnIndex, std::string &value) const override
    {
        if (columnIndex < 0) {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus Get(int columnIndex, int64_t &value) const override
    {
        if (columnIndex < 0) {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus Get(int columnIndex, double &value) const override
    {
        if (columnIndex < 0) {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus IsColumnNull(int columnIndex, bool &isNull) const override
    {
        if (columnIndex < 0) {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus GetRow(std::map<std::string, DistributedDB::VariantData> &data) const override
    {
        return DBStatus::OK;
    }
};

class RdbCursorTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
protected:
    static std::shared_ptr<MockResultSet> resultSet;
    static std::shared_ptr<RdbCursor> rdbCursor;
};
std::shared_ptr<MockResultSet> RdbCursorTest::resultSet = std::make_shared<MockResultSet>();
std::shared_ptr<RdbCursor> RdbCursorTest::rdbCursor = std::make_shared<RdbCursor>(std::move(resultSet));

/**
* @tc.name: RdbCursorTest001
* @tc.desc: RdbCacheCursor function and error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbCursorTest, RdbCursorTest001, TestSize.Level1)
{
    EXPECT_NE(rdbCursor, nullptr);
    std::vector<std::string> expectedNames = {"age", "identifier", "name", "phoneNumber"};
    std::vector<std::string> names;
    auto result = rdbCursor->GetColumnNames(names);
    EXPECT_EQ(result, GeneralError::E_OK);
    EXPECT_EQ(names, expectedNames);

    std::string colName = "colName";
    auto err = rdbCursor->GetColumnName(1, colName);
    EXPECT_EQ(err, GeneralError::E_OK);

    int32_t type = rdbCursor->GetColumnType(0);
    EXPECT_EQ(type, TYPE_INDEX<std::monostate>);
    type = rdbCursor->GetColumnType(-1);
    EXPECT_EQ(type, TYPE_INDEX<std::monostate>);

    int32_t count = rdbCursor->GetCount();
    EXPECT_EQ(count, MAX_DATA_NUM);

    err = rdbCursor->MoveToFirst();
    EXPECT_EQ(err, GeneralError::E_OK);

    err = rdbCursor->MoveToNext();
    EXPECT_EQ(err, GeneralError::E_OK);

    err = rdbCursor->MoveToPrev();
    EXPECT_EQ(err, GeneralError::E_OK);
}

/**
* @tc.name: RdbCursorTest002
* @tc.desc: RdbCacheCursor function and error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbCursorTest, RdbCursorTest002, TestSize.Level1)
{
    EXPECT_NE(rdbCursor, nullptr);
    DistributedData::VBucket data;
    auto result = rdbCursor->GetEntry(data);
    EXPECT_EQ(result, GeneralError::E_OK);

    result = rdbCursor->GetRow(data);
    EXPECT_EQ(result, GeneralError::E_OK);

    DistributedData::Value value;
    result = rdbCursor->Get(1, value);
    EXPECT_EQ(result, GeneralError::E_OK);
    result = rdbCursor->Get(-1, value);
    EXPECT_EQ(result, GeneralError::E_ERROR);

    std::string col = "col";
    result = rdbCursor->Get(col, value);
    EXPECT_EQ(result, GeneralError::E_ERROR);

    bool ret = rdbCursor->IsEnd();
    EXPECT_EQ(ret, true);

    result = rdbCursor->Close();
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Convert
* @tc.desc: Convert function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbCursorTest, Convert, TestSize.Level1)
{
    EXPECT_NE(rdbCursor, nullptr);
    DistributedDB::ResultSet::ColumnType dbColumnType = DistributedDB::ResultSet::ColumnType::INT64;
    int32_t result = rdbCursor->Convert(dbColumnType);
    EXPECT_EQ(result, TYPE_INDEX<int64_t>);

    dbColumnType = DistributedDB::ResultSet::ColumnType::STRING;
    result = rdbCursor->Convert(dbColumnType);
    EXPECT_EQ(result, TYPE_INDEX<std::string>);

    dbColumnType = DistributedDB::ResultSet::ColumnType::BLOB;
    result = rdbCursor->Convert(dbColumnType);
    EXPECT_EQ(result, TYPE_INDEX<std::vector<uint8_t>>);

    dbColumnType = DistributedDB::ResultSet::ColumnType::DOUBLE;
    result = rdbCursor->Convert(dbColumnType);
    EXPECT_EQ(result, TYPE_INDEX<double>);

    dbColumnType = DistributedDB::ResultSet::ColumnType::INVALID_TYPE;
    result = rdbCursor->Convert(dbColumnType);
    EXPECT_EQ(result, TYPE_INDEX<std::monostate>);

    dbColumnType = DistributedDB::ResultSet::ColumnType::NULL_VALUE;
    result = rdbCursor->Convert(dbColumnType);
    EXPECT_EQ(result, TYPE_INDEX<std::monostate>);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test