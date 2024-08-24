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
#define LOG_TAG "RdbAdaptor"
#include "rdb_delegate.h"

#include "crypto_manager.h"
#include "datashare_errno.h"
#include "datashare_radar_reporter.h"
#include "device_manager_adapter.h"
#include "extension_connect_adaptor.h"
#include "int_wrapper.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/secret_key_meta_data.h"
#include "resultset_json_formatter.h"
#include "log_print.h"
#include "rdb_errno.h"
#include "rdb_utils.h"
#include "scheduler_manager.h"
#include "string_wrapper.h"
#include "utils/anonymous.h"
#include "want_params.h"

namespace OHOS::DataShare {
constexpr static int32_t MAX_RESULTSET_COUNT = 16;
constexpr static int64_t TIMEOUT_TIME = 500;
std::atomic<int32_t> RdbDelegate::resultSetCount = 0;
ConcurrentMap<uint32_t, int32_t> RdbDelegate::resultSetCallingPids;
enum REMIND_TIMER_ARGS : int32_t {
    ARG_DB_PATH = 0,
    ARG_VERSION,
    ARG_URI,
    ARG_SUBSCRIBER_ID,
    ARG_BUNDLE_NAME,
    ARG_USER_ID,
    ARG_TIME,
    ARGS_SIZE
};
std::string RemindTimerFunc(const std::vector<std::string> &args)
{
    size_t size = args.size();
    if (size != ARGS_SIZE) {
        ZLOGE("RemindTimerFunc args size error, %{public}zu", size);
        return "";
    }
    std::string dbPath = args[ARG_DB_PATH];
    int version = std::strtol(args[ARG_VERSION].c_str(), nullptr, 0);
    Key key(args[ARG_URI], std::strtoll(args[ARG_SUBSCRIBER_ID].c_str(), nullptr, 0), args[ARG_BUNDLE_NAME]);
    int64_t reminderTime = std::strtoll(args[ARG_TIME].c_str(), nullptr, 0);
    int32_t userId = std::strtol(args[ARG_USER_ID].c_str(), nullptr, 0);
    SchedulerManager::GetInstance().SetTimer(dbPath, userId, version, key, reminderTime);
    return args[ARG_TIME];
}

RdbStoreConfig RdbDelegate::GetConfig(const DistributedData::StoreMetaData &meta, bool registerFunction)
{
    RdbStoreConfig config(meta.dataDir);
    config.SetCreateNecessary(false);
    config.SetHaMode(meta.haMode);
    config.SetBundleName(meta.bundleName);
    if (meta.isEncrypt) {
        DistributedData::SecretKeyMetaData secretKeyMeta;
        DistributedData::MetaDataManager::GetInstance().LoadMeta(meta.GetSecretKey(), secretKeyMeta, true);
        std::vector<uint8_t> decryptKey;
        DistributedData::CryptoManager::GetInstance().Decrypt(secretKeyMeta.sKey, decryptKey);
        config.SetEncryptKey(decryptKey);
        std::fill(decryptKey.begin(), decryptKey.end(), 0);
    }
    if (registerFunction) {
        config.SetScalarFunction("remindTimer", ARGS_SIZE, RemindTimerFunc);
    }
    return config;
}

RdbDelegate::RdbDelegate(const DistributedData::StoreMetaData &meta, int version,
    bool registerFunction, const std::string &extUriData, const std::string &backup)
{
    tokenId_ = meta.tokenId;
    bundleName_ = meta.bundleName;
    storeName_ = meta.storeId;
    extUri_ = extUriData;
    haMode_ = meta.haMode;
    backup_ = backup;

    RdbStoreConfig config = GetConfig(meta, registerFunction);
    DefaultOpenCallback callback;
    store_ = RdbHelper::GetRdbStore(config, version, callback, errCode_);
    if (errCode_ != E_OK) {
        ZLOGW("GetRdbStore failed, errCode is %{public}d, dir is %{public}s", errCode_,
            DistributedData::Anonymous::Change(meta.dataDir).c_str());
        RdbDelegate::TryAndSend(errCode_);
    }
}

void RdbDelegate::TryAndSend(int errCode)
{
    if (errCode != E_SQLITE_CORRUPT || (haMode_ == HAMode::SINGLE && (backup_ != DUAL_WRITE && backup_ != PERIODIC))) {
        return;
    }
    ZLOGE("Database corruption. BundleName: %{public}s. StoreName: %{public}s. ExtUri: %{public}s",
        bundleName_.c_str(), storeName_.c_str(), DistributedData::Anonymous::Change(extUri_).c_str());
    AAFwk::WantParams params;
    params.SetParam("BundleName", AAFwk::String::Box(bundleName_));
    params.SetParam("StoreName", AAFwk::String::Box(storeName_));
    params.SetParam("StoreStatus", AAFwk::Integer::Box(1));
    ExtensionConnectAdaptor::TryAndWait(extUri_, bundleName_, params);
}

int64_t RdbDelegate::Insert(const std::string &tableName, const DataShareValuesBucket &valuesBucket)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return 0;
    }
    int64_t rowId = 0;
    ValuesBucket bucket = RdbDataShareAdapter::RdbUtils::ToValuesBucket(valuesBucket);
    int ret = store_->Insert(rowId, tableName, bucket);
    if (ret != E_OK) {
        ZLOGE("Insert failed %{public}s %{public}d", tableName.c_str(), ret);
        RADAR_REPORT(__FUNCTION__, RadarReporter::SILENT_ACCESS, RadarReporter::PROXY_CALL_RDB,
            RadarReporter::FAILED, RadarReporter::ERROR_CODE, RadarReporter::INSERT_RDB_ERROR);
        if (ret == E_SQLITE_ERROR) {
            EraseStoreCache(tokenId_);
        }
    }
    return rowId;
}
int64_t RdbDelegate::Update(
    const std::string &tableName, const DataSharePredicates &predicate, const DataShareValuesBucket &valuesBucket)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return 0;
    }
    int changeCount = 0;
    ValuesBucket bucket = RdbDataShareAdapter::RdbUtils::ToValuesBucket(valuesBucket);
    RdbPredicates predicates = RdbDataShareAdapter::RdbUtils::ToPredicates(predicate, tableName);
    int ret = store_->Update(changeCount, bucket, predicates);
    if (ret != E_OK) {
        ZLOGE("Update failed  %{public}s %{public}d", tableName.c_str(), ret);
        RADAR_REPORT(__FUNCTION__, RadarReporter::SILENT_ACCESS, RadarReporter::PROXY_CALL_RDB,
            RadarReporter::FAILED, RadarReporter::ERROR_CODE, RadarReporter::UPDATE_RDB_ERROR);
        if (ret == E_SQLITE_ERROR) {
            EraseStoreCache(tokenId_);
        }
    }
    return changeCount;
}
int64_t RdbDelegate::Delete(const std::string &tableName, const DataSharePredicates &predicate)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return 0;
    }
    int changeCount = 0;
    RdbPredicates predicates = RdbDataShareAdapter::RdbUtils::ToPredicates(predicate, tableName);
    int ret = store_->Delete(changeCount, predicates);
    if (ret != E_OK) {
        ZLOGE("Delete failed  %{public}s %{public}d", tableName.c_str(), ret);
        RADAR_REPORT(__FUNCTION__, RadarReporter::SILENT_ACCESS, RadarReporter::PROXY_CALL_RDB,
            RadarReporter::FAILED, RadarReporter::ERROR_CODE, RadarReporter::DELETE_RDB_ERROR);
        if (ret == E_SQLITE_ERROR) {
            EraseStoreCache(tokenId_);
        }
    }
    return changeCount;
}

