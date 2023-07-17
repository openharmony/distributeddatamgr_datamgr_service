/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "mock/cursor_mock.h"
#include "rdb_result_set_impl.h"
#include "store/cursor.h"
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;

namespace OHOS::Test {
namespace DistributedRDBTest {
class RdbResultSetImplTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static inline const std::vector<std::pair<std::string, RdbResultSetImpl::ColumnType>> Names = {
        { "1_id", RdbResultSetImpl::ColumnType::TYPE_INTEGER },
        { "2_name", RdbResultSetImpl::ColumnType::TYPE_STRING },
        { "3_price", RdbResultSetImpl::ColumnType::TYPE_FLOAT },
        { "4_bool", RdbResultSetImpl::ColumnType::TYPE_INTEGER },
        { "5_bytes", RdbResultSetImpl::ColumnType::TYPE_BLOB }
    };
    static std::map<std::string, Value> makeEntry(int i);
    static RdbResultSetImpl::ColumnType getColumnType(std::string &name);
    std::shared_ptr<CursorMock::ResultSet> cursor;
    std::shared_ptr<RdbResultSetImpl> resultSet;
    static inline constexpr int32_t SIZE = 50;
};

void RdbResultSetImplTest::SetUpTestCase(void) {}

void RdbResultSetImplTest::TearDownTestCase(void) {}

void RdbResultSetImplTest::SetUp()
{
    cursor = std::make_shared<CursorMock::ResultSet>(SIZE);
    for (int i = 0; i < SIZE; i++) {
        (*cursor)[i] = makeEntry(i);
    }
    auto cursorMock = std::make_shared<CursorMock>(cursor);
    resultSet = std::make_shared<RdbResultSetImpl>(cursorMock);
}

void RdbResultSetImplTest::TearDown() {}

std::map<std::string, Value> RdbResultSetImplTest::makeEntry(int i)
{
    std::string str = "RdbResultSetImplTest";
    Bytes bytes(str.begin(), str.end());
    std::map<std::string, Value> entry = {
        { Names[0].first, i },
        { Names[1].first, "name_" + std::to_string(i) },
        { Names[2].first, 1.65 },
        { Names[3].first, i & 0x1 },
        { Names[4].first, bytes}
    };
    return entry;
}

RdbResultSetImpl::ColumnType RdbResultSetImplTest::getColumnType(string &name)
{
    RdbResultSetImpl::ColumnType columnType = RdbResultSetImpl::ColumnType::TYPE_NULL;
    std::find_if(Names.begin(), Names.end(), [&columnType, &name](auto &pr) {
        if (pr.first == name) {
            columnType = pr.second;
            return true;
        }
        return false;
    });
    return columnType;
}

/**
* @tc.name: RdbResultSetImplGetColumn
* @tc.desc: RdbResultSetImpl get col;
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(RdbResultSetImplTest, RdbResultSetImplGetColumn, TestSize.Level0)
{
    std::vector<std::string> names;
    resultSet->GetAllColumnNames(names);
    ASSERT_EQ(Names.size(), names.size());
    for (int i = 0; i < Names.size(); i++) {
        ASSERT_EQ(Names[i].first, names[i]);
        std::string colName;
        ASSERT_EQ(resultSet->GetColumnName(i, colName), NativeRdb::E_OK);
        ASSERT_EQ(colName, names[i]);
        RdbResultSetImpl::ColumnType columnType;
        ASSERT_EQ(resultSet->GetColumnType(i, columnType), NativeRdb::E_OK);
        ASSERT_EQ(columnType, getColumnType(colName));
    }
}

/**
* @tc.name: RdbResultSetImplGoTO
* @tc.desc: RdbResultSetImpl go to row;
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(RdbResultSetImplTest, RdbResultSetImplGoTO, TestSize.Level0)
{
    int count = 0;
    ASSERT_EQ(resultSet->GetRowCount(count), NativeRdb::E_OK);
    ASSERT_EQ(count, SIZE);

    int position = 0;
    ASSERT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);
    ASSERT_EQ(resultSet->GetRowIndex(position), NativeRdb::E_OK);
    ASSERT_EQ(position, 0);

    bool result;
    ASSERT_EQ(resultSet->IsStarted(result), NativeRdb::E_OK);
    ASSERT_FALSE(result);

    ASSERT_EQ(resultSet->GoToPreviousRow(), NativeRdb::E_ERROR);
    ASSERT_EQ(resultSet->GetRowIndex(position), NativeRdb::E_OK);
    ASSERT_EQ(position, -1);

    ASSERT_EQ(resultSet->IsStarted(result), NativeRdb::E_OK);
    ASSERT_TRUE(result);

    int absPosition = random() % SIZE;
    ASSERT_EQ(resultSet->GoToRow(absPosition), NativeRdb::E_OK);
    ASSERT_EQ(resultSet->GetRowIndex(position), NativeRdb::E_OK);
    ASSERT_EQ(position, absPosition);

    ASSERT_EQ(resultSet->GoToLastRow(), NativeRdb::E_OK);
    ASSERT_EQ(resultSet->GetRowIndex(position), NativeRdb::E_OK);
    ASSERT_EQ(position, SIZE - 1);

    ASSERT_EQ(resultSet->IsEnded(result), NativeRdb::E_OK);
    ASSERT_FALSE(result);

    ASSERT_EQ(resultSet->GoToNextRow(), NativeRdb::E_ERROR);
    ASSERT_EQ(resultSet->GetRowIndex(position), NativeRdb::E_OK);
    ASSERT_EQ(position, SIZE);

    ASSERT_EQ(resultSet->IsEnded(result), NativeRdb::E_OK);
    ASSERT_TRUE(result);

    ASSERT_EQ(resultSet->GoTo(-10), NativeRdb::E_OK);
    ASSERT_EQ(resultSet->GetRowIndex(position), NativeRdb::E_OK);
    ASSERT_EQ(position, SIZE - 10);
}

/**
* @tc.name: RdbResultSetImplGet
* @tc.desc: RdbResultSetImpl get data;
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(RdbResultSetImplTest, RdbResultSetImplGet, TestSize.Level0)
{
    ASSERT_EQ(resultSet->GoToFirstRow(), NativeRdb::E_OK);
    bool result = false;
    int index = 0;
    int64_t tmpInt;
    double tmpFlo;
    std::string tmpStr;
    Bytes tmpBlob;
    while (!result) {
        for (int i = 0; i < Names.size(); i++) {
            auto var = (*cursor)[index][Names[i].first];
            ValueProxy::Value value = ValueProxy::Convert(std::move(var));
            switch (Names[i].second) {
                case RdbResultSetImpl::ColumnType::TYPE_INTEGER:
                    ASSERT_EQ(resultSet->GetLong(i, tmpInt), NativeRdb::E_OK);
                    ASSERT_EQ(tmpInt, int64_t(value));
                    break;
                case RdbResultSetImpl::ColumnType::TYPE_FLOAT:
                    ASSERT_EQ(resultSet->GetDouble(i, tmpFlo), NativeRdb::E_OK);
                    ASSERT_EQ(tmpFlo, double(value));
                    break;
                case RdbResultSetImpl::ColumnType::TYPE_STRING:
                    ASSERT_EQ(resultSet->GetString(i, tmpStr), NativeRdb::E_OK);
                    ASSERT_EQ(tmpStr, std::string(value));
                    break;
                case RdbResultSetImpl::ColumnType::TYPE_BLOB:
                    ASSERT_EQ(resultSet->GetBlob(i, tmpBlob), NativeRdb::E_OK);
                    ASSERT_EQ(tmpBlob.size(), Bytes(value).size());
                    break;
                default:
                    break;
            }
        }
        resultSet->GoToNextRow();
        index++;
        resultSet->IsEnded(result);
    }
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test