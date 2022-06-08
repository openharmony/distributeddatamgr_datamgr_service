/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define LOG_TAG "SingleStoreImpl"
#include "single_store_impl.h"

#include "dds_trace.h"
#include "dev_manager.h"
#include "kvdb_service_client.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "store_result_set.h"
#include "store_util.h"
namespace OHOS::DistributedKv {
SingleStoreImpl::SingleStoreImpl(const AppId &appId, std::shared_ptr<DBStore> dbStore)
    : appId_(appId.appId), dbStore_(std::move(dbStore))
{
    storeId_ = dbStore_->GetStoreId();
    syncObserver_ = std::make_shared<SyncObserver>();
}

StoreId SingleStoreImpl::GetStoreId() const
{
    return { storeId_ };
}

Status SingleStoreImpl::Put(const Key &key, const Value &value)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", storeId_.c_str());
        return ALREADY_CLOSED;
    }

    DBKey dbKey = ToLocalDBKey(key);
    if (dbKey.empty()) {
        ZLOGE("invalid key:%{public}s, size:%{public}zu", StoreUtil::Anonymous(key.ToString()).c_str(), key.Size());
        return INVALID_ARGUMENT;
    }

    auto dbStatus = dbStore_->Put(dbKey, value);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, key:%{public}s, value size:%{public}zu", status,
            StoreUtil::Anonymous(key.ToString()).c_str(), value.Size());
    }
    // do auto sync process
    return status;
}

Status SingleStoreImpl::PutBatch(const std::vector<Entry> &entries)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", storeId_.c_str());
        return ALREADY_CLOSED;
    }

    std::vector<DBEntry> dbEntries;
    for (const auto &entry : entries) {
        DBEntry dbEntry;
        dbEntry.key = ToLocalDBKey(entry.key);
        if (dbEntry.key.empty()) {
            ZLOGE("invalid key:%{public}s, size:%{public}zu", StoreUtil::Anonymous(entry.key.ToString()).c_str(),
                entry.key.Size());
            return INVALID_ARGUMENT;
        }
        dbEntry.value = entry.value;
        dbEntries.push_back(std::move(dbEntry));
    }
    auto dbStatus = dbStore_->PutBatch(dbEntries);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, entries size:%{public}zu", status, entries.size());
    }
    // do auto sync process
    return status;
}

Status SingleStoreImpl::Delete(const Key &key)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", storeId_.c_str());
        return ALREADY_CLOSED;
    }

    DBKey dbKey = ToLocalDBKey(key);
    if (dbKey.empty()) {
        ZLOGE("invalid key:%{public}s, size:%{public}zu", StoreUtil::Anonymous(key.ToString()).c_str(), key.Size());
        return INVALID_ARGUMENT;
    }

    auto dbStatus = dbStore_->Delete(dbKey);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, key:%{public}s", status, StoreUtil::Anonymous(key.ToString()).c_str());
    }
    // do auto sync process
    return status;
}

Status SingleStoreImpl::DeleteBatch(const std::vector<Key> &keys)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", storeId_.c_str());
        return ALREADY_CLOSED;
    }

    std::vector<DBKey> dbKeys;
    for (const auto &key : keys) {
        DBKey dbKey = ToLocalDBKey(key);
        if (dbKey.empty()) {
            ZLOGE("invalid key:%{public}s, size:%{public}zu", StoreUtil::Anonymous(key.ToString()).c_str(), key.Size());
            return INVALID_ARGUMENT;
        }
        dbKeys.push_back(std::move(dbKey));
    }

    auto dbStatus = dbStore_->DeleteBatch(dbKeys);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, keys size:%{public}zu", status, keys.size());
    }
    // do auto sync process
    return status;
}

Status SingleStoreImpl::StartTransaction()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", storeId_.c_str());
        return ALREADY_CLOSED;
    }

    auto dbStatus = dbStore_->StartTransaction();
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x storeId:%{public}s", status, storeId_.c_str());
    }
    return status;
}

