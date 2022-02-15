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
#include "sqlite_single_ver_relational_storage_executor.h"
#include "data_transformer.h"
#include "db_common.h"

namespace DistributedDB {
SQLiteSingleVerRelationalStorageExecutor::SQLiteSingleVerRelationalStorageExecutor(sqlite3 *dbHandle, bool writable)
    : SQLiteStorageExecutor(dbHandle, writable, false)
{}

int SQLiteSingleVerRelationalStorageExecutor::CreateDistributedTable(const std::string &tableName, TableInfo &table,
    bool isUpgrade)
{
    if (dbHandle_ == nullptr) {
        return -E_INVALID_DB;
    }

    int errCode = SQLiteUtils::AnalysisSchema(dbHandle_, tableName, table);
    if (errCode != E_OK) {
        LOGE("[CreateDistributedTable] analysis table schema failed. %d", errCode);
        return errCode;
    }

    if (table.GetCreateTableSql().find("WITHOUT ROWID") != std::string::npos) {
        LOGE("[CreateDistributedTable] Not support create distributed table without rowid.");
        return -E_NOT_SUPPORT;
    }

    bool isTableEmpty = false;
    errCode = SQLiteUtils::CheckTableEmpty(dbHandle_, tableName, isTableEmpty);
    if (errCode != E_OK) {
        LOGE("[CreateDistributedTable] Check table [%s] is empty failed. %d", tableName.c_str(), errCode);
        return errCode;
    }

    if (!isUpgrade && !isTableEmpty) { // create distributed table should on an empty table
        LOGE("[CreateDistributedTable] Create distributed table should on an empty table when first create.");
        return -E_NOT_SUPPORT;
    }

    // create log table
    errCode = SQLiteUtils::CreateRelationalLogTable(dbHandle_, tableName);
    if (errCode != E_OK) {
        LOGE("[CreateDistributedTable] create log table failed");
        return errCode;
    }

    // add trigger
    errCode = SQLiteUtils::AddRelationalLogTableTrigger(dbHandle_, table);
    if (errCode != E_OK) {
        LOGE("[CreateDistributedTable] Add relational log table trigger failed.");
        return errCode;
    }
    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::UpgradeDistributedTable(const TableInfo &tableInfo,
    TableInfo &newTableInfo)
{
    if (dbHandle_ == nullptr) {
        return -E_INVALID_DB;
    }

    int errCode = SQLiteUtils::AnalysisSchema(dbHandle_, tableInfo.GetTableName(), newTableInfo);
    if (errCode != E_OK) {
        LOGE("[UpgradeDistributedTable] analysis table schema failed. %d", errCode);
        return errCode;
    }

    if (newTableInfo.GetCreateTableSql().find("WITHOUT ROWID") != std::string::npos) {
        LOGE("[UpgradeDistributedTable] Not support create distributed table without rowid.");
        return -E_NOT_SUPPORT;
    }

    // new table should has same or compatible upgrade
    errCode = tableInfo.CompareWithTable(newTableInfo);
    if (errCode == -E_RELATIONAL_TABLE_INCOMPATIBLE) {
        LOGE("[UpgradeDistributedTable] Not support with incompatible upgrade.");
        return -E_SCHEMA_MISMATCH;
    }

    errCode = AlterAuxTableForUpgrade(tableInfo, newTableInfo);
    if (errCode != E_OK) {
        LOGE("[UpgradeDistributedTable] Alter aux table for upgrade failed. %d", errCode);
    }

    return errCode;
}

namespace {
int GetDeviceTableName(sqlite3 *handle, const std::string &tableName, const std::string &device,
    std::vector<std::string> &deviceTables)
{
    if (device.empty() && tableName.empty()) { // device and table name should not both be empty
        return -E_INVALID_ARGS;
    }
    std::string deviceHash = DBCommon::TransferStringToHex(DBCommon::TransferHashString(device));
    std::string devicePattern = device.empty() ? "%" : deviceHash;
    std::string tablePattern = tableName.empty() ? "%" : tableName;
    std::string deviceTableName = DBConstant::RELATIONAL_PREFIX + tablePattern + "_" + devicePattern;

    const std::string checkSql = "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE '" +
        deviceTableName + "';";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(handle, checkSql, stmt);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        return errCode;
    }

    do {
        errCode = SQLiteUtils::StepWithRetry(stmt, false);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            break;
        } else if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            LOGE("Get table name failed. %d", errCode);
            break;
        }
        std::string realTableName;
        errCode = SQLiteUtils::GetColumnTextValue(stmt, 0, realTableName); // 0: table name result column index
        if (errCode != E_OK || realTableName.empty()) { // sqlite might return a row with NULL
            continue;
        }
        if (realTableName.rfind("_log") == (realTableName.length() - 4)) { // 4:suffix length of "_log"
            continue;
        }
        deviceTables.emplace_back(realTableName);
    } while (true);

    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}

std::vector<FieldInfo> GetUpgradeFields(const TableInfo &oldTableInfo, const TableInfo &newTableInfo)
{
    std::vector<FieldInfo> fields;
    auto itOld = oldTableInfo.GetFields().begin();
    auto itNew = newTableInfo.GetFields().begin();
    for (; itNew != newTableInfo.GetFields().end(); itNew++) {
        if (itOld == oldTableInfo.GetFields().end() || itOld->first != itNew->first) {
            fields.emplace_back(itNew->second);
            continue;
        }
        itOld++;
    }
    return fields;
}

