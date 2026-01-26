/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <cinttypes>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "kv_delegate.h"

#include "datashare_errno.h"
#include "directory/directory_manager.h"
#include "hiview_fault_adapter.h"
#include "grd_base/grd_error.h"
#include "grd_document/grd_document_api.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "log_debug.h"
#include "time_service_client.h"

namespace OHOS::DataShare {
constexpr int64_t TIME_LIMIT_BY_MILLISECONDS = 18 * 3600 * 1000; // 18 hours
constexpr int64_t CLOSE_BY_SECONDS = 60; // 1 min


// If using multi-process access, back up dataShare.db.map as well. Check config file to see whether
// using multi-process maccess
const char* g_backupFiles[] = {
    "dataShare.db",
    "dataShare.db.redo",
    "dataShare.db.safe",
    "dataShare.db.undo",
};
constexpr const char* BACKUP_SUFFIX = ".backup";
constexpr std::chrono::milliseconds COPY_TIME_OUT_MS = std::chrono::milliseconds(500);

// If isBackUp is true, remove db backup files. Otherwise remove source db files.
void KvDelegate::RemoveDbFile(bool isBackUp) const
{
    for (auto &fileName: g_backupFiles) {
        std::string dbPath = path_ + "/" + fileName;
        if (isBackUp) {
            dbPath += BACKUP_SUFFIX;
        }
        if (std::filesystem::exists(dbPath)) {
            std::error_code ec;
            bool success = std::filesystem::remove(dbPath, ec);
            if (!success) {
                ZLOGE("failed to remove file %{public}s, err: %{public}s", fileName, ec.message().c_str());
            }
        }
    }
}

bool KvDelegate::CopyFile(bool isBackup)
{
    std::filesystem::copy_options options = std::filesystem::copy_options::overwrite_existing;
    std::error_code code;
    bool ret = true;
    int index = 0;
    for (auto &fileName : g_backupFiles) {
        std::string src = path_ + "/" + fileName;
        std::string dst = src;
        isBackup ? dst.append(BACKUP_SUFFIX) : src.append(BACKUP_SUFFIX);
        TimeoutReport timeoutReport({"", "", "", __FUNCTION__, 0});
        // If src doesn't exist, error will be returned through `std::error_code`
        bool copyRet = std::filesystem::copy_file(src, dst, options, code);
        timeoutReport.Report(("file index:" + std::to_string(index)), COPY_TIME_OUT_MS);
        if (!copyRet) {
            ZLOGE("failed to copy file %{public}s, isBackup %{public}d, err: %{public}s",
                fileName, isBackup, code.message().c_str());
            ret = false;
            RemoveDbFile(isBackup);
            break;
        }
        index++;
    }
    return ret;
}

// Restore database data when it is broken somehow. Some failure of insertion / deletion / updates will be considered
// as database files damage, and therefore trigger the process of restoration.
void KvDelegate::Restore()
{
    // No need to lock because this inner method will only be called when upper methods lock up
    CopyFile(false);
    ZLOGD_MACRO("finish restoring kv");
}

// Backup database data by copying its key files. This mechanism might be costly, but acceptable when updating
// contents of KV database happens not so frequently now.
void KvDelegate::Backup()
{
    // No need to lock because this inner method will only be called when upper methods lock up
    ZLOGD_MACRO("backup kv");
    if (hasChange_) {
        CopyFile(true);
        hasChange_ = false;
    }
    ZLOGD_MACRO("finish backing up kv");
}

// Set hasChange_ to true. Caller can use this to control when to back up db.
void OHOS::DataShare::KvDelegate::NotifyBackup()
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    hasChange_ = true;
}

// The return val indicates whether the database has been restored
bool KvDelegate::RestoreIfNeed(int32_t dbStatus)
{
    // No need to lock because this inner method will only be called when upper methods lock up
    if (dbStatus == GRD_INVALID_FILE_FORMAT || dbStatus == GRD_REBUILD_DATABASE) {
        if (db_ != NULL) {
            GRD_DBClose(db_, GRD_DB_CLOSE);
            db_ = nullptr;
            isInitDone_ = false;
        }
        DataShareFaultInfo faultInfo{HiViewFaultAdapter::kvDBCorrupt, "", "", "", __FUNCTION__, dbStatus, ""};
        HiViewFaultAdapter::ReportDataFault(faultInfo);
        // If db is NULL, it has been closed.
        Restore();
        return true;
    }
    return false;
}

