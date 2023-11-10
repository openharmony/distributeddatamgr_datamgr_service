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
#define LOG_TAG "CacheCursorTest"
#include <gtest/gtest.h>
#include "log_print.h"
#include "cache_cursor.h"
#include "store/general_value.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedRdb;

namespace OHOS::Test {
namespace DistributedDataTest {
static constexpr int MAX_DATA_NUM = 100;
static constexpr int AGE = 25;
static constexpr const char *NAME = "tony";
static constexpr const char *PHONENUMBER = "10086";
class CacheCursorTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    static std::shared_ptr<CacheCursor> GetCursor();
protected:
    CacheCursorTest();
    void InitData();
    static std::shared_ptr<CacheCursor> cursor_;
};

std::shared_ptr<CacheCursor> CacheCursorTest::cursor_ = nullptr;

CacheCursorTest::CacheCursorTest() { }

std::shared_ptr<CacheCursor> CacheCursorTest::GetCursor()
{
    return cursor_;
}

void CacheCursorTest::SetUpTestCase(void)
{
    std::vector<VBucket> records;
    for (int i = 0; i < MAX_DATA_NUM; i++) {
        VBucket record;
        record["identifier"] = i;
        record["name"] = NAME;
        record["age"] = AGE;
        record["phoneNumber"] = PHONENUMBER;
        records.push_back(record);
    }
    cursor_ = std::make_shared<CacheCursor>(std::move(records));
}

void CacheCursorTest::TearDownTestCase() {}

void CacheCursorTest::SetUp() {}

void CacheCursorTest::TearDown() {}

HWTEST_F(CacheCursorTest, CacheCursorTest_001, TestSize.Level0)
{
    auto cursor = CacheCursorTest::GetCursor();
    ASSERT_NE(cursor, nullptr);
    std::vector<std::string> expectedNames {"age", "identifier", "name", "phoneNumber"};
    std::vector<std::string> names;
    auto err = cursor->GetColumnNames(names);
    EXPECT_EQ(names, expectedNames);
    std::string colName;
    err = cursor->GetColumnName(4, colName);
    EXPECT_EQ(err, GeneralError::E_INVALID_ARGS);

    int type = cursor->GetColumnType(0);
    EXPECT_EQ(type, 1);
    type = cursor->GetColumnType(4);
    EXPECT_EQ(type, -1);

    int count = cursor->GetCount();
    EXPECT_EQ(count, 100);

    err = cursor->MoveToFirst();
    EXPECT_EQ(err, GeneralError::E_OK);

    err = cursor->MoveToNext();
    EXPECT_EQ(err, GeneralError::E_OK);

    for (int i = 2; i < count; i++) {
        err = cursor->MoveToNext();
    }
    EXPECT_EQ(err, GeneralError::E_OK);
    err = cursor->MoveToNext();
    EXPECT_EQ(err, GeneralError::E_ERROR);

    err = cursor->MoveToPrev();
    EXPECT_EQ(err, GeneralError::E_NOT_SUPPORT);
}

HWTEST_F(CacheCursorTest, CacheCursorTest_002, TestSize.Level0)
{
    auto cursor = CacheCursorTest::GetCursor();
    ASSERT_NE(cursor, nullptr);
    auto err = cursor->MoveToFirst();

    DistributedData::VBucket data;
    err = cursor->GetEntry(data);
    EXPECT_EQ(err, GeneralError::E_OK);

    int64_t identifier = *std::get_if<int64_t>(&data["identifier"]);
    EXPECT_EQ(identifier, 0);
    std::string name = *std::get_if<std::string>(&data["name"]);
    EXPECT_EQ(name, "Tony");
    int64_t age = *std::get_if<int64_t>(&data["age"]);
    EXPECT_EQ(age, 25);
    std::string phoneNumber = *std::get_if<std::string>(&data["phoneNumber"]);
    EXPECT_EQ(phoneNumber, "10086");
    
    while (err == GeneralError::E_OK) {
        cursor->GetRow(data);
        err = cursor->MoveToNext();
    }

    identifier = *std::get_if<int64_t>(&data["identifier"]);
    EXPECT_EQ(identifier, 99);

    DistributedData::Value value;
    err = cursor->Get(0, value);
    age = *std::get_if<int64_t>(&value);
    EXPECT_EQ(age, 25);

    err = cursor->Get("name", value);
    name = *std::get_if<std::string>(&value);
    EXPECT_EQ(name, "Tony");

    bool ret = cursor->IsEnd();
    EXPECT_EQ(ret, false);

    err = cursor->Close();
    EXPECT_EQ(err, GeneralError::E_OK);
}
}
}