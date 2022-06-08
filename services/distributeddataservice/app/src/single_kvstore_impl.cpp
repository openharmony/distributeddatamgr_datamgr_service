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

#define LOG_TAG "SingleKvStoreImpl"

#include "single_kvstore_impl.h"
#include <fstream>
#include "account_delegate.h"
#include "auth_delegate.h"
#include "backup_handler.h"
#include "checker/checker_manager.h"
#include "constant.h"
#include "dds_trace.h"
#include "device_kvstore_impl.h"
#include "kvstore_data_service.h"
#include "kvstore_utils.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "permission_validator.h"
#include "query_helper.h"
#include "reporter.h"
#include "upgrade_manager.h"
#include "metadata/meta_data_manager.h"

#define DEFAUL_RETRACT "            "

namespace OHOS::DistributedKv {
using namespace OHOS::DistributedData;
static bool TaskIsBackground(pid_t pid)
{
    std::ifstream ifs("/proc/" + std::to_string(pid) + "/cgroup", std::ios::in);
    ZLOGD("pid %d open %d", pid, ifs.good());
    if (!ifs.good()) {
        return false;
    }

    while (!ifs.eof()) {
        const int cgroupLineLen = 256; // enough
        char buffer[cgroupLineLen] = { 0 };
        ifs.getline(buffer, sizeof(buffer));
        std::string line = buffer;

        size_t pos = line.find("background");
        if (pos != std::string::npos) {
            ifs.close();
            return true;
        }
    }
    ifs.close();
    return false;
}

SingleKvStoreImpl::~SingleKvStoreImpl()
{
    RemoveAllSyncOperation();
    ZLOGI("destructor");
}

SingleKvStoreImpl::SingleKvStoreImpl(const Options &options, const std::string &userId, const std::string &bundleName,
    const std::string &storeId, const std::string &appId, const std::string &directory,
    DistributedDB::KvStoreNbDelegate *delegate)
    : options_(options), deviceAccountId_(userId), bundleName_(bundleName), storeId_(storeId), appId_(appId),
      storePath_(Constant::Concatenate({ directory, storeId })), kvStoreNbDelegate_(delegate), observerMapMutex_(),
      observerMap_(), storeResultSetMutex_(), storeResultSetMap_(), openCount_(1),
      flowCtrl_(BURST_CAPACITY, SUSTAINED_CAPACITY)
{
}

Status SingleKvStoreImpl::Put(const Key &key, const Value &value)
{
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    auto trimmedKey = Constant::TrimCopy<std::vector<uint8_t>>(key.Data());
    // Restrict key and value size to interface specification.
    if (trimmedKey.size() == 0 || trimmedKey.size() > Constant::MAX_KEY_LENGTH ||
        value.Size() > Constant::MAX_VALUE_LENGTH) {
        ZLOGW("invalid_argument.");
        return Status::INVALID_ARGUMENT;
    }
    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("kvstore is not open");
        return Status::ILLEGAL_STATE;
    }
    DistributedDB::Key tmpKey = trimmedKey;
    DistributedDB::Value tmpValue = value.Data();
    DistributedDB::DBStatus status;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->Put(tmpKey, tmpValue);
    }
    if (status == DistributedDB::DBStatus::OK) {
        ZLOGD("succeed.");
        return Status::SUCCESS;
    }
    ZLOGW("failed status: %d.", static_cast<int>(status));

    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        ZLOGI("Put failed, distributeddb need recover.");
        return (Import(bundleName_) ? Status::RECOVER_SUCCESS : Status::RECOVER_FAILED);
    }

    return ConvertDbStatus(status);
}

Status SingleKvStoreImpl::ConvertDbStatus(DistributedDB::DBStatus status)
{
    switch (status) {
        case DistributedDB::DBStatus::BUSY: // fallthrough
        case DistributedDB::DBStatus::DB_ERROR:
            return Status::DB_ERROR;
        case DistributedDB::DBStatus::OK:
            return Status::SUCCESS;
        case DistributedDB::DBStatus::INVALID_ARGS:
            return Status::INVALID_ARGUMENT;
        case DistributedDB::DBStatus::NOT_FOUND:
            return Status::KEY_NOT_FOUND;
        case DistributedDB::DBStatus::INVALID_VALUE_FIELDS:
            return Status::INVALID_VALUE_FIELDS;
        case DistributedDB::DBStatus::INVALID_FIELD_TYPE:
            return Status::INVALID_FIELD_TYPE;
        case DistributedDB::DBStatus::CONSTRAIN_VIOLATION:
            return Status::CONSTRAIN_VIOLATION;
        case DistributedDB::DBStatus::INVALID_FORMAT:
            return Status::INVALID_FORMAT;
        case DistributedDB::DBStatus::INVALID_QUERY_FORMAT:
            return Status::INVALID_QUERY_FORMAT;
        case DistributedDB::DBStatus::INVALID_QUERY_FIELD:
            return Status::INVALID_QUERY_FIELD;
        case DistributedDB::DBStatus::NOT_SUPPORT:
            return Status::NOT_SUPPORT;
        case DistributedDB::DBStatus::TIME_OUT:
            return Status::TIME_OUT;
        case DistributedDB::DBStatus::OVER_MAX_LIMITS:
            return Status::OVER_MAX_SUBSCRIBE_LIMITS;
        case DistributedDB::DBStatus::EKEYREVOKED_ERROR: // fallthrough
        case DistributedDB::DBStatus::SECURITY_OPTION_CHECK_ERROR:
            return Status::SECURITY_LEVEL_ERROR;
        default:
            break;
    }
    return Status::ERROR;
}

Status SingleKvStoreImpl::Delete(const Key &key)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    auto trimmedKey = DistributedKv::Constant::TrimCopy<std::vector<uint8_t>>(key.Data());
    if (trimmedKey.size() == 0 || trimmedKey.size() > Constant::MAX_KEY_LENGTH) {
        ZLOGW("invalid argument.");
        return Status::INVALID_ARGUMENT;
    }
    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("kvstore is not open");
        return Status::ILLEGAL_STATE;
    }
    DistributedDB::Key tmpKey = trimmedKey;
    DistributedDB::DBStatus status;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->Delete(tmpKey);
    }
    if (status == DistributedDB::DBStatus::OK) {
        ZLOGD("succeed.");
        return Status::SUCCESS;
    }
    ZLOGW("failed status: %d.", static_cast<int>(status));
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        ZLOGI("Delete failed, distributeddb need recover.");
        return (Import(bundleName_) ? Status::RECOVER_SUCCESS : Status::RECOVER_FAILED);
    }
    return ConvertDbStatus(status);
}