int UpgradeFields(sqlite3 *db, const std::vector<std::string> &tables, const std::vector<FieldInfo> &fields)
{
    if (db == nullptr) {
        return -E_INVALID_ARGS;
    }

    int errCode = E_OK;
    for (auto table : tables) {
        for (auto field : fields) {
            std::string alterSql = "ALTER TABLE " + table + " ADD " + field.GetFieldName() + " " +
                field.GetDataType() + ";";
            errCode = SQLiteUtils::ExecuteRawSQL(db, alterSql);
            if (errCode != E_OK) {
                LOGE("Alter table failed. %d", errCode);
                break;
            }
        }
    }
    return errCode;
}

std::map<std::string, CompositeFields> GetChangedIndexes(const TableInfo &oldTableInfo, const TableInfo &newTableInfo)
{
    std::map<std::string, CompositeFields> indexes;
    auto itOld = oldTableInfo.GetIndexDefine().begin();
    auto itNew = newTableInfo.GetIndexDefine().begin();
    auto itOldEnd = oldTableInfo.GetIndexDefine().end();
    auto itNewEnd = newTableInfo.GetIndexDefine().end();

    while (itOld != itOldEnd && itNew != itNewEnd) {
        if (itOld->first == itNew->first) {
            if (itOld->second != itNew->second) {
                indexes.insert({itNew->first, itNew->second});
            }
            itOld++;
            itNew++;
        } else if (itOld->first < itNew->first) {
            indexes.insert({itOld->first,{}});
            itOld++;
        } else if (itOld->first > itNew->first) {
            indexes.insert({itNew->first, itNew->second});
            itNew++;
        }
    }

    while (itOld != itOldEnd) {
        indexes.insert({itOld->first,{}});
        itOld++;
    }

    while (itNew != itNewEnd) {
        indexes.insert({itNew->first, itNew->second});
        itNew++;
    }

    return indexes;
}

int Upgradeindexes(sqlite3 *db, const std::vector<std::string> &tables,
    const std::map<std::string, CompositeFields> &indexes)
{
    if (db == nullptr) {
        return -E_INVALID_ARGS;
    }

    int errCode = E_OK;
    for (const auto &table : tables) {
        for (const auto &index : indexes) {
            if (index.first.empty()) {
                continue;
            }
            std::string realIndexName = table + "_" + index.first;
            std::string deleteIndexSql = "DROP INDEX IF EXISTS " + realIndexName;
            errCode = SQLiteUtils::ExecuteRawSQL(db, deleteIndexSql);
            if (errCode != E_OK) {
                LOGE("Drop index failed. %d", errCode);
                return errCode;
            }

            if (index.second.empty()) { // empty means drop index only
                continue;
            }

            auto it = index.second.begin();
            std::string indexDefine = *it++;
            while (it != index.second.end()) {
                indexDefine += ", " + *it++;
            }
            std::string createIndexSql = "CREATE INDEX IF NOT EXISTS " + realIndexName + " ON " + table +
                "(" + indexDefine + ");";
            errCode = SQLiteUtils::ExecuteRawSQL(db, createIndexSql);
            if (errCode != E_OK) {
                LOGE("Create index failed. %d", errCode);
                break;
            }
        }
    }
    return errCode;
}
}

int SQLiteSingleVerRelationalStorageExecutor::AlterAuxTableForUpgrade(const TableInfo &oldTableInfo,
    const TableInfo &newTableInfo)
{
    std::vector<FieldInfo> upgradeFields = GetUpgradeFields(oldTableInfo, newTableInfo);
    std::map<std::string, CompositeFields> upgradeIndexces = GetChangedIndexes(oldTableInfo, newTableInfo);
    std::vector<std::string> deviceTables;
    int errCode = GetDeviceTableName(dbHandle_, oldTableInfo.GetTableName(), {}, deviceTables);
    if (errCode != E_OK) {
        LOGE("Get device table name for alter table failed. %d", errCode);
        return errCode;
    }

    LOGD("Begin to alter table: upgrade fields[%d], indexces[%d], deviceTable[%d]", upgradeFields.size(),
        upgradeIndexces.size(), deviceTables.size());
    errCode = UpgradeFields(dbHandle_, deviceTables, upgradeFields);
    if (errCode != E_OK) {
        LOGE("upgrade fields failed. %d", errCode);
        return errCode;
    }

    errCode = Upgradeindexes(dbHandle_, deviceTables, upgradeIndexces);
    if (errCode != E_OK) {
        LOGE("upgrade indexes failed. %d", errCode);
    }

    return E_OK;
}

