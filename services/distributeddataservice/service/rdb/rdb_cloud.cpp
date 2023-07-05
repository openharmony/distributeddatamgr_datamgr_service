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

#define LOG_TAG "RdbCloud"
#include "rdb_cloud.h"

#include "cloud/schema_meta.h"
#include "log_print.h"
#include "value_proxy.h"
#include "utils/anonymous.h"

namespace OHOS::DistributedRdb {
using namespace DistributedDB;
using namespace DistributedData;
RdbCloud::RdbCloud(std::shared_ptr<DistributedData::CloudDB> cloudDB)
    : cloudDB_(std::move(cloudDB))
{
}

DBStatus RdbCloud::BatchInsert(
    const std::string &tableName, std::vector<DBVBucket> &&record, std::vector<DBVBucket> &extend)
{
    DistributedData::VBuckets extends;
    auto error = cloudDB_->BatchInsert(tableName, ValueProxy::Convert(std::move(record)), extends);
    if (error == GeneralError::E_OK) {
        extend = ValueProxy::Convert(std::move(extends));
    }
    return ConvertStatus(static_cast<GeneralError>(error));
}

DBStatus RdbCloud::BatchUpdate(
    const std::string &tableName, std::vector<DBVBucket> &&record, std::vector<DBVBucket> &extend)
{
    DistributedData::VBuckets extends = ValueProxy::Convert(std::move(extend));
    auto error = cloudDB_->BatchUpdate(tableName, ValueProxy::Convert(std::move(record)), extends);
    if (error == GeneralError::E_OK) {
        extend = ValueProxy::Convert(std::move(extends));
    }
    return ConvertStatus(static_cast<GeneralError>(error));
}

DBStatus RdbCloud::BatchDelete(const std::string &tableName, std::vector<DBVBucket> &extend)
{
    auto error = cloudDB_->BatchDelete(tableName, ValueProxy::Convert(std::move(extend)));
    return ConvertStatus(static_cast<GeneralError>(error));
}

DBStatus RdbCloud::Query(const std::string &tableName, DBVBucket &extend, std::vector<DBVBucket> &data)
{
    auto cursor = cloudDB_->Query(tableName, ValueProxy::Convert(std::move(extend)));
    if (cursor == nullptr) {
        ZLOGE("cursor is null, table:%{public}s, extend:%{public}zu",
            Anonymous::Change(tableName).c_str(), extend.size());
        return ConvertStatus(static_cast<GeneralError>(E_ERROR));
    }
    int32_t count = cursor->GetCount();
    data.reserve(count);
    auto err = cursor->MoveToFirst();
    while (err == E_OK && count > 0) {
        DistributedData::VBucket entry;
        err = cursor->GetEntry(entry);
        if (err != E_OK) {
            break;
        }
        data.emplace_back(ValueProxy::Convert(std::move(entry)));
        err = cursor->MoveToNext();
        count--;
    }
    DistributedData::Value cursorFlag;
    cursor->Get(SchemaMeta::CURSOR_FIELD, cursorFlag);
    extend[SchemaMeta::CURSOR_FIELD] = ValueProxy::Convert(std::move(cursorFlag));
    if (cursor->IsEnd()) {
        ZLOGD("query end, table:%{public}s", Anonymous::Change(tableName).c_str());
        return DBStatus::QUERY_END;
    }
    return ConvertStatus(static_cast<GeneralError>(err));
}

std::pair<DBStatus, uint32_t> RdbCloud::Lock()
{
    auto error = cloudDB_->Lock();
    return std::make_pair(  // int64_t <-> uint32_t, s <-> ms
        ConvertStatus(static_cast<GeneralError>(error)), cloudDB_->AliveTime() * TO_MS);
}

DBStatus RdbCloud::UnLock()
{
    auto error = cloudDB_->Unlock();
    return ConvertStatus(static_cast<GeneralError>(error));
}

DBStatus RdbCloud::HeartBeat()
{
    auto error = cloudDB_->Heartbeat();
    return ConvertStatus(static_cast<GeneralError>(error));
}

DBStatus RdbCloud::Close()
{
    auto error = cloudDB_->Close();
    return ConvertStatus(static_cast<GeneralError>(error));
}

DBStatus RdbCloud::ConvertStatus(DistributedData::GeneralError error)
{
    switch (error) {
        case GeneralError::E_OK:
            return DBStatus::OK;
        case GeneralError::E_BUSY:
            return DBStatus::BUSY;
        case GeneralError::E_INVALID_ARGS:
            return DBStatus::INVALID_ARGS;
        case GeneralError::E_NOT_SUPPORT:
            return DBStatus::NOT_SUPPORT;
        case GeneralError::E_ERROR: // fallthrough
        case GeneralError::E_NOT_INIT:
        case GeneralError::E_ALREADY_CONSUMED:
        case GeneralError::E_ALREADY_CLOSED:
            return DBStatus::CLOUD_ERROR;
        default:
            ZLOGE("unknown error:0x%{public}x", error);
            break;
    }
    return DBStatus::CLOUD_ERROR;
}
} // namespace OHOS::DistributedRdb
