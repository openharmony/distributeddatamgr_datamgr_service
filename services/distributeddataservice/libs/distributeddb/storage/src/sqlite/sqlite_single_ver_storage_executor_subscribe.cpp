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

#include "sqlite_single_ver_storage_executor.h"

#include "log_print.h"
#include "db_common.h"
#include "db_errno.h"
#include "sqlite_single_ver_storage_executor_sql.h"

namespace DistributedDB {
using namespace TriggerMode;

int SQLiteSingleVerStorageExecutor::CheckQueryObjectLegal(QueryObject &queryObj) const
{
    int errCode = E_OK;
    SqliteQueryHelper helper = queryObj.GetQueryHelper(errCode);
    if (errCode != E_OK) {
        LOGE("Get query helper failed [%d]!", errCode);
        return errCode;
    }

    sqlite3_stmt *statement = nullptr;
    errCode = helper.GetQuerySyncStatement(dbHandle_, 0, INT64_MAX, statement); // (0, INT64_MAX):max range
    int ret = E_OK;
    SQLiteUtils::ResetStatement(statement, true, ret);
    if (ret != E_OK) {
        LOGW("Failed to reset statement. error:%d", ret);
    }
    return CheckCorruptedStatus(errCode);
}

int SQLiteSingleVerStorageExecutor::CheckMissQueryDataItem(sqlite3_stmt *stmt, const std::string &deviceName,
    DataItem &item)
{
    int errCode = SQLiteUtils::StepWithRetry(stmt, isMemDb_);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        // the value with same hashKey in DB matched the query
        std::vector<uint8_t> dev;
        errCode = SQLiteUtils::GetColumnBlobValue(stmt, SYNC_RES_DEVICE_INDEX, dev);
        if (errCode != E_OK) {
            LOGE("Get data device info failed. %d", errCode);
            return errCode;
        }
        auto timeStamp = static_cast<TimeStamp>(sqlite3_column_int64(stmt, SYNC_RES_TIME_INDEX));
        std::string device = std::string(dev.begin(), dev.end());
        // this data item should be neglected when it's out of date of it's from same device
        // otherwise, it should be erased after resolved the conflict
        item.neglect = (timeStamp > item.timeStamp) ||
            (timeStamp == item.timeStamp && device == DBCommon::TransferHashString(deviceName));
        return E_OK;
    } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        // the value with same hashKey in DB does not match the query, this data item should be neglected.
        item.neglect = true;
        return E_OK;
    }
    LOGE("Check sync data failed %d", errCode);
    return errCode;
}

// check the data with REMOTE_DEVICE_DATA_MISS_QUERY flag need neglect or not
int SQLiteSingleVerStorageExecutor::CheckMissQueryDataItems(sqlite3_stmt *&stmt, const SqliteQueryHelper &helper,
    const DeviceInfo &deviceInfo, std::vector<DataItem> &dataItems)
{
    int errCode = E_OK;
    for (auto &item : dataItems) {
        if ((item.flag & DataItem::REMOTE_DEVICE_DATA_MISS_QUERY) != 0 && !item.key.empty()) {
            errCode = helper.BindSyncDataCheckStmt(stmt, item.key);
            if (errCode != E_OK) {
                LOGE("Bind sync data check statement failed %d", errCode);
                break;
            }
            errCode = CheckMissQueryDataItem(stmt, deviceInfo.deviceName, item);
            if (errCode != E_OK) {
                LOGE("Check miss query data item failed. %d", errCode);
                return errCode;
            }
            SQLiteUtils::ResetStatement(stmt, false, errCode);
        }
    }
    return errCode;
}

