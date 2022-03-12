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

#include "sqlite_utils.h"

#include <climits>
#include <cstring>
#include <chrono>
#include <thread>
#include <mutex>
#include <map>
#include <algorithm>

#include "sqlite_import.h"
#include "securec.h"
#include "db_constant.h"
#include "db_errno.h"
#include "db_common.h"
#include "log_print.h"
#include "value_object.h"
#include "schema_utils.h"
#include "schema_constant.h"
#include "time_helper.h"
#include "platform_specific.h"

namespace DistributedDB {
namespace {
    const int BUSY_TIMEOUT_MS = 3000; // 3000ms for sqlite busy timeout.
    const int BUSY_SLEEP_TIME = 50; // sleep for 50us
    const int NO_SIZE_LIMIT = -1;
    const int MAX_STEP_TIMES = 8000;
    const int BIND_KEY_INDEX = 1;
    const int BIND_VAL_INDEX = 2;
    const int USING_STR_LEN = -1;
    const std::string WAL_MODE_SQL = "PRAGMA journal_mode=WAL;";
    const std::string SYNC_MODE_FULL_SQL = "PRAGMA synchronous=FULL;";
    const std::string SYNC_MODE_NORMAL_SQL = "PRAGMA synchronous=NORMAL;";
    const std::string USER_VERSION_SQL = "PRAGMA user_version;";
    const std::string BEGIN_SQL = "BEGIN TRANSACTION";
    const std::string BEGIN_IMMEDIATE_SQL = "BEGIN IMMEDIATE TRANSACTION";
    const std::string COMMIT_SQL = "COMMIT TRANSACTION";
    const std::string ROLLBACK_SQL = "ROLLBACK TRANSACTION";
    const std::string JSON_EXTRACT_BY_PATH_TEST_CREATED = "SELECT json_extract_by_path('{\"field\":0}', '$.field', 0);";
    const std::string DEFAULT_ATTACH_CIPHER = "PRAGMA cipher_default_attach_cipher=";
    const std::string DEFAULT_ATTACH_KDF_ITER = "PRAGMA cipher_default_attach_kdf_iter=5000";
    const std::string EXPORT_BACKUP_SQL = "SELECT export_database('backup');";
    const std::string CIPHER_CONFIG_SQL = "PRAGMA codec_cipher=";
    const std::string KDF_ITER_CONFIG_SQL = "PRAGMA codec_kdf_iter=5000;";
    const std::string BACK_CIPHER_CONFIG_SQL = "PRAGMA backup.codec_cipher=";
    const std::string BACK_KDF_ITER_CONFIG_SQL = "PRAGMA backup.codec_kdf_iter=5000;";
    const std::string META_CIPHER_CONFIG_SQL = "PRAGMA meta.codec_cipher=";
    const std::string META_KDF_ITER_CONFIG_SQL = "PRAGMA meta.codec_kdf_iter=5000;";

    const std::string DETACH_BACKUP_SQL = "DETACH 'backup'";
    const std::string UPDATE_META_SQL = "INSERT OR REPLACE INTO meta_data VALUES (?, ?);";

    bool g_configLog = false;
    std::mutex g_logMutex;

    void SqliteLogCallback(void *data, int err, const char *msg)
    {
        bool verboseLog = (data != nullptr);
        auto errType = static_cast<unsigned int>(err);
        errType &= 0xFF;
        if (errType == 0 || errType == SQLITE_CONSTRAINT || errType == SQLITE_SCHEMA ||
            errType == SQLITE_NOTICE || err == SQLITE_WARNING_AUTOINDEX) {
            if (verboseLog) {
                LOGD("[SQLite] Error[%d] sys[%d] %s ", err, errno, sqlite3_errstr(err));
            }
        } else if (errType == SQLITE_WARNING || errType == SQLITE_IOERR ||
            errType == SQLITE_CORRUPT || errType == SQLITE_CANTOPEN) {
            LOGI("[SQLite] Error[%d], sys[%d], %s", err, errno, sqlite3_errstr(err));
        } else {
            LOGE("[SQLite] Error[%d], sys[%d]", err, errno);
        }
    }

    // statement must not be null
    std::string GetColString(sqlite3_stmt *statement, int nCol)
    {
        std::string rowString;
        for (int i = 0; i < nCol; i++) {
            if (sqlite3_column_name(statement, i) != nullptr) {
                rowString += sqlite3_column_name(statement, i);
            }
            int blankFill = (i + 1) * 16 - rowString.size(); // each column width 16
            rowString.append(static_cast<std::string::size_type>((blankFill > 0) ? blankFill : 0), ' ');
        }
        return rowString;
    }
}

namespace TriggerMode {
const std::map<TriggerModeEnum, std::string> TRIGGER_MODE_MAP = {
    {TriggerModeEnum::NONE,   ""},
    {TriggerModeEnum::INSERT, "INSERT"},
    {TriggerModeEnum::UPDATE, "UPDATE"},
    {TriggerModeEnum::DELETE, "DELETE"},
};

std::string GetTriggerModeString(TriggerModeEnum mode)
{
    auto it = TRIGGER_MODE_MAP.find(mode);
    return (it == TRIGGER_MODE_MAP.end()) ? "" : it->second;
}
}

int SQLiteUtils::CreateDataBase(const OpenDbProperties &properties, sqlite3 *&dbTemp, bool setWal)
{
    uint64_t flag = SQLITE_OPEN_URI | SQLITE_OPEN_READWRITE;
    if (properties.createIfNecessary) {
        flag |= SQLITE_OPEN_CREATE;
    }
    std::string cipherName = GetCipherName(properties.cipherType);
    if (cipherName.empty()) {
        LOGE("[SQLite] GetCipherName failed");
        return -E_INVALID_ARGS;
    }
    std::string defaultAttachCipher = DEFAULT_ATTACH_CIPHER + cipherName + ";";
    std::vector<std::string> sqls {defaultAttachCipher, DEFAULT_ATTACH_KDF_ITER};
    if (setWal) {
        sqls.push_back(WAL_MODE_SQL);
    }

    std::string fileUrl = DBConstant::SQLITE_URL_PRE + properties.uri;
    int errCode = sqlite3_open_v2(fileUrl.c_str(), &dbTemp, flag, nullptr);
    if (errCode != SQLITE_OK) {
        LOGE("[SQLite] open database failed: %d - sys err(%d)", errCode, errno);
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
        goto END;
    }

    errCode = SetDataBaseProperty(dbTemp, properties, sqls);
    if (errCode != SQLITE_OK) {
        LOGE("[SQLite] SetDataBaseProperty failed: %d", errCode);
        goto END;
    }

END:
    if (errCode != E_OK && dbTemp != nullptr) {
        (void)sqlite3_close_v2(dbTemp);
        dbTemp = nullptr;
    }

    return errCode;
}

int SQLiteUtils::OpenDatabase(const OpenDbProperties &properties, sqlite3 *&db, bool setWal)
{
    {
        // Only for register the sqlite3 log callback
        std::lock_guard<std::mutex> lock(g_logMutex);
        if (!g_configLog) {
            sqlite3_config(SQLITE_CONFIG_LOG, &SqliteLogCallback, &properties.createIfNecessary);
            g_configLog = true;
        }
    }
    sqlite3 *dbTemp = nullptr;
    int errCode = CreateDataBase(properties, dbTemp, setWal);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = RegisterJsonFunctions(dbTemp);
    if (errCode != E_OK) {
        goto END;
    }
    // Set the synchroized mode, default for full mode.
    errCode = ExecuteRawSQL(dbTemp, SYNC_MODE_FULL_SQL);
    if (errCode != E_OK) {
        LOGE("SQLite sync mode failed: %d", errCode);
    }
END:
    if (errCode != E_OK && dbTemp != nullptr) {
        (void)sqlite3_close_v2(dbTemp);
        dbTemp = nullptr;
    }
    if (errCode != E_OK && errno == EKEYREVOKED) {
        errCode = -E_EKEYREVOKED;
    }

    db = dbTemp;
    return errCode;
}

int SQLiteUtils::GetStatement(sqlite3 *db, const std::string &sql, sqlite3_stmt *&statement)
{
    if (db == nullptr) {
        LOGE("Invalid db for statement");
        return -E_INVALID_DB;
    }

    // Prepare the new statement only when the input parameter is not null
    if (statement != nullptr) {
        return E_OK;
    }

    int errCode = sqlite3_prepare_v2(db, sql.c_str(), NO_SIZE_LIMIT, &statement, nullptr);
    if (errCode != SQLITE_OK) {
        LOGE("Prepare SQLite statement failed:%d", errCode);
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return errCode;
    }

    if (statement == nullptr) {
        return -E_INVALID_DB;
    }

    return E_OK;
}

int SQLiteUtils::BindTextToStatement(sqlite3_stmt *statement, int index, const std::string &str)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    // Check empty value.
    if (str.empty()) {
        sqlite3_bind_null(statement, index);
        return E_OK;
    }

    int errCode = sqlite3_bind_text(statement, index, str.c_str(), str.length(), SQLITE_TRANSIENT);
    if (errCode != SQLITE_OK) {
        LOGE("[SQLiteUtil][Bind text]Failed to bind the value:%d", errCode);
        return SQLiteUtils::MapSQLiteErrno(errCode);
    }

    return E_OK;
}

int SQLiteUtils::BindInt64ToStatement(sqlite3_stmt *statement, int index, int64_t value)
{
    // statement check outSide
    int errCode = sqlite3_bind_int64(statement, index, value);
    if (errCode != SQLITE_OK) {
        LOGE("[SQLiteUtil][Bind int64]Failed to bind the value:%d", errCode);
        return SQLiteUtils::MapSQLiteErrno(errCode);
    }

    return E_OK;
}

