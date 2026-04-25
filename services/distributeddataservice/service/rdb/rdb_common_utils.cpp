/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "rdb_common_utils.h"
#include "rdb_errno.h"
#include "rdb_types.h"
namespace OHOS::DistributedRdb {
using namespace OHOS::DistributedData;
using RdbStatus = OHOS::DistributedRdb::RdbStatus;
std::vector<std::string> RdbCommonUtils::GetSearchableTables(const RdbChangedData &changedData)
{
    std::vector<std::string> tables;
    for (auto &[key, value] : changedData.tableData) {
        if (value.isTrackedDataChange) {
            tables.push_back(key);
        }
    }
    return tables;
}

std::vector<std::string> RdbCommonUtils::GetP2PTables(const RdbChangedData &changedData)
{
    std::vector<std::string> tables;
    for (auto &[key, value] : changedData.tableData) {
        if (value.isP2pSyncDataChange) {
            tables.push_back(key);
        }
    }
    return tables;
}
std::vector<DistributedData::Reference> RdbCommonUtils::Convert(const std::vector<Reference> &references)
{
    std::vector<DistributedData::Reference> relationships;
    for (const auto &reference : references) {
        DistributedData::Reference relationship = { reference.sourceTable, reference.targetTable, reference.refFields };
        relationships.emplace_back(relationship);
    }
    return relationships;
}
int32_t RdbCommonUtils::ConvertNativeRdbStatus(int32_t status)
{
    switch (status) {
        case NativeRdb::E_OK:
            return GeneralError::E_OK;
        case NativeRdb::E_SQLITE_BUSY:
        case NativeRdb::E_DATABASE_BUSY:
        case NativeRdb::E_SQLITE_LOCKED:
            return GeneralError::E_BUSY;
        case NativeRdb::E_INVALID_ARGS:
        case NativeRdb::E_INVALID_ARGS_NEW:
            return GeneralError::E_INVALID_ARGS;
        case NativeRdb::E_ALREADY_CLOSED:
            return GeneralError::E_ALREADY_CLOSED;
        case NativeRdb::E_SQLITE_CORRUPT:
            return GeneralError::E_DB_CORRUPT;
        case NativeRdb::E_ROW_OUT_RANGE:
            return GeneralError::E_MOVE_DONE;
        case NativeRdb::E_SQLITE_CONSTRAINT:
            return GeneralError::E_CONSTRAIN_VIOLATION;
        case NativeRdb::E_SQLITE_FULL:
            return GeneralError::E_DB_IS_FULL;
        case NativeRdb::E_SQLITE_IOERR_FULL:
            return GeneralError::E_DB_IS_FULL;
        default:
            break;
    }
    return GeneralError::E_ERROR;
}

int32_t RdbCommonUtils::ConvertGeneralRdbStatus(int32_t status)
{
    switch (status) {
        case GeneralError::E_OK:
            return RdbStatus::RDB_OK;
        case GeneralError::E_BUSY:
            return RdbStatus::RDB_SQLITE_BUSY;
        case GeneralError::E_INVALID_ARGS:
            return RdbStatus::RDB_INVALID_ARGS;
        case GeneralError::E_DB_CORRUPT:
            return RdbStatus::RDB_SQLITE_CORRUPT;
        case GeneralError::E_DB_ERROR:
        case GeneralError::E_TABLE_NOT_FOUND:
            return RdbStatus::RDB_SQLITE_ERROR;
        case GeneralError::E_NOT_SUPPORT:
            return RdbStatus::RDB_NOT_SUPPORT;
        default:
            break;
    }
    return RdbStatus::RDB_ERROR;
}

constexpr ErrorInfo LOCAL_DEVICE_ERROR_INFO_ARRAY[] = {
    { DBStatus::NOT_SUPPORT, SyncResultCode::INVALID_ARGS, "Local Device Not Support." },
    { DBStatus::INVALID_ARGS, SyncResultCode::INVALID_ARGS, "Local Device Invalid Args." },
    { DBStatus::DISTRIBUTED_SCHEMA_NOT_FOUND, SyncResultCode::DISTRIBUTED_TABLE_NOT_SET,
        "Local Device Distributed Schema Not Found." },
    { DBStatus::DISTRIBUTED_SCHEMA_CHANGED, SyncResultCode::SCHEMA_CHANGED,
        "Local Device Distributed Schema Changed." },
    { DBStatus::INVALID_QUERY_FORMAT, SyncResultCode::INVALID_ARGS, "Local Device Invalid Query Format." },
    { DBStatus::BUSY, SyncResultCode::BUSY, "Local Database Is Busy." },
    { DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB, SyncResultCode::CORRUPTED,
        "Local Device Invalid Password Or Corrupted Db." },
    { DBStatus::INVALID_QUERY_FIELD, SyncResultCode::INVALID_ARGS, "Invalid Query Field." },
};

constexpr ErrorInfo REMOTE_DEVICE_ERROR_INFO_ARRAY[] = {
    { DBStatus::TABLE_FIELD_MISMATCH, SyncResultCode::TABLE_FIELD_MISMATCH, "Table Field Mismatch." },
    { DBStatus::DISTRIBUTED_SCHEMA_MISMATCH, SyncResultCode::DISTRIBUTED_SCHEMA_MISMATCH,
        "Distributed Schema Mismatch." },
    { DBStatus::SECURITY_OPTION_CHECK_ERROR, SyncResultCode::FAIL, "Security Option Check Error." },
    { DBStatus::BUSY, SyncResultCode::BUSY, "Database Is Busy." },
    { DBStatus::PERMISSION_CHECK_FORBID_SYNC, SyncResultCode::FAIL, "Permission Check Error." },
    { DBStatus::TIME_OUT, SyncResultCode::TIMEOUT, "Timeout." },
    { DBStatus::INVALID_QUERY_FORMAT, SyncResultCode::INVALID_ARGS, "Invalid Query Format." },
    { DBStatus::INVALID_QUERY_FIELD, SyncResultCode::INVALID_ARGS, "Invalid Query Field." },
    { DBStatus::DISTRIBUTED_SCHEMA_CHANGED, SyncResultCode::SCHEMA_CHANGED,
        "Distributed Schema Changed." },
    { DBStatus::NO_PERMISSION, SyncResultCode::FAIL, "No Permission." },
    { DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB, SyncResultCode::CORRUPTED,
        "Invalid Password Or Corrupted Db." },
    { DBStatus::NEED_CORRECT_TARGET_USER, SyncResultCode::FAIL, "Need Correct Target User." },
    { DBStatus::CONSTRAINT, SyncResultCode::CONSTRAINT_VIOLATION, "Violation Of Constraints." },
    { DBStatus::COMM_FAILURE, SyncResultCode::FAIL, "Communication Failure." },
    { DBStatus::NOT_SUPPORT, SyncResultCode::INVALID_ARGS, "Invalid Parameter." },
    { DBStatus::DISTRIBUTED_SCHEMA_NOT_FOUND, SyncResultCode::DISTRIBUTED_TABLE_NOT_SET,
        "Local Device Distributed Schema Not Found." },
};

constexpr size_t LOCAL_DEVICE_ERROR_INFO_COUNT =
    sizeof(LOCAL_DEVICE_ERROR_INFO_ARRAY) / sizeof(LOCAL_DEVICE_ERROR_INFO_ARRAY[0]);

ErrorInfo RdbCommonUtils::GetInterfaceErrorString(DistributedDB::DBStatus status)
{
    for (size_t i = 0; i < LOCAL_DEVICE_ERROR_INFO_COUNT; i++) {
        if (LOCAL_DEVICE_ERROR_INFO_ARRAY[i].dbStatus == status) {
            return LOCAL_DEVICE_ERROR_INFO_ARRAY[i];
        }
    }
    return { status, SyncResultCode::FAIL, "Sync is failed in Interface" };
}

constexpr size_t REMOTE_DEVICE_ERROR_INFO_COUNT =
    sizeof(REMOTE_DEVICE_ERROR_INFO_ARRAY) / sizeof(REMOTE_DEVICE_ERROR_INFO_ARRAY[0]);

ErrorInfo RdbCommonUtils::GetCallbackErrorString(DistributedDB::DBStatus status)
{
    for (size_t i = 0; i < REMOTE_DEVICE_ERROR_INFO_COUNT; i++) {
        if (REMOTE_DEVICE_ERROR_INFO_ARRAY[i].dbStatus == status) {
            return REMOTE_DEVICE_ERROR_INFO_ARRAY[i];
        }
    }
    return { status, SyncResultCode::FAIL, "Sync is failed in Callback" };
}
} // namespace OHOS::DistributedRdb