int SQLiteSingleVerStorageExecutor::CheckDataWithQuery(QueryObject query, std::vector<DataItem> &dataItems,
    const DeviceInfo &deviceInfo)
{
    int errCode = E_OK;
    if (query.Empty()) {
        LOGD("Query is empty, skip check.");
        return E_OK;
    }
    SqliteQueryHelper helper = query.GetQueryHelper(errCode);
    if (errCode != E_OK) {
        LOGE("Get query helper failed [%d]!", errCode);
        return errCode;
    }
    std::string sql;
    errCode = helper.GetSyncDataCheckSql(sql);
    if (errCode != E_OK) {
        LOGE("Get sync data check sql failed");
        return errCode;
    }
    sqlite3_stmt *stmt = nullptr;
    errCode = SQLiteUtils::GetStatement(dbHandle_, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Get statement fail. %d", errCode);
        return -E_INVALID_QUERY_FORMAT;
    }
    errCode = CheckMissQueryDataItems(stmt, helper, deviceInfo, dataItems);
    if (errCode != E_OK) {
        LOGE("check data with query failed. %d", errCode);
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return CheckCorruptedStatus(errCode);
}

namespace {
std::string FormatSubscribeTriggerSql(const std::string &subscribeId, const std::string &subscribeCondition,
    TriggerModeEnum mode)
{
    std::string triggerModeString = GetTriggerModeString(mode);
    std::string accessString = ((mode == TriggerModeEnum::DELETE) ?
        DBConstant::TRIGGER_REFERENCES_OLD : DBConstant::TRIGGER_REFERENCES_NEW);
    std::string keyStr = DBConstant::SUBSCRIBE_QUERY_PREFIX + DBCommon::TransferHashString(subscribeId);
    Key key {keyStr.begin(), keyStr.end()};
    std::string hexKeyStr = DBCommon::VectorToHexString(key);
    std::string triggerName = DBConstant::SUBSCRIBE_QUERY_PREFIX + subscribeId + "_ON_" + triggerModeString;
    return "CREATE TRIGGER IF NOT EXISTS " + triggerName + " AFTER " + triggerModeString + " \n"
        "ON sync_data\n"
        "WHEN " + subscribeCondition + "\n"
        "BEGIN\n"
        "    SELECT " + DBConstant::UPDATE_META_FUNC + "(x'" + hexKeyStr + "', NEW.TIMESTAMP);\n"
        "END;";
}
}

int SQLiteSingleVerStorageExecutor::AddSubscribeTrigger(QueryObject &query, const std::string &subscribeId)
{
    if (executorState_ == ExecutorState::CACHEDB || executorState_ == ExecutorState::CACHE_ATTACH_MAIN) {
        LOGE("Not support add subscribe in cache db.");
        return -E_NOT_SUPPORT;
    }
    int errCode = E_OK;
    SqliteQueryHelper helper = query.GetQueryHelper(errCode);
    if (errCode != E_OK) {
        LOGE("Get query helper failed. %d", errCode);
        return errCode;
    }
    // check if sqlite function is registered or not
    sqlite3_stmt *stmt = nullptr;
    errCode = SQLiteUtils::GetStatement(dbHandle_, "SELECT " + DBConstant::UPDATE_META_FUNC + "('K', 0);", stmt);
    if (errCode != E_OK) {
        LOGE("sqlite function %s has not been created.", DBConstant::UPDATE_META_FUNC.c_str());
        return -E_NOT_SUPPORT;
    }
    SQLiteUtils::ResetStatement(stmt, true, errCode);

    // Delete data API is actually an update operation, there is no need for DELETE trigger
    for (auto mode : {TriggerModeEnum::INSERT, TriggerModeEnum::UPDATE}) {
        std::string subscribeCondition;
        errCode = helper.GetSubscribeSql(subscribeId, mode, subscribeCondition);
        if (errCode != E_OK) {
            LOGE("Get subscribe trigger create sql failed. mode: %d, errCode: %d", mode, errCode);
            return errCode;
        }
        std::string sql = FormatSubscribeTriggerSql(subscribeId, subscribeCondition, mode);
        errCode = SQLiteUtils::ExecuteRawSQL(dbHandle_, sql);
        if (errCode != E_OK) {
            LOGE("Add subscribe trigger failed. mode: %d, errCode: %d", mode, errCode);
            return errCode;
        }
    }
    return E_OK;
}

int SQLiteSingleVerStorageExecutor::RemoveSubscribeTrigger(const std::vector<std::string> &subscribeIds)
{
    int errCode = E_OK;
    for (const auto &id : subscribeIds) {
        for (auto mode : {TriggerModeEnum::INSERT, TriggerModeEnum::UPDATE}) {
            const std::string trigger = DBConstant::SUBSCRIBE_QUERY_PREFIX + id + "_ON_" + GetTriggerModeString(mode);
            errCode = SQLiteUtils::DropTriggerByName(dbHandle_, trigger);
            if (errCode != E_OK) {
                LOGE("remove subscribe trigger failed. %d", errCode);
                break;
            }
        }
        if (errCode != E_OK) {
            LOGE("remove subscribe trigger for id %s failed. %d", id.c_str(), errCode);
            break;
        }
    }
    return errCode;
}

int SQLiteSingleVerStorageExecutor::RemoveTrigger(const std::vector<std::string> &triggers)
{
    int errCode = E_OK;
    for (const auto &trigger : triggers) {
        errCode = SQLiteUtils::DropTriggerByName(dbHandle_, trigger);
        if (errCode != E_OK) {
            LOGE("remove trigger failed. %d", errCode);
            break;
        }
    }
    return errCode;
}

int SQLiteSingleVerStorageExecutor::RemoveSubscribeTriggerWaterMark(const std::vector<std::string> &subscribeIds)
{
    sqlite3_stmt *statement = nullptr;
    const std::string sql = attachMetaMode_ ? REMOVE_ATTACH_META_VALUE_SQL : REMOVE_META_VALUE_SQL;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, sql, statement);
    if (errCode != E_OK) {
        LOGE("Get remove trigger water mark statement failed. %d", errCode);
        return errCode;
    }
    for (const auto &id : subscribeIds) {
        errCode = SQLiteUtils::BindTextToStatement(statement, 1, DBConstant::SUBSCRIBE_QUERY_PREFIX + id);
        if (errCode != E_OK) {
            LOGE("Bind mark key to statement failed. %d", errCode);
            break;
        }
        errCode = SQLiteUtils::StepWithRetry(statement, isMemDb_);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
        } else {
            LOGE("Remove trigger water mark failed. %d", errCode);
            break;
        }
        SQLiteUtils::ResetStatement(statement, false, errCode);
    }
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteSingleVerStorageExecutor::GetTriggers(const std::string &namePreFix, std::vector<std::string> &triggerNames)
{
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(dbHandle_, GET_SYNC_DATA_TIRGGER_SQL, stmt);
    if (errCode != E_OK) {
        LOGE("Get trigger query statement failed. %d", errCode);
        return errCode;
    }

    errCode = SQLiteUtils::BindTextToStatement(stmt, 1, namePreFix + "%");
    if (errCode != E_OK) {
        SQLiteUtils::ResetStatement(stmt, true, errCode);
        LOGE("Bind trigger name prefix to statement failed. %d", errCode);
        return errCode;
    }

    do {
        errCode = SQLiteUtils::StepWithRetry(stmt, isMemDb_);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            break;
        } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            std::string name;
            SQLiteUtils::GetColumnTextValue(stmt, 0, name);
            triggerNames.emplace_back(name);
        } else {
            LOGE("Get trigger by name prefix failed. %d", errCode);
            break;
        }
    } while (true);

    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}
} // namespace DistributedDB