int SQLiteUtils::BindBlobToStatement(sqlite3_stmt *statement, int index, const std::vector<uint8_t> &value,
    bool permEmpty)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    // Check empty value.
    if (value.empty() && !permEmpty) {
        LOGI("[SQLiteUtil][Bind blob]Invalid value");
        return -E_INVALID_ARGS;
    }

    int errCode;
    if (value.empty()) {
        errCode = sqlite3_bind_zeroblob(statement, index, -1); // -1 for zero-length blob.
    } else {
        errCode = sqlite3_bind_blob(statement, index, static_cast<const void *>(value.data()),
            value.size(), SQLITE_TRANSIENT);
    }

    if (errCode != SQLITE_OK) {
        LOGE("[SQLiteUtil][Bind blob]Failed to bind the value:%d", errCode);
        return SQLiteUtils::MapSQLiteErrno(errCode);
    }

    return E_OK;
}

void SQLiteUtils::ResetStatement(sqlite3_stmt *&statement, bool isNeedFinalize, int &errCode)
{
    if (statement == nullptr) {
        return;
    }

    int innerCode = SQLITE_OK;
    // if need finalize the statement, just goto finalize.
    if (isNeedFinalize) {
        goto FINAL;
    }

    // reset the statement firstly.
    innerCode = sqlite3_reset(statement);
    if (innerCode != SQLITE_OK) {
        LOGE("[SQLiteUtils] reset statement error:%d, sys:%d", innerCode, errno);
        isNeedFinalize = true;
    } else {
        sqlite3_clear_bindings(statement);
    }

FINAL:
    if (isNeedFinalize) {
        int finalizeResult = sqlite3_finalize(statement);
        if (finalizeResult != SQLITE_OK) {
            LOGD("[SQLiteUtils] finalize statement error:%d, sys:%d", finalizeResult, errno);
            innerCode = finalizeResult;
        }
        statement = nullptr;
    }

    if (innerCode != SQLITE_OK) { // the sqlite error code has higher priority.
        errCode = SQLiteUtils::MapSQLiteErrno(innerCode);
    }
}

int SQLiteUtils::StepWithRetry(sqlite3_stmt *statement, bool isMemDb)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    int errCode = E_OK;
    int retryCount = 0;
    do {
        errCode = sqlite3_step(statement);
        if ((errCode == SQLITE_LOCKED) && isMemDb) {
            std::this_thread::sleep_for(std::chrono::microseconds(BUSY_SLEEP_TIME));
            retryCount++;
        } else {
            break;
        }
    } while (retryCount <= MAX_STEP_TIMES);

    if (errCode != SQLITE_DONE && errCode != SQLITE_ROW) {
        LOGE("[SQLiteUtils] Step error:%d, sys:%d", errCode, errno);
    }

    return SQLiteUtils::MapSQLiteErrno(errCode);
}

int SQLiteUtils::BindPrefixKey(sqlite3_stmt *statement, int index, const Key &keyPrefix)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    const size_t maxKeySize = DBConstant::MAX_KEY_SIZE;
    // bind the first prefix key
    int errCode = BindBlobToStatement(statement, index, keyPrefix, true);
    if (errCode != SQLITE_OK) {
        LOGE("Bind the prefix first error:%d", errCode);
        return SQLiteUtils::MapSQLiteErrno(errCode);
    }

    // bind the second prefix key
    uint8_t end[maxKeySize];
    errno_t status = memset_s(end, maxKeySize, UCHAR_MAX, maxKeySize); // max byte value is 0xFF.
    if (status != EOK) {
        LOGE("memset error:%d", status);
        return -E_SECUREC_ERROR;
    }

    if (!keyPrefix.empty()) {
        status = memcpy_s(end, maxKeySize, keyPrefix.data(), keyPrefix.size());
        if (status != EOK) {
            LOGE("memcpy error:%d", status);
            return -E_SECUREC_ERROR;
        }
    }

    // index wouldn't be too large, just add one to the first index.
    errCode = sqlite3_bind_blob(statement, index + 1, end, maxKeySize, SQLITE_TRANSIENT);
    if (errCode != SQLITE_OK) {
        LOGE("Bind the prefix second error:%d", errCode);
        return SQLiteUtils::MapSQLiteErrno(errCode);
    }
    return E_OK;
}

int SQLiteUtils::BeginTransaction(sqlite3 *db, TransactType type)
{
    if (type == TransactType::IMMEDIATE) {
        return ExecuteRawSQL(db, BEGIN_IMMEDIATE_SQL);
    }

    return ExecuteRawSQL(db, BEGIN_SQL);
}

int SQLiteUtils::CommitTransaction(sqlite3 *db)
{
    return ExecuteRawSQL(db, COMMIT_SQL);
}

int SQLiteUtils::RollbackTransaction(sqlite3 *db)
{
    return ExecuteRawSQL(db, ROLLBACK_SQL);
}

int SQLiteUtils::ExecuteRawSQL(sqlite3 *db, const std::string &sql)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }
    char *errMsg = nullptr;
    int errCode = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (errCode != SQLITE_OK) {
        LOGE("[SQLiteUtils][ExecuteSQL] failed(%d), sys(%d)", errCode, errno);
    }

    if (errMsg != nullptr) {
        sqlite3_free(errMsg);
        errMsg = nullptr;
    }
    return SQLiteUtils::MapSQLiteErrno(errCode);
}

int SQLiteUtils::SetKey(sqlite3 *db, CipherType type, const CipherPassword &passwd, const bool &isMemDb)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    // in memory mode no need cipher
    if (isMemDb) {
        return E_OK;
    }

    if (passwd.GetSize() != 0) {
#ifndef OMIT_ENCRYPT
        int errCode = sqlite3_key(db, static_cast<const void *>(passwd.GetData()), static_cast<int>(passwd.GetSize()));
        if (errCode != SQLITE_OK) {
            LOGE("[SQLiteUtils][Setkey] config key failed:(%d)", errCode);
            return SQLiteUtils::MapSQLiteErrno(errCode);
        }

        errCode = SQLiteUtils::SetCipherSettings(db, type);
        if (errCode != E_OK) {
            LOGE("[SQLiteUtils][Setkey] set cipher settings failed:%d", errCode);
            return errCode;
        }
#else
        return -E_NOT_SUPPORT;
#endif
    }

    // verify key
    int errCode = SQLiteUtils::ExecuteRawSQL(db, USER_VERSION_SQL);
    if (errCode != E_OK) {
        LOGE("[SQLiteUtils][Setkey] verify version failed:%d", errCode);
        if (errno == EKEYREVOKED) {
            return -E_EKEYREVOKED;
        }
        if (errCode == -E_BUSY) {
            return errCode;
        }
        return -E_INVALID_PASSWD_OR_CORRUPTED_DB;
    }
    return E_OK;
}

int SQLiteUtils::GetColumnBlobValue(sqlite3_stmt *statement, int index, std::vector<uint8_t> &value)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }

    int keySize = sqlite3_column_bytes(statement, index);
    auto keyRead = static_cast<const uint8_t *>(sqlite3_column_blob(statement, index));
    if (keySize < 0) {
        LOGE("[SQLiteUtils][Column blob] size:%d", keySize);
        return -E_INVALID_DATA;
    } else if (keySize == 0 || keyRead == nullptr) {
        value.resize(0);
    } else {
        value.resize(keySize);
        value.assign(keyRead, keyRead + keySize);
    }

    return E_OK;
}

int SQLiteUtils::GetColumnTextValue(sqlite3_stmt *statement, int index, std::string &value)
{
    if (statement == nullptr) {
        return -E_INVALID_ARGS;
    }
    const unsigned char *val = sqlite3_column_text(statement, index);
    value = (val != nullptr) ? std::string(reinterpret_cast<const char *>(val)) : std::string();
    return E_OK;
}

int SQLiteUtils::AttachNewDatabase(sqlite3 *db, CipherType type, const CipherPassword &password,
    const std::string &attachDbAbsPath, const std::string &attachAsName)
{
    // example: "ATTACH '../new.db' AS backup KEY XXXX;"
    std::string attachSql = "ATTACH ? AS " + attachAsName + " KEY ?;"; // Internal interface not need verify alias name

    sqlite3_stmt* statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, attachSql, statement);
    if (errCode != E_OK) {
        return errCode;
    }
    // 1st is name.
    errCode = sqlite3_bind_text(statement, 1, attachDbAbsPath.c_str(), attachDbAbsPath.length(), SQLITE_TRANSIENT);
    if (errCode != SQLITE_OK) {
        LOGE("Bind the attached db name failed:%d", errCode);
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
        goto END;
    }
    // Passwords do not allow vector operations, so we can not use function BindBlobToStatement here.
    errCode = sqlite3_bind_blob(statement, 2, static_cast<const void *>(password.GetData()),  // 2 means password index.
        password.GetSize(), SQLITE_TRANSIENT);
    if (errCode != SQLITE_OK) {
        LOGE("Bind the attached key failed:%d", errCode);
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
        goto END;
    }

    errCode = SQLiteUtils::StepWithRetry(statement);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        LOGE("Execute the SQLite attach failed:%d", errCode);
        goto END;
    }
    errCode = SQLiteUtils::ExecuteRawSQL(db, WAL_MODE_SQL);
    if (errCode != E_OK) {
        LOGE("Set journal mode failed: %d", errCode);
    }

END:
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteUtils::CreateMetaDatabase(const std::string &metaDbPath)
{
    OpenDbProperties metaProperties {metaDbPath, true, false};
    sqlite3 *db = nullptr;
    int errCode = SQLiteUtils::OpenDatabase(metaProperties, db);
    if (errCode != E_OK) {
        LOGE("[CreateMetaDatabase] Failed to create the meta database[%d]", errCode);
    }
    if (db != nullptr) {
        (void)sqlite3_close_v2(db);
        db = nullptr;
    }
    return errCode;
}