Status SingleKvStoreImpl::Get(const Key &key, Value &value)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    auto trimmedKey = DistributedKv::Constant::TrimCopy<std::vector<uint8_t>>(key.Data());
    if (trimmedKey.empty() || trimmedKey.size() > DistributedKv::Constant::MAX_KEY_LENGTH) {
        return Status::INVALID_ARGUMENT;
    }
    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("kvstore is not open");
        return Status::ILLEGAL_STATE;
    }
    DistributedDB::Key tmpKey = trimmedKey;
    DistributedDB::Value tmpValue;
    DistributedDB::DBStatus status;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->Get(tmpKey, tmpValue);
    }
    ZLOGD("status: %d.", static_cast<int>(status));
    if (status == DistributedDB::DBStatus::OK) {
        Reporter::GetInstance()->VisitStatistic()->Report({bundleName_, __FUNCTION__});
        // Value don't have other write method.
        Value tmpValueForCopy(tmpValue);
        value = tmpValueForCopy;
        return Status::SUCCESS;
    }
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        ZLOGI("Get failed, distributeddb need recover.");
        std::string errorInfo;
        errorInfo.append(__FUNCTION__).append(": Get failed. ")
            .append("key is ").append(key.ToString())
            .append(". bundleName is ").append(bundleName_);
        DumpHelper::GetInstance().AddErrorInfo(errorInfo);
        return (Import(bundleName_) ? Status::RECOVER_SUCCESS : Status::RECOVER_FAILED);
    }
    return ConvertDbStatus(status);
}

Status SingleKvStoreImpl::SubscribeKvStore(const SubscribeType subscribeType, sptr<IKvStoreObserver> observer)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    ZLOGD("start.");
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    if (observer == nullptr) {
        return Status::INVALID_ARGUMENT;
    }
    std::shared_lock<std::shared_mutex> sharedLock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("kvstore is not open");
        return Status::ILLEGAL_STATE;
    }
    KvStoreObserverImpl *nbObserver = CreateObserver(subscribeType, observer);
    if (nbObserver == nullptr) {
        ZLOGW("new KvStoreObserverNbImpl failed");
        return Status::ERROR;
    }

    std::lock_guard<std::mutex> lock(observerMapMutex_);
    IRemoteObject *objectPtr = observer->AsObject().GetRefPtr();
    bool alreadySubscribed = (observerMap_.find(objectPtr) != observerMap_.end());
    if (alreadySubscribed) {
        delete nbObserver;
        return Status::STORE_ALREADY_SUBSCRIBE;
    }
    int dbObserverMode = ConvertToDbObserverMode(subscribeType);
    DistributedDB::Key emptyKey;
    DistributedDB::DBStatus dbStatus;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        dbStatus = kvStoreNbDelegate_->RegisterObserver(emptyKey, dbObserverMode, nbObserver);
    }
    Reporter::GetInstance()->VisitStatistic()->Report({bundleName_, __FUNCTION__});
    if (dbStatus == DistributedDB::DBStatus::OK) {
        observerMap_.insert(std::pair<IRemoteObject *, KvStoreObserverImpl *>(objectPtr, nbObserver));
        return Status::SUCCESS;
    }

    delete nbObserver;
    if (dbStatus == DistributedDB::DBStatus::INVALID_ARGS) {
        return Status::INVALID_ARGUMENT;
    }
    if (dbStatus == DistributedDB::DBStatus::DB_ERROR) {
        return Status::DB_ERROR;
    }
    return Status::ERROR;
}

KvStoreObserverImpl *SingleKvStoreImpl::CreateObserver(
    const SubscribeType subscribeType, sptr<IKvStoreObserver> observer)
{
    return new (std::nothrow) KvStoreObserverImpl(subscribeType, observer);
}
// Convert KvStore subscribe type to DistributeDB observer mode.
int SingleKvStoreImpl::ConvertToDbObserverMode(const SubscribeType subscribeType) const
{
    int dbObserverMode;
    if (subscribeType == SubscribeType::SUBSCRIBE_TYPE_LOCAL) {
        dbObserverMode = DistributedDB::OBSERVER_CHANGES_NATIVE;
    } else if (subscribeType == SubscribeType::SUBSCRIBE_TYPE_REMOTE) {
        dbObserverMode = DistributedDB::OBSERVER_CHANGES_FOREIGN;
    } else {
        dbObserverMode = DistributedDB::OBSERVER_CHANGES_FOREIGN | DistributedDB::OBSERVER_CHANGES_NATIVE;
    }
    return dbObserverMode;
}

// Convert KvStore sync mode to DistributeDB sync mode.
DistributedDB::SyncMode SingleKvStoreImpl::ConvertToDbSyncMode(SyncMode syncMode) const
{
    DistributedDB::SyncMode dbSyncMode;
    if (syncMode == SyncMode::PUSH) {
        dbSyncMode = DistributedDB::SyncMode::SYNC_MODE_PUSH_ONLY;
    } else if (syncMode == SyncMode::PULL) {
        dbSyncMode = DistributedDB::SyncMode::SYNC_MODE_PULL_ONLY;
    } else {
        dbSyncMode = DistributedDB::SyncMode::SYNC_MODE_PUSH_PULL;
    }
    return dbSyncMode;
}

Status SingleKvStoreImpl::UnSubscribeKvStore(const SubscribeType subscribeType, sptr<IKvStoreObserver> observer)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    if (observer == nullptr) {
        ZLOGW("observer invalid.");
        return Status::INVALID_ARGUMENT;
    }
    std::shared_lock<std::shared_mutex> sharedLock(storeNbDelegateMutex_);
    std::lock_guard<std::mutex> lock(observerMapMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("kvstore is not open");
        return Status::ILLEGAL_STATE;
    }
    IRemoteObject *objectPtr = observer->AsObject().GetRefPtr();
    auto nbObserver = observerMap_.find(objectPtr);
    if (nbObserver == observerMap_.end()) {
        ZLOGW("No existing observer to unsubscribe. Return success.");
        return Status::SUCCESS;
    }
    DistributedDB::DBStatus dbStatus;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        dbStatus = kvStoreNbDelegate_->UnRegisterObserver(nbObserver->second);
    }
    if (dbStatus == DistributedDB::DBStatus::OK) {
        delete nbObserver->second;
        observerMap_.erase(objectPtr);
    }
    Reporter::GetInstance()->VisitStatistic()->Report({bundleName_, __FUNCTION__});
    if (dbStatus == DistributedDB::DBStatus::OK) {
        return Status::SUCCESS;
    }

    ZLOGW("failed code=%d.", static_cast<int>(dbStatus));
    if (dbStatus == DistributedDB::DBStatus::NOT_FOUND) {
        return Status::STORE_NOT_SUBSCRIBE;
    }
    if (dbStatus == DistributedDB::DBStatus::INVALID_ARGS) {
        return Status::INVALID_ARGUMENT;
    }
    return Status::ERROR;
}

