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

#define LOG_TAG "ExtensionCloudDbImpl"
#include "cloud_db_impl.h"
#include "error/general_error.h"
#include "extension_util.h"
#include "cloud_cursor_impl.h"
#include "cloud_ext_types.h"
#include "basic_rust_types.h"
#include "log_print.h"
namespace OHOS::CloudData {
CloudDbImpl::CloudDbImpl(OhCloudExtCloudDatabase *database) : database_(database)
{
}

CloudDbImpl::~CloudDbImpl()
{
    if (database_ != nullptr) {
        OhCloudExtCloudDbFree(database_);
        database_ = nullptr;
    }
}

int32_t CloudDbImpl::Execute(const std::string &table, const std::string &sql, const DBVBucket &extend)
{
    OhCloudExtSql sqlInfo {
        .table = reinterpret_cast<const unsigned char *>(table.c_str()),
        .tableLen = table.size(),
        .sql = reinterpret_cast<const unsigned char *>(sql.c_str()),
        .sqlLen = sql.size()
    };
    auto data = ExtensionUtil::Convert(extend);
    if (data.first == nullptr) {
        return DBErr::E_ERROR;
    }
    auto status = OhCloudExtCloudDbExecuteSql(database_, &sqlInfo, data.first);
    OhCloudExtHashMapFree(data.first);
    return ExtensionUtil::ConvertStatus(status);
}

int32_t CloudDbImpl::BatchInsert(const std::string &table, DBVBuckets &&values, DBVBuckets &extends)
{
    DBVBuckets inValues = values;
    auto valIn = ExtensionUtil::Convert(std::move(inValues));
    auto exdIn = ExtensionUtil::Convert(std::move(extends));
    if (valIn.first == nullptr || exdIn.first == nullptr) {
        return DBErr::E_ERROR;
    }
    auto status = OhCloudExtCloudDbBatchInsert(database_, reinterpret_cast<const unsigned char *>(table.c_str()),
        table.size(), valIn.first, exdIn.first);
    if (status == ERRNO_SUCCESS) {
        extends = ExtensionUtil::ConvertBuckets(exdIn.first);
    }
    OhCloudExtVectorFree(valIn.first);
    OhCloudExtVectorFree(exdIn.first);
    return ExtensionUtil::ConvertStatus(status);
}

int32_t CloudDbImpl::BatchUpdate(const std::string &table, DBVBuckets &&values, DBVBuckets &extends)
{
    auto exdIn = ExtensionUtil::Convert(std::move(extends));
    auto valIn = ExtensionUtil::Convert(std::move(values));
    if (valIn.first == nullptr || exdIn.first == nullptr) {
        return DBErr::E_ERROR;
    }
    auto status = OhCloudExtCloudDbBatchUpdate(database_, reinterpret_cast<const unsigned char *>(table.c_str()),
        table.size(), valIn.first, exdIn.first);
    if (status == ERRNO_SUCCESS && exdIn.first != nullptr) {
        extends = ExtensionUtil::ConvertBuckets(exdIn.first);
    }
    OhCloudExtVectorFree(exdIn.first);
    OhCloudExtVectorFree(valIn.first);
    return ExtensionUtil::ConvertStatus(status);
}

int32_t CloudDbImpl::BatchUpdate(const std::string &table, DBVBuckets &&values, const DBVBuckets &extends)
{
    auto exdIn = ExtensionUtil::Convert(std::move(const_cast<DBVBuckets &>(extends)));
    auto valIn = ExtensionUtil::Convert(std::move(values));
    if (valIn.first == nullptr || exdIn.first == nullptr) {
        return DBErr::E_ERROR;
    }
    auto status = OhCloudExtCloudDbBatchUpdate(database_, reinterpret_cast<const unsigned char *>(table.c_str()),
        table.size(), valIn.first, exdIn.first);
    OhCloudExtVectorFree(exdIn.first);
    OhCloudExtVectorFree(valIn.first);
    return ExtensionUtil::ConvertStatus(status);
}

int32_t CloudDbImpl::BatchDelete(const std::string &table, const DBVBuckets &extends)
{
    auto data = ExtensionUtil::Convert(std::move(const_cast<DBVBuckets &>(extends)));
    if (data.first == nullptr) {
        return DBErr::E_ERROR;
    }
    auto status = OhCloudExtCloudDbBatchDelete(database_, reinterpret_cast<const unsigned char *>(table.c_str()),
        table.size(), data.first);
    OhCloudExtVectorFree(data.first);
    return ExtensionUtil::ConvertStatus(status);
}

std::shared_ptr<DBCursor> CloudDbImpl::Query(const std::string &table, const DBVBucket &extend)
{
    std::string cursor;
    auto it = extend.find(DistributedData::SchemaMeta::CURSOR_FIELD);
    if (it != extend.end()) {
        cursor = std::get<std::string>(it->second);
    }
    OhCloudExtQueryInfo info {
        .table = reinterpret_cast<const unsigned char *>(table.c_str()),
        .tableLen = table.size(),
        .cursor = reinterpret_cast<const unsigned char *>(cursor.c_str()),
        .cursorLen = cursor.size()
    };
    OhCloudExtCloudDbData *cloudData = nullptr;
    auto status = OhCloudExtCloudDbBatchQuery(database_, &info, &cloudData);
    return (status == ERRNO_SUCCESS && cloudData != nullptr) ? std::make_shared<CloudCursorImpl>(cloudData) : nullptr;
}

int32_t CloudDbImpl::Lock()
{
    int expire = 0;
    auto status = OhCloudExtCloudDbLock(database_, &expire);
    interval_ = expire;
    return ExtensionUtil::ConvertStatus(status);
}

int32_t CloudDbImpl::Heartbeat()
{
    return ExtensionUtil::ConvertStatus(OhCloudExtCloudDbHeartbeat(database_));
}

int32_t CloudDbImpl::Unlock()
{
    auto status = OhCloudExtCloudDbUnlock(database_);
    return ExtensionUtil::ConvertStatus(status);
}

int64_t CloudDbImpl::AliveTime()
{
    return interval_;
}

int32_t CloudDbImpl::Close()
{
    if (database_ != nullptr) {
        OhCloudExtCloudDbFree(database_);
        database_ = nullptr;
    }
    return DBErr::E_OK;
}
} // namespace OHOS::CloudData