Status SingleStoreImpl::Commit()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", storeId_.c_str());
        return ALREADY_CLOSED;
    }

    auto dbStatus = dbStore_->Commit();
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x storeId:%{public}s", status, dbStore_->GetStoreId().c_str());
    }
    return status;
}

Status SingleStoreImpl::Rollback()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", storeId_.c_str());
        return ALREADY_CLOSED;
    }

    auto dbStatus = dbStore_->Rollback();
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x storeId:%{public}s", status, storeId_.c_str());
    }
    return status;
}

Status SingleStoreImpl::SubscribeKvStore(SubscribeType type, std::shared_ptr<Observer> observer)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", storeId_.c_str());
        return ALREADY_CLOSED;
    }

    if (observer == nullptr) {
        ZLOGE("invalid observer is null");
        return INVALID_ARGUMENT;
    }

    uint32_t realType = type;
    std::shared_ptr<ObserverBridge> bridge = nullptr;
    observers_.Compute(uintptr_t(observer.get()),
        [this, &realType, observer, &bridge](const auto &, std::pair<uint32_t, std::shared_ptr<ObserverBridge>> &pair) {
            if ((pair.first & realType) == realType) {
                return (pair.first != 0);
            }
            if (pair.first == 0) {
                auto release = BridgeReleaser();
                StoreId storeId{ storeId_ };
                AppId appId{ appId_ };
                pair.second = std::shared_ptr<ObserverBridge>(
                    new ObserverBridge(appId, storeId, observer, GetConvert()), release);
            }
            bridge = pair.second;
            realType = (realType & (~pair.first));
            pair.first = pair.first | realType;
            return (pair.first != 0);
        });

    if (bridge == nullptr) {
        return STORE_ALREADY_SUBSCRIBE;
    }

    Status status = SUCCESS;
    if ((realType & SUBSCRIBE_TYPE_LOCAL) == SUBSCRIBE_TYPE_LOCAL) {
        auto dbStatus = dbStore_->RegisterObserver({}, DistributedDB::OBSERVER_CHANGES_NATIVE, bridge.get());
        status = StoreUtil::ConvertStatus(dbStatus);
    }

    if ((realType & SUBSCRIBE_TYPE_REMOTE) == SUBSCRIBE_TYPE_REMOTE) {
        bridge->RegisterRemoteObserver();
    }

    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, type:%{public}d, observer:0x%x", status, type, StoreUtil::Anonymous(bridge.get()));
    }
    return status;
}

Status SingleStoreImpl::UnSubscribeKvStore(SubscribeType type, std::shared_ptr<Observer> observer)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", storeId_.c_str());
        return ALREADY_CLOSED;
    }

    if (observer == nullptr) {
        ZLOGE("invalid observer is null");
        return INVALID_ARGUMENT;
    }

    uint32_t realType = type;
    std::shared_ptr<ObserverBridge> bridge = nullptr;
    observers_.ComputeIfPresent(uintptr_t(observer.get()),
        [&realType, observer, &bridge](const auto &, std::pair<uint32_t, std::shared_ptr<ObserverBridge>> &pair) {
            if ((pair.first & realType) == 0) {
                return (pair.first != 0);
            }
            realType = (realType & pair.first);
            pair.first = (pair.first & (~realType));
            bridge = pair.second;
            return (pair.first != 0);
        });

    if (bridge == nullptr) {
        return STORE_NOT_SUBSCRIBE;
    }

    Status status = SUCCESS;
    if ((realType & SUBSCRIBE_TYPE_LOCAL) == SUBSCRIBE_TYPE_LOCAL) {
        auto dbStatus = dbStore_->UnRegisterObserver(bridge.get());
        status = StoreUtil::ConvertStatus(dbStatus);
    }

    if ((realType & SUBSCRIBE_TYPE_REMOTE) == SUBSCRIBE_TYPE_REMOTE) {
        bridge->UnregisterRemoteObserver();
    }

    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, type:%{public}d, observer:0x%x", status, type, StoreUtil::Anonymous(bridge.get()));
    }
    return status;
}

