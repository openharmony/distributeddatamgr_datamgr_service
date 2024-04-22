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
#include "rdb_query.h"
#include "relational_store_manager.h"
#include "value_proxy.h"
#include "utils/anonymous.h"

namespace OHOS::DistributedRdb {
using namespace DistributedDB;
using namespace DistributedData;
RdbCloud::RdbCloud(std::shared_ptr<DistributedData::CloudDB> cloudDB, BindAssets* bindAssets)
    : cloudDB_(std::move(cloudDB)), snapshots_(bindAssets)
{
}

DBStatus RdbCloud::BatchInsert(
    const std::string &tableName, std::vector<DBVBucket> &&record, std::vector<DBVBucket> &extend)
{
    extend.resize(record.size());
    VBuckets extends = ValueProxy::Convert(std::move(extend));
    VBuckets records = ValueProxy::Convert(std::move(record));
    std::set<std::string> skipAssets;
    PostEvent(records, skipAssets, extends, DistributedData::AssetEvent::UPLOAD);
    VBuckets temp = records;
    auto error = cloudDB_->BatchInsert(tableName, std::move(records), extends);
    PostEvent(temp, skipAssets, extends, DistributedData::AssetEvent::UPLOAD_FINISHED);
    extend = ValueProxy::Convert(std::move(extends));
    return ConvertStatus(static_cast<GeneralError>(error));
}

DBStatus RdbCloud::BatchUpdate(
    const std::string &tableName, std::vector<DBVBucket> &&record, std::vector<DBVBucket> &extend)
{
    extend.resize(record.size());
    VBuckets extends = ValueProxy::Convert(std::move(extend));
    VBuckets records = ValueProxy::Convert(std::move(record));
    std::set<std::string> skipAssets;
    PostEvent(records, skipAssets, extends, DistributedData::AssetEvent::UPLOAD);
    VBuckets temp = records;
    auto error = cloudDB_->BatchUpdate(tableName, std::move(records), extends);
    PostEvent(temp, skipAssets, extends, DistributedData::AssetEvent::UPLOAD_FINISHED);
    extend = ValueProxy::Convert(std::move(extends));
    return ConvertStatus(static_cast<GeneralError>(error));
}

DBStatus RdbCloud::BatchDelete(const std::string &tableName, std::vector<DBVBucket> &extend)
{
    VBuckets extends = ValueProxy::Convert(std::move(extend));
    auto error = cloudDB_->BatchDelete(tableName, extends);
    extend = ValueProxy::Convert(std::move(extends));
    return ConvertStatus(static_cast<GeneralError>(error));
}

DBStatus RdbCloud::Query(const std::string &tableName, DBVBucket &extend, std::vector<DBVBucket> &data)
{
    auto [nodes, status] = ConvertQuery(extend);
    std::shared_ptr<Cursor> cursor = nullptr;
    if (status == GeneralError::E_OK && !nodes.empty()) {
        RdbQuery query;
        query.SetQueryNodes(tableName, std::move(nodes));
        cursor = cloudDB_->Query(query, ValueProxy::Convert(std::move(extend)));
    } else {
        cursor = cloudDB_->Query(tableName, ValueProxy::Convert(std::move(extend)));
    }
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

DistributedData::GeneralError RdbCloud::PreSharing(const std::string& tableName, VBuckets& extend)
{
    return static_cast<GeneralError>(cloudDB_->PreSharing(tableName, extend));
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

std::pair<DBStatus, std::string> RdbCloud::GetEmptyCursor(const std::string &tableName)
{
    auto [error, cursor] = cloudDB_->GetEmptyCursor(tableName);
    return { ConvertStatus(static_cast<GeneralError>(error)), cursor };
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
        case GeneralError::E_RECORD_EXIST_CONFLICT:
            return DBStatus::CLOUD_RECORD_EXIST_CONFLICT;
        default:
            ZLOGI("error:0x%{public}x", error);
            break;
    }
    return DBStatus::CLOUD_ERROR;
}

std::pair<RdbCloud::QueryNodes, DistributedData::GeneralError> RdbCloud::ConvertQuery(RdbCloud::DBVBucket& extend)
{
    auto it = extend.find(TYPE_FIELD);
    if (it == extend.end() || std::get<int64_t>(it->second) != static_cast<int64_t>(CloudQueryType::QUERY_FIELD)) {
        return { {}, GeneralError::E_ERROR };
    }
    it = extend.find(QUERY_FIELD);
    if (it == extend.end()) {
        ZLOGE("error, no QUERY_FIELD!");
        return { {}, GeneralError::E_ERROR };
    }

    auto bytes = std::get_if<DistributedDB::Bytes>(&it->second);
    std::vector<DistributedDB::QueryNode> nodes;
    DBStatus status = DB_ERROR;
    if (bytes != nullptr) {
        nodes = DistributedDB::RelationalStoreManager::ParserQueryNodes(*bytes, status);
    }
    if (status != OK) {
        ZLOGE("error, ParserQueryNodes failed, status:%{public}d", status);
        return { {}, GeneralError::E_ERROR };
    }
    return { ConvertQuery(std::move(nodes)), GeneralError::E_OK };
}

RdbCloud::QueryNodes RdbCloud::ConvertQuery(RdbCloud::DBQueryNodes&& nodes)
{
    QueryNodes queryNodes;
    queryNodes.reserve(nodes.size());
    for (auto& node : nodes) {
        QueryNode queryNode;
        queryNode.fieldName = std::move(node.fieldName);
        queryNode.fieldValue = ValueProxy::Convert(std::move(node.fieldValue));
        switch (node.type) {
            case QueryNodeType::IN:
                queryNode.op = QueryOperation::IN;
                break;
            case QueryNodeType::OR:
                queryNode.op = QueryOperation::OR;
                break;
            case QueryNodeType::AND:
                queryNode.op = QueryOperation::AND;
                break;
            case QueryNodeType::EQUAL_TO:
                queryNode.op = QueryOperation::EQUAL_TO;
                break;
            case QueryNodeType::BEGIN_GROUP:
                queryNode.op = QueryOperation::BEGIN_GROUP;
                break;
            case QueryNodeType::END_GROUP:
                queryNode.op = QueryOperation::END_GROUP;
                break;
            default:
                ZLOGE("invalid operation:0x%{public}d", node.type);
                return {};
        }
        queryNodes.emplace_back(std::move(queryNode));
    }
    return queryNodes;
}

void RdbCloud::PostEvent(VBuckets& records, std::set<std::string>& skipAssets, VBuckets& extend,
    DistributedData::AssetEvent eventId)
{
    int32_t index = 0;
    for (auto& record : records) {
        DataBucket& ext = extend[index++];
        for (auto& [key, value] : record) {
            PostEvent(value, ext, skipAssets, eventId);
        }
    }
}

void RdbCloud::PostEvent(DistributedData::Value& value, DataBucket& extend, std::set<std::string>& skipAssets,
    DistributedData::AssetEvent eventId)
{
    if (value.index() != TYPE_INDEX<DistributedData::Asset> && value.index() != TYPE_INDEX<DistributedData::Assets>) {
        return;
    }

    if (value.index() == TYPE_INDEX<DistributedData::Asset>) {
        auto* asset = Traits::get_if<DistributedData::Asset>(&value);
        PostEventAsset(*asset, extend, skipAssets, eventId);
    }

    if (value.index() == TYPE_INDEX<DistributedData::Assets>) {
        auto* assets = Traits::get_if<DistributedData::Assets>(&value);
        for (auto& asset : *assets) {
            PostEventAsset(asset, extend, skipAssets, eventId);
        }
    }
}

void RdbCloud::PostEventAsset(DistributedData::Asset& asset, DataBucket& extend, std::set<std::string>& skipAssets,
    DistributedData::AssetEvent eventId)
{
    if (snapshots_->bindAssets == nullptr) {
        return;
    }
    auto it = snapshots_->bindAssets->find(asset.uri);
    if (it == snapshots_->bindAssets->end() || it->second == nullptr) {
        return;
    }

    if (eventId == DistributedData::UPLOAD) {
        it->second->Upload(asset);
        if (it->second->GetAssetStatus(asset) == TransferStatus::STATUS_WAIT_UPLOAD) {
            skipAssets.insert(asset.uri);
            extend[SchemaMeta::ERROR_FIELD] = DBStatus::CLOUD_RECORD_EXIST_CONFLICT;
        }
    }

    if (eventId == DistributedData::UPLOAD_FINISHED) {
        auto skip = skipAssets.find(asset.uri);
        if (skip != skipAssets.end()) {
            return;
        }
        it->second->Uploaded(asset);
    }
}
} // namespace OHOS::DistributedRdb
