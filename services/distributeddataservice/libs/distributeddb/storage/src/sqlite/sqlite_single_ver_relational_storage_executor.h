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
#ifndef SQLITE_SINGLE_VER_RELATIONAL_STORAGE_EXECUTOR_H
#define SQLITE_SINGLE_VER_RELATIONAL_STORAGE_EXECUTOR_H
#ifdef RELATIONAL_STORE

#include "macro_utils.h"
#include "db_types.h"
#include "sqlite_utils.h"
#include "sqlite_storage_executor.h"
#include "relational_store_delegate.h"
#include "query_object.h"

namespace DistributedDB {
class SQLiteSingleVerRelationalStorageExecutor : public SQLiteStorageExecutor {
public:
    SQLiteSingleVerRelationalStorageExecutor(sqlite3 *dbHandle, bool writable);
    ~SQLiteSingleVerRelationalStorageExecutor() override = default;

    // Delete the copy and assign constructors
    DISABLE_COPY_ASSIGN_MOVE(SQLiteSingleVerRelationalStorageExecutor);

    int CreateDistributedTable(const std::string &tableName, TableInfo &table);

    int StartTransaction(TransactType type);
    int Commit();
    int Rollback();

    int SetTableInfo(QueryObject query);

    // For Get sync data
    int GetSyncDataByQuery(std::vector<DataItem> &dataItems, size_t appendLength, QueryObject query,
        const DataSizeSpecInfo &dataSizeInfo, const std::pair<TimeStamp, TimeStamp> &timeRange) const;
    int GetDeletedSyncDataByTimestamp(std::vector<DataItem> &dataItems, size_t appendLength,
        TimeStamp begin, TimeStamp end, const DataSizeSpecInfo &dataSizeInfo) const;

    // operation of meta data
    int GetKvData(const Key &key, Value &value) const;
    int PutKvData(const Key &key, const Value &value) const;
    int DeleteMetaData(const std::vector<Key> &keys) const;
    int DeleteMetaDataByPrefixKey(const Key &keyPrefix) const;
    int GetAllMetaKeys(std::vector<Key> &keys) const;

    // For Put sync data
    int SaveSyncItems(const QueryObject &object, std::vector<DataItem> &dataItems,
        const std::string &deviceName, TimeStamp &timeStamp);

    int AnalysisRelationalSchema(const std::string &tableName, TableInfo &tableInfo);

    int CheckDBModeForRelational();

private:
    int PrepareForSyncDataByTime(TimeStamp begin, TimeStamp end,
        sqlite3_stmt *&statement, bool getDeletedData) const;

    int GetSyncDataItems(std::vector<DataItem> &dataItems, sqlite3_stmt *statement,
        size_t appendLength, const DataSizeSpecInfo &dataSizeInfo) const;

    int GetDataItemForSync(sqlite3_stmt *statement, DataItem &dataItem) const;

    int SaveSyncDataItems(const QueryObject &object, std::vector<DataItem> &dataItems,
        const std::string &deviceName, TimeStamp &timeStamp);
    int SaveSyncDataItem(sqlite3_stmt *statement, const DataItem &dataItem);

    int DeleteSyncDataItem(const DataItem &dataItem);

    int SaveSyncLog(sqlite3_stmt *statement, const DataItem &dataItem, TimeStamp &maxTimestamp);
    int PrepareForSavingData(const QueryObject &object, const std::string &deviceName, sqlite3_stmt *&statement) const;
    int PrepareForSavingLog(const QueryObject &object, const std::string &deviceName, sqlite3_stmt *&statement) const;

    TableInfo table_;
};
} // namespace DistributedDB
#endif
#endif // SQLITE_SINGLE_VER_RELATIONAL_STORAGE_EXECUTOR_H