int SQLiteUtils::CheckIntegrity(sqlite3 *db, const std::string &sql)
{
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        LOGE("Prepare the integrity check statement error:%d", errCode);
        return errCode;
    }
    int resultCnt = 0;
    bool checkResultOK = false;
    do {
        errCode = SQLiteUtils::StepWithRetry(statement);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            break;
        } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            auto result = reinterpret_cast<const char *>(sqlite3_column_text(statement, 0));
            if (result == nullptr) {
                continue;
            }
            resultCnt = (resultCnt > 1) ? resultCnt : (resultCnt + 1);
            if (strcmp(result, "ok") == 0) {
                checkResultOK = true;
            }
        } else {
            checkResultOK = false;
            LOGW("Step for the integrity check failed:%d", errCode);
            break;
        }
    } while (true);
    if (resultCnt == 1 && checkResultOK) {
        errCode = E_OK;
    } else {
        errCode = -E_INVALID_PASSWD_OR_CORRUPTED_DB;
    }
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}
#ifdef RELATIONAL_STORE
namespace { // anonymous namespace for schema analysis
int AnalysisSchemaSqlAndTrigger(sqlite3 *db, const std::string &tableName, TableInfo &table)
{
    std::string sql = "select type, sql from sqlite_master where tbl_name = ?";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] Prepare the analysis schema sql and trigger statement error:%d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::BindTextToStatement(statement, 1, tableName);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] Bind table name failed:%d", errCode);
        SQLiteUtils::ResetStatement(statement, true, errCode);
        return errCode;
    }

    errCode = -E_NOT_FOUND;
    std::vector<std::string> triggerList;
    do {
        int err = SQLiteUtils::StepWithRetry(statement);
        if (err == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            break;
        } else if (err == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            errCode = E_OK;
            std::string type;
            (void) SQLiteUtils::GetColumnTextValue(statement, 0, type);
            if (type == "table") {
                std::string createTableSql;
                (void) SQLiteUtils::GetColumnTextValue(statement, 1, createTableSql); // 1 means create table sql
                table.SetCreateTableSql(createTableSql);
            }
        } else {
            LOGE("[AnalysisSchema] Step for the analysis create table sql and trigger failed:%d", err);
            errCode = SQLiteUtils::MapSQLiteErrno(err);
            break;
        }
    } while (true);
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}

int GetSchemaIndexList(sqlite3 *db, const std::string &tableName, std::vector<std::string> &indexList)
{
    std::string sql = "pragma index_list(" + tableName + ")";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] Prepare the get schema index list statement error:%d", errCode);
        return errCode;
    }

    do {
        errCode = SQLiteUtils::StepWithRetry(statement);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            break;
        } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            std::string indexName;
            (void) SQLiteUtils::GetColumnTextValue(statement, 1, indexName);  // 1 means index name
            std::string origin;
            (void) SQLiteUtils::GetColumnTextValue(statement, 3, origin);  // 3 means index type, whether unique
            if (origin == "c") { // 'c' means index created by user declare
                indexList.push_back(indexName);
            }
        } else {
            LOGW("[AnalysisSchema] Step for the get schema index list failed:%d", errCode);
            break;
        }
    } while (true);
    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}

int AnalysisSchemaIndexDefine(sqlite3 *db, const std::string &indexName, CompositeFields &indexDefine)
{
    auto sql = "pragma index_info(" + indexName + ")";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] Prepare the analysis schema index statement error:%d", errCode);
        return errCode;
    }

    do {
        errCode = SQLiteUtils::StepWithRetry(statement);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            break;
        } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            std::string indexField;
            (void) SQLiteUtils::GetColumnTextValue(statement, 2, indexField);  // 2 means index's column name.
            indexDefine.push_back(indexField);
        } else {
            LOGW("[AnalysisSchema] Step for the analysis schema index failed:%d", errCode);
            break;
        }
    } while (true);

    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}

int AnalysisSchemaIndex(sqlite3 *db, const std::string &tableName, TableInfo &table)
{
    std::vector<std::string> indexList;
    int errCode = GetSchemaIndexList(db, tableName, indexList);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] get schema index list failed.");
        return errCode;
    }

    for (const auto &indexName : indexList) {
        CompositeFields indexDefine;
        errCode = AnalysisSchemaIndexDefine(db, indexName, indexDefine);
        if (errCode != E_OK) {
            LOGE("[AnalysisSchema] analysis schema index columns failed.");
            return errCode;
        }
        table.AddIndexDefine(indexName, indexDefine);
    }
    return E_OK;
}

namespace {
bool CheckFieldName(const std::string &fieldName)
{
    auto iter = std::find_if_not(fieldName.begin(), fieldName.end(), [](char c) {
            return (std::isalnum(c) || c == '_');
        });
    return iter == fieldName.end();
}
}

int SetFieldInfo(sqlite3_stmt *statement, TableInfo &table)
{
    FieldInfo field;
    field.SetColumnId(sqlite3_column_int(statement, 0));  // 0 means column id index

    std::string tmpString;
    (void) SQLiteUtils::GetColumnTextValue(statement, 1, tmpString);  // 1 means column name index
    if (!CheckFieldName(tmpString)) {
        LOGE("[AnalysisSchema] unsupported field name.");
        return -E_NOT_SUPPORT;
    }
    field.SetFieldName(tmpString);

    (void) SQLiteUtils::GetColumnTextValue(statement, 2, tmpString);  // 2 means datatype index
    field.SetDataType(tmpString);

    field.SetNotNull(static_cast<bool>(sqlite3_column_int64(statement, 3)));  // 3 means whether null index

    (void) SQLiteUtils::GetColumnTextValue(statement, 4, tmpString);  // 4 means default value index
    if (!tmpString.empty()) {
        field.SetDefaultValue(tmpString);
    }

    if (sqlite3_column_int64(statement, 5) != 0) {  // 5 means primary key index
        if (!table.GetPrimaryKey().empty()) {
            // Primary key is already set, usually because the primary key has multiple fields, not support
            LOGE("[AnalysisSchema] Not support for composite primary key");
            return -E_NOT_SUPPORT;
        }
        table.SetPrimaryKey(field.GetFieldName());
    }
    table.AddField(field);
    return E_OK;
}

int AnalysisSchemaFieldDefine(sqlite3 *db, const std::string &tableName, TableInfo &table)
{
    std::string sql = "pragma table_info(" + tableName + ")";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] Prepare the analysis schema field statement error:%d", errCode);
        return errCode;
    }

    do {
        errCode = SQLiteUtils::StepWithRetry(statement);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
            break;
        } else if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            errCode = SetFieldInfo(statement, table);
            if (errCode != E_OK) {
                break;
            }
        } else {
            LOGW("[AnalysisSchema] Step for the analysis schema field failed:%d", errCode);
            break;
        }
    } while (true);

    if (table.GetPrimaryKey().empty()) {
        table.SetPrimaryKey("rowid");
    }

    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}
} // end of anonymous namespace for schema analysis

int SQLiteUtils::AnalysisSchema(sqlite3 *db, const std::string &tableName, TableInfo &table)
{
    int errCode = AnalysisSchemaSqlAndTrigger(db, tableName, table);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] Analysis sql and trigger failed. errCode = [%d]", errCode);
        return errCode;
    }

    errCode = AnalysisSchemaIndex(db, tableName, table);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] Analysis index failed.");
        return errCode;
    }

    errCode = AnalysisSchemaFieldDefine(db, tableName, table);
    if (errCode != E_OK) {
        LOGE("[AnalysisSchema] Analysis field failed.");
        return errCode;
    }

    table.SetTableName(tableName);
    return E_OK;
}
#endif
#ifndef OMIT_ENCRYPT
int SQLiteUtils::ExportDatabase(sqlite3 *db, CipherType type, const CipherPassword &passwd,
    const std::string &newDbName)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    int errCode = AttachNewDatabase(db, type, passwd, newDbName);
    if (errCode != E_OK) {
        LOGE("Attach New Db fail!");
        return errCode;
    }
    errCode = SQLiteUtils::ExecuteRawSQL(db, EXPORT_BACKUP_SQL);
    if (errCode != E_OK) {
        LOGE("Execute the SQLite export failed:%d", errCode);
    }

    int detachError = SQLiteUtils::ExecuteRawSQL(db, DETACH_BACKUP_SQL);
    if (errCode == E_OK) {
        errCode = detachError;
        if (detachError != E_OK) {
            LOGE("Execute the SQLite detach failed:%d", errCode);
        }
    }
    return errCode;
}

int SQLiteUtils::Rekey(sqlite3 *db, const CipherPassword &passwd)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    int errCode = sqlite3_rekey(db, static_cast<const void *>(passwd.GetData()), static_cast<int>(passwd.GetSize()));
    if (errCode != E_OK) {
        LOGE("SQLite rekey failed:(%d)", errCode);
        return SQLiteUtils::MapSQLiteErrno(errCode);
    }

    return E_OK;
}
#else
int SQLiteUtils::ExportDatabase(sqlite3 *db, CipherType type, const CipherPassword &passwd,
    const std::string &newDbName)
{
    (void)db;
    (void)type;
    (void)passwd;
    (void)newDbName;
    return -E_NOT_SUPPORT;
}

int SQLiteUtils::Rekey(sqlite3 *db, const CipherPassword &passwd)
{
    (void)db;
    (void)passwd;
    return -E_NOT_SUPPORT;
}
#endif

