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
#include "sqlite_utils.h"

#include <mutex>

#include "doc_errno.h"
#include "log_print.h"

namespace DocumentDB {
const int MAX_BLOB_READ_SIZE = 5 * 1024 * 1024; // 5M limit
const int MAX_TEXT_READ_SIZE = 5 * 1024 * 1024; // 5M limit
const int BUSY_TIMEOUT_MS = 3000;               // 3000ms for sqlite busy timeout.
const std::string BEGIN_SQL = "BEGIN TRANSACTION";
const std::string BEGIN_IMMEDIATE_SQL = "BEGIN IMMEDIATE TRANSACTION";
const std::string COMMIT_SQL = "COMMIT TRANSACTION";
const std::string ROLLBACK_SQL = "ROLLBACK TRANSACTION";

namespace {
int MapSqliteError(int errCode)
{
    switch (errCode) {
        case SQLITE_OK:
            return E_OK;
        case SQLITE_PERM:
        case SQLITE_CANTOPEN:
        case SQLITE_READONLY:
            return -E_FILE_OPERATION;
        case SQLITE_NOTADB:
            return -E_INVALID_FILE_FORMAT;
        case SQLITE_BUSY:
            return -E_RESOURCE_BUSY;
        default:
            return -E_ERROR;
    }
}

std::mutex g_logConfigMutex;
bool g_configLog = false;
} // namespace

void SQLiteUtils::SqliteLogCallback(void *data, int err, const char *msg)
{
#ifdef RUNNING_ON_SIMULATED_ENV
    GLOGD("[SQLite] err=%d sys=%d %s msg=%s", err, errno, sqlite3_errstr(err), msg);
#else
    GLOGD("[SQLite] err=%d sys=%d %s", err, errno, sqlite3_errstr(err));
#endif
}

int SQLiteUtils::CreateDataBase(const std::string &path, int flag, sqlite3 *&db)
{
    {
        std::lock_guard<std::mutex> lock(g_logConfigMutex);
        if (!g_configLog) {
            sqlite3_config(SQLITE_CONFIG_LOG, &SqliteLogCallback, nullptr);
            g_configLog = true;
        }
    }

    int errCode = sqlite3_open_v2(path.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (errCode != SQLITE_OK) {
        GLOGE("Open database failed. %d", errCode);
        if (db != nullptr) {
            (void)sqlite3_close_v2(db);
            db = nullptr;
        }
        return MapSqliteError(errCode);
    }

    errCode = sqlite3_busy_timeout(db, BUSY_TIMEOUT_MS);
    if (errCode != SQLITE_OK) {
        GLOGE("Set busy timeout failed:%d", errCode);
    }
    return MapSqliteError(errCode);
}

int SQLiteUtils::GetStatement(sqlite3 *db, const std::string &sql, sqlite3_stmt *&statement)
{
    if (db == nullptr) {
        GLOGE("Invalid db for get statement");
        return -E_INVALID_ARGS;
    }

    // Prepare the new statement only when the input parameter is not null
    if (statement != nullptr) {
        return E_OK;
    }
    int errCode = sqlite3_prepare_v2(db, sql.c_str(), -1, &statement, nullptr);
    if (errCode != SQLITE_OK) {
        GLOGE("Prepare SQLite statement failed:%d", errCode);
        (void)SQLiteUtils::ResetStatement(statement, true);
        return MapSqliteError(errCode);
    }

    if (statement == nullptr) {
        return -E_ERROR;
    }

    return E_OK;
}

int SQLiteUtils::StepWithRetry(sqlite3_stmt *statement)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    int errCode = sqlite3_step(statement);
    if (errCode != SQLITE_DONE && errCode != SQLITE_ROW) {
        GLOGE("[SQLiteUtils] Step error:%d, sys:%d", errCode, errno);
    }

    return errCode;
}

int SQLiteUtils::ResetStatement(sqlite3_stmt *&statement, bool finalize)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    int errCode = E_OK;
    if (!finalize) {
        errCode = sqlite3_reset(statement);
        if (errCode != SQLITE_OK) {
            GLOGE("[SQLiteUtils] reset statement error:%d, sys:%d", errCode, errno);
            goto FINALIZE;
        }

        (void)sqlite3_clear_bindings(statement);
        return errCode;
    }

FINALIZE:
    int finalizeResult = sqlite3_finalize(statement);
    if (finalizeResult != SQLITE_OK) {
        GLOGE("[SQLiteUtils] finalize statement error:%d, sys:%d", finalizeResult, errno);
    }
    statement = nullptr;
    return (errCode == SQLITE_OK ? finalizeResult : errCode);
}