std::pair<int32_t, int32_t> KvDelegate::Upsert(const std::string &collectionName, const std::string &filter,
    const std::string &value)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (!Init()) {
        ZLOGE("init failed, %{public}s", collectionName.c_str());
        return std::make_pair(E_ERROR, 0);
    }
    int32_t count = GRD_UpsertDoc(db_, collectionName.c_str(), filter.c_str(), value.c_str(), 0);
    if (count <= 0) {
        ZLOGE("GRD_UpSertDoc failed,status %{public}d", count);
        RestoreIfNeed(count);
        return std::make_pair(count, 0);
    }
    Flush();
    return std::make_pair(E_OK, count);
}

std::pair<int32_t, int32_t> KvDelegate::Delete(const std::string &collectionName, const std::string &filter)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (!Init()) {
        ZLOGE("init failed, %{public}s", collectionName.c_str());
        return std::make_pair(E_ERROR, 0);
    }
    std::vector<std::string> queryResults;

    int32_t status = GetBatch(collectionName, filter, "{\"id_\": true}", queryResults);
    if (status != E_OK) {
        ZLOGE("db GetBatch failed, %{public}s %{public}d", filter.c_str(), status);
        // `GetBatch` should decide whether to restore before errors are returned, so skip restoration here.
        return std::make_pair(status, 0);
    }
    int32_t deleteCount = 0;
    for (auto &result : queryResults) {
        auto count = GRD_DeleteDoc(db_, collectionName.c_str(), result.c_str(), 0);
        if (count < 0) {
            ZLOGE("GRD_DeleteDoc failed,status %{public}d %{public}s", count, result.c_str());
            if (RestoreIfNeed(count)) {
                return std::make_pair(count, 0);
            }
            continue;
        }
        deleteCount += count;
    }
    Flush();
    if (queryResults.size() > 0) {
        ZLOGI("Delete, %{public}s, count %{public}zu", collectionName.c_str(), queryResults.size());
    }
    return std::make_pair(E_OK, deleteCount);
}

void KvDelegate::ScheduleBackup()
{
    auto currTime = MiscServices::TimeServiceClient::GetInstance()->GetBootTimeMs();
    if (currTime < 0) {
	    // Only do an error log print since schedule time does not rely on the time gets by GetBootTimeMs().
        ZLOGE("GetBootTimeMs failed, status %{public}" PRId64 "", currTime);
    } else {
        absoluteWaitTime_ = currTime + TIME_LIMIT_BY_MILLISECONDS;
    }

    taskId_ = executors_->Schedule(std::chrono::seconds(waitTime_), [this]() {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        GRD_DBClose(db_, GRD_DB_CLOSE);
        db_ = nullptr;
        isInitDone_ = false;
        taskId_ = ExecutorPool::INVALID_TASK_ID;
        Backup();
    });
}

void KvDelegate::GarbageCollect()
{
    cleanTaskId_ =  executors_->Schedule(std::chrono::seconds(CLOSE_BY_SECONDS), [this]() {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        auto ret = GRD_DBClose(db_, GRD_DB_CLOSE);
        if (ret == 0) {
            db_ = nullptr;
            isInitDone_ = false;
            cleanTaskId_ = ExecutorPool::INVALID_TASK_ID;
        }
    });
}

