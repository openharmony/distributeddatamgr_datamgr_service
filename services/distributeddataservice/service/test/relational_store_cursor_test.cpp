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
#define LOG_TAG "RelationalStoreCursorTest"

#include "relational_store_cursor.h"

#include "gtest/gtest.h"
#include "rdb_errno.h"
#include "rdb_types.h"
#include "log_print.h"
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
class MockRdbResultSet : public NativeRdb::ResultSet {
public:
    MockRdbResultSet() {}
    virtual ~MockRdbResultSet() {}

    int GetAllColumnNames(std::vector<std::string> &columnNames) override
    {
        columnNames = { "age", "identifier", "name", "phoneNumber" };
        return NativeRdb::E_OK;
    }

    int GetColumnName(int columnIndex, std::string &columnName) override
    {
        columnName = "name";
        return NativeRdb::E_OK;
    }

    int GetColumnType(int columnIndex, ColumnType &columnType) override
    {
        columnType = ColumnType::TYPE_NULL;
        return NativeRdb::E_OK;
    }

    int GetRowCount(int &count) override
    {
        count = MAX_DATA_NUM;
        return NativeRdb::E_OK;
    }

    int GoToFirstRow() override
    {
        return NativeRdb::E_OK;
    }

    int GoToNextRow() override
    {
        return NativeRdb::E_OK;
    }

    int GoToPreviousRow() override
    {
        return NativeRdb::E_OK;
    }

    int GetRow(NativeRdb::RowEntity &rowEntity) override
    {
        return NativeRdb::E_OK;
    }

    int Get(int32_t col, NativeRdb::ValueObject &value) override
    {
        return NativeRdb::E_OK;
    }

    int GetColumnIndex(const std::string &columnName, int &columnIndex) override
    {
        columnIndex = 1;
        return NativeRdb::E_OK;
    }

    int Close() override
    {
        return NativeRdb::E_OK;
    }

    int IsEnded(bool &result) override
    {
        result = true;
        return NativeRdb::E_OK;
    }
    int GetColumnCount(int &count) override
    {
        return NativeRdb::E_OK;
    }

    int GetRowIndex(int &position) const override
    {
        return NativeRdb::E_OK;
    }

    int GoTo(int offset) override
    {
        return NativeRdb::E_OK;
    }

    int GoToRow(int position) override
    {
        return NativeRdb::E_OK;
    }

    int GoToLastRow() override
    {
        return NativeRdb::E_OK;
    }

    int IsStarted(bool &result) const override
    {
        return NativeRdb::E_OK;
    }

    int IsAtFirstRow(bool &result) const override
    {
        return NativeRdb::E_OK;
    }

    int IsAtLastRow(bool &result) override
    {
        return NativeRdb::E_OK;
    }

    int GetBlob(int columnIndex, std::vector<uint8_t> &blob) override
    {
        return NativeRdb::E_OK;
    }

    int GetString(int columnIndex, std::string &value) override
    {
        return NativeRdb::E_OK;
    }

    int GetInt(int columnIndex, int &value) override
    {
        return NativeRdb::E_OK;
    }

    int GetLong(int columnIndex, int64_t &value) override
    {
        return NativeRdb::E_OK;
    }

    int GetDouble(int columnIndex, double &value) override
    {
        return NativeRdb::E_OK;
    }

    int IsColumnNull(int columnIndex, bool &isNull) override
    {
        return NativeRdb::E_OK;
    }

    bool IsClosed() const override
    {
        return true;
    }

    int GetAsset(int32_t col, NativeRdb::ValueObject::Asset &value) override
    {
        return NativeRdb::E_OK;
    }

    int GetAssets(int32_t col, NativeRdb::ValueObject::Assets &value) override
    {
        return NativeRdb::E_OK;
    }

    int GetSize(int columnIndex, size_t &size) override
    {
        return NativeRdb::E_OK; 
    }
};

class RelationalStoreCursorTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};

protected:
    static std::shared_ptr<MockRdbResultSet> resultSet;
    static std::shared_ptr<RelationalStoreCursor> relationalStoreCursor;
};
std::shared_ptr<MockRdbResultSet> RelationalStoreCursorTest::resultSet(new MockRdbResultSet());
std::shared_ptr<RelationalStoreCursor> RelationalStoreCursorTest::relationalStoreCursor =
    std::make_shared<RelationalStoreCursor>(resultSet);

/**
* @tc.name: RelationalStoreCursorTest001
* @tc.desc: RelationalStoreCursor function and error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RelationalStoreCursorTest, RelationalStoreCursorTest001, TestSize.Level1)
{
    EXPECT_NE(relationalStoreCursor, nullptr);
    std::vector<std::string> expectedNames = {"age", "identifier", "name", "phoneNumber"};
    std::vector<std::string> names;
    auto result = relationalStoreCursor->GetColumnNames(names);
    EXPECT_EQ(result, GeneralError::E_OK);
    EXPECT_EQ(names, expectedNames);

    std::string colName;
    auto err = relationalStoreCursor->GetColumnName(1, colName);
    EXPECT_EQ(err, GeneralError::E_OK);

    int32_t type = relationalStoreCursor->GetColumnType(0);
    EXPECT_EQ(type, static_cast<int32_t>(ColumnType::TYPE_NULL));
    type = relationalStoreCursor->GetColumnType(-1);
    EXPECT_EQ(type, static_cast<int32_t>(ColumnType::TYPE_NULL));

    int32_t count = relationalStoreCursor->GetCount();
    EXPECT_EQ(count, MAX_DATA_NUM);

    err = relationalStoreCursor->MoveToFirst();
    EXPECT_EQ(err, GeneralError::E_OK);

    err = relationalStoreCursor->MoveToNext();
    EXPECT_EQ(err, GeneralError::E_OK);

    err = relationalStoreCursor->MoveToPrev();
    EXPECT_EQ(err, GeneralError::E_OK);
}

/**
* @tc.name: RelationalStoreCursorTest002
* @tc.desc: RelationalStoreCursor function and error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RelationalStoreCursorTest, RelationalStoreCursorTest002, TestSize.Level1)
{
    EXPECT_NE(relationalStoreCursor, nullptr);
    DistributedData::VBucket data;
    auto result = relationalStoreCursor->GetEntry(data);
    EXPECT_EQ(result, GeneralError::E_OK);

    result = relationalStoreCursor->GetRow(data);
    EXPECT_EQ(result, GeneralError::E_OK);

    DistributedData::Value value;
    result = relationalStoreCursor->Get(1, value);
    EXPECT_EQ(result, GeneralError::E_OK);
    result = relationalStoreCursor->Get(-1, value);
    EXPECT_EQ(result, GeneralError::E_INVALID_ARGS);

    result = relationalStoreCursor->Get("col", value);
    EXPECT_EQ(result, GeneralError::E_OK);
    bool ret = relationalStoreCursor->IsEnd();
    EXPECT_EQ(ret, true);

    result = relationalStoreCursor->Close();
    EXPECT_EQ(result, GeneralError::E_OK);
}

} // namespace DistributedRDBTest
} // namespace OHOS::Test