Status SingleKvStoreImpl::GetEntries(const Key &prefixKey, std::vector<Entry> &entries)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    auto trimmedPrefix = Constant::TrimCopy<std::vector<uint8_t>>(prefixKey.Data());
    if (trimmedPrefix.size() > Constant::MAX_KEY_LENGTH) {
        return Status::INVALID_ARGUMENT;
    }
    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("kvstore is not open");
        return Status::ILLEGAL_STATE;
    }
    DistributedDB::Key tmpKeyPrefix = trimmedPrefix;
    std::vector<DistributedDB::Entry> dbEntries;
    DistributedDB::DBStatus status;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->GetEntries(tmpKeyPrefix, dbEntries);
    }
    ZLOGI("result DBStatus: %d", static_cast<int>(status));
    if (status == DistributedDB::DBStatus::OK) {
        entries.reserve(dbEntries.size());
        ZLOGD("vector size: %zu status: %d.", dbEntries.size(), static_cast<int>(status));
        for (auto const &dbEntry : dbEntries) {
            Key tmpKey(dbEntry.key);
            Value tmpValue(dbEntry.value);
            Entry entry;
            entry.key = tmpKey;
            entry.value = tmpValue;
            entries.push_back(entry);
        }
        return Status::SUCCESS;
    }
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        ZLOGE("GetEntries failed, distributeddb need recover.");
        return (Import(bundleName_) ? Status::RECOVER_SUCCESS : Status::RECOVER_FAILED);
    }
    if (status == DistributedDB::DBStatus::BUSY || status == DistributedDB::DBStatus::DB_ERROR) {
        return Status::DB_ERROR;
    }
    if (status == DistributedDB::DBStatus::NOT_FOUND) {
        ZLOGI("DB return NOT_FOUND, no matching result. Return success with empty list.");
        return Status::SUCCESS;
    }
    if (status == DistributedDB::DBStatus::INVALID_ARGS) {
        return Status::INVALID_ARGUMENT;
    }
    if (status == DistributedDB::DBStatus::EKEYREVOKED_ERROR ||
        status == DistributedDB::DBStatus::SECURITY_OPTION_CHECK_ERROR) {
        return Status::SECURITY_LEVEL_ERROR;
    }
    return Status::ERROR;
}

Status SingleKvStoreImpl::GetEntriesWithQuery(const std::string &query, std::vector<Entry> &entries)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    ZLOGI("begin");
    bool isSuccess = false;
    DistributedDB::Query dbQuery = QueryHelper::StringToDbQuery(query, isSuccess);
    if (!isSuccess) {
        ZLOGE("StringToDbQuery failed.");
        return Status::INVALID_ARGUMENT;
    } else {
        ZLOGD("StringToDbQuery success.");
    }
    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("kvstore is not open");
        return Status::ILLEGAL_STATE;
    }
    std::vector<DistributedDB::Entry> dbEntries;
    DistributedDB::DBStatus status;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->GetEntries(dbQuery, dbEntries);
    }
    ZLOGI("result DBStatus: %d", static_cast<int>(status));
    if (status == DistributedDB::DBStatus::OK) {
        entries.reserve(dbEntries.size());
        ZLOGD("vector size: %zu status: %d.", dbEntries.size(), static_cast<int>(status));
        for (auto const &dbEntry : dbEntries) {
            Key tmpKey(dbEntry.key);
            Value tmpValue(dbEntry.value);
            Entry entry;
            entry.key = tmpKey;
            entry.value = tmpValue;
            entries.push_back(entry);
        }
        return Status::SUCCESS;
    }
    if (status == DistributedDB::DBStatus::NOT_FOUND) {
        ZLOGI("DB return NOT_FOUND, no matching result. Return success with empty list.");
        return Status::SUCCESS;
    }
    return ConvertDbStatus(status);
}

void SingleKvStoreImpl::GetResultSet(const Key &prefixKey,
                                     std::function<void(Status, sptr<IKvStoreResultSet>)> callback)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        callback(Status::EXCEED_MAX_ACCESS_RATE, nullptr);
        return;
    }
    auto trimmedPrefix = Constant::TrimCopy<std::vector<uint8_t>>(prefixKey.Data());
    if (trimmedPrefix.size() > Constant::MAX_KEY_LENGTH) {
        callback(Status::INVALID_ARGUMENT, nullptr);
        return;
    }

    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("kvstore is not open");
        callback(Status::ILLEGAL_STATE, nullptr);
        return;
    }
    DistributedDB::Key tmpKeyPrefix = trimmedPrefix;
    DistributedDB::KvStoreResultSet *dbResultSet = nullptr;
    DistributedDB::DBStatus status;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->GetEntries(tmpKeyPrefix, dbResultSet);
    }
    ZLOGI("result DBStatus: %d", static_cast<int>(status));
    if (status == DistributedDB::DBStatus::OK) {
        std::lock_guard<std::mutex> lg(storeResultSetMutex_);
        sptr<KvStoreResultSetImpl> storeResultSet = CreateResultSet(dbResultSet, tmpKeyPrefix);
        callback(Status::SUCCESS, storeResultSet);
        storeResultSetMap_.emplace(storeResultSet->AsObject().GetRefPtr(), storeResultSet);
        return;
    }
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        bool success = Import(bundleName_);
        callback(success ? Status::RECOVER_SUCCESS : Status::RECOVER_FAILED, nullptr);
        return;
    }
    callback(ConvertDbStatus(status), nullptr);
}

void SingleKvStoreImpl::GetResultSetWithQuery(const std::string &query,
                                              std::function<void(Status, sptr<IKvStoreResultSet>)> callback)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        callback(Status::EXCEED_MAX_ACCESS_RATE, nullptr);
        return;
    }
    bool isSuccess = false;
    DistributedDB::Query dbQuery = QueryHelper::StringToDbQuery(query, isSuccess);
    if (!isSuccess) {
        ZLOGE("StringToDbQuery failed.");
        return;
    } else {
        ZLOGD("StringToDbQuery success.");
    }
    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("kvstore is not open");
        callback(Status::ILLEGAL_STATE, nullptr);
        return;
    }
    DistributedDB::KvStoreResultSet *dbResultSet = nullptr;
    DistributedDB::DBStatus status;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->GetEntries(dbQuery, dbResultSet);
    }
    ZLOGI("result DBStatus: %d", static_cast<int>(status));
    if (status == DistributedDB::DBStatus::OK) {
        std::lock_guard<std::mutex> lg(storeResultSetMutex_);
        sptr<KvStoreResultSetImpl> storeResultSet = CreateResultSet(dbResultSet, {});
        callback(Status::SUCCESS, storeResultSet);
        storeResultSetMap_.emplace(storeResultSet->AsObject().GetRefPtr(), storeResultSet);
        return;
    }
    switch (status) {
        case DistributedDB::DBStatus::BUSY:
        case DistributedDB::DBStatus::DB_ERROR: {
            callback(Status::DB_ERROR, nullptr);
            break;
        }
        case DistributedDB::DBStatus::INVALID_ARGS: {
            callback(Status::INVALID_ARGUMENT, nullptr);
            break;
        }
        case DistributedDB::DBStatus::INVALID_QUERY_FORMAT: {
            callback(Status::INVALID_QUERY_FORMAT, nullptr);
            break;
        }
        case DistributedDB::DBStatus::INVALID_QUERY_FIELD: {
            callback(Status::INVALID_QUERY_FIELD, nullptr);
            break;
        }
        case DistributedDB::DBStatus::NOT_SUPPORT: {
            callback(Status::NOT_SUPPORT, nullptr);
            break;
        }
        case DistributedDB::DBStatus::EKEYREVOKED_ERROR: // fallthrough
        case DistributedDB::DBStatus::SECURITY_OPTION_CHECK_ERROR:
            callback(Status::SECURITY_LEVEL_ERROR, nullptr);
            break;
        default: {
            callback(Status::ERROR, nullptr);
            break;
        }
    }
}