int SQLiteUtils::GetVersion(const OpenDbProperties &properties, int &version)
{
    if (properties.uri.empty()) {
        return -E_INVALID_ARGS;
    }

    sqlite3 *dbTemp = nullptr;
    // Please make sure the database file exists and is working properly
    std::string fileUrl = DBConstant::SQLITE_URL_PRE + properties.uri;
    int errCode = sqlite3_open_v2(fileUrl.c_str(), &dbTemp, SQLITE_OPEN_URI | SQLITE_OPEN_READONLY, nullptr);
    if (errCode != SQLITE_OK) {
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
        LOGE("Open database failed: %d, sys:%d", errCode, errno);
        goto END;
    }

    errCode = SQLiteUtils::SetKey(dbTemp, properties.cipherType, properties.passwd, properties.isMemDb);
    if (errCode != E_OK) {
        LOGE("Set key failed: %d", errCode);
        goto END;
    }

    errCode = GetVersion(dbTemp, version);

END:
    if (dbTemp != nullptr) {
        (void)sqlite3_close_v2(dbTemp);
        dbTemp = nullptr;
    }
    return errCode;
}

int SQLiteUtils::GetVersion(sqlite3 *db, int &version)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    std::string strSql = "PRAGMA user_version;";
    sqlite3_stmt *statement = nullptr;
    int errCode = sqlite3_prepare(db, strSql.c_str(), -1, &statement, nullptr);
    if (errCode != SQLITE_OK || statement == nullptr) {
        LOGE("[SqlUtil][GetVer] sqlite3_prepare failed.");
        errCode = SQLiteUtils::MapSQLiteErrno(errCode);
        return errCode;
    }

    if (sqlite3_step(statement) == SQLITE_ROW) {
        // Get pragma user_version at first column
        version = sqlite3_column_int(statement, 0);
        LOGD("[SqlUtil][GetVer] db version=%d", version);
    } else {
        LOGE("[SqlUtil][GetVer] Get db user_version failed.");
        errCode = SQLiteUtils::MapSQLiteErrno(SQLITE_ERROR);
    }

    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteUtils::GetJournalMode(sqlite3 *db, std::string &mode)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    std::string sql = "PRAGMA journal_mode;";
    sqlite3_stmt *statement = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, statement);
    if (errCode != E_OK || statement == nullptr) {
        return errCode;
    }

    errCode = SQLiteUtils::StepWithRetry(statement);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        errCode = SQLiteUtils::GetColumnTextValue(statement, 0, mode);
    } else {
        LOGE("[SqlUtil][GetJournal] Get db journal_mode failed.");
    }

    SQLiteUtils::ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteUtils::SetUserVer(const OpenDbProperties &properties, int version)
{
    if (properties.uri.empty()) {
        return -E_INVALID_ARGS;
    }

    // Please make sure the database file exists and is working properly
    sqlite3 *db = nullptr;
    int errCode = SQLiteUtils::OpenDatabase(properties, db);
    if (errCode != E_OK) {
        return errCode;
    }

    // Set user version
    errCode = SQLiteUtils::SetUserVer(db, version);
    if (errCode != E_OK) {
        LOGE("Set user version fail: %d", errCode);
        goto END;
    }

END:
    if (db != nullptr) {
        (void)sqlite3_close_v2(db);
        db = nullptr;
    }

    return errCode;
}

int SQLiteUtils::SetUserVer(sqlite3 *db, int version)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }
    std::string userVersionSql = "PRAGMA user_version=" + std::to_string(version) + ";";
    return SQLiteUtils::ExecuteRawSQL(db, userVersionSql);
}

int SQLiteUtils::MapSQLiteErrno(int errCode)
{
    if (errCode == SQLITE_OK) {
        return E_OK;
    } else if (errCode == SQLITE_IOERR) {
        if (errno == EKEYREVOKED) {
            return -E_EKEYREVOKED;
        }
    } else if (errCode == SQLITE_CORRUPT || errCode == SQLITE_NOTADB) {
        return -E_INVALID_PASSWD_OR_CORRUPTED_DB;
    } else if (errCode == SQLITE_LOCKED || errCode == SQLITE_BUSY) {
        return -E_BUSY;
    } else if (errCode == SQLITE_ERROR && errno == EKEYREVOKED) {
        return -E_EKEYREVOKED;
    }
    return -errCode;
}

int SQLiteUtils::SetBusyTimeout(sqlite3 *db, int timeout)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    // Set the default busy handler to retry automatically before returning SQLITE_BUSY.
    int errCode = sqlite3_busy_timeout(db, timeout);
    if (errCode != SQLITE_OK) {
        LOGE("[SQLite] set busy timeout failed:%d", errCode);
    }

    return SQLiteUtils::MapSQLiteErrno(errCode);
}

#ifndef OMIT_ENCRYPT
int SQLiteUtils::ExportDatabase(const std::string &srcFile, CipherType type, const CipherPassword &srcPasswd,
    const std::string &targetFile, const CipherPassword &passwd)
{
    std::vector<std::string> createTableSqls;
    OpenDbProperties option = {srcFile, true, false, createTableSqls, type, srcPasswd};
    sqlite3 *db = nullptr;
    int errCode = SQLiteUtils::OpenDatabase(option, db);
    if (errCode != E_OK) {
        LOGE("Open db error while exporting:%d", errCode);
        return errCode;
    }

    errCode = SQLiteUtils::ExportDatabase(db, type, passwd, targetFile);
    if (db != nullptr) {
        (void)sqlite3_close_v2(db);
        db = nullptr;
    }
    return errCode;
}
#else
int SQLiteUtils::ExportDatabase(const std::string &srcFile, CipherType type, const CipherPassword &srcPasswd,
    const std::string &targetFile, const CipherPassword &passwd)
{
    (void)srcFile;
    (void)type;
    (void)srcPasswd;
    (void)targetFile;
    (void)passwd;
    return -E_NOT_SUPPORT;
}
#endif

int SQLiteUtils::SaveSchema(const OpenDbProperties &properties)
{
    if (properties.uri.empty()) {
        return -E_INVALID_ARGS;
    }

    sqlite3 *db = nullptr;
    int errCode = OpenDatabase(properties, db);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = SaveSchema(db, properties.schema);
    (void)sqlite3_close_v2(db);
    db = nullptr;
    return errCode;
}

int SQLiteUtils::SaveSchema(sqlite3 *db, const std::string &strSchema)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    sqlite3_stmt *statement = nullptr;
    std::string sql = "INSERT OR REPLACE INTO meta_data VALUES(?,?);";
    int errCode = GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        return errCode;
    }

    Key schemaKey;
    DBCommon::StringToVector(DBConstant::SCHEMA_KEY, schemaKey);
    errCode = BindBlobToStatement(statement, BIND_KEY_INDEX, schemaKey, false);
    if (errCode != E_OK) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }

    Value schemaValue;
    DBCommon::StringToVector(strSchema, schemaValue);
    errCode = BindBlobToStatement(statement, BIND_VAL_INDEX, schemaValue, false);
    if (errCode != E_OK) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }

    errCode = StepWithRetry(statement); // memory db does not support schema
    if (errCode != MapSQLiteErrno(SQLITE_DONE)) {
        LOGE("[SqlUtil][SetSchema] StepWithRetry fail, errCode=%d.", errCode);
        ResetStatement(statement, true, errCode);
        return errCode;
    }
    errCode = E_OK;
    ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteUtils::GetSchema(const OpenDbProperties &properties, std::string &strSchema)
{
    sqlite3 *db = nullptr;
    int errCode = OpenDatabase(properties, db);
    if (errCode != E_OK) {
        return errCode;
    }

    int version = 0;
    errCode = GetVersion(db, version);
    if (version <= 0 || errCode != E_OK) {
        // if version does exist, it represents database is error
        (void)sqlite3_close_v2(db);
        db = nullptr;
        return -E_INVALID_VERSION;
    }

    errCode = GetSchema(db, strSchema);
    (void)sqlite3_close_v2(db);
    db = nullptr;
    return errCode;
}

int SQLiteUtils::GetSchema(sqlite3 *db, std::string &strSchema)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    sqlite3_stmt *statement = nullptr;
    std::string sql = "SELECT value FROM meta_data WHERE key=?;";
    int errCode = GetStatement(db, sql, statement);
    if (errCode != E_OK) {
        return errCode;
    }

    Key schemakey;
    DBCommon::StringToVector(DBConstant::SCHEMA_KEY, schemakey);
    errCode = BindBlobToStatement(statement, 1, schemakey, false);
    if (errCode != E_OK) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }

    errCode = StepWithRetry(statement); // memory db does not support schema
    if (errCode == MapSQLiteErrno(SQLITE_DONE)) {
        ResetStatement(statement, true, errCode);
        return -E_NOT_FOUND;
    } else if (errCode != MapSQLiteErrno(SQLITE_ROW)) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }

    Value schemaValue;
    errCode = GetColumnBlobValue(statement, 0, schemaValue);
    if (errCode != E_OK) {
        ResetStatement(statement, true, errCode);
        return errCode;
    }
    DBCommon::VectorToString(schemaValue, strSchema);
    ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteUtils::IncreaseIndex(sqlite3 *db, const IndexName &name, const IndexInfo &info, SchemaType type,
    uint32_t skipSize)
{
    if (db == nullptr) {
        LOGE("[IncreaseIndex] Sqlite DB not exists.");
        return -E_INVALID_DB;
    }
    if (name.empty()) {
        LOGE("[IncreaseIndex] Name can not be empty.");
        return -E_NOT_PERMIT;
    }
    if (info.empty()) {
        LOGE("[IncreaseIndex] Info can not be empty.");
        return -E_NOT_PERMIT;
    }
    std::string indexName = SchemaUtils::FieldPathString(name);
    std::string sqlCommand = "CREATE INDEX IF NOT EXISTS '" + indexName + "' ON sync_data (";
    for (uint32_t i = 0; i < info.size(); i++) {
        if (i != 0) {
            sqlCommand += ", ";
        }
        std::string extractSql = SchemaObject::GenerateExtractSQL(type, info[i].first, info[i].second,
            skipSize);
        if (extractSql.empty()) { // Unlikely
            LOGE("[IncreaseIndex] GenerateExtractSQL fail at field=%u.", i);
            return -E_INTERNAL_ERROR;
        }
        sqlCommand += extractSql;
    }
    sqlCommand += ") WHERE (flag&0x01=0);";
    return SQLiteUtils::ExecuteRawSQL(db, sqlCommand);
}

