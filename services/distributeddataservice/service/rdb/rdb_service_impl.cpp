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

#define LOG_TAG "RdbServiceImpl"

#include "rdb_service_impl.h"
#include "account_delegate.h"
#include "checker/checker_manager.h"
#include "communication_provider.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "kvstore_meta_manager.h"

using OHOS::DistributedKv::AccountDelegate;
using OHOS::DistributedKv::KvStoreMetaManager;
using OHOS::DistributedKv::MetaData;
using OHOS::AppDistributedKv::CommunicationProvider;
using OHOS::DistributedData::CheckerManager;
using DistributedDB::RelationalStoreManager;

namespace OHOS::DistributedRdb {
RdbServiceImpl::DeathRecipientImpl::DeathRecipientImpl(const DeathCallback& callback)
    : callback_(callback)
{
    ZLOGI("construct");
}

RdbServiceImpl::DeathRecipientImpl::~DeathRecipientImpl()
{
    ZLOGI("destroy");
}

void RdbServiceImpl::DeathRecipientImpl::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    ZLOGI("enter");
    if (callback_) {
        callback_();
    }
}

RdbServiceImpl::RdbServiceImpl()
    : timer_("SyncerTimer", -1), autoLaunchObserver_(this)
{
    ZLOGI("construct");
    timer_.Setup();
    DistributedDB::RelationalStoreManager::SetAutoLaunchRequestCallback(
        [this](const std::string& identifier, DistributedDB::AutoLaunchParam &param) {
            return ResolveAutoLaunch(identifier, param);
        });
}

bool RdbServiceImpl::ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param)
{
    std::string identifierHex = TransferStringToHex(identifier);
    ZLOGI("%{public}.6s", identifierHex.c_str());
    std::map<std::string, MetaData> entries;
    if (!KvStoreMetaManager::GetInstance().GetFullMetaData(entries, KvStoreMetaManager::RDB)) {
        ZLOGE("get meta failed");
        return false;
    }
    ZLOGI("size=%{public}d", static_cast<int32_t>(entries.size()));

    for (const auto& [key, entry] : entries) {
        auto aIdentifier = DistributedDB::RelationalStoreManager::GetRelationalStoreIdentifier(
            entry.kvStoreMetaData.deviceAccountId, entry.kvStoreMetaData.appId, entry.kvStoreMetaData.storeId);
        ZLOGI("%{public}s %{public}s %{public}s", entry.kvStoreMetaData.deviceAccountId.c_str(),
              entry.kvStoreMetaData.appId.c_str(), entry.kvStoreMetaData.storeId.c_str());
        if (aIdentifier != identifier) {
            continue;
        }
        ZLOGI("find identifier %{public}s", entry.kvStoreMetaData.storeId.c_str());
        param.userId = entry.kvStoreMetaData.deviceAccountId;
        param.appId = entry.kvStoreMetaData.appId;
        param.storeId = entry.kvStoreMetaData.storeId;
        param.path = entry.kvStoreMetaData.dataDir;
        param.option.storeObserver = &autoLaunchObserver_;
        return true;
    }

    ZLOGE("not find identifier");
    return false;
}

void RdbServiceImpl::OnClientDied(pid_t pid)
{
    ZLOGI("client dead pid=%{public}d", pid);
    syncers_.ComputeIfPresent(pid, [this](const auto& key, StoreSyncersType& syncers) {
        syncerNum_ -= static_cast<int32_t>(syncers.size());
        for (const auto& [name, syncer] : syncers) {
            timer_.Unregister(syncer->GetTimerId());
        }
        syncers_.Erase(key);
    });
    notifiers_.Erase(pid);
    identifiers_.EraseIf([pid](const auto& key, pid_t& value) {
        return pid == value;
    });
}

bool RdbServiceImpl::CheckAccess(const RdbSyncerParam &param)
{
    return !CheckerManager::GetInstance().GetAppId(param.bundleName_, GetCallingUid()).empty();
}

