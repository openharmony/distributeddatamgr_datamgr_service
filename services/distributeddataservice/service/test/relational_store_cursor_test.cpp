/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "log_print.h"
#include "rdb_errno.h"
#include "rdb_types.h"
#include "result_set.h"
#include "store/general_value.h"

using namespace OHOS;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
namespace DistributedRDBTest {
static constexpr int AGE = 25;
static constexpr const char *NAME = "tony";
static constexpr double SALARY = 1311.0;
static constexpr bool FLAG = true;
static constexpr int INDEX_SALARY = 2;
static constexpr int INDEX_FLAG = 3;
static constexpr int INDEX_BLOB = 4;
static std::vector<uint8_t> BLOB = { 0x01, 0x02, 0x03, 0x04, 0x05 };
static std::vector<std::string> EXCEPTED_COLUMN_NAMES = { "age", "blob", "flag", "name", "salary" };
static std::vector<NativeRdb::ColumnType> EXCEPTED_COLUMN_TYPE = { NativeRdb::ColumnType::TYPE_INTEGER,
    NativeRdb::ColumnType::TYPE_BLOB, NativeRdb::ColumnType::TYPE_INTEGER, NativeRdb::ColumnType::TYPE_STRING,
    NativeRdb::ColumnType::TYPE_FLOAT };
class MockRdbResultSet : public NativeRdb::ResultSet {
public:
    NativeRdb::RowEntity entity;
    MockRdbResultSet() {}
    virtual ~MockRdbResultSet() {}

    int GetAllColumnNames(std::vector<std::string> &columnNames) override
    {
        columnNames = EXCEPTED_COLUMN_NAMES;
        return NativeRdb::E_OK;
    }

    int GetColumnName(int columnIndex, std::string &columnName) override
    {
        if (columnIndex < 0 || columnIndex >= EXCEPTED_COLUMN_NAMES.size()) {
            return NativeRdb::E_COLUMN_OUT_RANGE;
        }
        columnName = EXCEPTED_COLUMN_NAMES[columnIndex];
        return NativeRdb::E_OK;
    }

    int GetColumnType(int columnIndex, ColumnType &columnType) override
    {
        if (columnIndex < 0 || columnIndex >= EXCEPTED_COLUMN_TYPE.size()) {
            return NativeRdb::E_COLUMN_OUT_RANGE;
        }
        columnType = EXCEPTED_COLUMN_TYPE[columnIndex];
        return NativeRdb::E_OK;
    }

    int GetRowCount(int &count) override
    {
        count = 1;
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
        rowEntity = entity;
        return NativeRdb::E_OK;
    }

    int Get(int32_t col, NativeRdb::ValueObject &value) override
    {
        value = entity.Get(col);
        return NativeRdb::E_OK;
    }

    int GetColumnIndex(const std::string &columnName, int &columnIndex) override
    {
        int index = 0;
        for (auto &name : EXCEPTED_COLUMN_NAMES) {
            if (name == columnName) {
                columnIndex = index;
                return NativeRdb::E_OK;
            }
            index++;
        }
        return NativeRdb::E_ERROR;
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
        count = EXCEPTED_COLUMN_NAMES.size();
        return NativeRdb::E_OK;
    }