int SQLiteUtils::ChangeIndex(sqlite3 *db, const IndexName &name, const IndexInfo &info, SchemaType type,
    uint32_t skipSize)
{
    // Currently we change index by drop it then create it, SQLite "REINDEX" may be used in the future
    int errCode = DecreaseIndex(db, name);
    if (errCode != OK) {
        LOGE("[ChangeIndex] Decrease fail=%d.", errCode);
        return errCode;
    }
    errCode = IncreaseIndex(db, name, info, type, skipSize);
    if (errCode != OK) {
        LOGE("[ChangeIndex] Increase fail=%d.", errCode);
        return errCode;
    }
    return E_OK;
}

int SQLiteUtils::DecreaseIndex(sqlite3 *db, const IndexName &name)
{
    if (db == nullptr) {
        LOGE("[DecreaseIndex] Sqlite DB not exists.");
        return -E_INVALID_DB;
    }
    if (name.empty()) {
        LOGE("[DecreaseIndex] Name can not be empty.");
        return -E_NOT_PERMIT;
    }
    std::string indexName = SchemaUtils::FieldPathString(name);
    std::string sqlCommand = "DROP INDEX IF EXISTS '" + indexName + "';";
    return ExecuteRawSQL(db, sqlCommand);
}

int SQLiteUtils::RegisterJsonFunctions(sqlite3 *db)
{
    if (db == nullptr) {
        LOGE("Sqlite DB not exists.");
        return -E_INVALID_DB;
    }
    int errCode = sqlite3_create_function_v2(db, "calc_hash_key", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
        nullptr, &CalcHashKey, nullptr, nullptr, nullptr);
    if (errCode != SQLITE_OK) {
        LOGE("sqlite3_create_function_v2 about calc_hash_key returned %d", errCode);
        return MapSQLiteErrno(errCode);
    }
#ifdef USING_DB_JSON_EXTRACT_AUTOMATICALLY
    errCode = ExecuteRawSQL(db, JSON_EXTRACT_BY_PATH_TEST_CREATED);
    if (errCode == E_OK) {
        LOGI("json_extract_by_path already created.");
    } else {
        // Specify need 3 parameter in json_extract_by_path function
        errCode = sqlite3_create_function_v2(db, "json_extract_by_path", 3, SQLITE_UTF8 | SQLITE_DETERMINISTIC,
            nullptr, &JsonExtractByPath, nullptr, nullptr, nullptr);
        if (errCode != SQLITE_OK) {
            LOGE("sqlite3_create_function_v2 about json_extract_by_path returned %d", errCode);
            return MapSQLiteErrno(errCode);
        }
    }
#endif
    return E_OK;
}

namespace {
void SchemaObjectDestructor(SchemaObject *inObject)
{
    delete inObject;
    inObject = nullptr;
}
}
#ifdef RELATIONAL_STORE
int SQLiteUtils::RegisterCalcHash(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &CalcHashKey;
    return SQLiteUtils::RegisterFunction(db, "calc_hash", 1, nullptr, func);
}

void SQLiteUtils::GetSysTime(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 1 || argv == nullptr) {
        LOGE("Parameter does not meet restrictions.");
        return;
    }

    sqlite3_result_int64(ctx, (sqlite3_int64)TimeHelper::GetSysCurrentTime());
}

int SQLiteUtils::RegisterGetSysTime(sqlite3 *db)
{
    TransactFunc func;
    func.xFunc = &GetSysTime;
    return SQLiteUtils::RegisterFunction(db, "get_sys_time", 1, nullptr, func);
}

int SQLiteUtils::CreateRelationalMetaTable(sqlite3 *db)
{
    std::string sql =
        "CREATE TABLE IF NOT EXISTS " + DBConstant::RELATIONAL_PREFIX + "metadata(" \
        "key    BLOB PRIMARY KEY NOT NULL," \
        "value  BLOB);";

    int errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    if (errCode != E_OK) {
        LOGE("[SQLite] execute create table sql failed");
        return errCode;
    }
    return E_OK;
}

int SQLiteUtils::CreateRelationalLogTable(sqlite3 *db, const std::string &oriTableName)
{
    const std::string tableName = DBConstant::RELATIONAL_PREFIX + oriTableName + "_log";
    std::string sql =
        "CREATE TABLE IF NOT EXISTS " + tableName + "(" \
        "data_key    INT NOT NULL," \
        "device      BLOB," \
        "ori_device  BLOB," \
        "timestamp   INT  NOT NULL," \
        "wtimestamp  INT  NOT NULL," \
        "flag        INT  NOT NULL," \
        "hash_key    BLOB NOT NULL,"
        "PRIMARY KEY(device,hash_key));"
        "CREATE INDEX IF NOT EXISTS " + DBConstant::RELATIONAL_PREFIX + "time_flag_index ON " + tableName +
            "(timestamp, flag);"
        "CREATE INDEX IF NOT EXISTS " + DBConstant::RELATIONAL_PREFIX + "hashkey_index ON " + tableName + "(hash_key);";

    int errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    if (errCode != E_OK) {
        LOGE("[SQLite] execute create table sql failed");
    }
    return errCode;
}

namespace {
std::string GetInsertTrigger(const TableInfo &table)
{
    std::string insertTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    insertTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_INSERT AFTER INSERT \n";
    insertTrigger += "ON " + table.GetTableName() + "\n";
    insertTrigger += "BEGIN\n";
    insertTrigger += "\t INSERT OR REPLACE INTO ";
    insertTrigger += DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    insertTrigger += " (data_key, device, ori_device, timestamp, wtimestamp, flag, hash_key)";
    insertTrigger += " VALUES (new.rowid, '', '',";
    insertTrigger += " get_sys_time(0), get_sys_time(0), 0x02,";
    insertTrigger += " calc_hash(new." + table.GetPrimaryKey() + "));\n";
    insertTrigger += "END;";
    return insertTrigger;
}

std::string GetUpdateTrigger(const TableInfo &table)
{
    std::string updateTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    updateTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_UPDATE AFTER UPDATE \n";
    updateTrigger += "ON " + table.GetTableName() + "\n";
    updateTrigger += "BEGIN\n";
    updateTrigger += "\t UPDATE " + DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    updateTrigger += " SET timestamp=get_sys_time(0), device='', flag=0x22";
    updateTrigger += " where hash_key=calc_hash(old." + table.GetPrimaryKey() + ") and flag&0x02=0x02;\n";
    updateTrigger += "END;";
    return updateTrigger;
}

std::string GetDeleteTrigger(const TableInfo &table)
{
    std::string deleteTrigger = "CREATE TRIGGER IF NOT EXISTS ";
    deleteTrigger += "naturalbase_rdb_" + table.GetTableName() + "_ON_DELETE BEFORE DELETE \n";
    deleteTrigger += "ON " + table.GetTableName() + "\n";
    deleteTrigger += "BEGIN\n";
    deleteTrigger += "\t UPDATE " + DBConstant::RELATIONAL_PREFIX + table.GetTableName() + "_log";
    deleteTrigger += " SET flag=0x03,timestamp=get_sys_time(0)";
    deleteTrigger += " where hash_key=calc_hash(old." + table.GetPrimaryKey() + ") and flag&0x02=0x02;\n";
    deleteTrigger += "END;";
    return deleteTrigger;
}
}

int SQLiteUtils::AddRelationalLogTableTrigger(sqlite3 *db, const TableInfo &table)
{
    std::vector<std::string> sqls = {GetInsertTrigger(table), GetUpdateTrigger(table), GetDeleteTrigger(table)};
    // add insert,update,delete trigger
    for (const auto &sql : sqls) {
        int errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
        if (errCode != E_OK) {
            LOGE("[SQLite] execute create log trigger sql failed");
            return errCode;
        }
    }
    return E_OK;
}

int SQLiteUtils::CreateSameStuTable(sqlite3 *db, const TableInfo &baseTbl, const std::string &newTableName)
{
    std::string sql = "CREATE TABLE IF NOT EXISTS " + newTableName + "(";
    const std::map<FieldName, FieldInfo> &fields = baseTbl.GetFields();
    for (uint32_t cid = 0; cid < fields.size(); ++cid) {
        std::string fieldName = baseTbl.GetFieldName(cid);
        sql += fieldName + " " + fields.at(fieldName).GetDataType();
        if (fields.at(fieldName).IsNotNull()) {
            sql += " NOT NULL";
        }
        if (fields.at(fieldName).HasDefaultValue()) {
            sql += " DEFAULT " + fields.at(fieldName).GetDefaultValue();
        }
        if (fieldName == baseTbl.GetPrimaryKey()) {
            sql += " PRIMARY KEY";
        }
        sql += ",";
    }
    sql.pop_back();
    sql += ");";
    int errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    if (errCode != E_OK) {
        LOGE("[SQLite] execute create table sql failed");
    }
    return errCode;
}

int SQLiteUtils::CloneIndexes(sqlite3 *db, const std::string &oriTableName, const std::string &newTableName)
{
    std::string sql =
        "SELECT 'CREATE ' || CASE WHEN il.'unique' THEN 'UNIQUE ' ELSE '' END || 'INDEX IF NOT EXISTS ' || '" +
            newTableName + "_' || il.name || ' ON ' || '" + newTableName +
            "' || '(' || GROUP_CONCAT(ii.name) || ');' "
        "FROM sqlite_master AS m,"
            "pragma_index_list(m.name) AS il,"
            "pragma_index_info(il.name) AS ii "
        "WHERE m.type='table' AND m.name='" + oriTableName + "' AND il.origin='c' "
        "GROUP BY il.name;";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        LOGE("Prepare the clone sql failed:%d", errCode);
        return errCode;
    }

    sql.clear();
    while (true) {
        errCode = SQLiteUtils::StepWithRetry(stmt, false);
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
            std::string indexSql;
            (void)GetColumnTextValue(stmt, 0, indexSql);
            sql += indexSql;
            continue;
        }
        if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
            errCode = E_OK;
        }
        (void)ResetStatement(stmt, true, errCode);
        break;
    }

    if (errCode != E_OK) {
        return errCode;
    }
    errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
    if (errCode != E_OK) {
        LOGE("[SQLite] execute create table sql failed");
    }
    return errCode;
}