Status SingleStoreImpl::Get(const Key &key, Value &value)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", storeId_.c_str());
        return ALREADY_CLOSED;
    }

    DBKey dbKey = ToWholeDBKey(key);
    if (dbKey.empty()) {
        ZLOGE("invalid key:%{public}s, size:%{public}zu", key.ToString().c_str(), key.Size());
        return INVALID_ARGUMENT;
    }

    DBValue dbValue;
    auto dbStatus = dbStore_->Get(dbKey, dbValue);
    value = std::move(dbValue);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, key:%{public}s", status, key.ToString().c_str());
    }
    return status;
}

Status SingleStoreImpl::GetEntries(const Key &prefix, std::vector<Entry> &entries) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    DBKey dbPrefix = GetPrefix(prefix);
    if (dbPrefix.empty() && !prefix.Empty()) {
        ZLOGE("invalid prefix:%{public}s, size:%{public}zu", prefix.ToString().c_str(), prefix.Size());
        return INVALID_ARGUMENT;
    }

    DBQuery dbQuery = DBQuery::Select();
    dbQuery.PrefixKey(dbPrefix);
    auto status = GetEntries(dbQuery, entries);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, prefix:%{public}s", status, prefix.ToString().c_str());
    }
    return status;
}

Status SingleStoreImpl::GetEntries(const DataQuery &query, std::vector<Entry> &entries) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    DBQuery dbQuery = *(query.query_);
    dbQuery.PrefixKey(GetPrefix(query));
    auto status = GetEntries(dbQuery, entries);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, query:%{public}s", status, query.ToString().c_str());
    }
    return status;
}

Status SingleStoreImpl::GetResultSet(const Key &prefix, std::shared_ptr<ResultSet> &resultSet) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    DBKey dbPrefix = GetPrefix(prefix);
    if (dbPrefix.empty() && !prefix.Empty()) {
        ZLOGE("invalid prefix:%{public}s, size:%{public}zu", prefix.ToString().c_str(), prefix.Size());
        return INVALID_ARGUMENT;
    }

    DBQuery dbQuery = DistributedDB::Query::Select();
    dbQuery.PrefixKey(dbPrefix);
    auto status = GetResultSet(dbQuery, resultSet);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, prefix:%{public}s", status, prefix.ToString().c_str());
    }
    return status;
}

Status SingleStoreImpl::GetResultSet(const DataQuery &query, std::shared_ptr<ResultSet> &resultSet) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    DBQuery dbQuery = *(query.query_);
    dbQuery.PrefixKey(GetPrefix(query));
    auto status = GetResultSet(dbQuery, resultSet);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, query:%{public}s", status, query.ToString().c_str());
    }
    return status;
}

Status SingleStoreImpl::CloseResultSet(std::shared_ptr<ResultSet> &resultSet)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    if (resultSet == nullptr) {
        ZLOGE("input is nullptr");
        return INVALID_ARGUMENT;
    }

    auto status = resultSet->Close();
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x storeId:%{public}s", status, storeId_.c_str());
    }
    resultSet = nullptr;
    return status;
}

Status SingleStoreImpl::GetCount(const DataQuery &query, int &result) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", storeId_.c_str());
        return ALREADY_CLOSED;
    }

    DBQuery dbQuery = *(query.query_);
    dbQuery.PrefixKey(GetPrefix(query));
    auto dbStatus = dbStore_->GetCount(dbQuery, result);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x query:%{public}s", status, query.ToString().c_str());
    }
    return status;
}

Status SingleStoreImpl::GetSecurityLevel(SecurityLevel &secLevel) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", storeId_.c_str());
        return ALREADY_CLOSED;
    }

    DistributedDB::SecurityOption option;
    auto dbStatus = dbStore_->GetSecurityOption(option);
    secLevel = static_cast<SecurityLevel>(StoreUtil::GetSecLevel(option));
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, security:[%{public}d, %{public}d]", status, option.securityFlag,
            option.securityLabel);
    }
    return status;
}