KvStoreResultSetImpl *SingleKvStoreImpl::CreateResultSet(
    DistributedDB::KvStoreResultSet *resultSet, const DistributedDB::Key &prix)
{
    return new (std::nothrow) KvStoreResultSetImpl(resultSet, prix);
}

Status SingleKvStoreImpl::GetCountWithQuery(const std::string &query, int &result)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    ZLOGI("begin");
    bool isSuccess = false;
    DistributedDB::Query dbQuery = QueryHelper::StringToDbQuery(query, isSuccess);
    if (!isSuccess) {
        ZLOGE("StringToDbQuery failed.");
        return Status::INVALID_ARGUMENT;
    } else {
        ZLOGD("StringToDbQuery success.");
    }
    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("kvstore is not open");
        return Status::ILLEGAL_STATE;
    }
    DistributedDB::DBStatus status;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->GetCount(dbQuery, result);
    }
    ZLOGI("result DBStatus: %d", static_cast<int>(status));
    switch (status) {
        case DistributedDB::DBStatus::OK: {
            return Status::SUCCESS;
        }
        case DistributedDB::DBStatus::BUSY:
        case DistributedDB::DBStatus::DB_ERROR: {
            return Status::DB_ERROR;
        }
        case DistributedDB::DBStatus::INVALID_ARGS: {
            return Status::INVALID_ARGUMENT;
        }
        case DistributedDB::DBStatus::INVALID_QUERY_FORMAT: {
            return Status::INVALID_QUERY_FORMAT;
        }
        case DistributedDB::DBStatus::INVALID_QUERY_FIELD: {
            return Status::INVALID_QUERY_FIELD;
        }
        case DistributedDB::DBStatus::NOT_SUPPORT: {
            return Status::NOT_SUPPORT;
        }
        case DistributedDB::DBStatus::NOT_FOUND: {
            ZLOGE("DB return NOT_FOUND, no matching result. Return success with count 0.");
            result = 0;
            return Status::SUCCESS;
        }
        case DistributedDB::DBStatus::EKEYREVOKED_ERROR: // fallthrough
        case DistributedDB::DBStatus::SECURITY_OPTION_CHECK_ERROR:
            return Status::SECURITY_LEVEL_ERROR;
        default: {
            return Status::ERROR;
        }
    }
}

Status SingleKvStoreImpl::CloseResultSet(sptr<IKvStoreResultSet> resultSet)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    if (resultSet == nullptr) {
        return Status::INVALID_ARGUMENT;
    }
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    std::lock_guard<std::mutex> lg(storeResultSetMutex_);
    Status status;
    auto it = storeResultSetMap_.find(resultSet->AsObject().GetRefPtr());
    if (it == storeResultSetMap_.end()) {
        ZLOGE("ResultSet not found in this store.");
        return Status::INVALID_ARGUMENT;
    }
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = it->second->CloseResultSet(kvStoreNbDelegate_);
    }
    if (status == Status::SUCCESS) {
        storeResultSetMap_.erase(it);
    } else {
        ZLOGE("CloseResultSet failed.");
    }
    return status;
}

Status SingleKvStoreImpl::RemoveDeviceData(const std::string &device)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    // map UUID to UDID
    std::string deviceUDID = AppDistributedKv::CommunicationProvider::GetInstance().GetUuidByNodeId(device);
    if (deviceUDID.empty()) {
        ZLOGE("can't get nodeid");
        return Status::ERROR;
    }
    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("kvstore is not open");
        return Status::ILLEGAL_STATE;
    }
    DistributedDB::DBStatus status;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->RemoveDeviceData(deviceUDID);
    }
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        ZLOGE("RemoveDeviceData failed, distributeddb need recover.");
        return (Import(bundleName_) ? Status::RECOVER_SUCCESS : Status::RECOVER_FAILED);
    }
    if (status == DistributedDB::DBStatus::OK) {
        return Status::SUCCESS;
    }
    if (status == DistributedDB::DBStatus::EKEYREVOKED_ERROR ||
        status == DistributedDB::DBStatus::SECURITY_OPTION_CHECK_ERROR) {
        return Status::SECURITY_LEVEL_ERROR;
    }
    return Status::ERROR;
}

Status SingleKvStoreImpl::Sync(const std::vector<std::string> &deviceIds, SyncMode mode,
                               uint32_t allowedDelayMs, uint64_t sequenceId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    ZLOGD("start.");
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    uint32_t delayMs = GetSyncDelayTime(allowedDelayMs);
    {
        std::unique_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
        if ((waitingSyncCount_ > 0) &&
            (lastSyncDeviceIds_ == deviceIds) && (lastSyncMode_ == mode) && (lastSyncDelayMs_ == delayMs)) {
            return Status::SUCCESS;
        }
        lastSyncDeviceIds_ = deviceIds;
        lastSyncMode_ = mode;
        lastSyncDelayMs_ = delayMs;
    }
    return AddSync(deviceIds, mode, delayMs, sequenceId);
}

Status SingleKvStoreImpl::Sync(const std::vector<std::string> &deviceIds, SyncMode mode,
                               const std::string &query, uint64_t sequenceId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    ZLOGD("start.");
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    uint32_t delayMs = GetSyncDelayTime(0);
    return AddSync(deviceIds, mode, query, delayMs, sequenceId);
}

Status SingleKvStoreImpl::AddSync(const std::vector<std::string> &deviceIds, SyncMode mode, uint32_t delayMs,
                                  uint64_t sequenceId)
{
    ZLOGD("start.");
    waitingSyncCount_++;
    return KvStoreSyncManager::GetInstance()->AddSyncOperation(reinterpret_cast<uintptr_t>(this), delayMs,
        std::bind(&SingleKvStoreImpl::DoSync, this, deviceIds, mode, std::placeholders::_1, sequenceId),
        std::bind(&SingleKvStoreImpl::DoSyncComplete, this, std::placeholders::_1, "", sequenceId));
}

Status SingleKvStoreImpl::AddSync(const std::vector<std::string> &deviceIds, SyncMode mode,
                                  const std::string &query, uint32_t delayMs, uint64_t sequenceId)
{
    ZLOGD("start.");
    waitingSyncCount_++;
    return KvStoreSyncManager::GetInstance()->AddSyncOperation(reinterpret_cast<uintptr_t>(this), delayMs,
        std::bind(&SingleKvStoreImpl::DoQuerySync, this, deviceIds, mode, query, std::placeholders::_1, sequenceId),
        std::bind(&SingleKvStoreImpl::DoSyncComplete, this, std::placeholders::_1, query, sequenceId));
}