int SQLiteSingleVerRelationalStorageExecutor::StartTransaction(TransactType type)
{
    if (dbHandle_ == nullptr) {
        LOGE("Begin transaction failed, dbHandle is null.");
        return -E_INVALID_DB;
    }
    int errCode = SQLiteUtils::BeginTransaction(dbHandle_, type);
    if (errCode != E_OK) {
        LOGE("Begin transaction failed, errCode = %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::Commit()
{
    if (dbHandle_ == nullptr) {
        return -E_INVALID_DB;
    }

    return SQLiteUtils::CommitTransaction(dbHandle_);
}

int SQLiteSingleVerRelationalStorageExecutor::Rollback()
{
    if (dbHandle_ == nullptr) {
        return -E_INVALID_DB;
    }
    int errCode = SQLiteUtils::RollbackTransaction(dbHandle_);
    if (errCode != E_OK) {
        LOGE("sqlite single ver storage executor rollback fail! errCode = [%d]", errCode);
    }
    return errCode;
}

void SQLiteSingleVerRelationalStorageExecutor::SetTableInfo(const TableInfo &tableInfo)
{
    table_ = tableInfo;
}

static int GetDataValueByType(sqlite3_stmt *statement, DataValue &value, int cid)
{
    int errCode = E_OK;
    int storageType = sqlite3_column_type(statement, cid);
    switch (storageType) {
        case SQLITE_INTEGER: {
            value = static_cast<int64_t>(sqlite3_column_int64(statement, cid));
            break;
        }
        case SQLITE_FLOAT: {
            value = sqlite3_column_double(statement, cid);
            break;
        }
        case SQLITE_BLOB: {
            std::vector<uint8_t> blobValue;
            errCode = SQLiteUtils::GetColumnBlobValue(statement, cid, blobValue);
            if (errCode != E_OK) {
                return errCode;
            }
            auto blob = new (std::nothrow) Blob;
            if (blob == nullptr) {
                return -E_OUT_OF_MEMORY;
            }
            blob->WriteBlob(blobValue.data(), static_cast<uint32_t>(blobValue.size()));
            errCode = value.Set(blob);
            break;
        }
        case SQLITE_NULL: {
            break;
        }
        case SQLITE3_TEXT: {
            const char *colValue = reinterpret_cast<const char *>(sqlite3_column_text(statement, cid));
            if (colValue == nullptr) {
                value.ResetValue();
            } else {
                value = std::string(colValue);
                if (value.GetType() == StorageType::STORAGE_TYPE_NULL) {
                    errCode = -E_OUT_OF_MEMORY;
                }
            }
            break;
        }
        default: {
            break;
        }
    }
    return errCode;
}

static int BindDataValueByType(sqlite3_stmt *statement, const std::optional<DataValue> &data, int cid)
{
    int errCode = E_OK;
    StorageType type = data.value().GetType();
    switch (type) {
        case StorageType::STORAGE_TYPE_BOOL: {
            bool boolData = false;
            (void)data.value().GetBool(boolData);
            errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_int(statement, cid, boolData));
            break;
        }

        case StorageType::STORAGE_TYPE_INTEGER: {
            int64_t intData = 0;
            (void)data.value().GetInt64(intData);
            errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_int64(statement, cid, intData));
            break;
        }

        case StorageType::STORAGE_TYPE_REAL: {
            double doubleData = 0;
            (void)data.value().GetDouble(doubleData);
            errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_double(statement, cid, doubleData));
            break;
        }

        case StorageType::STORAGE_TYPE_TEXT: {
            std::string strData;
            (void)data.value().GetText(strData);
            errCode = SQLiteUtils::BindTextToStatement(statement, cid, strData);
            break;
        }

        case StorageType::STORAGE_TYPE_BLOB: {
            Blob blob;
            (void)data.value().GetBlob(blob);
            std::vector<uint8_t> blobData(blob.GetData(), blob.GetData() + blob.GetSize());
            errCode = SQLiteUtils::BindBlobToStatement(statement, cid, blobData, true);
            break;
        }

        case StorageType::STORAGE_TYPE_NULL: {
            errCode = SQLiteUtils::MapSQLiteErrno(sqlite3_bind_null(statement, cid));
            break;
        }

        default:
            break;
    }
    return errCode;
}

static int GetLogData(sqlite3_stmt *logStatement, LogInfo &logInfo)
{
    logInfo.dataKey = sqlite3_column_int(logStatement, 0);  // 0 means dataKey index

    std::vector<uint8_t> dev;
    int errCode = SQLiteUtils::GetColumnBlobValue(logStatement, 1, dev);  // 1 means dev index
    if (errCode != E_OK) {
        return errCode;
    }
    logInfo.device = std::string(dev.begin(), dev.end());

    std::vector<uint8_t> oriDev;
    errCode = SQLiteUtils::GetColumnBlobValue(logStatement, 2, oriDev);  // 2 means ori_dev index
    if (errCode != E_OK) {
        return errCode;
    }
    logInfo.originDev = std::string(oriDev.begin(), oriDev.end());
    logInfo.timestamp = static_cast<uint64_t>(sqlite3_column_int64(logStatement, 3));  // 3 means timestamp index
    logInfo.wTimeStamp = static_cast<uint64_t>(sqlite3_column_int64(logStatement, 4));  // 4 means w_timestamp index
    logInfo.flag = static_cast<uint64_t>(sqlite3_column_int64(logStatement, 5));  // 5 means flag index
    logInfo.flag &= (~DataItem::LOCAL_FLAG);
    return SQLiteUtils::GetColumnBlobValue(logStatement, 6, logInfo.hashKey);  // 6 means hashKey index
}

static size_t GetDataItemSerialSize(DataItem &item, size_t appendLen)
{
    // timestamp and local flag: 3 * uint64_t, version(uint32_t), key, value, origin dev and the padding size.
    // the size would not be very large.
    static const size_t maxOrigDevLength = 40;
    size_t devLength = std::max(maxOrigDevLength, item.origDev.size());
    size_t dataSize = (Parcel::GetUInt64Len() * 3 + Parcel::GetUInt32Len() + Parcel::GetVectorCharLen(item.key) +
                        Parcel::GetVectorCharLen(item.value) + devLength + appendLen);
    return dataSize;
}