int SQLiteUtils::BindBlobToStatement(sqlite3_stmt *statement, int index, const std::vector<uint8_t> &value)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    int errCode;
    if (value.empty()) {
        errCode = sqlite3_bind_zeroblob(statement, index, -1); // -1 for zero-length blob.
    } else {
        errCode = sqlite3_bind_blob(statement, index, static_cast<const void *>(value.data()), value.size(),
            SQLITE_TRANSIENT);
    }

    if (errCode != SQLITE_OK) {
        GLOGE("[SQLiteUtil][Bind blob] Failed to bind the value:%d", errCode);
    }
    return errCode;
}

int SQLiteUtils::GetColumnBlobValue(sqlite3_stmt *statement, int index, std::vector<uint8_t> &value)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    int keySize = sqlite3_column_bytes(statement, index);
    if (keySize < 0 || keySize > MAX_BLOB_READ_SIZE) {
        GLOGW("[SQLiteUtils][Column blob] size over limit:%d", keySize);
        value.resize(MAX_BLOB_READ_SIZE + 1); // Reset value size to invalid
        return E_OK;                          // Return OK for continue get data, but value is invalid
    }

    auto keyRead = static_cast<const uint8_t *>(sqlite3_column_blob(statement, index));
    if (keySize == 0 || keyRead == nullptr) {
        value.resize(0);
    } else {
        value.resize(keySize);
        value.assign(keyRead, keyRead + keySize);
    }

    return E_OK;
}

int SQLiteUtils::BindTextToStatement(sqlite3_stmt *statement, int index, const std::string &value)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    int errCode = sqlite3_bind_text(statement, index, value.c_str(), value.length(), SQLITE_TRANSIENT);
    if (errCode != SQLITE_OK) {
        GLOGE("[SQLiteUtil][Bind text]Failed to bind the value:%d", errCode);
        return errCode;
    }

    return E_OK;
}

int SQLiteUtils::GetColumnTextValue(sqlite3_stmt *statement, int index, std::string &value)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    int valSize = sqlite3_column_bytes(statement, index);
    if (valSize < 0 || valSize > MAX_TEXT_READ_SIZE) {
        GLOGW("[SQLiteUtils][Column text] size over limit:%d", valSize);
        value.resize(MAX_TEXT_READ_SIZE + 1); // Reset value size to invalid
        return E_OK;                          // Return OK for continue get data, but value is invalid
    }

    const unsigned char *val = sqlite3_column_text(statement, index);
    if (valSize == 0 || val == nullptr) {
        value = {};
    } else {
        value = std::string(reinterpret_cast<const char *>(val));
    }

    return E_OK;
}

int SQLiteUtils::BeginTransaction(sqlite3 *db, TransactType type)
{
    if (type == TransactType::IMMEDIATE) {
        return ExecSql(db, BEGIN_IMMEDIATE_SQL);
    }

    return ExecSql(db, BEGIN_SQL);
}

int SQLiteUtils::CommitTransaction(sqlite3 *db)
{
    return ExecSql(db, COMMIT_SQL);
}

int SQLiteUtils::RollbackTransaction(sqlite3 *db)
{
    return ExecSql(db, ROLLBACK_SQL);
}

int SQLiteUtils::ExecSql(sqlite3 *db, const std::string &sql)
{
    if (db == nullptr || sql.empty()) {
        return -E_INVALID_ARGS;
    }

    char *errMsg = nullptr;
    int errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (errCode != SQLITE_OK && errMsg != nullptr) {
        GLOGE("Execute sql failed. %d err: %s", errCode, errMsg);
    }

    sqlite3_free(errMsg);
    return MapSqliteError(errCode);
}

int SQLiteUtils::ExecSql(sqlite3 *db, const std::string &sql, const std::function<int(sqlite3_stmt *)> &bindCallback,
    const std::function<int(sqlite3_stmt *)> &resultCallback)
{
    if (db == nullptr || sql.empty()) {
        return -E_INVALID_ARGS;
    }
    bool bindFinish = true;
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        goto END;
    }

    do {
        if (bindCallback) {
            errCode = bindCallback(stmt);
            if (errCode != E_OK && errCode != -E_UNFINISHED) {
                goto END;
            }
            bindFinish = (errCode != -E_UNFINISHED); // continue bind if unfinished
        }

        while (true) {
            errCode = SQLiteUtils::StepWithRetry(stmt);
            if (errCode == SQLITE_DONE) {
                errCode = E_OK; // Step finished
                break;
            } else if (errCode != SQLITE_ROW) {
                goto END; // Step return error
            }
            if (resultCallback != nullptr && ((errCode = resultCallback(stmt)) != E_OK)) {
                goto END;
            }
        }
        errCode = SQLiteUtils::ResetStatement(stmt, false);
    } while (!bindFinish);

END:
    (void)SQLiteUtils::ResetStatement(stmt, true);
    return MapSqliteError(errCode);
}
} // namespace DocumentDB
