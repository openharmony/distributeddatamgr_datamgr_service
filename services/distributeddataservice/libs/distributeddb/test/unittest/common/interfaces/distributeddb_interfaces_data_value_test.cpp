/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include <gtest/gtest.h>

#include "distributeddb_tools_unit_test.h"
#include "data_value.h"

using namespace testing::ext;
using namespace DistributedDB;

namespace {
const int ONE_HUNDERED = 100;
const char DEFAULT_CHAR = 'D';
const std::string DEFAULT_TEXT = "This is a text";

void DataValueDefaultNullCheck(DataValue &dataValue)
{
    /**
     * @tc.steps: step1. create a dataValue
     * @tc.expected: dataValue type is null.
     */
    dataValue.ResetValue();
    EXPECT_EQ(dataValue.GetType(), StorageType::STORAGE_TYPE_NULL);
}

void DataValueDoubleCheck(DataValue &dataValue)
{
    /**
     * @tc.steps: step1. create a dataValue and set true
     * @tc.expected: dataValue type is double.
     */
    double targetDoubleVal = 1.0;
    dataValue = targetDoubleVal;
    EXPECT_EQ(dataValue.GetType(), StorageType::STORAGE_TYPE_REAL);
    /**
     * @tc.steps: step2. get a double from dataValue
     * @tc.expected: get ok and value is equal to targetDoubleVal.
     */
    double val = 0;
    EXPECT_EQ(dataValue.GetDouble(val), E_OK);
    EXPECT_EQ(val, targetDoubleVal);
}

void DataValueInt64Check(DataValue &dataValue)
{
    /**
     * @tc.steps: step1. create a dataValue and set INTE64_MAX
     * @tc.expected: dataValue type is int64.
     */
    int64_t targetInt64Val = INT64_MAX;
    dataValue = targetInt64Val;
    EXPECT_EQ(dataValue.GetType(), StorageType::STORAGE_TYPE_INTEGER);
    /**
     * @tc.steps: step2. get a int64 from dataValue
     * @tc.expected: get ok and value is equal to INTE64_MAX.
     */
    int64_t val = INT64_MIN;
    EXPECT_EQ(dataValue.GetInt64(val), E_OK);
    EXPECT_EQ(val, targetInt64Val);
}

void DataValueStringCheck(DataValue &dataValue)
{
    /**
     * @tc.steps: step1. set a string
     * @tc.expected: dataValue type is string.
     */
    dataValue.SetText(DEFAULT_TEXT);
    EXPECT_EQ(dataValue.GetType(), StorageType::STORAGE_TYPE_TEXT);
    /**
     * @tc.steps: step2. get a string from dataValue
     * @tc.expected: get ok and string is equal to DEFAULT_TEXT.
     */
    std::string val;
    EXPECT_EQ(dataValue.GetText(val), E_OK);
    EXPECT_EQ(val, DEFAULT_TEXT);
}

void DataValueBlobCheck(DataValue &dataValue)
{
    /**
     * @tc.steps: step1. set a blob
     * @tc.expected: dataValue type is blob.
     */
    Blob targetVal;
    std::vector<char> vector(ONE_HUNDERED, DEFAULT_CHAR);
    targetVal.WriteBlob(reinterpret_cast<const uint8_t *>(vector.data()), vector.size());
    dataValue.SetBlob(targetVal);
    EXPECT_EQ(dataValue.GetType(), StorageType::STORAGE_TYPE_BLOB);
    /**
     * @tc.steps: step2. get a blob from dataValue
     * @tc.expected: get ok and value is equal to DEFAULT_TEXT.
     */
    Blob val;
    EXPECT_EQ(dataValue.GetBlob(val), E_OK);
    for (int i = 0; i < ONE_HUNDERED; i++) {
        EXPECT_EQ(targetVal.GetData()[i], val.GetData()[i]);
    }
}

const std::vector<void (*)(DataValue&)> g_checkFuncList = {
    &DataValueDefaultNullCheck, &DataValueInt64Check,
    &DataValueDoubleCheck, &DataValueStringCheck, &DataValueBlobCheck
};

class DistributedDBInterfacesDataValueTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBInterfacesDataValueTest::SetUpTestCase(void)
{
}

void DistributedDBInterfacesDataValueTest::TearDownTestCase(void)
{
}

void DistributedDBInterfacesDataValueTest::SetUp(void)
{
}

void DistributedDBInterfacesDataValueTest::TearDown(void)
{
}

/**
 * @tc.name: DataValueCheck001
 * @tc.desc: To test dataValue work correctly when data is nullptr, bool, string, double, int64, uint8_t*.
 * @tc.type: Func
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBInterfacesDataValueTest, DataValueCheck001, TestSize.Level1)
{
    for (const auto &func : g_checkFuncList) {
        DataValue dataValue;
        func(dataValue);
    }
}

/**
 * @tc.name: DataValueCheck002
 * @tc.desc: To test dataValue work correctly when different type overwrite into dataValue.
 * @tc.type: Func
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBInterfacesDataValueTest, DataValueCheck002, TestSize.Level1)
{
    for (uint32_t lastWriteIndex = 0; lastWriteIndex < g_checkFuncList.size(); lastWriteIndex++) {
        DataValue dataValue;
        for (uint32_t i = 0; i < g_checkFuncList.size(); i++) {
            uint32_t index = (lastWriteIndex + i + 1) % static_cast<int>(g_checkFuncList.size());
            g_checkFuncList[index](dataValue);
        }
    }
}
}