bool KvDelegate::Init()
{
    if (isInitDone_) {
        if (executors_ != nullptr) {
            executors_->Reset(cleanTaskId_, std::chrono::seconds(CLOSE_BY_SECONDS));
            auto currTime = MiscServices::TimeServiceClient::GetInstance()->GetBootTimeMs();
            if (currTime < 0) {
                ZLOGE("GetBootTimeMs failed, status %{public}" PRId64 "", currTime);
                // still return true since kv can work normally
                return true;
            }
            if (currTime < absoluteWaitTime_) {
                executors_->Reset(taskId_, std::chrono::seconds(waitTime_));
            }
        }
        return true;
    }
    int status = GRD_DBOpen(
        (path_ + "/dataShare.db").c_str(), nullptr, GRD_DB_OPEN_CREATE | GRD_DB_OPEN_CHECK_FOR_ABNORMAL, &db_);
    if (status != GRD_OK || db_ == nullptr) {
        ZLOGE("GRD_DBOpen failed,status %{public}d", status);
        RestoreIfNeed(status);
        return false;
    }
    if (executors_ != nullptr) {
        // schedule first backup task
        ScheduleBackup();
        // dynamically close DB
        GarbageCollect();
    }
    status = GRD_CreateCollection(db_, TEMPLATE_TABLE, nullptr, 0);
    if (status != GRD_OK) {
        // If opeaning db succeeds, it is rare to fail to create tables
        ZLOGE("GRD_CreateCollection template table failed,status %{public}d", status);
        RestoreIfNeed(status);
        return false;
    }

    status = GRD_CreateCollection(db_, DATA_TABLE, nullptr, 0);
    if (status != GRD_OK) {
        // If opeaning db succeeds, it is rare to fail to create tables
        ZLOGE("GRD_CreateCollection data table failed,status %{public}d", status);
        RestoreIfNeed(status);
        return false;
    }

    status = GRD_CreateCollection(db_, PROXYDATA_TABLE, nullptr, 0);
    if (status != GRD_OK) {
        // If opeaning db succeeds, it is rare to fail to create tables
        ZLOGE("GRD_CreateCollection data table failed,status %{public}d", status);
        RestoreIfNeed(status);
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
            ZLOGE("GRD_DBClose failed,status %{public}d", status);
        }
    }
}

std::pair<int32_t, int32_t> KvDelegate::Upsert(const std::string &collectionName, const KvData &value)
{
    std::string id = value.GetId();
    if (value.HasVersion() && value.GetVersion() != 0) {
        int version = -1;
        if (GetVersion(collectionName, id, version)) {
            if (value.GetVersion() <= version) {
                ZLOGE("GetVersion failed,%{public}s id %{public}s %{public}d %{public}d", collectionName.c_str(),
                      id.c_str(), value.GetVersion(), version);
                return std::make_pair(E_VERSION_NOT_NEWER, 0);
            }
        }
    }
    return Upsert(collectionName, id, value.GetValue());
}

int32_t KvDelegate::Get(const std::string &collectionName, const Id &id, std::string &value)
{
    std::string filter = DistributedData::Serializable::Marshall(id);
    if (Get(collectionName, filter, "{}", value) != E_OK) {
        ZLOGE("Get failed, %{public}s %{public}s", collectionName.c_str(), filter.c_str());
        return E_ERROR;
    }
    return E_OK;
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
        ZLOGE("Unmarshall failed,data %{public}s", value.c_str());
        return false;
    }
    version = data.GetVersion();
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
        ZLOGE("GRD_FindDoc failed,status %{public}d", status);
        RestoreIfNeed(status);
        return status;
    }
    status = GRD_Next(resultSet);
    if (status != GRD_OK) {
        GRD_FreeResultSet(resultSet);
        ZLOGE("GRD_Next failed,status %{public}d", status);
        RestoreIfNeed(status);
        return status;
    }
    char *value = nullptr;
    status = GRD_GetValue(resultSet, &value);
    if (status != GRD_OK || value == nullptr) {
        GRD_FreeResultSet(resultSet);
        ZLOGE("GRD_GetValue failed,status %{public}d", status);
        RestoreIfNeed(status);
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
        ZLOGE("GRD_Flush failed,status %{public}d", status);
        RestoreIfNeed(status);
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
    int status = GRD_FindDoc(db_, collectionName.c_str(), query, GRD_DOC_ID_DISPLAY, &resultSet);
    if (status != GRD_OK || resultSet == nullptr) {
        ZLOGE("GRD_FindDoc failed,status %{public}d", status);
        RestoreIfNeed(status);
        return status;
    }
    char *value = nullptr;
    while (GRD_Next(resultSet) == GRD_OK) {
        status = GRD_GetValue(resultSet, &value);
        if (status != GRD_OK || value == nullptr) {
            GRD_FreeResultSet(resultSet);
            ZLOGE("GRD_GetValue failed,status %{public}d", status);
            RestoreIfNeed(status);
            return status;
        }
        result.emplace_back(value);
        GRD_FreeValue(value);
    }
    GRD_FreeResultSet(resultSet);
    return E_OK;
}

KvDelegate::KvDelegate(const std::string &path, const std::shared_ptr<ExecutorPool> &executors)
    : path_(path), executors_(executors)
{
}
} // namespace OHOS::DataShare