std::pair<int64_t, int64_t> RdbDelegate::InsertEx(const std::string &tableName,
    const DataShareValuesBucket &valuesBucket)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return std::make_pair(E_DB_ERROR, 0);
    }
    int64_t rowId = 0;
    ValuesBucket bucket = RdbDataShareAdapter::RdbUtils::ToValuesBucket(valuesBucket);
    int ret = store_->Insert(rowId, tableName, bucket);
    if (ret != E_OK) {
        ZLOGE("Insert failed %{public}s %{public}d", tableName.c_str(), ret);
        RADAR_REPORT(__FUNCTION__, RadarReporter::SILENT_ACCESS, RadarReporter::PROXY_CALL_RDB,
            RadarReporter::FAILED, RadarReporter::ERROR_CODE, RadarReporter::INSERT_RDB_ERROR);
        if (ret == E_SQLITE_ERROR) {
            EraseStoreCache(tokenId_);
        }
        RdbDelegate::TryAndSend(ret);
        return std::make_pair(E_DB_ERROR, rowId);
    }
    return std::make_pair(E_OK, rowId);
}

std::pair<int64_t, int64_t> RdbDelegate::UpdateEx(
    const std::string &tableName, const DataSharePredicates &predicate, const DataShareValuesBucket &valuesBucket)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return std::make_pair(E_DB_ERROR, 0);
    }
    int changeCount = 0;
    ValuesBucket bucket = RdbDataShareAdapter::RdbUtils::ToValuesBucket(valuesBucket);
    RdbPredicates predicates = RdbDataShareAdapter::RdbUtils::ToPredicates(predicate, tableName);
    int ret = store_->Update(changeCount, bucket, predicates);
    if (ret != E_OK) {
        ZLOGE("Update failed  %{public}s %{public}d", tableName.c_str(), ret);
        RADAR_REPORT(__FUNCTION__, RadarReporter::SILENT_ACCESS, RadarReporter::PROXY_CALL_RDB,
            RadarReporter::FAILED, RadarReporter::ERROR_CODE, RadarReporter::UPDATE_RDB_ERROR);
        if (ret == E_SQLITE_ERROR) {
            EraseStoreCache(tokenId_);
        }
        RdbDelegate::TryAndSend(ret);
        return std::make_pair(E_DB_ERROR, changeCount);
    }
    return std::make_pair(E_OK, changeCount);
}

