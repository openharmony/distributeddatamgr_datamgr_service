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
#define LOG_TAG "KvAdaptor"
#include "kv_delegate.h"
#include "datashare_errno.h"
#include "directory_manager.h"
#include "grd_document/grd_document_api.h"
#include "grd_base/grd_error.h"
#include "ipc_skeleton.h"
#include "log_print.h"

namespace OHOS::DataShare {
int64_t KvDelegate::Upsert(const std::string &collectionName, const std::string &filter, const std::string &value)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (!Init()) {
        ZLOGE("init failed, %{public}s", collectionName.c_str());
        return E_ERROR;
    }
    int count = GRD_UpsertDoc(db_, collectionName.c_str(), filter.c_str(), value.c_str(), 0);
    if (count <= 0) {
        ZLOGE("GRD_UpSertDoc failed，status %{public}d", count);
        return count;
    }
    Flush();
    return E_OK;
}

int64_t KvDelegate::Delete(const std::string &collectionName, const std::string &filter)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (!Init()) {
        ZLOGE("init failed, %{public}s", collectionName.c_str());
        return E_ERROR;
    }
    int count = GRD_DeleteDoc(db_, collectionName.c_str(), filter.c_str(), 0);
    if (count <= 0) {
        ZLOGE("GRD_UpSertDoc failed，status %{public}d", count);
        return count;
    }
    Flush();
    return E_OK;
}

bool KvDelegate::Init()
{
    if (isInitDone_) {
        return true;
    }
    int status = GRD_DBOpen((path_ + "/dataShare.db").c_str(), nullptr, GRD_DB_OPEN_CREATE, &db_);
    ZLOGE("GRD_DBOpen hanluhanlu，path111 %{public}s", path_.c_str());
    if (status != GRD_OK || db_ == nullptr) {
        ZLOGE("GRD_DBOpen failed，status %{public}d", status);
        return false;
    }

    status = GRD_CreateCollection(db_, TEMPLATE_TABLE.c_str(), nullptr, 0);
    if (status != GRD_OK) {
        ZLOGE("GRD_CreateCollection template table failed，status %{public}d", status);
        return false;
    }

    status = GRD_CreateCollection(db_, DATA_TABLE.c_str(), nullptr, 0);
    if (status != GRD_OK) {
        ZLOGE("GRD_CreateCollection data table failed，status %{public}d", status);
        return false;
    }
    isInitDone_ = true;
    return true;
}

KvDelegate::~KvDelegate()
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (isInitDone_) {
        int status = GRD_DBClose(db_, 0);
        if (status != GRD_OK) {
            ZLOGE("GRD_DBClose failed，status %{public}d", status);
        }
    }
}

int32_t KvDelegate::Upsert(const std::string &collectionName, const KvData &value)
{
    std::string id = DistributedData::Serializable::Marshall(*value.GetId());
    if (value.HasVersion() && value.GetVersion() != 0) {
        int version = -1;
        if (GetVersion(collectionName, id, version)) {
            if (value.GetVersion() <= version) {
                ZLOGE("GetVersion failed，%{public}s id %{private}s %{public}d %{public}d", collectionName.c_str(),
                      id.c_str(), static_cast<int>(value.GetVersion()), version);
                return E_VERSION_NOT_NEWER;
            }
        }
    }
    return Upsert(collectionName, id, DistributedData::Serializable::Marshall(value.GetValue()));
}

int32_t KvDelegate::DeleteById(const std::string &collectionName, const Id &id)
{
    return Delete(collectionName, DistributedData::Serializable::Marshall(id));
}

int32_t KvDelegate::Get(const std::string &collectionName, const Id &id, std::string &value)
{
    std::string filter = DistributedData::Serializable::Marshall(id);
    if (Get(collectionName, filter, "{}", value) != E_OK) {
        ZLOGE("Get failed, %{public}s %{public}s", collectionName.c_str(), filter.c_str());
        return false;
    }
    return true;
}

bool KvDelegate::GetVersion(const std::string &collectionName, const std::string &filter, int &version)
{
    std::string value;
    if (Get(collectionName, filter, "{}", value) != E_OK) {
        ZLOGE("Get failed, %{public}s %{public}s", collectionName.c_str(), filter.c_str());
        return false;
    }
    VersionData data(-1);
    if (!DistributedData::Serializable::Unmarshall(value, data)) {
        ZLOGE("Unmarshall failed，data %{public}s", value.c_str());
        return false;
    }
    version = data;
    return true;
}

int32_t KvDelegate::Get(
    const std::string &collectionName, const std::string &filter, const std::string &projection, std::string &result)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (!Init()) {
        ZLOGE("init failed, %{public}s", collectionName.c_str());
        return E_ERROR;
    }
    Query query;
    query.filter = filter.c_str();
    query.projection = projection.c_str();
    GRD_ResultSet *resultSet = nullptr;
    int status = GRD_FindDoc(db_, collectionName.c_str(), query, 0, &resultSet);
    if (status != GRD_OK || resultSet == nullptr) {
        ZLOGE("GRD_FindDoc failed，status %{public}d", status);
        return status;
    }
    status = GRD_Next(resultSet);
    if (status != GRD_OK) {
        GRD_FreeResultSet(resultSet);
        ZLOGE("GRD_Next failed，status %{public}d", status);
        return status;
    }
    char *value = nullptr;
    status = GRD_GetValue(resultSet, &value);
    if (status != GRD_OK || value == nullptr) {
        GRD_FreeResultSet(resultSet);
        ZLOGE("GRD_GetValue failed，status %{public}d", status);
        return status;
    }
    result = value;
    GRD_FreeValue(value);
    GRD_FreeResultSet(resultSet);
    return E_OK;
}

void KvDelegate::Flush()
{
    int status = GRD_Flush(db_, GRD_DB_FLUSH_ASYNC);
    if (status != GRD_OK) {
        ZLOGE("GRD_Flush failed，status %{public}d", status);
    }
}

int32_t KvDelegate::GetBatch(const std::string &collectionName, const std::string &filter,
    const std::string &projection, std::vector<std::string> &result)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (!Init()) {
        ZLOGE("init failed, %{public}s", collectionName.c_str());
        return E_ERROR;
    }
    Query query;
    query.filter = filter.c_str();
    query.projection = projection.c_str();
    GRD_ResultSet *resultSet;
    int status = GRD_FindDoc(db_, collectionName.c_str(), query, 0, &resultSet);
    if (status != GRD_OK || resultSet == nullptr) {
        ZLOGE("GRD_UpSertDoc failed，status %{public}d", status);
        return status;
    }
    char *value = nullptr;
    while (GRD_Next(resultSet) == GRD_OK) {
        status = GRD_GetValue(resultSet, &value);
        if (status != GRD_OK || value == nullptr) {
            GRD_FreeResultSet(resultSet);
            ZLOGE("GRD_GetValue failed，status %{public}d", status);
            return status;
        }
        result.emplace_back(value);
        GRD_FreeValue(value);
    }
    GRD_FreeResultSet(resultSet);
    return E_OK;
}
KvDelegate::KvDelegate(const std::string &path) : path_(path) {
    if (path_.empty()) {
        path_ = "/data/app/el1/0/database/distributeddata/kvdb"; // todo hanlu delete
    }
}
} // namespace OHOS::DataShare