std::string RdbServiceImpl::ObtainDistributedTableName(const std::string &device, const std::string &table)
{
    ZLOGI("device=%{public}.6s table=%{public}s", device.c_str(), table.c_str());
    auto uuid = AppDistributedKv::CommunicationProvider::GetInstance().GetUuidByNodeId(device);
    if (uuid.empty()) {
        ZLOGE("get uuid failed");
        return "";
    }
    return DistributedDB::RelationalStoreManager::GetDistributedTableName(uuid, table);
}

int32_t RdbServiceImpl::InitNotifier(const RdbSyncerParam& param, const sptr<IRemoteObject> notifier)
{
    if (!CheckAccess(param)) {
        ZLOGE("permission error");
        return RDB_ERROR;
    }
    
    pid_t pid = GetCallingPid();
    auto recipient = new(std::nothrow) DeathRecipientImpl([this, pid] {
        OnClientDied(pid);
    });
    if (recipient == nullptr) {
        ZLOGE("malloc recipient failed");
        return RDB_ERROR;
    }
    
    if (!notifier->AddDeathRecipient(recipient)) {
        ZLOGE("link to death failed");
        return RDB_ERROR;
    }
    notifiers_.Insert(pid, iface_cast<RdbNotifierProxy>(notifier));
    ZLOGI("success pid=%{public}d", pid);

    return RDB_OK;
}

void RdbServiceImpl::OnDataChange(pid_t pid, const DistributedDB::StoreChangedData &data)
{
    DistributedDB::StoreProperty property;
    data.GetStoreProperty(property);
    ZLOGI("%{public}d %{public}s", pid, property.storeId.c_str());
    if (pid == 0) {
        auto identifier = RelationalStoreManager::GetRelationalStoreIdentifier(property.userId, property.appId,
                                                                               property.storeId);
        auto pair = identifiers_.Find(TransferStringToHex(identifier));
        if (!pair.first) {
            ZLOGI("client doesn't subscribe");
            return;
        }
        pid = pair.second;
        ZLOGI("fixed pid=%{public}d", pid);
    }
    notifiers_.ComputeIfPresent(pid, [&data, &property] (const auto& key, const sptr<RdbNotifierProxy>& value) {
        std::string device = data.GetDataChangeDevice();
        auto networkId = CommunicationProvider::GetInstance().ToNodeId(device);
        value->OnChange(property.storeId, { networkId });
    });
}

void RdbServiceImpl::SyncerTimeout(std::shared_ptr<RdbSyncer> syncer)
{
    ZLOGI("%{public}s", syncer->GetStoreId().c_str());
    syncers_.ComputeIfPresent(syncer->GetPid(), [this, &syncer](const auto& key, StoreSyncersType& syncers) {
        syncers.erase(syncer->GetStoreId());
        syncerNum_--;
    });
}

std::shared_ptr<RdbSyncer> RdbServiceImpl::GetRdbSyncer(const RdbSyncerParam &param)
{
    if (!CheckAccess(param)) {
        ZLOGE("permission error");
        return nullptr;
    }
    
    pid_t pid = GetCallingPid();
    pid_t uid = GetCallingUid();
    std::shared_ptr<RdbSyncer> syncer;

    syncers_.Compute(pid, [this, &param, pid, uid, &syncer] (const auto& key, StoreSyncersType& syncers) {
        auto it = syncers.find(param.storeName_);
        if (it != syncers.end()) {
            syncer = it->second;
            timer_.Unregister(syncer->GetTimerId());
            uint32_t timerId = timer_.Register([this, syncer]() { SyncerTimeout(syncer); }, SYNCER_TIMEOUT, true);
            syncer->SetTimerId(timerId);
            return true;
        }
        if (syncers.size() >= MAX_SYNCER_PER_PROCESS) {
            ZLOGE("%{public}d exceed MAX_PROCESS_SYNCER_NUM", pid);
            return false;
        }
        if (syncerNum_ >= MAX_SYNCER_NUM) {
            ZLOGE("no available syncer");
            return false;
        }
        auto syncer_ = std::make_shared<RdbSyncer>(param, new (std::nothrow) RdbStoreObserverImpl(this, pid));
        if (syncer_->Init(pid, uid) != 0) {
            return false;
        }
        syncers[param.storeName_] = syncer_;
        syncer = syncer_;
        syncerNum_++;
        uint32_t timerId = timer_.Register([this, syncer]() { SyncerTimeout(syncer); }, SYNCER_TIMEOUT, true);
        syncer->SetTimerId(timerId);
        return true;
    });

    if (syncer != nullptr) {
        identifiers_.Insert(syncer->GetIdentifier(), pid);
    } else {
        ZLOGE("syncer is nullptr");
    }
    return syncer;
}