int SQLiteSingleVerRelationalStorageExecutor::GetKvData(const Key &key, Value &value) const
{
    static const std::string SELECT_META_VALUE_SQL = "SELECT value FROM " + DBConstant::RELATIONAL_PREFIX +
        "metadata WHERE key=?;";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, SELECT_META_VALUE_SQL, statement);
    if (errCode != E_OK) {
        goto END;
    }

    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, key, false); // first arg.
    if (errCode != E_OK) {
        goto END;
    }

    errCode = SQLiteUtils::StepWithRetry(statement, isMemDb_);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = -E_NOT_FOUND;
        goto END;
    } else if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        goto END;
    }

    errCode = SQLiteUtils::GetColumnBlobValue(statement, 0, value); // only one result.
    END:
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::PutKvData(const Key &key, const Value &value) const
{
    static const std::string INSERT_META_SQL = "INSERT OR REPLACE INTO " + DBConstant::RELATIONAL_PREFIX +
        "metadata VALUES(?,?);";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, INSERT_META_SQL, statement);
    if (errCode != E_OK) {
        goto ERROR;
    }

    errCode = SQLiteUtils::BindBlobToStatement(statement, 1, key, false);  // 1 means key index
    if (errCode != E_OK) {
        LOGE("[SingleVerExe][BindPutKv]Bind key error:%d", errCode);
        goto ERROR;
    }

    errCode = SQLiteUtils::BindBlobToStatement(statement, 2, value, true);  // 2 means value index
    if (errCode != E_OK) {
        LOGE("[SingleVerExe][BindPutKv]Bind value error:%d", errCode);
        goto ERROR;
    }
    errCode = SQLiteUtils::StepWithRetry(statement, isMemDb_);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    }
ERROR:
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::DeleteMetaData(const std::vector<Key> &keys) const
{
    static const std::string REMOVE_META_VALUE_SQL = "DELETE FROM " + DBConstant::RELATIONAL_PREFIX +
        "metadata WHERE key=?;";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, REMOVE_META_VALUE_SQL, statement);
    if (errCode != E_OK) {
        return errCode;
    }

    for (const auto &key : keys) {
        errCode = SQLiteUtils::BindBlobToStatement(statement, 1, key, false); // first arg.
        if (errCode != E_OK) {
            break;
        }

        errCode = SQLiteUtils::StepWithRetry(statement, isMemDb_);
        if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            break;
        }
        errCode = E_OK;
        SQLiteUtils::ResetStatement(statement, false, errCode);
    }
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return CheckCorruptedStatus(errCode);
}

int SQLiteSingleVerRelationalStorageExecutor::DeleteMetaDataByPrefixKey(const Key &keyPrefix) const
{
    static const std::string REMOVE_META_VALUE_BY_KEY_PREFIX_SQL = "DELETE FROM " + DBConstant::RELATIONAL_PREFIX +
        "metadata WHERE key>=? AND key<=?;";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, REMOVE_META_VALUE_BY_KEY_PREFIX_SQL, statement);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = SQLiteUtils::BindPrefixKey(statement, 1, keyPrefix); // 1 is first arg.
    if (errCode == E_OK) {
        errCode = SQLiteUtils::StepWithRetry(statement, isMemDb_);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
        }
    }
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return CheckCorruptedStatus(errCode);
}