uint32_t SingleKvStoreImpl::GetSyncDelayTime(uint32_t allowedDelayMs) const
{
    uint32_t delayMs = allowedDelayMs;
    if (delayMs == 0) {
        bool isBackground = TaskIsBackground(IPCSkeleton::GetCallingPid());
        if (isBackground) {
            // delay schedule
            delayMs = defaultSyncDelayMs_ ? defaultSyncDelayMs_ : KvStoreSyncManager::SYNC_DEFAULT_DELAY_MS;
        }
    } else {
        if (delayMs < KvStoreSyncManager::SYNC_MIN_DELAY_MS) {
            delayMs = KvStoreSyncManager::SYNC_MIN_DELAY_MS;
        }
        if (delayMs > KvStoreSyncManager::SYNC_MAX_DELAY_MS) {
            delayMs = KvStoreSyncManager::SYNC_MAX_DELAY_MS;
        }
    }
    return delayMs;
}

Status SingleKvStoreImpl::RemoveAllSyncOperation()
{
    return KvStoreSyncManager::GetInstance()->RemoveSyncOperation(reinterpret_cast<uintptr_t>(this));
}

void SingleKvStoreImpl::DoSyncComplete(const std::map<std::string, DistributedDB::DBStatus> &devicesSyncResult,
                                       const std::string &query, uint64_t sequenceId)
{
    DdsTrace trace(std::string("DdsTrace " LOG_TAG "::") + std::string(__FUNCTION__));
    std::map<std::string, Status> resultMap;
    for (auto device : devicesSyncResult) {
        resultMap[device.first] = ConvertDbStatus(device.second);
    }
    syncRetries_ = 0;
    ZLOGD("callback.");
    if (syncCallback_ != nullptr) {
        syncCallback_->SyncCompleted(resultMap, sequenceId);
    }
}

Status SingleKvStoreImpl::DoQuerySync(const std::vector<std::string> &deviceIds, SyncMode mode,
    const std::string &query, const KvStoreSyncManager::SyncEnd &syncEnd, uint64_t sequenceId)
{
    ZLOGD("start.");
    std::vector<std::string> deviceUuids = MapNodeIdToUuids(deviceIds);
    if (deviceUuids.empty()) {
        ZLOGE("not found deviceIds.");
        return Status::ERROR;
    }
    DistributedDB::SyncMode dbMode;
    if (mode == SyncMode::PUSH) {
        dbMode = DistributedDB::SyncMode::SYNC_MODE_PUSH_ONLY;
    } else if (mode == SyncMode::PULL) {
        dbMode = DistributedDB::SyncMode::SYNC_MODE_PULL_ONLY;
    } else {
        dbMode = DistributedDB::SyncMode::SYNC_MODE_PUSH_PULL;
    }
    bool isSuccess = false;
    DistributedDB::Query dbQuery = QueryHelper::StringToDbQuery(query, isSuccess);
    if (!isSuccess) {
        ZLOGE("StringToDbQuery failed.");
        return Status::INVALID_ARGUMENT;
    }
    ZLOGD("StringToDbQuery success.");
    DistributedDB::DBStatus status;
    {
        std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
        if (kvStoreNbDelegate_ == nullptr) {
            ZLOGE("kvstore is not open");
            return Status::ILLEGAL_STATE;
        }
        waitingSyncCount_--;
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->Sync(deviceUuids, dbMode, syncEnd, dbQuery, false);
        ZLOGD("end: %d", static_cast<int>(status));
    }
    Reporter::GetInstance()->VisitStatistic()->Report({bundleName_, __FUNCTION__});
    if (status == DistributedDB::DBStatus::BUSY) {
        if (syncRetries_ < KvStoreSyncManager::SYNC_RETRY_MAX_COUNT) {
            syncRetries_++;
            auto addStatus = AddSync(deviceIds, mode, query, KvStoreSyncManager::SYNC_DEFAULT_DELAY_MS, sequenceId);
            if (addStatus == Status::SUCCESS) {
                return addStatus;
            }
        }
    }
    return ConvertDbStatus(status);
}

Status SingleKvStoreImpl::DoSync(const std::vector<std::string> &deviceIds, SyncMode mode,
                                 const KvStoreSyncManager::SyncEnd &syncEnd, uint64_t sequenceId)
{
    ZLOGD("start.");
    std::vector<std::string> deviceUuids = MapNodeIdToUuids(deviceIds);
    if (deviceUuids.empty()) {
        ZLOGE("not found deviceIds.");
        return Status::ERROR;
    }
    DistributedDB::SyncMode dbMode = ConvertToDbSyncMode(mode);
    DistributedDB::DBStatus status;
    {
        std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
        if (kvStoreNbDelegate_ == nullptr) {
            ZLOGE("kvstore is not open");
            return Status::ILLEGAL_STATE;
        }
        waitingSyncCount_--;
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->Sync(deviceUuids, dbMode, syncEnd);
        ZLOGD("end: %d", static_cast<uint32_t>(status));
    }
    Reporter::GetInstance()->VisitStatistic()->Report({bundleName_, __FUNCTION__});
    if (status == DistributedDB::DBStatus::BUSY) {
        if (syncRetries_ < KvStoreSyncManager::SYNC_RETRY_MAX_COUNT) {
            syncRetries_++;
            auto addStatus = AddSync(deviceUuids, mode, KvStoreSyncManager::SYNC_DEFAULT_DELAY_MS, sequenceId);
            if (addStatus == Status::SUCCESS) {
                return addStatus;
            }
        }
    }
    return ConvertDbStatus(status);
}

std::vector<std::string> SingleKvStoreImpl::MapNodeIdToUuids(const std::vector<std::string> &deviceIds)
{
    std::vector<std::string> deviceUuids;
    for (auto const &nodeId : deviceIds) {
        std::string uuid = AppDistributedKv::CommunicationProvider::GetInstance().GetUuidByNodeId(nodeId);
        if (!uuid.empty()) {
            deviceUuids.emplace_back(uuid);
        }
    }
    return deviceUuids;
}

Status SingleKvStoreImpl::DoSubscribe(const std::vector<std::string> &deviceIds,
                                      const std::string &query, const KvStoreSyncManager::SyncEnd &syncEnd)
{
    ZLOGD("start.");
    std::vector<std::string> deviceUuids = MapNodeIdToUuids(deviceIds);
    if (deviceUuids.empty()) {
        ZLOGE("not found deviceIds.");
        return Status::ERROR;
    }
    bool isSuccess = false;
    DistributedDB::Query dbQuery = QueryHelper::StringToDbQuery(query, isSuccess);
    if (!isSuccess) {
        ZLOGE("StringToDbQuery failed.");
        return Status::INVALID_ARGUMENT;
    }
    ZLOGD("StringToDbQuery success.");
    DistributedDB::DBStatus status;
    {
        std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
        if (kvStoreNbDelegate_ == nullptr) {
            ZLOGE("kvstore is not open");
            std::string errorInfo;
            errorInfo.append(__FUNCTION__).append(": Subscribe failed because kvstore is not open. ")
                .append("bundleName is ").append(bundleName_);
            DumpHelper::GetInstance().AddErrorInfo(errorInfo);
            return Status::ILLEGAL_STATE;
        }
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->SubscribeRemoteQuery(deviceUuids, syncEnd, dbQuery, false);
        ZLOGD("end: %u", static_cast<uint32_t>(status));
    }
    Reporter::GetInstance()->VisitStatistic()->Report({bundleName_, __FUNCTION__});
    return ConvertDbStatus(status);
}