int32_t RdbServiceImpl::SetDistributedTables(const RdbSyncerParam &param, const std::vector<std::string> &tables)
{
    ZLOGI("enter");
    auto syncer = GetRdbSyncer(param);
    if (syncer == nullptr) {
        return RDB_ERROR;
    }
    return syncer->SetDistributedTables(tables);
}

int32_t RdbServiceImpl::DoSync(const RdbSyncerParam &param, const SyncOption &option,
                               const RdbPredicates &predicates, SyncResult &result)
{
    auto syncer = GetRdbSyncer(param);
    if (syncer == nullptr) {
        return RDB_ERROR;
    }
    return syncer->DoSync(option, predicates, result);
}

void RdbServiceImpl::OnAsyncComplete(pid_t pid, uint32_t seqNum, const SyncResult &result)
{
    ZLOGI("pid=%{public}d seqnum=%{public}u", pid, seqNum);
    notifiers_.ComputeIfPresent(pid, [seqNum, &result] (const auto& key, const sptr<RdbNotifierProxy>& value) {
        value->OnComplete(seqNum, result);
    });
}

int32_t RdbServiceImpl::DoAsync(const RdbSyncerParam &param, uint32_t seqNum, const SyncOption &option,
                                const RdbPredicates &predicates)
{
    pid_t pid = GetCallingPid();
    ZLOGI("seq num=%{public}u", seqNum);
    auto syncer = GetRdbSyncer(param);
    if (syncer == nullptr) {
        return RDB_ERROR;
    }
    return syncer->DoAsync(option, predicates,
                           [this, pid, seqNum] (const SyncResult& result) {
                               OnAsyncComplete(pid, seqNum, result);
                           });
}

std::string RdbServiceImpl::TransferStringToHex(const std::string &origStr)
{
    if (origStr.empty()) {
        return "";
    }
    const char *hex = "0123456789abcdef";
    std::string tmp;
    for (auto item : origStr) {
        auto currentByte = static_cast<uint8_t>(item);
        tmp.push_back(hex[currentByte >> 4]); // high 4 bit to one hex.
        tmp.push_back(hex[currentByte & 0x0F]); // low 4 bit to one hex.
    }
    return tmp;
}

std::string RdbServiceImpl::GenIdentifier(const RdbSyncerParam &param)
{
    pid_t uid = GetCallingUid();
    std::string userId = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(uid);
    std::string appId = CheckerManager::GetInstance().GetAppId(param.bundleName_, uid);
    std::string identifier = RelationalStoreManager::GetRelationalStoreIdentifier(userId, appId, param.storeName_);
    return TransferStringToHex(identifier);
}

int32_t RdbServiceImpl::DoSubscribe(const RdbSyncerParam& param)
{
    pid_t pid = GetCallingPid();
    auto identifier = GenIdentifier(param);
    ZLOGI("%{public}s %{public}.6s %{public}d", param.storeName_.c_str(), identifier.c_str(), pid);
    identifiers_.Insert(identifier, pid);
    return RDB_OK;
}

int32_t RdbServiceImpl::DoUnSubscribe(const RdbSyncerParam& param)
{
    auto identifier = GenIdentifier(param);
    ZLOGI("%{public}s %{public}.6s", param.storeName_.c_str(), identifier.c_str());
    identifiers_.Erase(identifier);
    return RDB_OK;
}
} // namespace OHOS::DistributedRdb
