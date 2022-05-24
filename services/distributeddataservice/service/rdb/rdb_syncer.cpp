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

#define LOG_TAG "RdbSyncer"

#include "rdb_syncer.h"

#include "account_delegate.h"
#include "checker/checker_manager.h"
#include "log_print.h"
#include "kvstore_utils.h"
#include "kvstore_meta_manager.h"

using OHOS::DistributedKv::KvStoreUtils;
using OHOS::DistributedKv::AccountDelegate;
using OHOS::DistributedKv::KvStoreMetaManager;
using OHOS::AppDistributedKv::CommunicationProvider;

namespace OHOS::DistributedRdb {
RdbSyncer::RdbSyncer(const RdbSyncerParam& param, RdbStoreObserverImpl* observer)
    : param_(param), observer_(observer)
{
    ZLOGI("construct %{public}s", param_.storeName_.c_str());
}

RdbSyncer::~RdbSyncer() noexcept
{
    ZLOGI("destroy %{public}s", param_.storeName_.c_str());
    if ((manager_ != nullptr) && (delegate_ != nullptr)) {
        manager_->CloseStore(delegate_);
    }
    delete manager_;
    if (observer_ != nullptr) {
        delete observer_;
    }
}

void RdbSyncer::SetTimerId(uint32_t timerId)
{
    timerId_ = timerId;
}

uint32_t RdbSyncer::GetTimerId() const
{
    return timerId_;
}

pid_t RdbSyncer::GetPid() const
{
    return pid_;
}

std::string RdbSyncer::GetIdentifier() const
{
    return DistributedDB::RelationalStoreManager::GetRelationalStoreIdentifier(GetUserId(), GetAppId(), GetStoreId());
}

std::string RdbSyncer::GetUserId() const
{
    return AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(uid_);
}

std::string RdbSyncer::GetBundleName() const
{
    return param_.bundleName_;
}

std::string RdbSyncer::GetAppId() const
{
    return DistributedData::CheckerManager::GetInstance().GetAppId(param_.bundleName_, uid_);
}

std::string RdbSyncer::GetPath() const
{
    return param_.realPath_;
}

std::string RdbSyncer::GetStoreId() const
{
    return param_.storeName_;
}

int32_t RdbSyncer::Init(pid_t pid, pid_t uid)
{
    ZLOGI("enter");
    pid_ = pid;
    uid_ = uid;
    if (CreateMetaData() != RDB_OK) {
        ZLOGE("create meta data failed");
        return RDB_ERROR;
    }
    ZLOGI("success");
    return RDB_OK;
}

int32_t RdbSyncer::CreateMetaData()
{
    DistributedKv::KvStoreMetaData meta;
    meta.kvStoreType =static_cast<DistributedKv::KvStoreType>(RDB_DEVICE_COLLABORATION);
    meta.appId = GetAppId();
    meta.appType = "harmony";
    meta.bundleName = GetBundleName();
    meta.deviceId = CommunicationProvider::GetInstance().GetLocalDevice().deviceId;
    meta.storeId = GetStoreId();
    meta.userId = AccountDelegate::GetInstance()->GetCurrentAccountId(GetBundleName());
    meta.uid = uid_;
    meta.deviceAccountId = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(uid_);
    meta.dataDir = GetPath();

    std::string metaString = meta.Marshal();
    std::vector<uint8_t> metaValue(metaString.begin(), metaString.end());
    auto metaKey = KvStoreMetaManager::GetMetaKey(meta.deviceAccountId, "default", meta.bundleName, meta.storeId);
    DistributedKv::KvStoreMetaData oldMeta;
    if (KvStoreMetaManager::GetInstance().GetKvStoreMeta(metaKey, oldMeta) == DistributedKv::Status::SUCCESS) {
        if (metaString == oldMeta.Marshal()) {
            ZLOGI("ignore same meta");
            return RDB_OK;
        }
    }
    auto status = KvStoreMetaManager::GetInstance().CheckUpdateServiceMeta(metaKey, DistributedKv::UPDATE, metaValue);
    return static_cast<int32_t>(status);
}

DistributedDB::RelationalStoreDelegate* RdbSyncer::GetDelegate()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (manager_ == nullptr) {
        manager_ = new(std::nothrow) DistributedDB::RelationalStoreManager(GetAppId(), GetUserId());
    }
    if (manager_ == nullptr) {
        ZLOGE("malloc manager failed");
        return nullptr;
    }

