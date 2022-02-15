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
#ifndef DATA_TRANSFORMER_H
#define DATA_TRANSFORMER_H
#ifdef RELATIONAL_STORE

#include <optional>
#include <vector>
#include "data_value.h"
#include "db_types.h"
#include "relational_schema_object.h"

namespace DistributedDB {
using RowData = std::vector<DataValue>;
using OptRowData = std::vector<DataValue>;

struct LogInfo {
    int dataKey = -1;
    std::string device;
    std::string originDev;
    TimeStamp timestamp = 0;
    TimeStamp wTimeStamp = 0;
    uint64_t flag = 0;
    Key hashKey; // primary key hash value
};

struct RowDataWithLog {
    LogInfo logInfo;
    RowData rowData;
};

struct OptRowDataWithLog {
    LogInfo logInfo;
    OptRowData optionalData;
};

struct TableDataWithLog {
    std::string tableName;
    std::vector<RowDataWithLog> dataList;
};

struct OptTableDataWithLog {
    std::string tableName;
    std::vector<OptRowDataWithLog> dataList;
};

class DataTransformer {
public:
    static int TransformTableData(const TableDataWithLog &tableDataWithLog,
        const std::vector<FieldInfo> &fieldInfoList, std::vector<DataItem> &dataItems);
    static int TransformDataItem(const std::vector<DataItem> &dataItems, const std::vector<FieldInfo> &remoteFieldInfo,
        const std::vector<FieldInfo> &localFieldInfo, OptTableDataWithLog &tableDataWithLog);

    static int SerializeDataItem(const RowDataWithLog &data, const std::vector<FieldInfo> &fieldInfo,
        DataItem &dataItem);
    static int DeSerializeDataItem(const DataItem &dataItem, OptRowDataWithLog &data,
        const std::vector<FieldInfo> &remoteFieldInfo, std::vector<int> &indexMapping);
    static void ReduceMapping(const std::vector<FieldInfo> &remoteFieldInfo,
        const std::vector<FieldInfo> &localFieldInfo, std::vector<int> &indexMapping);

private:
    static int SerializeValue(Value &value, const RowData &rowData, const std::vector<FieldInfo> &fieldInfoList);
    static int DeSerializeValue(const Value &value, OptRowData &optionalData,
        const std::vector<FieldInfo> &remoteFieldInfo, std::vector<int> &indexMapping);

    static uint32_t CalDataValueLength(const DataValue &dataValue);
};
}

#endif
#endif // DATA_TRANSFORMER_H