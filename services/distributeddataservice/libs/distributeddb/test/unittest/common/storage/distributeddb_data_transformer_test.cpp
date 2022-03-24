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

#include "data_transformer.h"

using namespace testing::ext;
using namespace DistributedDB;
namespace {
const int ONE_HUNDERED = 100;
const char DEFAULT_CHAR = 'D';
const std::string DEFAULT_TEXT = "This is a text";
const std::vector<uint8_t> DEFAULT_BLOB(ONE_HUNDERED, DEFAULT_CHAR);
void SetNull(DataValue &dataValue)
{
    dataValue.ResetValue();
}

void SetInt64(DataValue &dataValue)
{
    dataValue = INT64_MAX;
}

void SetDouble(DataValue &dataValue)
{
    dataValue = 1.0;
}

void SetText(DataValue &dataValue)
{
    dataValue.SetText(DEFAULT_TEXT);
}

void SetBlob(DataValue &dataValue)
{
    Blob blob;
    blob.WriteBlob(DEFAULT_BLOB.data(), DEFAULT_BLOB.size());
    dataValue.SetBlob(blob);
}

std::map<StorageType, void(*)(DataValue&)> g_typeMapFunction = {
    {StorageType::STORAGE_TYPE_NULL,    &SetNull},
    {StorageType::STORAGE_TYPE_INTEGER, &SetInt64},
    {StorageType::STORAGE_TYPE_REAL,    &SetDouble},
    {StorageType::STORAGE_TYPE_TEXT,    &SetText},
    {StorageType::STORAGE_TYPE_BLOB,    &SetBlob}
};

void GenerateRowData(const std::vector<FieldInfo> &fieldInfoList, RowData &rowData)
{
    for (auto &item: fieldInfoList) {
        DataValue dataValue;
        StorageType type = StorageType::STORAGE_TYPE_NULL;
        if (g_typeMapFunction.find(item.GetStorageType()) != g_typeMapFunction.end()) {
            type = item.GetStorageType();
        }
        g_typeMapFunction[type](dataValue);
        rowData.push_back(std::move(dataValue));
    }
}

void GenerateTableDataWithLog(const std::vector<FieldInfo> &fieldInfoList, TableDataWithLog &tableDataWithLog)
{
    tableDataWithLog.tableName = DEFAULT_TEXT;
    for (int i = 0; i < ONE_HUNDERED; i++) {
        RowDataWithLog rowDataWithLog;
        GenerateRowData(fieldInfoList, rowDataWithLog.rowData);
        LogInfo logInfo;
        // choose first element as primary key
        logInfo.dataKey = i;
        tableDataWithLog.dataList.push_back(std::move(rowDataWithLog));
    }
}

bool Equal(const LogInfo &origin, const LogInfo &target)
{
    if (origin.dataKey != target.dataKey) {
        return false;
    }
    if (origin.device != target.device) {
        return false;
    }
    if (origin.flag != target.flag) {
        return false;
    }
    if (origin.hashKey != target.hashKey) {
        return false;
    }
    if (origin.originDev != target.originDev) {
        return false;
    }
    if (origin.timestamp != target.timestamp) {
        return false;
    }
    if (origin.wTimestamp != target.wTimestamp) {
        return false;
    }
    return true;
}

bool Equal(const RowDataWithLog &origin, const OptRowDataWithLog &target)
{
    if (!Equal(origin.logInfo, target.logInfo)) {
        return false;
    }
    if (origin.rowData.size() != target.optionalData.size()) {
        return false;
    }
    for (uint32_t i = 0; i < origin.rowData.size(); i++) {
        const auto &originData = origin.rowData[i];
        const auto &targetData = target.optionalData[i];
        if (originData != targetData) {
            LOGD("VALUE NOT EQUAL!");
            return false;
        }
    }
    return true;
}

bool Equal(TableDataWithLog origin, OptTableDataWithLog target)
{
    if (origin.dataList.size() != target.dataList.size()) {
        return false;
    }
    for (uint32_t i = 0; i < origin.dataList.size(); i++) {
        RowDataWithLog originData = origin.dataList[i];
        OptRowDataWithLog targetData = target.dataList[i];
        if (!Equal(originData, targetData)) {
            return false;
        }
    }
    return true;
}
}

class DistributedDBDataTransformerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DistributedDBDataTransformerTest::SetUpTestCase(void)
{
}

void DistributedDBDataTransformerTest::TearDownTestCase(void)
{
}

void DistributedDBDataTransformerTest::SetUp(void)
{
}

void DistributedDBDataTransformerTest::TearDown(void)
{
}

/**
 * @tc.name: DataTransformerCheck001
 * @tc.desc: To test transformer work correctly when data contains nullptr, bool, string, double, int64, uint8_t*.
 * @tc.type: Func
 * @tc.require:
 * @tc.author: zhangqiquan
 */
HWTEST_F(DistributedDBDataTransformerTest, DataTransformerCheck001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. generate the fieldInfoList which contains nullptr, bool, string, double, int64, uint8_t*.
     */
    std::vector<FieldInfo> fieldInfoList;
    int count = 0;
    for (const auto &item : g_typeMapFunction) {
        FieldInfo fieldInfo;
        fieldInfo.SetStorageType(item.first);
        fieldInfo.SetFieldName(std::to_string(count++));
        fieldInfoList.push_back(fieldInfo);
    }

    /**
     * @tc.steps: step2. generate an originData by fieldInfoLiist.
     */
    TableDataWithLog originData;
    GenerateTableDataWithLog(fieldInfoList, originData);

    /**
     * @tc.steps: step3. transform originData to KV data and transform back to relationData.
     * @tc.expected: get ok and value has no change
     */
    std::vector<DataItem> dataItemList;
    EXPECT_EQ(DataTransformer::TransformTableData(originData, fieldInfoList, dataItemList), E_OK);

    OptTableDataWithLog targetData;
    EXPECT_EQ(DataTransformer::TransformDataItem(dataItemList, fieldInfoList, fieldInfoList, targetData), E_OK);

    EXPECT_TRUE(Equal(originData, targetData));
}