    if (delegate_ == nullptr) {
        DistributedDB::RelationalStoreDelegate::Option option;
        option.observer = observer_;
        ZLOGI("path=%{public}s storeId=%{public}s", GetPath().c_str(), GetStoreId().c_str());
        DistributedDB::DBStatus status = manager_->OpenStore(GetPath(), GetStoreId(), option, delegate_);
        if (status != DistributedDB::DBStatus::OK) {
            ZLOGE("open store failed status=%{public}d", status);
            return nullptr;
        }
        ZLOGI("open store success");
    }

    return delegate_;
}

int32_t RdbSyncer::SetDistributedTables(const std::vector<std::string> &tables)
{
    auto* delegate = GetDelegate();
    if (delegate == nullptr) {
        ZLOGE("delegate is nullptr");
        return RDB_ERROR;
    }
    
    for (const auto& table : tables) {
        ZLOGI("%{public}s", table.c_str());
        if (delegate->CreateDistributedTable(table) != DistributedDB::DBStatus::OK) {
            ZLOGE("create distributed table failed");
            return RDB_ERROR;
        }
    }
    ZLOGE("create distributed table success");
    return RDB_OK;
}

std::vector<std::string> RdbSyncer::GetConnectDevices()
{
    auto deviceInfos = AppDistributedKv::CommunicationProvider::GetInstance().GetRemoteNodesBasicInfo();
    std::vector<std::string> devices;
    for (const auto& deviceInfo : deviceInfos) {
        devices.push_back(deviceInfo.deviceId);
    }
    ZLOGI("size=%{public}u", static_cast<uint32_t>(devices.size()));
    for (const auto& device: devices) {
        ZLOGI("%{public}s", KvStoreUtils::ToBeAnonymous(device).c_str());
    }
    return devices;
}

std::vector<std::string> RdbSyncer::NetworkIdToUUID(const std::vector<std::string> &networkIds)
{
    std::vector<std::string> uuids;
    for (const auto& networkId : networkIds) {
        auto uuid = CommunicationProvider::GetInstance().GetUuidByNodeId(networkId);
        if (uuid.empty()) {
            ZLOGE("%{public}s failed", KvStoreUtils::ToBeAnonymous(networkId).c_str());
            continue;
        }
        uuids.push_back(uuid);
        ZLOGI("%{public}s <--> %{public}s", KvStoreUtils::ToBeAnonymous(networkId).c_str(),
              KvStoreUtils::ToBeAnonymous(uuid).c_str());
    }
    return uuids;
}

void RdbSyncer::HandleSyncStatus(const std::map<std::string, std::vector<DistributedDB::TableStatus>> &syncStatus,
                                 SyncResult &result)
{
    for (const auto& status : syncStatus) {
        auto res = DistributedDB::DBStatus::OK;
        for (const auto& tableStatus : status.second) {
            if (tableStatus.status != DistributedDB::DBStatus::OK) {
                res = tableStatus.status;
                break;
            }
        }
        std::string uuid = CommunicationProvider::GetInstance().ToNodeId(status.first);
        if (uuid.empty()) {
            ZLOGE("%{public}.6s failed", status.first.c_str());
            continue;
        }
        ZLOGI("%{public}.6s=%{public}d", uuid.c_str(), res);
        result[uuid] = res;
    }
}
void RdbSyncer::EqualTo(const RdbPredicateOperation &operation, DistributedDB::Query &query)
{
    query.EqualTo(operation.field_, operation.values_[0]);
    ZLOGI("field=%{public}s value=%{public}s", operation.field_.c_str(), operation.values_[0].c_str());
}