static int GetAllKeys(sqlite3_stmt *statement, std::vector<Key> &keys)
{
    if (statement == nullptr) {
        return -E_INVALID_DB;
    }
    int errCode;
    do {
        errCode = SQLiteUtils::StepWithRetry(statement, false);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            Key key;
            errCode = SQLiteUtils::GetColumnBlobValue(statement, 0, key);
            if (errCode != E_OK) {
                break;
            }

            keys.push_back(std::move(key));
        } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            break;
        } else {
            LOGE("SQLite step for getting all keys failed:%d", errCode);
            break;
        }
    } while (true);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetAllMetaKeys(std::vector<Key> &keys) const
{
    static const std::string SELECT_ALL_META_KEYS = "SELECT key FROM " + DBConstant::RELATIONAL_PREFIX + "metadata;";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, SELECT_ALL_META_KEYS, statement);
    if (errCode != E_OK) {
        LOGE("[Relational][GetAllKey] Get statement failed:%d", errCode);
        return errCode;
    }
    errCode = GetAllKeys(statement, keys);
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::PrepareForSavingLog(const QueryObject &object,
    const std::string &deviceName, sqlite3_stmt *&logStmt) const
{
    std::string devName = DBCommon::TransferHashString(deviceName);
    const std::string tableName = DBConstant::RELATIONAL_PREFIX + object.GetTableName() + "_log";
    std::string dataFormat = "?, '" + deviceName + "', ?, ?, ?, ?, ?";

    std::string sql = "INSERT OR REPLACE INTO " + tableName +
        " (data_key, device, ori_device, timestamp, wtimestamp, flag, hash_key) VALUES (" + dataFormat + ");";
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, logStmt);
    if (errCode != E_OK) {
        LOGE("[info statement] Get statement fail!");
        return -E_INVALID_QUERY_FORMAT;
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::PrepareForSavingData(const QueryObject &object,
    sqlite3_stmt *&statement) const
{
    std::string colName;
    std::string dataFormat;
    for (size_t colId = 0; colId < table_.GetFields().size(); ++colId) {
        colName += table_.GetFieldName(colId) + ",";
        dataFormat += "?,";
    }
    colName.pop_back();
    dataFormat.pop_back();

    const std::string sql = "INSERT OR REPLACE INTO " + table_.GetTableName() +
        " (" + colName + ") VALUES (" + dataFormat + ");";
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, statement);
    if (errCode != E_OK) {
        LOGE("[info statement] Get statement fail!");
        errCode = -E_INVALID_QUERY_FORMAT;
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::SaveSyncLog(sqlite3_stmt *statement, const DataItem &dataItem,
    TimeStamp &maxTimestamp, int64_t rowid)
{
    std::string sql = "select * from " + DBConstant::RELATIONAL_PREFIX + baseTblName_ + "_log where hash_key = ?;";
    sqlite3_stmt *queryStmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, queryStmt);
    if (errCode != E_OK) {
        LOGE("[info statement] Get statement fail!");
        return -E_INVALID_QUERY_FORMAT;
    }
    errCode = SQLiteUtils::BindBlobToStatement(queryStmt, 1, dataItem.hashKey);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(queryStmt, true, errCode);
        return errCode;
    }

    LogInfo logInfoGet;
    errCode = SQLiteUtils::StepWithRetry(queryStmt, isMemDb_);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        errCode = -E_NOT_FOUND;
    } else {
        errCode = GetLogData(queryStmt, logInfoGet);
    }
    SQLiteUtils::ResetStatement(queryStmt, true, errCode);

    LogInfo logInfoBind;
    logInfoBind.hashKey = dataItem.hashKey;
    logInfoBind.device = dataItem.dev;
    logInfoBind.timestamp = dataItem.timeStamp;
    logInfoBind.flag = dataItem.flag;
    logInfoBind.wTimeStamp = maxTimestamp;

    if (errCode == -E_NOT_FOUND) { // insert
        logInfoBind.originDev = dataItem.dev;
    } else if (errCode == E_OK) { // update
        logInfoBind.wTimeStamp = logInfoGet.wTimeStamp;
        logInfoBind.originDev = logInfoGet.originDev;
    } else {
        return errCode;
    }

    // bind
    SQLiteUtils::BindInt64ToStatement(statement, 1, rowid);  // 1 means dataKey index
    std::vector<uint8_t> originDev(logInfoBind.originDev.begin(), logInfoBind.originDev.end());
    SQLiteUtils::BindBlobToStatement(statement, 2, originDev);  // 2 means ori_dev index
    SQLiteUtils::BindInt64ToStatement(statement, 3, logInfoBind.timestamp);  // 3 means timestamp index
    SQLiteUtils::BindInt64ToStatement(statement, 4, logInfoBind.wTimeStamp);  // 4 means w_timestamp index
    SQLiteUtils::BindInt64ToStatement(statement, 5, logInfoBind.flag);  // 5 means flag index
    SQLiteUtils::BindBlobToStatement(statement, 6, logInfoBind.hashKey);  // 6 means hashKey index
    errCode = SQLiteUtils::StepWithRetry(statement, isMemDb_);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        return E_OK;
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::DeleteSyncDataItem(const DataItem &dataItem, sqlite3_stmt *&stmt)
{
    if (stmt == nullptr) {
        const std::string sql = "DELETE FROM " + table_.GetTableName() + " WHERE rowid IN ("
            "SELECT data_key FROM " + DBConstant::RELATIONAL_PREFIX + baseTblName_ + "_log "
            "WHERE hash_key=? AND device=? AND flag&0x01=0);";
        int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
        if (errCode != E_OK) {
            LOGE("[DeleteSyncDataItem] Get statement fail!");
            return -E_INVALID_QUERY_FORMAT;
        }
    }

    int errCode = SQLiteUtils::BindBlobToStatement(stmt, 1, dataItem.hashKey); // 1 means hash_key index
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindTextToStatement(stmt, 2, dataItem.dev); // 2 means device index
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, isMemDb_);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    }
    SQLiteUtils::ResetStatement(stmt, false, errCode);  // Finalize outside.
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::SaveSyncDataItem(const DataItem &dataItem, sqlite3_stmt *&saveDataStmt,
    sqlite3_stmt *&rmDataStmt, int64_t &rowid)
{
    if ((dataItem.flag & DataItem::DELETE_FLAG) != 0) {
        return DeleteSyncDataItem(dataItem, rmDataStmt);
    }

    std::map<std::string, FieldInfo> colInfos = table_.GetFields();
    std::vector<FieldInfo> fieldInfos;
    for (const auto &col: colInfos) {
        fieldInfos.push_back(col.second);
    }

    std::vector<int> indexMapping;

    OptRowDataWithLog data;
    int errCode = DataTransformer::DeSerializeDataItem(dataItem, data, fieldInfos, indexMapping);
    if (errCode != E_OK) {
        LOGE("[RelationalStorageExecutor] DeSerialize dataItem failed! errCode = [%d]", errCode);
        return errCode;
    }

    if (data.optionalData.size() != table_.GetFields().size()) {
        LOGW("Remote data has different fields with local data. Remote size:%zu, local size:%zu",
            data.optionalData.size(), table_.GetFields().size());
    }

    auto putSize = std::min(data.optionalData.size(), table_.GetFields().size());
    for (size_t cid = 0; cid < putSize; ++cid) {
        const auto &fieldData = data.optionalData[cid];
        errCode = BindDataValueByType(saveDataStmt, fieldData, cid + 1);
        if (errCode != E_OK) {
            LOGE("Bind data failed, errCode:%d, cid:%d.", errCode, cid + 1);
            return errCode;
        }
    }

    errCode = SQLiteUtils::StepWithRetry(saveDataStmt, isMemDb_);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        rowid = SQLiteUtils::GetLastRowId(dbHandle_);
        errCode = E_OK;
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::DeleteSyncLog(const DataItem &dataItem, sqlite3_stmt *&stmt)
{
    if (stmt == nullptr) {
        const std::string sql = "DELETE FROM " + DBConstant::RELATIONAL_PREFIX + baseTblName_ + "_log "
                                "WHERE hash_key=? AND device=?";
        int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
        if (errCode != E_OK) {
            LOGE("[DeleteSyncLog] Get statement fail!");
            return errCode;
        }
    }

    int errCode = SQLiteUtils::BindBlobToStatement(stmt, 1, dataItem.hashKey); // 1 means hashkey index
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindTextToStatement(stmt, 2, dataItem.dev); // 2 means device index
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        return errCode;
    }
    errCode = SQLiteUtils::StepWithRetry(stmt, isMemDb_);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        errCode = E_OK;
    }
    SQLiteUtils::ResetStatement(stmt, false, errCode);  // Finalize outside.
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::ProcessMissQueryData(const DataItem &item, sqlite3_stmt *&rmDataStmt,
    sqlite3_stmt *&rmLogStmt)
{
    int errCode = DeleteSyncDataItem(item, rmDataStmt);
    if (errCode != E_OK) {
        return errCode;
    }
    return DeleteSyncLog(item, rmLogStmt);
}