int SQLiteUtils::RegisterFunction(sqlite3 *db, const std::string &funcName, int nArg, void *uData, TransactFunc &func)
{
    if (db == nullptr) {
        LOGE("Sqlite DB not exists.");
        return -E_INVALID_DB;
    }

    int errCode = sqlite3_create_function_v2(db, funcName.c_str(), nArg, SQLITE_UTF8 | SQLITE_DETERMINISTIC, uData,
        func.xFunc, func.xStep, func.xFinal, func.xDestroy);
    if (errCode != SQLITE_OK) {
        LOGE("sqlite3_create_function_v2 about [%s] returned %d", funcName.c_str(), errCode);
        return MapSQLiteErrno(errCode);
    }
    return E_OK;
}
#endif
int SQLiteUtils::RegisterFlatBufferFunction(sqlite3 *db, const std::string &inSchema)
{
    if (db == nullptr) {
        LOGE("Sqlite DB not exists.");
        return -E_INVALID_DB;
    }
    auto heapSchemaObj = new (std::nothrow) SchemaObject;
    if (heapSchemaObj == nullptr) {
        return -E_OUT_OF_MEMORY;
    }
    int errCode = heapSchemaObj->ParseFromSchemaString(inSchema);
    if (errCode != E_OK) { // Unlikely, it has been parsed before
        delete heapSchemaObj;
        heapSchemaObj = nullptr;
        return -E_INTERNAL_ERROR;
    }
    if (heapSchemaObj->GetSchemaType() != SchemaType::FLATBUFFER) { // Do not need to register FlatBufferExtract
        delete heapSchemaObj;
        heapSchemaObj = nullptr;
        return E_OK;
    }
    errCode = sqlite3_create_function_v2(db, SchemaObject::GetExtractFuncName(SchemaType::FLATBUFFER).c_str(),
        3, SQLITE_UTF8 | SQLITE_DETERMINISTIC, heapSchemaObj, &FlatBufferExtractByPath, nullptr, nullptr, // 3 args
        reinterpret_cast<void(*)(void*)>(SchemaObjectDestructor));
    // About the release of heapSchemaObj: SQLite guarantee that at following case, sqlite will invoke the destructor
    // (that is SchemaObjectDestructor) we passed to it. See sqlite.org for more information.
    // The destructor is invoked when the function is deleted, either by being overloaded or when the database
    // connection closes. The destructor is also invoked if the call to sqlite3_create_function_v2() fails
    if (errCode != SQLITE_OK) {
        LOGE("sqlite3_create_function_v2 about flatbuffer_extract_by_path return=%d.", errCode);
        // As mentioned above, SQLite had invoked the SchemaObjectDestructor to release the heapSchemaObj
        return MapSQLiteErrno(errCode);
    }
    return E_OK;
}

void SQLiteUtils::UpdateMetaDataWithinTrigger(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 2 || argv == nullptr) { // 2 : Number of parameters for sqlite register function
        LOGE("[UpdateMetaDataWithinTrigger] Invalid parameter, argc=%d.", argc);
        return;
    }
    auto *handle = static_cast<sqlite3 *>(sqlite3_user_data(ctx));
    if (handle == nullptr) {
        sqlite3_result_error(ctx, "Sqlite context is invalid.", USING_STR_LEN);
        LOGE("Sqlite context is invalid.");
        return;
    }
    auto *keyPtr = static_cast<const uint8_t *>(sqlite3_value_blob(argv[0])); // 0 : first argv for key
    int keyLen = sqlite3_value_bytes(argv[0]); // 0 : first argv for key
    if (keyPtr == nullptr || keyLen <= 0 || keyLen > static_cast<int>(DBConstant::MAX_KEY_SIZE)) {
        sqlite3_result_error(ctx, "key is invalid.", USING_STR_LEN);
        LOGE("key is invalid.");
        return;
    }
    auto val = sqlite3_value_int64(argv[1]); // 1 : second argv for value

    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(handle, UPDATE_META_SQL, stmt);
    if (errCode != E_OK) {
        sqlite3_result_error(ctx, "Get update meta_data statement failed.", USING_STR_LEN);
        LOGE("Get update meta_data statement failed. %d", errCode);
        return;
    }

    Key key(keyPtr, keyPtr + keyLen);
    errCode = SQLiteUtils::BindBlobToStatement(stmt, BIND_KEY_INDEX, key, false);
    if (errCode != E_OK) {
        sqlite3_result_error(ctx, "Bind key to statement failed.", USING_STR_LEN);
        LOGE("Bind key to statement failed. %d", errCode);
        goto END;
    }

    errCode = SQLiteUtils::BindInt64ToStatement(stmt, BIND_VAL_INDEX, val);
    if (errCode != E_OK) {
        sqlite3_result_error(ctx, "Bind value to statement failed.", USING_STR_LEN);
        LOGE("Bind value to statement failed. %d", errCode);
        goto END;
    }

    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    if (errCode != SQLiteUtils::MapSQLiteErrno(SQLITE_DONE)) {
        sqlite3_result_error(ctx, "Execute the update meta_data attach failed.", USING_STR_LEN);
        LOGE("Execute the update meta_data attach failed. %d", errCode);
    }
END:
    SQLiteUtils::ResetStatement(stmt, true, errCode);
}

int SQLiteUtils::RegisterMetaDataUpdateFunction(sqlite3 *db)
{
    int errCode = sqlite3_create_function_v2(db, DBConstant::UPDATE_META_FUNC.c_str(),
        2, // 2: argc for register function
        SQLITE_UTF8 | SQLITE_DETERMINISTIC, db, &SQLiteUtils::UpdateMetaDataWithinTrigger, nullptr, nullptr, nullptr);
    if (errCode != SQLITE_OK) {
        LOGE("sqlite3_create_function_v2 about %s returned %d", DBConstant::UPDATE_META_FUNC.c_str(), errCode);
    }
    return SQLiteUtils::MapSQLiteErrno(errCode);
}

struct ValueParseCache {
    ValueObject valueParsed;
    std::vector<uint8_t> valueOriginal;
};

namespace {
inline bool IsDeleteRecord(const uint8_t *valueBlob, int valueBlobLen)
{
    return (valueBlob == nullptr) || (valueBlobLen <= 0); // In fact, sqlite guarantee valueBlobLen not negative
}

// Use the same cache id as sqlite use for json_extract which is substituted by our json_extract_by_path
// A negative cache-id enables sharing of cache between different operation during the same statement
constexpr int VALUE_CACHE_ID = -429938;

void ValueParseCacheFree(ValueParseCache *inCache)
{
    delete inCache;
    inCache = nullptr;
}

// We don't use cache array since we only cache value column of sqlite table, see sqlite implementation for compare.
const ValueObject *ParseValueThenCacheOrGetFromCache(sqlite3_context *ctx, const uint8_t *valueBlob,
    uint32_t valueBlobLen, uint32_t offset)
{
    // Note: All parameter had already been check inside JsonExtractByPath, only called by JsonExtractByPath
    auto cached = static_cast<ValueParseCache *>(sqlite3_get_auxdata(ctx, VALUE_CACHE_ID));
    if (cached != nullptr) { // A previous cache exist
        if (cached->valueOriginal.size() == valueBlobLen) {
            if (std::memcmp(cached->valueOriginal.data(), valueBlob, valueBlobLen) == 0) {
                // Cache match
                return &(cached->valueParsed);
            }
        }
    }
    // No cache or cache mismatch
    auto newCache = new (std::nothrow) ValueParseCache;
    if (newCache == nullptr) {
        sqlite3_result_error(ctx, "[ParseValueCache] OOM.", USING_STR_LEN);
        LOGE("[ParseValueCache] OOM.");
        return nullptr;
    }
    int errCode = newCache->valueParsed.Parse(valueBlob, valueBlob + valueBlobLen, offset);
    if (errCode != E_OK) {
        sqlite3_result_error(ctx, "[ParseValueCache] Parse fail.", USING_STR_LEN);
        LOGE("[ParseValueCache] Parse fail, errCode=%d.", errCode);
        delete newCache;
        newCache = nullptr;
        return nullptr;
    }
    newCache->valueOriginal.assign(valueBlob, valueBlob + valueBlobLen);
    sqlite3_set_auxdata(ctx, VALUE_CACHE_ID, newCache, reinterpret_cast<void(*)(void*)>(ValueParseCacheFree));
    // If sqlite3_set_auxdata fail, it will immediately call ValueParseCacheFree to delete newCache;
    // Next time sqlite3_set_auxdata will call ValueParseCacheFree to delete newCache of this time;
    // At the end, newCache will be eventually deleted when call sqlite3_reset or sqlite3_finalize;
    // Since sqlite3_set_auxdata may fail, we have to call sqlite3_get_auxdata other than return newCache directly.
    auto cacheInAuxdata = static_cast<ValueParseCache *>(sqlite3_get_auxdata(ctx, VALUE_CACHE_ID));
    if (cacheInAuxdata == nullptr) {
        return nullptr;
    }
    return &(cacheInAuxdata->valueParsed);
}
}