std::pair<int64_t, int64_t> RdbDelegate::DeleteEx(const std::string &tableName, const DataSharePredicates &predicate)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return std::make_pair(E_DB_ERROR, 0);
    }
    int changeCount = 0;
    RdbPredicates predicates = RdbDataShareAdapter::RdbUtils::ToPredicates(predicate, tableName);
    int ret = store_->Delete(changeCount, predicates);
    if (ret != E_OK) {
        ZLOGE("Delete failed  %{public}s %{public}d", tableName.c_str(), ret);
        RADAR_REPORT(__FUNCTION__, RadarReporter::SILENT_ACCESS, RadarReporter::PROXY_CALL_RDB,
            RadarReporter::FAILED, RadarReporter::ERROR_CODE, RadarReporter::DELETE_RDB_ERROR);
        if (ret == E_SQLITE_ERROR) {
            EraseStoreCache(tokenId_);
        }
        RdbDelegate::TryAndSend(ret);
        return std::make_pair(E_DB_ERROR, changeCount);
    }
    return std::make_pair(E_OK, changeCount);
}

std::pair<int, std::shared_ptr<DataShareResultSet>> RdbDelegate::Query(const std::string &tableName,
    const DataSharePredicates &predicates, const std::vector<std::string> &columns,
    const int32_t callingPid)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return std::make_pair(errCode_, nullptr);
    }
    int count = resultSetCount.fetch_add(1);
    ZLOGD("start query %{public}d", count);
    if (count > MAX_RESULTSET_COUNT && IsLimit(count)) {
        resultSetCount--;
        return std::make_pair(E_ERROR, nullptr);
    }
    RdbPredicates rdbPredicates = RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, tableName);
    std::shared_ptr<NativeRdb::ResultSet> resultSet = store_->QueryByStep(rdbPredicates, columns);
    if (resultSet == nullptr) {
        RADAR_REPORT(__FUNCTION__, RadarReporter::SILENT_ACCESS, RadarReporter::PROXY_CALL_RDB,
            RadarReporter::FAILED, RadarReporter::ERROR_CODE, RadarReporter::QUERY_RDB_ERROR);
        ZLOGE("Query failed %{public}s, pid: %{public}d", tableName.c_str(), callingPid);
        resultSetCount--;
        return std::make_pair(E_ERROR, nullptr);
    }
    int err = resultSet->GetRowCount(count);
    RdbDelegate::TryAndSend(err);
    if (err == E_SQLITE_ERROR) {
        ZLOGE("query failed, err:%{public}d, pid:%{public}d", E_SQLITE_ERROR, callingPid);
        EraseStoreCache(tokenId_);
    }
    resultSetCallingPids.Compute(callingPid, [](const uint32_t &, int32_t &value) {
        ++value;
        return true;
    });
    int64_t beginTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    auto bridge = RdbDataShareAdapter::RdbUtils::ToResultSetBridge(resultSet);
    std::shared_ptr<DataShareResultSet> result = { new DataShareResultSet(bridge), [callingPid, beginTime](auto p) {
        ZLOGD("release resultset");
        resultSetCount--;
        int64_t endTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        if (endTime - beginTime > TIMEOUT_TIME) {
            ZLOGE("pid %{public}d query time is %{public}" PRId64 ", %{public}d resultSet is used.", callingPid,
                (endTime - beginTime), resultSetCount.load());
        }
        resultSetCallingPids.ComputeIfPresent(callingPid, [](const uint32_t &, int32_t &value) {
            --value;
            return value > 0;
        });
        delete p;
    }};
    return std::make_pair(E_OK, result);
}