int SQLiteSingleVerRelationalStorageExecutor::SaveSyncDataItems(const QueryObject &object,
    std::vector<DataItem> &dataItems, const std::string &deviceName, TimeStamp &maxTimestamp)
{
    sqlite3_stmt *saveDataStmt = nullptr;
    int errCode = PrepareForSavingData(object, saveDataStmt);
    if (errCode != E_OK) {
        LOGE("[RelationalStorage] Get saveDataStmt fail!");
        return errCode;
    }

    sqlite3_stmt *saveLogStmt = nullptr;
    errCode = PrepareForSavingLog(object, deviceName, saveLogStmt);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(saveDataStmt, true, errCode);
        LOGE("[RelationalStorage] Get saveLogStmt fail!");
        return errCode;
    }

    sqlite3_stmt *rmDataStmt = nullptr;
    sqlite3_stmt *rmLogStmt = nullptr;
    for (auto &item : dataItems) {
        if (item.neglect) { // Do not save this record if it is neglected
            continue;
        }
        item.dev = deviceName;

        if ((item.flag & DataItem::REMOTE_DEVICE_DATA_MISS_QUERY) != 0) {
            errCode = ProcessMissQueryData(item, rmDataStmt, rmLogStmt);
            if (errCode != E_OK) {
                break;
            }
            continue;
        }
        int64_t rowid = -1;
        errCode = SaveSyncDataItem(item, saveDataStmt, rmDataStmt, rowid);
        if (errCode != E_OK && errCode != -E_NOT_FOUND) {
            LOGE("Save sync dataitem failed:%d.", errCode);
            break;
        }

        errCode = SaveSyncLog(saveLogStmt, item, maxTimestamp, rowid);
        if (errCode != E_OK) {
            LOGE("Save sync log failed:%d.", errCode);
            break;
        }
        maxTimestamp = std::max(item.timeStamp, maxTimestamp);
        // Need not reset rmDataStmt and rmLogStmt here.
        SQLiteUtils::ResetStatement(saveDataStmt, false, errCode);
        SQLiteUtils::ResetStatement(saveLogStmt, false, errCode);
    }

    if (errCode == -E_NOT_FOUND) {
        errCode = E_OK;
    }
    SQLiteUtils::ResetStatement(rmDataStmt, true, errCode);
    SQLiteUtils::ResetStatement(rmLogStmt, true, errCode);
    SQLiteUtils::ResetStatement(saveLogStmt, true, errCode);
    SQLiteUtils::ResetStatement(saveDataStmt, true, errCode);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::SaveSyncItems(const QueryObject &object, std::vector<DataItem> &dataItems,
    const std::string &deviceName, const TableInfo &table, TimeStamp &timeStamp)
{
    int errCode = StartTransaction(TransactType::IMMEDIATE);
    if (errCode != E_OK) {
        return errCode;
    }
    baseTblName_ = object.GetTableName();
    SetTableInfo(table);
    const std::string tableName = DBCommon::GetDistributedTableName(deviceName, baseTblName_);
    table_.SetTableName(tableName);
    errCode = SaveSyncDataItems(object, dataItems, deviceName, timeStamp);
    if (errCode == E_OK) {
        errCode = Commit();
    } else {
        (void)Rollback(); // Keep the error code of the first scene
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetDataItemForSync(sqlite3_stmt *stmt, DataItem &dataItem,
    bool isGettingDeletedData) const
{
    RowDataWithLog data;
    int errCode = GetLogData(stmt, data.logInfo);
    if (errCode != E_OK) {
        LOGE("relational data value transfer to kv fail");
        return errCode;
    }

    std::vector<FieldInfo> fieldInfos;
    if (!isGettingDeletedData) {
        for (size_t cid = 0; cid < table_.GetFields().size(); ++cid) {
            DataValue value;
            errCode = GetDataValueByType(stmt, value, cid + DBConstant::RELATIONAL_LOG_TABLE_FIELD_NUM);
            if (errCode != E_OK) {
                return errCode;
            }
            auto iter = table_.GetFields().find(table_.GetFieldName(cid));
            if (iter == table_.GetFields().end()) {
                LOGE("Get field name failed.");
                return -E_INTERNAL_ERROR;
            }
            fieldInfos.push_back(iter->second);
            data.rowData.push_back(value);
        }
    }

    errCode = DataTransformer::SerializeDataItem(data, fieldInfos, dataItem);
    if (errCode != E_OK) {
        LOGE("relational data value transfer to kv fail");
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetMissQueryData(std::vector<DataItem> &dataItems, size_t &dataTotalSize,
    const Key &cursorHashKey, sqlite3_stmt *fullStmt, size_t appendLength, const DataSizeSpecInfo &dataSizeInfo)
{
    int errCode = E_OK;
    while (true) {
        DataItem item;
        errCode = SQLiteUtils::StepWithRetry(fullStmt, isMemDb_);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            errCode = GetDataItemForSync(fullStmt, item, false);
            if (errCode != E_OK) {
                break;
            }
        } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = -E_FINISHED;
            break;
        } else {
            LOGE("Get full data failed:%d.", errCode);
            break;
        }
        if (item.hashKey == cursorHashKey) {
            break;
        }
        item.value = {};
        item.flag |= DataItem::REMOTE_DEVICE_DATA_MISS_QUERY;

        // If dataTotalSize value is bigger than blockSize value , reserve the surplus data item.
        dataTotalSize += GetDataItemSerialSize(item, appendLength);
        if ((dataTotalSize > dataSizeInfo.blockSize && !dataItems.empty()) ||
            dataItems.size() >= dataSizeInfo.packetSize) {
            errCode = -E_UNFINISHED;
            break;
        } else {
            dataItems.push_back(std::move(item));
        }
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::GetSyncDataByQuery(std::vector<DataItem> &dataItems, size_t appendLength,
    const DataSizeSpecInfo &sizeInfo, std::function<int(sqlite3 *, sqlite3_stmt *&, sqlite3_stmt *&, bool &)> getStmt,
    const TableInfo &tableInfo)
{
    baseTblName_ = tableInfo.GetTableName();
    SetTableInfo(tableInfo);
    sqlite3_stmt *queryStmt = nullptr;
    sqlite3_stmt *fullStmt = nullptr;
    bool isGettingDeletedData = false;
    int errCode = getStmt(dbHandle_, queryStmt, fullStmt, isGettingDeletedData);
    if (errCode != E_OK) {
        return errCode;
    }

    size_t dataTotalSize = 0;
    size_t overLongSize = 0;
    do {
        DataItem item;
        errCode = SQLiteUtils::StepWithRetry(queryStmt, isMemDb_);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            errCode = GetDataItemForSync(queryStmt, item, isGettingDeletedData);
            if (errCode != E_OK) {
                break;
            }
        } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            LOGD("Get sync data finished, size of packet:%zu, number of item:%zu", dataTotalSize, dataItems.size());
            errCode = -E_FINISHED;
        } else {
            LOGE("Get sync data error:%d", errCode);
            break;
        }

        // Get REMOTE_DEVICE_DATA_MISS_QUERY data.
        if (fullStmt != nullptr) {
            errCode = GetMissQueryData(dataItems, dataTotalSize, item.hashKey, fullStmt, appendLength, sizeInfo);
        }
        if (errCode != E_OK) {
            break;
        }

        // If one record is over 4M, ignore it.
        if (item.value.size() > DBConstant::MAX_VALUE_SIZE) {
            overLongSize++;
            continue;
        }

        // If dataTotalSize value is bigger than blockSize value , reserve the surplus data item.
        dataTotalSize += GetDataItemSerialSize(item, appendLength);
        if ((dataTotalSize > sizeInfo.blockSize && !dataItems.empty()) || dataItems.size() >= sizeInfo.packetSize) {
            errCode = -E_UNFINISHED;
            break;
        } else {
            dataItems.push_back(std::move(item));
        }
    } while (true);
    if (overLongSize != 0) {
        LOGW("Over 4M records:%zu.", overLongSize);
    }
    SQLiteUtils::ResetStatement(queryStmt, true, errCode);
    SQLiteUtils::ResetStatement(fullStmt, true, errCode);
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::CheckDBModeForRelational()
{
    std::string journalMode;
    int errCode = SQLiteUtils::GetJournalMode(dbHandle_, journalMode);

    for (auto &c : journalMode) { // convert to lowercase
        c = static_cast<char>(std::tolower(c));
    }

    if (errCode == E_OK && journalMode != "wal") {
        LOGE("Not support journal mode %s for relational db, expect wal mode.", journalMode.c_str());
        return -E_NOT_SUPPORT;
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::DeleteDistributedDeviceTable(const std::string &device,
    const std::string &tableName)
{
    std::vector<std::string> deviceTables;
    int errCode = GetDeviceTableName(dbHandle_, tableName, device, deviceTables);
    if (errCode != E_OK) {
        LOGE("Get device table name for alter table failed. %d", errCode);
        return errCode;
    }

    LOGD("Begin to delete device table: deviceTable[%d]", deviceTables.size());
    for (const auto &table : deviceTables) {
        std::string deleteSql = "DROP TABLE IF EXISTS " + table + ";"; // drop the found table
        errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, deleteSql);
        if (errCode != E_OK) {
            LOGE("Delete device data failed. %d", errCode);
            break;
        }
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::DeleteDistributedLogTable(const std::string &tableName)
{
    if (tableName.empty()) {
        return -E_INVALID_ARGS;
    }
    std::string logTableName = DBConstant::RELATIONAL_PREFIX + tableName + "_log";
    std::string deleteSql = "DROP TABLE IF EXISTS " + logTableName + ";";
    int errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, deleteSql);
    if (errCode != E_OK) {
        LOGE("Delete distributed log table failed. %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::CheckAndCleanDistributedTable(const std::vector<std::string> &tableNames,
    std::vector<std::string> &missingTables)
{
    if (tableNames.empty()) {
        return E_OK;
    }
    const std::string checkSql = "SELECT name FROM sqlite_master WHERE type='table' AND name=?;";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, checkSql, stmt);
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        return errCode;
    }
    for (const auto &tableName : tableNames) {
        errCode = SQLiteUtils::BindTextToStatement(stmt, 1, tableName); // 1: tablename bind index
        if (errCode != E_OK) {
            LOGE("Bind table name to check distributed table statement failed. %d", errCode);
            break;
        }

        errCode = SQLiteUtils::StepWithRetry(stmt, false);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) { // The table in schema was dropped
            errCode = DeleteDistributedDeviceTable({}, tableName); // Clean the auxiliary tables for the dropped table
            if (errCode != E_OK) {
                LOGE("Delete device tables for missing distributed table failed. %d", errCode);
                break;
            }
            errCode = DeleteDistributedLogTable(tableName);
            if (errCode != E_OK) {
                LOGE("Delete log tables for missing distributed table failed. %d", errCode);
                break;
            }
            missingTables.emplace_back(tableName);
        } else if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            LOGE("Check distributed table failed. %s", errCode);
            break;
        }
        errCode = E_OK; // Check result ok for distributed table is still exists
        SQLiteUtils::ResetStatement(stmt, false, errCode);
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return CheckCorruptedStatus(errCode);
}

int SQLiteSingleVerRelationalStorageExecutor::CreateDistributedDeviceTable(const std::string &device,
    const TableInfo &baseTbl)
{
    if (dbHandle_ == nullptr) {
        return -E_INVALID_DB;
    }

    if (device.empty() || !baseTbl.IsValid()) {
        return -E_INVALID_ARGS;
    }

    std::string deviceTableName = DBCommon::GetDistributedTableName(device, baseTbl.GetTableName());
    int errCode = SQLiteUtils::CreateSameStuTable(dbHandle_, baseTbl, deviceTableName);
    if (errCode != E_OK) {
        LOGE("Create device table failed. %d", errCode);
        return errCode;
    }

    errCode = SQLiteUtils::CloneIndexes(dbHandle_, baseTbl.GetTableName(), deviceTableName);
    if (errCode != E_OK) {
        LOGE("Copy index to device table failed. %d", errCode);
    }
    return errCode;
}

int SQLiteSingleVerRelationalStorageExecutor::CheckQueryObjectLegal(const TableInfo &table, QueryObject &query)
{
    if (dbHandle_ == nullptr) {
        return -E_INVALID_DB;
    }

    int errCode = E_OK;
    SqliteQueryHelper helper = query.GetQueryHelper(errCode);
    if (errCode != E_OK) {
        LOGE("Get query helper for check query failed. %d", errCode);
        return errCode;
    }

    if (!query.IsQueryForRelationalDB()) {
        LOGE("Not support for this query type.");
        return -E_NOT_SUPPORT;
    }

    SyncTimeRange defaultTimeRange;
    sqlite3_stmt *stmt = nullptr;
    errCode = helper.GetRelationalQueryStatement(dbHandle_, defaultTimeRange.beginTime, defaultTimeRange.endTime, {},
        stmt);
    if (errCode != E_OK) {
        LOGE("Get query statement for check query failed. %d", errCode);
        return errCode;
    }

    TableInfo newTable;
    SQLiteUtils::AnalysisSchema(dbHandle_, table.GetTableName(), newTable);
    if (errCode != E_OK) {
        LOGE("Check new schema failed. %d", errCode);
    } else {
        errCode = table.CompareWithTable(newTable);
        if (errCode != -E_RELATIONAL_TABLE_EQUAL && errCode != -E_RELATIONAL_TABLE_COMPATIBLE) {
            LOGE("Check schema failed, schema was changed. %d", errCode);
            errCode = -E_DISTRIBUTED_SCHEMA_CHANGED;
        } else {
            errCode = E_OK;
        }
    }

    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}
} // namespace DistributedDB
#endif