Status SingleStoreImpl::RemoveDeviceData(const std::string &device)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", storeId_.c_str());
        return ALREADY_CLOSED;
    }

    auto dbStatus = dbStore_->RemoveDeviceData(DevManager::GetInstance().ToUUID(device));
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status != SUCCESS) {
        ZLOGE("status:0x%{public}x, device:%{public}s", status, StoreUtil::Anonymous(device).c_str());
    }
    return status;
}

Status SingleStoreImpl::Sync(const std::vector<std::string> &devices, SyncMode mode, uint32_t allowedDelayMs)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    KVDBService::SyncInfo syncInfo;
    syncInfo.seqId = KvStoreUtils::GenerateSequenceId();
    syncInfo.mode = mode;
    syncInfo.delay = mode;
    return DoSync(syncInfo, syncObserver_);
}

Status SingleStoreImpl::Sync(const std::vector<std::string> &devices, SyncMode mode, const DataQuery &query,
    std::shared_ptr<SyncCallback> syncCallback)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    KVDBService::SyncInfo syncInfo;
    syncInfo.seqId = KvStoreUtils::GenerateSequenceId();
    syncInfo.mode = mode;
    syncInfo.devices = devices;
    syncInfo.query = query.ToString();
    return DoSync(syncInfo, syncCallback);
}

Status SingleStoreImpl::RegisterSyncCallback(std::shared_ptr<SyncCallback> callback)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);
    if (callback == nullptr) {
        ZLOGW("INVALID_ARGUMENT.");
        return INVALID_ARGUMENT;
    }
    syncObserver_->Add(callback);
    return SUCCESS;
}

Status SingleStoreImpl::UnRegisterSyncCallback()
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);
    syncObserver_->Clean();
    return SUCCESS;
}

Status SingleStoreImpl::SetSyncParam(const KvSyncParam &syncParam)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), true);
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    return service->SetSyncParam({ appId_ }, { storeId_ }, syncParam);
}

Status SingleStoreImpl::GetSyncParam(KvSyncParam &syncParam)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), true);
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    return service->GetSyncParam({ appId_ }, { storeId_ }, syncParam);
}

Status SingleStoreImpl::SetCapabilityEnabled(bool enabled) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), true);
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    if (enabled) {
        return service->EnableCapability({ appId_ }, { storeId_ });
    }
    return service->DisableCapability({ appId_ }, { storeId_ });
}

Status SingleStoreImpl::SetCapabilityRange(
    const std::vector<std::string> &localLabels, const std::vector<std::string> &remoteLabels) const
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), true);
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    return service->SetCapability({ appId_ }, { storeId_ }, localLabels, remoteLabels);
}

Status SingleStoreImpl::SubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), true);
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    return service->AddSubscribeInfo({ appId_ }, { storeId_ }, devices, query.ToString());
}

Status SingleStoreImpl::UnsubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__), true);
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    return service->RmvSubscribeInfo({ appId_ }, { storeId_ }, devices, query.ToString());
}

Status SingleStoreImpl::Close()
{
    observers_.Clear();
    syncObserver_->Clean();
    std::shared_ptr<KVDBServiceClient> service;
    {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        service = (syncCallback_ != nullptr) ? KVDBServiceClient::GetInstance() : nullptr;
        syncCallback_ = nullptr;
    }
    if (service != nullptr) {
        service->UnregisterSyncCallback({ appId_ }, { storeId_ });
    }
    std::unique_lock<decltype(rwMutex_)> lock(rwMutex_);
    dbStore_ = nullptr;
    return SUCCESS;
}

std::vector<uint8_t> SingleStoreImpl::ToLocalDBKey(const Key &key) const
{
    return GetPrefix(key);
}

std::vector<uint8_t> SingleStoreImpl::ToWholeDBKey(const Key &key) const
{
    return GetPrefix(key);
}

