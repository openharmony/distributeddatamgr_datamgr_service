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
RdbCloud::RdbCloud(std::shared_ptr<DistributedData::CloudDB> cloudDBm, const std::string &bundleName)
    : cloudDB_(std::move(cloudDB)), prefix_(PREFIX + bundleName + SUFFIX)
{
}

DBStatus RdbCloud::BatchInsert(
    const std::string &tableName, std::vector<DBVBucket> &&record, std::vector<DBVBucket> &extend)
{
    DistributedData::VBuckets extends = ValueProxy::Convert(std::move(extend));
    DistributedData::VBuckets records = ValueProxy::Convert(std::move(record));
    RemoveDeletedAsset(records);
    auto error = cloudDB_->BatchInsert(tableName, std::move(records), extends);
    extend = ValueProxy::Convert(std::move(extends));
    return ConvertStatus(static_cast<GeneralError>(error));
}

DBStatus RdbCloud::BatchUpdate(
    const std::string &tableName, std::vector<DBVBucket> &&record, std::vector<DBVBucket> &extend)
{
    DistributedData::VBuckets extends = ValueProxy::Convert(std::move(extend));
    auto error = cloudDB_->BatchUpdate(tableName, ValueProxy::Convert(std::move(record)), extends);
    extend = ValueProxy::Convert(std::move(extends));
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
        AddUri(entry);
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
        case GeneralError::E_NETWORK_ERROR:
            return DBStatus::CLOUD_NETWORK_ERROR;
        case GeneralError::E_LOCKED_BY_OTHERS:
            return DBStatus::CLOUD_LOCK_ERROR;
        case GeneralError::E_RECODE_LIMIT_EXCEEDED:
            return DBStatus::CLOUD_FULL_RECORDS;
        case GeneralError::E_NO_SPACE_FOR_ASSET:
            return DBStatus::CLOUD_ASSET_SPACE_INSUFFICIENT;
        default:
            ZLOGI("error:0x%{public}x", error);
            break;
    }
    return DBStatus::CLOUD_ERROR;
}

void RdbCloud::RemoveDeletedAsset(DistributedData::VBuckets &buckets)
{
    for (auto &bucket : buckets) {
        for (auto &[key, value] : bucket) {
            if (value.index() == TYPE_INDEX<DistributedData::Asset>) {
                DistributedData::Asset asset = std::get<DistributedData::Asset>(value);
                if (asset.status == DistributedData::Asset::Status::STATUS_DELETE) {
                    value = Value();
                }
            } else if (value.index() == TYPE_INDEX<DistributedData::Assets>) {
                DistributedData::Assets assets = std::get<DistributedData::Assets>(value);
                DistributedData::Assets result;
                for (auto &asset : assets) {
                    if (asset.status != DistributedData::Asset::Status::STATUS_DELETE) {
                        result.emplace_back(asset);
                    }
                }
                value = std::move(result);
            }
        }
    }
}

void RdbCloud::AddUri(DistributedData::VBucket &bucket)
{
    for (auto &[key, value] : bucket) {
        if (value.index() == TYPE_INDEX<DistributedData::Asset>) {
            DistributedData::Asset asset = std::get<DistributedData::Asset>(value);
            asset.uri = prefix_ + asset.path;
            value = std::move(asset);
        } else if (value.index() == TYPE_INDEX<DistributedData::Assets>) {
            DistributedData::Assets assets = std::get<DistributedData::Assets>(value);
            for (auto &asset : assets) {
                asset.uri = prefix_ + asset.path;
            }
            value = std::move(assets);
        }
    }
}
} // namespace OHOS::DistributedRdb