Status SingleKvStoreImpl::DoUnSubscribe(const std::vector<std::string> &deviceIds, const std::string &query,
                                        const KvStoreSyncManager::SyncEnd &syncEnd)
{
    ZLOGD("start.");
    std::vector<std::string> deviceUuids = MapNodeIdToUuids(deviceIds);
    if (deviceUuids.empty()) {
        ZLOGE("not found deviceIds.");
        return Status::ERROR;
    }
    bool isSuccess = false;
    DistributedDB::Query dbQuery = QueryHelper::StringToDbQuery(query, isSuccess);
    if (!isSuccess) {
        ZLOGE("StringToDbQuery failed.");
        return Status::INVALID_ARGUMENT;
    }
    ZLOGD("StringToDbQuery success.");
    DistributedDB::DBStatus status;
    {
        std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
        if (kvStoreNbDelegate_ == nullptr) {
            ZLOGE("kvstore is not open");
            return Status::ILLEGAL_STATE;
        }
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->UnSubscribeRemoteQuery(deviceUuids, syncEnd, dbQuery, false);
        ZLOGD("end: %u", static_cast<uint32_t>(status));
    }
    return ConvertDbStatus(status);
}

Status SingleKvStoreImpl::AddSubscribe(const std::vector<std::string> &deviceIds, const std::string &query,
                                       uint32_t delayMs, uint64_t sequenceId)
{
    ZLOGD("start.");
    return KvStoreSyncManager::GetInstance()->AddSyncOperation(reinterpret_cast<uintptr_t>(this), delayMs,
        std::bind(&SingleKvStoreImpl::DoSubscribe, this, deviceIds, query, std::placeholders::_1),
        std::bind(&SingleKvStoreImpl::DoSyncComplete, this, std::placeholders::_1, "", sequenceId));
}

Status SingleKvStoreImpl::AddUnSubscribe(const std::vector<std::string> &deviceIds, const std::string &query,
                                         uint32_t delayMs, uint64_t sequenceId)
{
    ZLOGD("start.");
    return KvStoreSyncManager::GetInstance()->AddSyncOperation(reinterpret_cast<uintptr_t>(this), delayMs,
        std::bind(&SingleKvStoreImpl::DoUnSubscribe, this, deviceIds, query, std::placeholders::_1),
        std::bind(&SingleKvStoreImpl::DoSyncComplete, this, std::placeholders::_1, "", sequenceId));
}

Status SingleKvStoreImpl::Subscribe(const std::vector<std::string> &deviceIds,
                                    const std::string &query, uint64_t sequenceId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    ZLOGD("start.");
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    uint32_t delayMs = GetSyncDelayTime(0);
    return AddSubscribe(deviceIds, query, delayMs, sequenceId);
}

Status SingleKvStoreImpl::UnSubscribe(const std::vector<std::string> &deviceIds,
                                      const std::string &query, uint64_t sequenceId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    ZLOGD("start.");
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    uint32_t delayMs = GetSyncDelayTime(0);
    return AddUnSubscribe(deviceIds, query, delayMs, sequenceId);
}

InnerStatus SingleKvStoreImpl::Close(DistributedDB::KvStoreDelegateManager *kvStoreDelegateManager)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    ZLOGW("start Close");
    if (openCount_ > 1) {
        openCount_--;
        return InnerStatus::DECREASE_REFCOUNT;
    }
    Status status = ForceClose(kvStoreDelegateManager);
    if (status == Status::SUCCESS) {
        return InnerStatus::SUCCESS;
    }
    return InnerStatus::ERROR;
}

Status SingleKvStoreImpl::ForceClose(DistributedDB::KvStoreDelegateManager *kvStoreDelegateManager)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    ZLOGI("start, current openCount is %d.", openCount_);
    std::unique_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr || kvStoreDelegateManager == nullptr) {
        ZLOGW("get nullptr");
        return Status::INVALID_ARGUMENT;
    }
    RemoveAllSyncOperation();
    ZLOGI("start to clean observer");
    std::lock_guard<std::mutex> observerMapLockGuard(observerMapMutex_);
    for (auto observer = observerMap_.begin(); observer != observerMap_.end();) {
        DistributedDB::DBStatus dbStatus = kvStoreNbDelegate_->UnRegisterObserver(observer->second);
        if (dbStatus == DistributedDB::DBStatus::OK) {
            delete observer->second;
            observer = observerMap_.erase(observer);
        } else {
            ZLOGW("UnSubscribeKvStore failed during ForceClose, status %d.", dbStatus);
            return Status::ERROR;
        }
    }
    ZLOGI("start to clean resultset");
    std::lock_guard<std::mutex> lg(storeResultSetMutex_);
    for (auto resultSetPair = storeResultSetMap_.begin(); resultSetPair != storeResultSetMap_.end();) {
        Status status = (resultSetPair->second)->CloseResultSet(kvStoreNbDelegate_);
        if (status != Status::SUCCESS) {
            ZLOGW("CloseResultSet failed during ForceClose, errCode %d", status);
            return status;
        }
        resultSetPair = storeResultSetMap_.erase(resultSetPair);
    }
    DistributedDB::DBStatus status = kvStoreDelegateManager->CloseKvStore(kvStoreNbDelegate_);
    if (status == DistributedDB::DBStatus::OK) {
        kvStoreNbDelegate_ = nullptr;
        ZLOGI("end.");
        return Status::SUCCESS;
    }
    ZLOGI("failed with error code %d.", status);
    return Status::ERROR;
}

Status SingleKvStoreImpl::ReKey(const std::vector<uint8_t> &key)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("delegate is null.");
        return Status::DB_ERROR;
    }
    DistributedDB::CipherPassword password;
    auto status = password.SetValue(key.data(), key.size());
    if (status != DistributedDB::CipherPassword::ErrorCode::OK) {
        ZLOGE("Failed to set the passwd.");
        return Status::DB_ERROR;
    }
    DistributedDB::DBStatus dbStatus;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        dbStatus = kvStoreNbDelegate_->Rekey(password);
    }
    if (dbStatus == DistributedDB::DBStatus::OK) {
        return Status::SUCCESS;
    }
    return Status::ERROR;
}

Status SingleKvStoreImpl::RegisterSyncCallback(sptr<IKvStoreSyncCallback> callback)
{
    syncCallback_ = std::move(callback);
    return Status::SUCCESS;
}

Status SingleKvStoreImpl::UnRegisterSyncCallback()
{
    syncCallback_ = nullptr;
    return Status::SUCCESS;
}