void SQLiteUtils::JsonExtractByPath(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 3 || argv == nullptr) { // 3 parameters, which are value, path and offset
        LOGE("[JsonExtract] Invalid parameter, argc=%d.", argc);
        return;
    }
    auto valueBlob = static_cast<const uint8_t *>(sqlite3_value_blob(argv[0]));
    int valueBlobLen = sqlite3_value_bytes(argv[0]);
    if (IsDeleteRecord(valueBlob, valueBlobLen)) {
        // Currently delete records are filtered out of query and create-index sql, so not allowed here.
        sqlite3_result_error(ctx, "[JsonExtract] Delete record not allowed.", USING_STR_LEN);
        LOGE("[JsonExtract] Delete record not allowed.");
        return;
    }
    auto path = reinterpret_cast<const char *>(sqlite3_value_text(argv[1]));
    int offset = sqlite3_value_int(argv[2]); // index 2 is the third parameter
    if ((path == nullptr) || (offset < 0)) {
        sqlite3_result_error(ctx, "[JsonExtract] Path nullptr or offset invalid.", USING_STR_LEN);
        LOGE("[JsonExtract] Path nullptr or offset=%d invalid.", offset);
        return;
    }
    FieldPath outPath;
    int errCode = SchemaUtils::ParseAndCheckFieldPath(path, outPath);
    if (errCode != E_OK) {
        sqlite3_result_error(ctx, "[JsonExtract] Path illegal.", USING_STR_LEN);
        LOGE("[JsonExtract] Path=%s illegal.", path);
        return;
    }
    // Parameter Check Done Here
    const ValueObject *valueObj = ParseValueThenCacheOrGetFromCache(ctx, valueBlob, static_cast<uint32_t>(valueBlobLen),
        static_cast<uint32_t>(offset));
    if (valueObj == nullptr) {
        return; // Necessary had been printed in ParseValueThenCacheOrGetFromCache
    }
    JsonExtractInnerFunc(ctx, *valueObj, outPath);
}

namespace {
inline bool IsExtractableType(FieldType inType)
{
    return (inType != FieldType::LEAF_FIELD_NULL && inType != FieldType::LEAF_FIELD_ARRAY &&
        inType != FieldType::LEAF_FIELD_OBJECT && inType != FieldType::INTERNAL_FIELD_OBJECT);
}
}

void SQLiteUtils::JsonExtractInnerFunc(sqlite3_context *ctx, const ValueObject &inValue, const FieldPath &inPath)
{
    FieldType outType = FieldType::LEAF_FIELD_NULL; // Default type null for invalid-path(path not exist)
    int errCode = inValue.GetFieldTypeByFieldPath(inPath, outType);
    if (errCode != E_OK && errCode != -E_INVALID_PATH) {
        sqlite3_result_error(ctx, "[JsonExtract] GetFieldType fail.", USING_STR_LEN);
        LOGE("[JsonExtract] GetFieldType fail, errCode=%d.", errCode);
        return;
    }
    FieldValue outValue;
    if (IsExtractableType(outType)) {
        errCode = inValue.GetFieldValueByFieldPath(inPath, outValue);
        if (errCode != E_OK) {
            sqlite3_result_error(ctx, "[JsonExtract] GetFieldValue fail.", USING_STR_LEN);
            LOGE("[JsonExtract] GetFieldValue fail, errCode=%d.", errCode);
            return;
        }
    }
    // FieldType null, array, object do not have value, all these FieldValue will be regarded as null in JsonReturn.
    ExtractReturn(ctx, outType, outValue);
}

// NOTE!!! This function is performance sensitive !!! Carefully not to allocate memory often!!!
void SQLiteUtils::FlatBufferExtractByPath(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    if (ctx == nullptr || argc != 3 || argv == nullptr) { // 3 parameters, which are value, path and offset
        LOGE("[FlatBufferExtract] Invalid parameter, argc=%d.", argc);
        return;
    }
    auto schema = static_cast<SchemaObject *>(sqlite3_user_data(ctx));
    if (schema == nullptr || !schema->IsSchemaValid() || (schema->GetSchemaType() != SchemaType::FLATBUFFER)) {
        sqlite3_result_error(ctx, "[FlatBufferExtract] No SchemaObject or invalid.", USING_STR_LEN);
        LOGE("[FlatBufferExtract] No SchemaObject or invalid.");
        return;
    }
    // Get information from argv
    auto valueBlob = static_cast<const uint8_t *>(sqlite3_value_blob(argv[0]));
    int valueBlobLen = sqlite3_value_bytes(argv[0]);
    if (IsDeleteRecord(valueBlob, valueBlobLen)) {
        // Currently delete records are filtered out of query and create-index sql, so not allowed here.
        sqlite3_result_error(ctx, "[FlatBufferExtract] Delete record not allowed.", USING_STR_LEN);
        LOGE("[FlatBufferExtract] Delete record not allowed.");
        return;
    }
    auto path = reinterpret_cast<const char *>(sqlite3_value_text(argv[1]));
    int offset = sqlite3_value_int(argv[2]); // index 2 is the third parameter
    if ((path == nullptr) || (offset < 0) || (static_cast<uint32_t>(offset) != schema->GetSkipSize())) {
        sqlite3_result_error(ctx, "[FlatBufferExtract] Path null or offset invalid.", USING_STR_LEN);
        LOGE("[FlatBufferExtract] Path null or offset=%d(skipsize=%u) invalid.", offset, schema->GetSkipSize());
        return;
    }
    FlatBufferExtractInnerFunc(ctx, *schema, RawValue{valueBlob, valueBlobLen}, path);
}

namespace {
constexpr uint32_t FLATBUFFER_MAX_CACHE_SIZE = 102400; // 100 KBytes

void FlatBufferCacheFree(std::vector<uint8_t> *inCache)
{
    delete inCache;
    inCache = nullptr;
}
}

void SQLiteUtils::FlatBufferExtractInnerFunc(sqlite3_context *ctx, const SchemaObject &schema, const RawValue &inValue,
    RawString inPath)
{
    // All parameter had already been check inside FlatBufferExtractByPath, only called by FlatBufferExtractByPath
    if (schema.GetSkipSize() % SchemaConstant::SECURE_BYTE_ALIGN == 0) {
        TypeValue outExtract;
        int errCode = schema.ExtractValue(ValueSource::FROM_DBFILE, inPath, inValue, outExtract, nullptr);
        if (errCode != E_OK) {
            sqlite3_result_error(ctx, "[FlatBufferExtract] ExtractValue fail.", USING_STR_LEN);
            LOGE("[FlatBufferExtract] ExtractValue fail, errCode=%d.", errCode);
            return;
        }
        ExtractReturn(ctx, outExtract.first, outExtract.second);
        return;
    }
    // Not byte-align secure, we have to make a cache for copy. Check whether cache had already exist.
    auto cached = static_cast<std::vector<uint8_t> *>(sqlite3_get_auxdata(ctx, VALUE_CACHE_ID)); // Share the same id
    if (cached == nullptr) {
        // Make the cache
        auto newCache = new (std::nothrow) std::vector<uint8_t>;
        if (newCache == nullptr) {
            sqlite3_result_error(ctx, "[FlatBufferExtract] OOM.", USING_STR_LEN);
            LOGE("[FlatBufferExtract] OOM.");
            return;
        }
        newCache->resize(FLATBUFFER_MAX_CACHE_SIZE);
        sqlite3_set_auxdata(ctx, VALUE_CACHE_ID, newCache, reinterpret_cast<void(*)(void*)>(FlatBufferCacheFree));
        // If sqlite3_set_auxdata fail, it will immediately call FlatBufferCacheFree to delete newCache;
        // Next time sqlite3_set_auxdata will call FlatBufferCacheFree to delete newCache of this time;
        // At the end, newCache will be eventually deleted when call sqlite3_reset or sqlite3_finalize;
        // Since sqlite3_set_auxdata may fail, we have to call sqlite3_get_auxdata other than return newCache directly.
        // See sqlite.org for more information.
        cached = static_cast<std::vector<uint8_t> *>(sqlite3_get_auxdata(ctx, VALUE_CACHE_ID));
    }
    if (cached == nullptr) {
        LOGW("[FlatBufferExtract] Something wrong with Auxdata, but it is no matter without cache.");
    }
    TypeValue outExtract;
    int errCode = schema.ExtractValue(ValueSource::FROM_DBFILE, inPath, inValue, outExtract, cached);
    if (errCode != E_OK) {
        sqlite3_result_error(ctx, "[FlatBufferExtract] ExtractValue fail.", USING_STR_LEN);
        LOGE("[FlatBufferExtract] ExtractValue fail, errCode=%d.", errCode);
        return;
    }
    ExtractReturn(ctx, outExtract.first, outExtract.second);
}

void SQLiteUtils::ExtractReturn(sqlite3_context *ctx, FieldType type, const FieldValue &value)
{
    if (ctx == nullptr) {
        return;
    }
    switch (type) {
        case FieldType::LEAF_FIELD_BOOL:
            sqlite3_result_int(ctx, (value.boolValue ? 1 : 0));
            break;
        case FieldType::LEAF_FIELD_INTEGER:
            sqlite3_result_int(ctx, value.integerValue);
            break;
        case FieldType::LEAF_FIELD_LONG:
            sqlite3_result_int64(ctx, value.longValue);
            break;
        case FieldType::LEAF_FIELD_DOUBLE:
            sqlite3_result_double(ctx, value.doubleValue);
            break;
        case FieldType::LEAF_FIELD_STRING:
            // The SQLITE_TRANSIENT value means that the content will likely change in the near future and
            // that SQLite should make its own private copy of the content before returning.
            sqlite3_result_text(ctx, value.stringValue.c_str(), -1, SQLITE_TRANSIENT); // -1 mean use the string length
            break;
        default:
            // All other type regard as null
            sqlite3_result_null(ctx);
    }
    return;
}