    int GetRowIndex(int &position) const override
    {
        position = 0;
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
    void SetUp()
    {
        std::shared_ptr<MockRdbResultSet> resultSet = std::make_shared<MockRdbResultSet>();
        NativeRdb::RowEntity entity;
        entity.Clear(5);
        entity.Put("age", 0, NativeRdb::ValueObject(AGE));
        entity.Put("name", 1, NativeRdb::ValueObject(NAME));
        entity.Put("salary", INDEX_SALARY, NativeRdb::ValueObject(SALARY));
        entity.Put("flag", INDEX_FLAG, NativeRdb::ValueObject(FLAG));
        entity.Put("blob", INDEX_BLOB, NativeRdb::ValueObject(BLOB));
        resultSet->entity = entity;
        relationalStoreCursor_ = std::make_shared<RelationalStoreCursor>(*resultSet, std::move(resultSet));
    };
    void TearDown(){};

private:
    std::shared_ptr<RelationalStoreCursor> relationalStoreCursor_;
};

/**
* @tc.name: RelationalStoreCursorTest_GetColumnNames
* @tc.desc: Test GetColumnNames function of RelationalStoreCursor.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RelationalStoreCursorTest, RelationalStoreCursorTest_GetColumnNames, TestSize.Level1)
{
    ASSERT_NE(relationalStoreCursor_, nullptr);
    std::vector<std::string> names;
    auto result = relationalStoreCursor_->GetColumnNames(names);
    EXPECT_EQ(result, GeneralError::E_OK);
    EXPECT_EQ(names, EXCEPTED_COLUMN_NAMES);
}

/**
* @tc.name: RelationalStoreCursorTest_GetColumnName
* @tc.desc: Test GetColumnName function of RelationalStoreCursor.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RelationalStoreCursorTest, RelationalStoreCursorTest_GetColumnName, TestSize.Level1)
{
    ASSERT_NE(relationalStoreCursor_, nullptr);
    std::string colName;
    auto result = relationalStoreCursor_->GetColumnName(1, colName);
    EXPECT_EQ(colName, EXCEPTED_COLUMN_NAMES[1]);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: RelationalStoreCursorTest_GetColumnType
* @tc.desc: Test GetColumnType function of RelationalStoreCursor.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RelationalStoreCursorTest, RelationalStoreCursorTest_GetColumnType, TestSize.Level1)
{
    ASSERT_NE(relationalStoreCursor_, nullptr);
    int32_t type = relationalStoreCursor_->GetColumnType(0);
    EXPECT_EQ(type, static_cast<int32_t>(EXCEPTED_COLUMN_TYPE[0]));
}

/**
* @tc.name: RelationalStoreCursorTest_GetCount
* @tc.desc: Test GetCount function of RelationalStoreCursor.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RelationalStoreCursorTest, RelationalStoreCursorTest_GetCount, TestSize.Level1)
{
    ASSERT_NE(relationalStoreCursor_, nullptr);
    int32_t count = relationalStoreCursor_->GetCount();
    EXPECT_EQ(count, 1);
}

/**
* @tc.name: RelationalStoreCursorTest_MoveOperations
* @tc.desc: Test move operations (MoveToFirst, MoveToNext, MoveToPrev) of RelationalStoreCursor.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RelationalStoreCursorTest, RelationalStoreCursorTest_MoveOperations, TestSize.Level1)
{
    ASSERT_NE(relationalStoreCursor_, nullptr);

    auto result = relationalStoreCursor_->MoveToFirst();
    EXPECT_EQ(result, GeneralError::E_OK);

    result = relationalStoreCursor_->MoveToNext();
    EXPECT_EQ(result, GeneralError::E_OK);

    result = relationalStoreCursor_->MoveToPrev();
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: RelationalStoreCursorTest_GetEntry
* @tc.desc: Test GetEntry function of RelationalStoreCursor to retrieve row data.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RelationalStoreCursorTest, RelationalStoreCursorTest_GetEntry, TestSize.Level1)
{
    ASSERT_NE(relationalStoreCursor_, nullptr);
    DistributedData::VBucket data;
    auto result = relationalStoreCursor_->GetEntry(data);
    EXPECT_EQ(result, GeneralError::E_OK);
    auto ageIt = Traits::get_if<int64_t>(&data["age"]);
    ASSERT_NE(ageIt, nullptr);
    EXPECT_EQ(*ageIt, AGE);

    auto nameIt = Traits::get_if<std::string>(&data["name"]);
    ASSERT_NE(nameIt, nullptr);
    EXPECT_EQ(*nameIt, NAME);

    auto blobIt = Traits::get_if<std::vector<uint8_t>>(&data["blob"]);
    ASSERT_NE(blobIt, nullptr);
    EXPECT_EQ(*blobIt, BLOB);

    auto doubleIt = Traits::get_if<double>(&data["salary"]);
    ASSERT_NE(doubleIt, nullptr);
    EXPECT_EQ(*doubleIt, SALARY);

    auto boolIt = Traits::get_if<bool>(&data["flag"]);
    ASSERT_NE(boolIt, nullptr);
    EXPECT_EQ(*boolIt, FLAG);
}

/**
* @tc.name: RelationalStoreCursorTest_GetByIndex
* @tc.desc: Test Get function with column index of RelationalStoreCursor.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RelationalStoreCursorTest, RelationalStoreCursorTest_GetByIndex, TestSize.Level1)
{
    ASSERT_NE(relationalStoreCursor_, nullptr);
    DistributedData::Value value;
    auto result = relationalStoreCursor_->Get(1, value);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: RelationalStoreCursorTest_GetByName
* @tc.desc: Test Get function with column name of RelationalStoreCursor.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RelationalStoreCursorTest, RelationalStoreCursorTest_GetByName, TestSize.Level1)
{
    ASSERT_NE(relationalStoreCursor_, nullptr);
    DistributedData::Value value;
    auto result = relationalStoreCursor_->Get("age", value);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: RelationalStoreCursorTest_IsEndAndClose
* @tc.desc: Test IsEnd and Close functions of RelationalStoreCursor.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(RelationalStoreCursorTest, RelationalStoreCursorTest_IsEndAndClose, TestSize.Level1)
{
    ASSERT_NE(relationalStoreCursor_, nullptr);
    bool ret = relationalStoreCursor_->IsEnd();
    EXPECT_EQ(ret, true);

    auto result = relationalStoreCursor_->Close();
    EXPECT_EQ(result, GeneralError::E_OK);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test