Key SingleStoreImpl::ToKey(DBKey &&key) const
{
    return std::move(key);
}

std::function<void(ObserverBridge *)> SingleStoreImpl::BridgeReleaser()
{
    return [this](ObserverBridge *obj) {
        if (obj == nullptr) {
            return;
        }
        Status status = ALREADY_CLOSED;
        {
            std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
            if (dbStore_ != nullptr) {
                auto dbStatus = dbStore_->UnRegisterObserver(obj);
                status = StoreUtil::ConvertStatus(dbStatus);
            }
        }
        Status remote = obj->UnregisterRemoteObserver();
        if (status != SUCCESS || remote != SUCCESS) {
            ZLOGE("status:0x%{public}x remote:0x%{public}x observer:0x%{public}x", status, remote,
                StoreUtil::Anonymous(obj));
        }

        delete obj;
    };
}

Status SingleStoreImpl::GetResultSet(const DistributedDB::Query &query, std::shared_ptr<ResultSet> &resultSet) const
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", storeId_.c_str());
        return ALREADY_CLOSED;
    }

    DistributedDB::KvStoreResultSet *dbResultSet = nullptr;
    auto status = dbStore_->GetEntries(query, dbResultSet);
    if (dbResultSet == nullptr) {
        return StoreUtil::ConvertStatus(status);
    }
    resultSet = std::make_shared<StoreResultSet>(dbResultSet, dbStore_, GetConvert());
    return SUCCESS;
}

Status SingleStoreImpl::GetEntries(const DistributedDB::Query &query, std::vector<Entry> &entries) const
{
    std::shared_lock<decltype(rwMutex_)> lock(rwMutex_);
    if (dbStore_ == nullptr) {
        ZLOGE("db:%{public}s already closed!", storeId_.c_str());
        return ALREADY_CLOSED;
    }

    std::vector<DBEntry> dbEntries;
    auto dbStatus = dbStore_->GetEntries(query, dbEntries);
    entries.resize(dbEntries.size());
    auto it = entries.begin();
    for (auto &dbEntry : dbEntries) {
        auto &entry = *it;
        entry.key = ToKey(std::move(dbEntry.key));
        entry.value = std::move(dbEntry.value);
        ++it;
    }
    return StoreUtil::ConvertStatus(dbStatus);
}

std::vector<uint8_t> SingleStoreImpl::GetPrefix(const Key &prefix) const
{
    auto begin = std::find_if(prefix.Data().begin(), prefix.Data().end(), [](int ch) { return !std::isspace(ch); });
    auto rBegin = std::find_if(prefix.Data().rbegin(), prefix.Data().rend(), [](int ch) { return !std::isspace(ch); });
    auto end = static_cast<decltype(begin)>(rBegin.base());
    std::vector<uint8_t> dbKey;
    dbKey.assign(begin, end);
    if (dbKey.size() >= MAX_KEY_LENGTH) {
        dbKey.clear();
    }
    return dbKey;
}

std::vector<uint8_t> SingleStoreImpl::GetPrefix(const DataQuery &query) const
{
    return GetPrefix(Key(query.prefix_));
}

SingleStoreImpl::Convert SingleStoreImpl::GetConvert() const
{
    return [](const DBKey &key, std::string &deviceId) {
        deviceId = "";
        return Key(key);
    };
}

Status SingleStoreImpl::DoSync(const SyncInfo &syncInfo, std::shared_ptr<SyncCallback> observer)
{
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }

    auto syncAgent = service->GetSyncAgent({ appId_ });
    if (syncAgent == nullptr) {
        ZLOGE("failed! invalid agent app:%{public}s, store:%{public}s!", appId_.c_str(), storeId_.c_str());
        return ILLEGAL_STATE;
    }
    syncAgent->AddSyncCallback(observer, syncInfo.seqId);
    return service->Sync({ appId_ }, { storeId_ }, syncInfo);
}
} // namespace OHOS::DistributedKv