void SQLiteUtils::CalcHashKey(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    // 1 means that the function only needs one parameter, namely key
    if (ctx == nullptr || argc != 1 || argv == nullptr) {
        LOGE("Parameter does not meet restrictions.");
        return;
    }
    auto keyBlob = static_cast<const uint8_t *>(sqlite3_value_blob(argv[0]));
    if (keyBlob == nullptr) {
        sqlite3_result_error(ctx, "Parameters is invalid.", USING_STR_LEN);
        LOGE("Parameters is invalid.");
        return;
    }
    int blobLen = sqlite3_value_bytes(argv[0]);
    std::vector<uint8_t> value(keyBlob, keyBlob + blobLen);
    std::vector<uint8_t> hashValue;
    int errCode = DBCommon::CalcValueHash(value, hashValue);
    if (errCode != E_OK) {
        sqlite3_result_error(ctx, "Get hash value error.", USING_STR_LEN);
        LOGE("Get hash value error.");
        return;
    }
    sqlite3_result_blob(ctx, hashValue.data(), hashValue.size(), SQLITE_TRANSIENT);
    return;
}

int SQLiteUtils::GetDbSize(const std::string &dir, const std::string &dbName, uint64_t &size)
{
    std::string dataDir = dir + "/" + dbName + DBConstant::SQLITE_DB_EXTENSION;
    uint64_t localDbSize = 0;
    int errCode = OS::CalFileSize(dataDir, localDbSize);
    if (errCode != E_OK) {
        LOGD("Failed to get the db file size, errCode:%d", errCode);
        return errCode;
    }

    std::string shmFileName = dataDir + "-shm";
    uint64_t localshmFileSize = 0;
    errCode = OS::CalFileSize(shmFileName, localshmFileSize);
    if (errCode != E_OK) {
        localshmFileSize = 0;
    }

    std::string walFileName = dataDir + "-wal";
    uint64_t localWalFileSize = 0;
    errCode = OS::CalFileSize(walFileName, localWalFileSize);
    if (errCode != E_OK) {
        localWalFileSize = 0;
    }

    // 64-bit system is Suffice. Computer storage is less than uint64_t max
    size += (localDbSize + localshmFileSize + localWalFileSize);
    return E_OK;
}

int SQLiteUtils::ExplainPlan(sqlite3 *db, const std::string &execSql, bool isQueryPlan)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }

    sqlite3_stmt *statement = nullptr;
    std::string explainSql = (isQueryPlan ? "explain query plan " : "explain ") + execSql;
    int errCode = GetStatement(db, explainSql, statement);
    if (errCode != E_OK) {
        return errCode;
    }

    bool isFirst = true;
    errCode = StepWithRetry(statement); // memory db does not support schema
    while (errCode == MapSQLiteErrno(SQLITE_ROW)) {
        int nCol = sqlite3_column_count(statement);
        nCol = std::min(nCol, 8); // Read 8 column at most

        if (isFirst) {
            LOGD("#### %s", GetColString(statement, nCol).c_str());
            isFirst = false;
        }

        std::string rowString;
        for (int i = 0; i < nCol; i++) {
            if (sqlite3_column_text(statement, i) != nullptr) {
                rowString += reinterpret_cast<const std::string::value_type *>(sqlite3_column_text(statement, i));
            }
            int blankFill = (i + 1) * 16 - rowString.size(); // each column width 16
            rowString.append(static_cast<std::string::size_type>((blankFill > 0) ? blankFill : 0), ' ');
        }
        LOGD("#### %s", rowString.c_str());
        errCode = StepWithRetry(statement);
    }
    if (errCode != MapSQLiteErrno(SQLITE_DONE)) {
        LOGE("[SqlUtil][Explain] StepWithRetry fail, errCode=%d.", errCode);
        ResetStatement(statement, true, errCode);
        return errCode;
    }
    errCode = E_OK;
    ResetStatement(statement, true, errCode);
    return errCode;
}

int SQLiteUtils::SetDataBaseProperty(sqlite3 *db, const OpenDbProperties &properties,
    const std::vector<std::string> &sqls)
{
    // Set the default busy handler to retry automatically before returning SQLITE_BUSY.
    int errCode = SetBusyTimeout(db, BUSY_TIMEOUT_MS);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = SQLiteUtils::SetKey(db, properties.cipherType, properties.passwd, properties.isMemDb);
    if (errCode != E_OK) {
        LOGD("SQLiteUtils::SetKey fail!!![%d]", errCode);
        return errCode;
    }

    for (const auto &sql : sqls) {
        errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
        if (errCode != E_OK) {
            LOGE("[SQLite] execute sql failed: %d", errCode);
            return errCode;
        }
    }
    // Create table if not exist according the sqls.
    if (properties.createIfNecessary) {
        for (const auto &sql : properties.sqls) {
            errCode = SQLiteUtils::ExecuteRawSQL(db, sql);
            if (errCode != E_OK) {
                LOGE("[SQLite] execute preset sqls failed");
                return errCode;
            }
        }
    }
    return E_OK;
}

#ifndef OMIT_ENCRYPT
int SQLiteUtils::SetCipherSettings(sqlite3 *db, CipherType type)
{
    if (db == nullptr) {
        return -E_INVALID_DB;
    }
    std::string cipherName = GetCipherName(type);
    if (cipherName.empty()) {
        return -E_INVALID_ARGS;
    }
    std::string cipherConfig = CIPHER_CONFIG_SQL + cipherName + ";";
    int errCode = SQLiteUtils::ExecuteRawSQL(db, cipherConfig);
    if (errCode != E_OK) {
        LOGE("[SQLiteUtils][SetCipherSettings] config cipher failed:%d", errCode);
        return errCode;
    }
    errCode = SQLiteUtils::ExecuteRawSQL(db, KDF_ITER_CONFIG_SQL);
    if (errCode != E_OK) {
        LOGE("[SQLiteUtils][SetCipherSettings] config iter failed:%d", errCode);
        return errCode;
    }
    return errCode;
}

std::string SQLiteUtils::GetCipherName(CipherType type)
{
    if (type == CipherType::AES_256_GCM || type == CipherType::DEFAULT) {
        return "'aes-256-gcm'";
    }
    return "";
}
#endif

int SQLiteUtils::DropTriggerByName(sqlite3 *db, const std::string &name)
{
    const std::string dropTriggerSql = "DROP TRIGGER " + name + ";";
    int errCode = SQLiteUtils::ExecuteRawSQL(db, dropTriggerSql);
    if (errCode != E_OK) {
        LOGE("Remove trigger failed. %d", errCode);
    }
    return errCode;
}

int SQLiteUtils::ExpandedSql(sqlite3_stmt *stmt, std::string &basicString)
{
    if (stmt == nullptr) {
        return -E_INVALID_ARGS;
    }
    char *eSql = sqlite3_expanded_sql(stmt);
    if (eSql == nullptr) {
        LOGE("expand statement to sql failed.");
        return -E_INVALID_DATA;
    }
    basicString = std::string(eSql);
    sqlite3_free(eSql);
    return E_OK;
}

void SQLiteUtils::ExecuteCheckPoint(sqlite3 *db)
{
    if (db == nullptr) {
        return;
    }

    int chkResult = sqlite3_wal_checkpoint_v2(db, nullptr, SQLITE_CHECKPOINT_TRUNCATE, nullptr, nullptr);
    LOGI("SQLite checkpoint result:%d", chkResult);
}

int SQLiteUtils::CheckTableEmpty(sqlite3 *db, const std::string &tableName, bool &isEmpty)
{
    if (db == nullptr) {
        return -E_INVALID_ARGS;
    }

    std::string cntSql = "SELECT min(rowid) FROM " + tableName + ";";
    sqlite3_stmt *stmt = nullptr;
    int errCode = SQLiteUtils::GetStatement(db, cntSql, stmt);
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = SQLiteUtils::StepWithRetry(stmt, false);
    if (errCode == SQLiteUtils::MapSQLiteErrno(SQLITE_ROW)) {
        if (sqlite3_column_type(stmt, 0) == SQLITE_NULL) {
            isEmpty = true;
        } else {
            isEmpty = false;
        }
        errCode = E_OK;
    }

    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return SQLiteUtils::MapSQLiteErrno(errCode);
}

int SQLiteUtils::SetPersistWalMode(sqlite3 *db)
{
    if (db == nullptr) {
        return -E_INVALID_ARGS;
    }
    int opCode = 1;
    int errCode = sqlite3_file_control(db, "main", SQLITE_FCNTL_PERSIST_WAL, &opCode);
    if (errCode != SQLITE_OK) {
        LOGE("Set persist wal mode failed. %d", errCode);
    }
    return SQLiteUtils::MapSQLiteErrno(errCode);
}

int SQLiteUtils::CheckSchemaChanged(sqlite3_stmt *stmt, const TableInfo &table, int offset)
{
    if (stmt == nullptr) {
        return  -E_INVALID_ARGS;
    }

    int columnNum = sqlite3_column_count(stmt);
    if (columnNum - offset != static_cast<int>(table.GetFields().size())) {
        LOGE("Schema field number does not match.");
        return -E_DISTRIBUTED_SCHEMA_CHANGED;
    }

    auto fields = table.GetFields();
    for (int i = offset; i < columnNum; i++) {
        const char *name = sqlite3_column_name(stmt, i);
        std::string colName = (name == nullptr) ? std::string() : name;
        const char *declType = sqlite3_column_decltype(stmt, i);
        std::string colType = (declType == nullptr) ? std::string() : declType;
        transform(colType.begin(), colType.end(), colType.begin(), ::tolower);

        auto it = fields.find(colName);
        if (it == fields.end() || it->second.GetDataType() != colType) {
            LOGE("Schema field define does not match.");
            return -E_DISTRIBUTED_SCHEMA_CHANGED;
        }
    }
    return E_OK;
}

int64_t SQLiteUtils::GetLastRowId(sqlite3 *db)
{
    if (db == nullptr) {
        return -1;
    }
    return sqlite3_last_insert_rowid(db);
}
} // namespace DistributedDB