Status SingleKvStoreImpl::PutBatch(const std::vector<Entry> &entries)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("delegate is null.");
        return Status::DB_ERROR;
    }
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }

    // temporary transform.
    std::vector<DistributedDB::Entry> dbEntries;
    for (auto &entry : entries) {
        DistributedDB::Entry dbEntry;

        std::vector<uint8_t> keyData = Constant::TrimCopy<std::vector<uint8_t>>(entry.key.Data());
        if (keyData.size() == 0 || keyData.size() > Constant::MAX_KEY_LENGTH) {
            ZLOGE("invalid key.");
            return Status::INVALID_ARGUMENT;
        }

        dbEntry.key = keyData;
        dbEntry.value = entry.value.Data();
        dbEntries.push_back(dbEntry);
    }
    DistributedDB::DBStatus status;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->PutBatch(dbEntries);
    }
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        std::string errorInfo;
        errorInfo.append(__FUNCTION__).append(": PutBatch failed. ")
            .append("bundleName is ").append(bundleName_);
        DumpHelper::GetInstance().AddErrorInfo(errorInfo);
        ZLOGE("PutBatch failed, distributeddb need recover.");
        return (Import(bundleName_) ? Status::RECOVER_SUCCESS : Status::RECOVER_FAILED);
    }

    if (status == DistributedDB::DBStatus::EKEYREVOKED_ERROR ||
        status == DistributedDB::DBStatus::SECURITY_OPTION_CHECK_ERROR) {
        ZLOGE("delegate PutBatch failed.");
        return Status::SECURITY_LEVEL_ERROR;
    }

    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("delegate PutBatch failed.");
        return Status::DB_ERROR;
    }

    Reporter::GetInstance()->VisitStatistic()->Report({bundleName_, __FUNCTION__});
    return Status::SUCCESS;
}

Status SingleKvStoreImpl::DeleteBatch(const std::vector<Key> &keys)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("delegate is null.");
        return Status::DB_ERROR;
    }
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }

    // temporary transform.
    std::vector<DistributedDB::Key> dbKeys;
    for (auto &key : keys) {
        std::vector<uint8_t> keyData = Constant::TrimCopy<std::vector<uint8_t>>(key.Data());
        if (keyData.size() == 0 || keyData.size() > Constant::MAX_KEY_LENGTH) {
            ZLOGE("invalid key.");
            return Status::INVALID_ARGUMENT;
        }

        DistributedDB::Key keyTmp = keyData;
        dbKeys.push_back(keyTmp);
    }
    DistributedDB::DBStatus status;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->DeleteBatch(dbKeys);
    }
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        std::string errorInfo;
        errorInfo.append(__FUNCTION__).append(": DeleteBatch failed. ")
            .append("bundleName is ").append(bundleName_);
        DumpHelper::GetInstance().AddErrorInfo(errorInfo);
        ZLOGE("DeleteBatch failed, distributeddb need recover.");
        return (Import(bundleName_) ? Status::RECOVER_SUCCESS : Status::RECOVER_FAILED);
    }

    if (status == DistributedDB::DBStatus::EKEYREVOKED_ERROR ||
        status == DistributedDB::DBStatus::SECURITY_OPTION_CHECK_ERROR) {
        ZLOGE("delegate DeleteBatch failed.");
        return Status::SECURITY_LEVEL_ERROR;
    }

    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("delegate DeleteBatch failed.");
        return Status::DB_ERROR;
    }

    Reporter::GetInstance()->VisitStatistic()->Report({bundleName_, __FUNCTION__});
    return Status::SUCCESS;
}

Status SingleKvStoreImpl::StartTransaction()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("delegate is null.");
        return Status::DB_ERROR;
    }
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    DistributedDB::DBStatus status;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->StartTransaction();
    }
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        std::string errorInfo;
        errorInfo.append(__FUNCTION__).append(": StartTransaction failed. ")
            .append("bundleName is ").append(bundleName_);
        DumpHelper::GetInstance().AddErrorInfo(errorInfo);
        ZLOGE("StartTransaction failed, distributeddb need recover.");
        return (Import(bundleName_) ? Status::RECOVER_SUCCESS : Status::RECOVER_FAILED);
    }
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("delegate return error.");
        return Status::DB_ERROR;
    }
    Reporter::GetInstance()->VisitStatistic()->Report({bundleName_, __FUNCTION__});
    return Status::SUCCESS;
}

Status SingleKvStoreImpl::Commit()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("delegate is null.");
        return Status::DB_ERROR;
    }
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    DistributedDB::DBStatus status;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->Commit();
    }
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        std::string errorInfo;
        errorInfo.append(__FUNCTION__).append(": Commit failed. ")
            .append("bundleName is ").append(bundleName_);
        DumpHelper::GetInstance().AddErrorInfo(errorInfo);
        ZLOGE("Commit failed, distributeddb need recover.");
        return (Import(bundleName_) ? Status::RECOVER_SUCCESS : Status::RECOVER_FAILED);
    }
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("delegate return error.");
        return Status::DB_ERROR;
    }

    Reporter::GetInstance()->VisitStatistic()->Report({bundleName_, __FUNCTION__});
    return Status::SUCCESS;
}

Status SingleKvStoreImpl::Rollback()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));

    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        ZLOGE("delegate is null.");
        return Status::DB_ERROR;
    }
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    DistributedDB::DBStatus status;
    {
        DdsTrace trace(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));
        status = kvStoreNbDelegate_->Rollback();
    }
    if (status == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB) {
        std::string errorInfo;
        errorInfo.append(__FUNCTION__).append(": Rollback failed. ")
            .append("bundleName is ").append(bundleName_);
        DumpHelper::GetInstance().AddErrorInfo(errorInfo);
        ZLOGE("Rollback failed, distributeddb need recover.");
        return (Import(bundleName_) ? Status::RECOVER_SUCCESS : Status::RECOVER_FAILED);
    }
    if (status != DistributedDB::DBStatus::OK) {
        ZLOGE("delegate return error.");
        return Status::DB_ERROR;
    }

    Reporter::GetInstance()->VisitStatistic()->Report({bundleName_, __FUNCTION__});
    return Status::SUCCESS;
}

Status SingleKvStoreImpl::Control(KvControlCmd cmd, const KvParam &inputParam, sptr<KvParam> &output)
{
    output = nullptr;
    switch (cmd) {
        case KvControlCmd::SET_SYNC_PARAM: {
            if (inputParam.Size() != sizeof(uint32_t)) {
                return Status::IPC_ERROR;
            }
            uint32_t allowedDelayMs = TransferByteArrayToType<uint32_t>(inputParam.Data());
            ZLOGE("SET_SYNC_PARAM in %{public}d ms", allowedDelayMs);
            if (allowedDelayMs > 0 && allowedDelayMs < KvStoreSyncManager::SYNC_MIN_DELAY_MS) {
                return Status::INVALID_ARGUMENT;
            }
            if (allowedDelayMs > KvStoreSyncManager::SYNC_MAX_DELAY_MS) {
                return Status::INVALID_ARGUMENT;
            }
            defaultSyncDelayMs_ = allowedDelayMs;
            ZLOGE("SET_SYNC_PARAM save %{public}d ms", defaultSyncDelayMs_);
            return Status::SUCCESS;
        }
        case KvControlCmd::GET_SYNC_PARAM: {
            output = new KvParam(TransferTypeToByteArray<uint32_t>(defaultSyncDelayMs_));
            ZLOGE("GET_SYNC_PARAM read %{public}d ms", defaultSyncDelayMs_);
            return Status::SUCCESS;
        }
        default: {
            ZLOGE("control invalid command.");
            return Status::ERROR;
        }
    }
}

