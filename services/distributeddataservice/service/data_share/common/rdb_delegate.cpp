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
#include "device_manager_adapter.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/secret_key_meta_data.h"
#include "resultset_json_formatter.h"
#include "log_print.h"
#include "rdb_utils.h"
#include "scheduler_manager.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
constexpr static int32_t MAX_RESULTSET_COUNT = 16;
constexpr static int64_t TIMEOUT_TIME = 500;
std::atomic<int32_t> RdbDelegate::resultSetCount = 0;
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

RdbDelegate::RdbDelegate(const std::string &dir, int version, bool registerFunction,
    bool isEncrypt, const std::string &secretMetaKey)
{
    RdbStoreConfig config(dir);
    config.SetCreateNecessary(false);
    if (isEncrypt) {
        DistributedData::SecretKeyMetaData secretKeyMeta;
        DistributedData::MetaDataManager::GetInstance().LoadMeta(secretMetaKey, secretKeyMeta, true);
        std::vector<uint8_t> decryptKey;
        DistributedData::CryptoManager::GetInstance().Decrypt(secretKeyMeta.sKey, decryptKey);
        config.SetEncryptKey(decryptKey);
        std::fill(decryptKey.begin(), decryptKey.end(), 0);
    }
    if (registerFunction) {
        config.SetScalarFunction("remindTimer", ARGS_SIZE, RemindTimerFunc);
    }
    DefaultOpenCallback callback;
    store_ = RdbHelper::GetRdbStore(config, version, callback, errCode_);
    if (errCode_ != E_OK) {
        ZLOGW("GetRdbStore failed, errCode is %{public}d, dir is %{public}s", errCode_,
            DistributedData::Anonymous::Change(dir).c_str());
    }
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
    }
    return changeCount;
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
    if (count > MAX_RESULTSET_COUNT) {
        ZLOGE("resultSetCount is full");
        resultSetCount--;
        return std::make_pair(E_ERROR, nullptr);
    }
    RdbPredicates rdbPredicates = RdbDataShareAdapter::RdbUtils::ToPredicates(predicates, tableName);
    std::shared_ptr<NativeRdb::ResultSet> resultSet = store_->QueryByStep(rdbPredicates, columns);
    if (resultSet == nullptr) {
        ZLOGE("Query failed %{public}s", tableName.c_str());
        resultSetCount--;
        return std::make_pair(E_ERROR, nullptr);
    }
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
    ResultSetJsonFormatter formatter(std::move(resultSet));
    return DistributedData::Serializable::Marshall(formatter);
}

std::shared_ptr<NativeRdb::ResultSet> RdbDelegate::QuerySql(const std::string &sql)
{
    if (store_ == nullptr) {
        ZLOGE("store is null");
        return nullptr;
    }
    return store_->QuerySql(sql);
}
} // namespace OHOS::DataShare