void RdbSyncer::NotEqualTo(const RdbPredicateOperation &operation, DistributedDB::Query &query)
{
    query.NotEqualTo(operation.field_, operation.values_[0]);
    ZLOGI("field=%{public}s value=%{public}s", operation.field_.c_str(), operation.values_[0].c_str());
}

void RdbSyncer::And(const RdbPredicateOperation &operation, DistributedDB::Query &query)
{
    query.And();
    ZLOGI("");
}

void RdbSyncer::Or(const RdbPredicateOperation &operation, DistributedDB::Query &query)
{
    query.Or();
    ZLOGI("");
}

void RdbSyncer::OrderBy(const RdbPredicateOperation &operation, DistributedDB::Query &query)
{
    bool isAsc = operation.values_[0] == "true";
    query.OrderBy(operation.field_, isAsc);
    ZLOGI("field=%{public}s isAsc=%{public}s", operation.field_.c_str(), operation.values_[0].c_str());
}

void RdbSyncer::Limit(const RdbPredicateOperation &operation, DistributedDB::Query &query)
{
    char *end = nullptr;
    int limit = static_cast<int>(strtol(operation.field_.c_str(), &end, DECIMAL_BASE));
    int offset = static_cast<int>(strtol(operation.values_[0].c_str(), &end, DECIMAL_BASE));
    if (limit < 0) {
        limit = 0;
    }
    if (offset < 0) {
        offset = 0;
    }
    query.Limit(limit, offset);
    ZLOGI("limit=%{public}d offset=%{public}d", limit, offset);
}

DistributedDB::Query RdbSyncer::MakeQuery(const RdbPredicates &predicates)
{
    ZLOGI("table=%{public}s", predicates.table_.c_str());
    auto query = DistributedDB::Query::Select(predicates.table_);
    for (const auto& operation : predicates.operations_) {
        if (operation.operator_ >= 0 && operation.operator_ < OPERATOR_MAX) {
            HANDLES[operation.operator_](operation, query);
        }
    }
    return query;
}

int32_t RdbSyncer::DoSync(const SyncOption &option, const RdbPredicates &predicates, SyncResult &result)
{
    ZLOGI("enter");
    auto* delegate = GetDelegate();
    if (delegate == nullptr) {
        ZLOGE("delegate is nullptr");
        return RDB_ERROR;
    }

    std::vector<std::string> devices;
    if (predicates.devices_.empty()) {
        devices = NetworkIdToUUID(GetConnectDevices());
    } else {
        devices = NetworkIdToUUID(predicates.devices_);
    }

    ZLOGI("delegate sync");
    return delegate->Sync(devices, static_cast<DistributedDB::SyncMode>(option.mode),
                          MakeQuery(predicates), [&result] (const auto& syncStatus) {
                              HandleSyncStatus(syncStatus, result);
                          }, true);
}

int32_t RdbSyncer::DoAsync(const SyncOption &option, const RdbPredicates &predicates, const SyncCallback& callback)
{
    auto* delegate = GetDelegate();
    if (delegate == nullptr) {
        ZLOGE("delegate is nullptr");
        return RDB_ERROR;
    }

    std::vector<std::string> devices;
    if (predicates.devices_.empty()) {
        devices = NetworkIdToUUID(GetConnectDevices());
    } else {
        devices = NetworkIdToUUID(predicates.devices_);
    }

    ZLOGI("delegate sync");
    return delegate->Sync(devices, static_cast<DistributedDB::SyncMode>(option.mode),
                          MakeQuery(predicates), [callback] (const auto& syncStatus) {
                              SyncResult result;
                              HandleSyncStatus(syncStatus, result);
                              callback(result);
                          }, false);
}
}