void SingleKvStoreImpl::IncreaseOpenCount()
{
    openCount_++;
}

bool SingleKvStoreImpl::Import(const std::string &bundleName) const
{
    ZLOGI("Single KvStoreImpl Import start");
    StoreMetaData metaData;
    metaData.user = deviceAccountId_;
    metaData.bundleName = bundleName;
    metaData.storeId = storeId_;
    metaData.deviceId = DeviceKvStoreImpl::GetLocalDeviceId();
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    return std::make_unique<BackupHandler>()->SingleKvStoreRecover(metaData, kvStoreNbDelegate_);
}

Status SingleKvStoreImpl::SetCapabilityEnabled(bool enabled)
{
    ZLOGD("begin.");
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }

    std::string key;
    std::string devId = DeviceKvStoreImpl::GetLocalDeviceId();
    if (devId.empty()) {
        ZLOGE("get device id empty.");
        return Status::ERROR;
    }

    StrategyMeta params = {devId, deviceAccountId_, Constant::DEFAULT_GROUP_ID, bundleName_, storeId_};
    KvStoreMetaManager::GetInstance().GetStrategyMetaKey(params, key);
    if (key.empty()) {
        ZLOGE("get key empty.");
        return Status::ERROR;
    }
    ZLOGD("end.");
    return KvStoreMetaManager::GetInstance().SaveStrategyMetaEnable(key, enabled);
}

Status SingleKvStoreImpl::SetCapabilityRange(const std::vector<std::string> &localLabels,
                                             const std::vector<std::string> &remoteSupportLabels)
{
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }

    std::string key;
    std::string devId = DeviceKvStoreImpl::GetLocalDeviceId();
    if (devId.empty()) {
        ZLOGE("get device id empty.");
        return Status::ERROR;
    }

    StrategyMeta params = {devId, deviceAccountId_, Constant::DEFAULT_GROUP_ID, bundleName_, storeId_};
    KvStoreMetaManager::GetInstance().GetStrategyMetaKey(params, key);
    if (key.empty()) {
        ZLOGE("get key empty.");
        return Status::ERROR;
    }

    return KvStoreMetaManager::GetInstance().SaveStrategyMetaLabels(key, localLabels, remoteSupportLabels);
}

Status SingleKvStoreImpl::GetSecurityLevel(SecurityLevel &securityLevel)
{
    std::shared_lock<std::shared_mutex> lock(storeNbDelegateMutex_);
    if (kvStoreNbDelegate_ == nullptr) {
        return Status::STORE_NOT_OPEN;
    }

    DistributedDB::SecurityOption option;
    auto status = kvStoreNbDelegate_->GetSecurityOption(option);
    if (status == DistributedDB::DBStatus::NOT_SUPPORT) {
        return Status::NOT_SUPPORT;
    }

    if (status != DistributedDB::DBStatus::OK) {
        return Status::DB_ERROR;
    }

    switch (option.securityLabel) {
        case DistributedDB::NOT_SET:
        case DistributedDB::S0:
        case DistributedDB::S1:
        case DistributedDB::S2:
            securityLevel = static_cast<SecurityLevel>(option.securityLabel);
            break;
        case DistributedDB::S3:
            securityLevel = option.securityFlag ? S3 : S3_EX;
            break;
        case DistributedDB::S4:
            securityLevel = S4;
            break;
        default:
            break;
    }
    return Status::SUCCESS;
}

void SingleKvStoreImpl::OnDump(int fd) const
{
    auto query = DistributedDB::Query::Select();
    query.PrefixKey({ });
    int count = 0;
    kvStoreNbDelegate_->GetCount(query, count);
    dprintf(fd, DEFAUL_RETRACT"------------------------------------------------------\n");
    dprintf(fd, DEFAUL_RETRACT"StoreID    : %s\n", storeId_.c_str());
    dprintf(fd, DEFAUL_RETRACT"StorePath  : %s\n", storePath_.c_str());

    dprintf(fd, DEFAUL_RETRACT"Options :\n");
    dprintf(fd, DEFAUL_RETRACT"    backup          : %d\n", static_cast<int>(options_.backup));
    dprintf(fd, DEFAUL_RETRACT"    encrypt         : %d\n", static_cast<int>(options_.encrypt));
    dprintf(fd, DEFAUL_RETRACT"    autoSync        : %d\n", static_cast<int>(options_.autoSync));
    dprintf(fd, DEFAUL_RETRACT"    persistent      : %d\n", static_cast<int>(options_.persistent));
    dprintf(fd, DEFAUL_RETRACT"    kvStoreType     : %d\n", static_cast<int>(options_.kvStoreType));
    dprintf(fd, DEFAUL_RETRACT"    createIfMissing : %d\n", static_cast<int>(options_.createIfMissing));
    dprintf(fd, DEFAUL_RETRACT"    schema          : %s\n", options_.schema.c_str());
    dprintf(fd, DEFAUL_RETRACT"    entriesCount    : %d\n", count);
}

void SingleKvStoreImpl::DumpStoreName(int fd) const
{
    dprintf(fd, DEFAUL_RETRACT"------------------------------------------------------\n");
    dprintf(fd, DEFAUL_RETRACT"StoreID    : %s\n", storeId_.c_str());
}

std::string SingleKvStoreImpl::GetStoreId()
{
    return storeId_;
}

std::string SingleKvStoreImpl::GetStorePath() const
{
    return storePath_;
}

void SingleKvStoreImpl::SetCompatibleIdentify(const std::string &changedDevice)
{
    bool flag = false;
    auto capability = UpgradeManager::GetInstance().GetCapability(changedDevice, flag);
    if (!flag || capability.version >= CapMetaData::CURRENT_VERSION) {
        ZLOGE("get peer capability %{public}d, or not older version", flag);
        return;
    }

    auto peerUserId = 0; // peer user id reversed here
    auto groupType =
        AuthDelegate::GetInstance()->GetGroupType(std::stoi(deviceAccountId_), peerUserId, changedDevice, appId_);
    flag = false;
    std::string compatibleUserId = UpgradeManager::GetIdentifierByType(groupType, flag);
    if (!flag) {
        ZLOGE("failed to get identifier by group type %{public}d", groupType);
        return;
    }
    // older version use bound account syncIdentifier instead of user syncIdentifier
    ZLOGI("compatible user:%{public}s", compatibleUserId.c_str());
    auto syncIdentifier =
        DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier(compatibleUserId, appId_, storeId_);
    kvStoreNbDelegate_->SetEqualIdentifier(syncIdentifier, { changedDevice });
}

void SingleKvStoreImpl::SetCompatibleIdentify()
{
    KvStoreTuple tuple = { deviceAccountId_, appId_, storeId_ };
    UpgradeManager::SetCompatibleIdentifyByType(kvStoreNbDelegate_, tuple, IDENTICAL_ACCOUNT_GROUP);
    UpgradeManager::SetCompatibleIdentifyByType(kvStoreNbDelegate_, tuple, PEER_TO_PEER_GROUP);
}
}  // namespace OHOS::DistributedKv