std::string RdbDelegate::Query(const std::string &sql, const std::vector<std::string> &selectionArgs)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return "";
    }
    auto resultSet = store_->QueryByStep(sql, selectionArgs);
    if (resultSet == nullptr) {
        ZLOGE("Query failed %{private}s", sql.c_str());
        return "";
    }
    int rowCount;
    if (resultSet->GetRowCount(rowCount) == E_SQLITE_ERROR) {
        ZLOGE("query failed, err:%{public}d", E_SQLITE_ERROR);
        EraseStoreCache(tokenId_);
    }
    ResultSetJsonFormatter formatter(std::move(resultSet));
    return DistributedData::Serializable::Marshall(formatter);
}

std::shared_ptr<NativeRdb::ResultSet> RdbDelegate::QuerySql(const std::string &sql)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return nullptr;
    }
    auto resultSet = store_->QuerySql(sql);
    if (resultSet == nullptr) {
        ZLOGE("Query failed %{private}s", sql.c_str());
        return resultSet;
    }
    int rowCount;
    if (resultSet->GetRowCount(rowCount) == E_SQLITE_ERROR) {
        ZLOGE("query failed, err:%{public}d", E_SQLITE_ERROR);
        EraseStoreCache(tokenId_);
    }
    return resultSet;
}

bool RdbDelegate::IsInvalid()
{
    return store_ == nullptr;
}

bool RdbDelegate::IsLimit(int count)
{
    bool isFull = true;
    for (int32_t retryCount = 0; retryCount < RETRY; retryCount++) {
        std::this_thread::sleep_for(WAIT_TIME);
        if (resultSetCount.load() <= MAX_RESULTSET_COUNT) {
            isFull = false;
            break;
        }
    }
    if (!isFull) {
        return false;
    }
    std::string logStr;
    resultSetCallingPids.ForEach([&logStr](const uint32_t &key, const int32_t &value) {
        logStr += std::to_string(key) + ":" + std::to_string(value) + ";";
        return false;
    });
    ZLOGE("resultSetCount is full, owner is %{public}s", logStr.c_str());
    return true;
}
} // namespace OHOS::DataShare