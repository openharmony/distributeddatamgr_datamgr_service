/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "KVDBServiceImpl"
#include "kvdb_service_impl.h"

#include <chrono>
#include <cinttypes>

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "auto_sync_matrix.h"
#include "backup_manager.h"
#include "checker/checker_manager.h"
#include "cloud/change_event.h"
#include "cloud/cloud_server.h"
#include "communication_provider.h"
#include "communicator_context.h"
#include "crypto_manager.h"
#include "device_manager_adapter.h"
#include "directory/directory_manager.h"
#include "dump/dump_manager.h"
#include "eventcenter/event_center.h"
#include "ipc_skeleton.h"
#include "kvdb_general_store.h"
#include "kvdb_query.h"
#include "log_print.h"
#include "matrix_event.h"
#include "metadata/appid_meta_data.h"
#include "metadata/capability_meta_data.h"
#include "metadata/switches_meta_data.h"
#include "permit_delegate.h"
#include "query_helper.h"
#include "store/store_info.h"
#include "upgrade.h"
#include "utils/anonymous.h"
#include "utils/constant.h"
#include "utils/converter.h"
#include "water_version_manager.h"

namespace OHOS::DistributedKv {
using namespace OHOS::DistributedData;
using namespace OHOS::AppDistributedKv;
using namespace OHOS::Security::AccessToken;
using system_clock = std::chrono::system_clock;
using DMAdapter = DistributedData::DeviceManagerAdapter;
using DumpManager = OHOS::DistributedData::DumpManager;
using CommContext = OHOS::DistributedData::CommunicatorContext;
__attribute__((used)) KVDBServiceImpl::Factory KVDBServiceImpl::factory_;
KVDBServiceImpl::Factory::Factory()
{
    FeatureSystem::GetInstance().RegisterCreator("kv_store", [this]() {
        if (product_ == nullptr) {
            product_ = std::make_shared<KVDBServiceImpl>();
        }
        return product_;
    });
    auto creator = [](const StoreMetaData &metaData) -> GeneralStore* {
        auto store = new (std::nothrow) KVDBGeneralStore(metaData);
        if (store != nullptr && !store->IsValid()) {
            delete store;
            store = nullptr;
        }
        return store;
    };
    AutoCache::GetInstance().RegCreator(KvStoreType::SINGLE_VERSION, creator);
    AutoCache::GetInstance().RegCreator(KvStoreType::DEVICE_COLLABORATION, creator);
}

KVDBServiceImpl::Factory::~Factory()
{
    product_ = nullptr;
}

KVDBServiceImpl::KVDBServiceImpl()
{
    EventCenter::GetInstance().Subscribe(DeviceMatrix::MATRIX_META_FINISHED, [this](const Event &event) {
        auto &matrixEvent = static_cast<const MatrixEvent &>(event);
        auto deviceId = matrixEvent.GetDeviceId();
        auto refCount = matrixEvent.StealRefCount();
        std::vector<StoreMetaData> metaData;
        auto prefix = StoreMetaData::GetPrefix({ DMAdapter::GetInstance().GetLocalDevice().uuid });
        if (!MetaDataManager::GetInstance().LoadMeta(prefix, metaData)) {
            ZLOGE("load meta failed!");
            return;
        }

        uint16_t mask = matrixEvent.GetMatrixData().statics;
        for (const auto &data : metaData) {
            if (data.dataType != DataType::TYPE_STATICS) {
                OldOnlineSync(data, deviceId, refCount, matrixEvent.GetMatrixData().dynamic);
                continue;
            }
            auto code = DeviceMatrix::GetInstance().GetCode(data);
            if ((code == DeviceMatrix::INVALID_MASK) || (mask & code) != code) {
                OldOnlineSync(data, deviceId, refCount, matrixEvent.GetMatrixData().dynamic);
                continue;
            }
            SyncInfo syncInfo;
            syncInfo.mode = GetSyncMode(true, IsRemoteChange(data, deviceId));
            syncInfo.delay = 0;
            syncInfo.devices = { deviceId };
            ZLOGI("[online] appId:%{public}s, storeId:%{public}s", data.bundleName.c_str(),
                Anonymous::Change(data.storeId).c_str());
            KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(data.tokenId), syncInfo.delay,
                std::bind(&KVDBServiceImpl::DoSync, this, data, syncInfo, std::placeholders::_1, ACTION_SYNC),
                std::bind(&KVDBServiceImpl::DoComplete, this, data, syncInfo, refCount, std::placeholders::_1));
        }
    });
}

void KVDBServiceImpl::OldOnlineSync(
    const StoreMetaData &data, const std::string &deviceId, RefCount refCount, uint16_t mask)
{
    StoreMetaDataLocal localMetaData;
    MetaDataManager::GetInstance().LoadMeta(data.GetKeyLocal(), localMetaData, true);
    if (!localMetaData.HasPolicy(PolicyType::IMMEDIATE_SYNC_ON_ONLINE)) {
        return;
    }

    auto code = DeviceMatrix::GetInstance().GetCode(data);
    if ((mask & code) != code) {
        return;
    }

    auto policy = localMetaData.GetPolicy(PolicyType::IMMEDIATE_SYNC_ON_ONLINE);
    SyncInfo syncInfo;
    syncInfo.mode = PUSH_PULL;
    syncInfo.delay = 0;
    syncInfo.devices = { deviceId };
    if (policy.IsValueEffect()) {
        syncInfo.delay = policy.valueUint;
    }
    ZLOGI("[online old] appId:%{public}s, storeId:%{public}s", data.bundleName.c_str(),
        Anonymous::Change(data.storeId).c_str());
    auto delay = GetSyncDelayTime(syncInfo.delay, { data.storeId });
    KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(data.tokenId), delay,
        std::bind(&KVDBServiceImpl::DoSync, this, data, syncInfo, std::placeholders::_1, ACTION_SYNC),
        std::bind(&KVDBServiceImpl::DoComplete, this, data, syncInfo, refCount, std::placeholders::_1));
}

KVDBServiceImpl::~KVDBServiceImpl()
{
    DumpManager::GetInstance().RemoveHandler("FEATURE_INFO", uintptr_t(this));
}

void KVDBServiceImpl::Init()
{
    auto process = [this](const Event &event) {
        const auto &evt = static_cast<const CloudEvent &>(event);
        const auto &storeInfo = evt.GetStoreInfo();
        StoreMetaData meta;
        meta.storeId = storeInfo.storeName;
        meta.bundleName = storeInfo.bundleName;
        meta.user = std::to_string(storeInfo.user);
        meta.deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
        if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true)) {
            if (meta.user == "0") {
                ZLOGE("meta empty, bundleName:%{public}s, storeId:%{public}s, user = %{public}s",
                    meta.bundleName.c_str(), meta.GetStoreAlias().c_str(), meta.user.c_str());
                return;
            }
            meta.user = "0";
            StoreMetaDataLocal localMeta;
            if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKeyLocal(), localMeta, true) || !localMeta.isPublic ||
                !MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true)) {
                ZLOGE("meta empty, not public store. bundleName:%{public}s, storeId:%{public}s, user = %{public}s",
                    meta.bundleName.c_str(), meta.GetStoreAlias().c_str(), meta.user.c_str());
                return;
            }
        }
        if (meta.storeType < StoreMetaData::StoreType::STORE_KV_BEGIN ||
            meta.storeType > StoreMetaData::StoreType::STORE_KV_END) {
            return;
        }
        auto watchers = GetWatchers(meta.tokenId, meta.storeId);
        auto store = AutoCache::GetInstance().GetStore(meta, watchers);
        if (store == nullptr) {
            ZLOGE("store null, storeId:%{public}s", meta.GetStoreAlias().c_str());
            return;
        }
        store->RegisterDetailProgressObserver(nullptr);
    };
    EventCenter::GetInstance().Subscribe(CloudEvent::CLOUD_SYNC, process);
}

void KVDBServiceImpl::RegisterKvServiceInfo()
{
    OHOS::DistributedData::DumpManager::Config serviceInfoConfig;
    serviceInfoConfig.fullCmd = "--feature-info";
    serviceInfoConfig.abbrCmd = "-f";
    serviceInfoConfig.dumpName = "FEATURE_INFO";
    serviceInfoConfig.dumpCaption = { "| Display all the service statistics" };
    DumpManager::GetInstance().AddConfig("FEATURE_INFO", serviceInfoConfig);
}

void KVDBServiceImpl::RegisterHandler()
{
    Handler handler =
        std::bind(&KVDBServiceImpl::DumpKvServiceInfo, this, std::placeholders::_1, std::placeholders::_2);
    DumpManager::GetInstance().AddHandler("FEATURE_INFO", uintptr_t(this), handler);
}

void KVDBServiceImpl::DumpKvServiceInfo(int fd, std::map<std::string, std::vector<std::string>> &params)
{
    (void)params;
    std::string info;
    dprintf(fd, "-------------------------------------KVDBServiceInfo------------------------------\n%s\n",
        info.c_str());
}

Status KVDBServiceImpl::GetStoreIds(const AppId &appId, std::vector<StoreId> &storeIds)
{
    std::vector<StoreMetaData> metaData;
    auto user = AccountDelegate::GetInstance()->GetUserByToken(IPCSkeleton::GetCallingTokenID());
    auto deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    auto prefix = StoreMetaData::GetPrefix({ deviceId, std::to_string(user), "default", appId.appId });
    auto instanceId = GetInstIndex(IPCSkeleton::GetCallingTokenID(), appId);
    MetaDataManager::GetInstance().LoadMeta(prefix, metaData, true);
    for (auto &item : metaData) {
        if (item.storeType > KvStoreType::MULTI_VERSION || item.instanceId != instanceId) {
            continue;
        }
        storeIds.push_back({ item.storeId });
    }
    ZLOGD("appId:%{public}s store size:%{public}zu", appId.appId.c_str(), storeIds.size());
    return SUCCESS;
}

Status KVDBServiceImpl::Delete(const AppId &appId, const StoreId &storeId)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    if (metaData.instanceId < 0) {
        return ILLEGAL_STATE;
    }

    auto tokenId = IPCSkeleton::GetCallingTokenID();
    syncAgents_.ComputeIfPresent(tokenId, [&appId, &storeId](auto &key, SyncAgent &syncAgent) {
        if (syncAgent.pid_ != IPCSkeleton::GetCallingPid()) {
            ZLOGW("agent already changed! old pid:%{public}d new pid:%{public}d appId:%{public}s",
                IPCSkeleton::GetCallingPid(), syncAgent.pid_, appId.appId.c_str());
            return true;
        }
        syncAgent.delayTimes_.erase(storeId);
        return true;
    });
    MetaDataManager::GetInstance().DelMeta(metaData.GetKey());
    MetaDataManager::GetInstance().DelMeta(metaData.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(metaData.GetSecretKey(), true);
    MetaDataManager::GetInstance().DelMeta(metaData.GetStrategyKey());
    MetaDataManager::GetInstance().DelMeta(metaData.GetKeyLocal(), true);
    PermitDelegate::GetInstance().DelCache(metaData.GetKey());
    AutoCache::GetInstance().CloseStore(tokenId, storeId);
    ZLOGD("appId:%{public}s storeId:%{public}s instanceId:%{public}d", appId.appId.c_str(),
        Anonymous::Change(storeId.storeId).c_str(), metaData.instanceId);
    return SUCCESS;
}

Status KVDBServiceImpl::Close(const AppId &appId, const StoreId &storeId)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    if (metaData.instanceId < 0) {
        return ILLEGAL_STATE;
    }
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    AutoCache::GetInstance().CloseStore(tokenId, storeId);
    ZLOGD("appId:%{public}s storeId:%{public}s instanceId:%{public}d", appId.appId.c_str(),
        Anonymous::Change(storeId.storeId).c_str(), metaData.instanceId);
    return SUCCESS;
}

Status KVDBServiceImpl::CloudSync(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    if (!MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData)) {
        ZLOGE("invalid, appId:%{public}s storeId:%{public}s",
            appId.appId.c_str(), Anonymous::Change(storeId.storeId).c_str());
        return Status::INVALID_ARGUMENT;
    }
    return DoCloudSync(metaData, syncInfo);
}

void KVDBServiceImpl::OnAsyncComplete(uint32_t tokenId, uint64_t seqNum, ProgressDetail &&detail)
{
    ZLOGI("tokenId=%{public}x seqnum=%{public}" PRIu64, tokenId, seqNum);
    auto [success, agent] = syncAgents_.Find(tokenId);
    if (success && agent.notifier_ != nullptr) {
        agent.notifier_->SyncCompleted(seqNum, std::move(detail));
    }
}

Status KVDBServiceImpl::Sync(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    auto delay = GetSyncDelayTime(syncInfo.delay, storeId);
    if (metaData.isAutoSync && syncInfo.seqId == std::numeric_limits<uint64_t>::max()) {
        DeviceMatrix::GetInstance().OnChanged(metaData);
        StoreMetaDataLocal localMeta;
        MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyLocal(), localMeta, true);
        if (!localMeta.HasPolicy(IMMEDIATE_SYNC_ON_CHANGE)) {
            ZLOGW("appId:%{public}s storeId:%{public}s no IMMEDIATE_SYNC_ON_CHANGE ", appId.appId.c_str(),
                Anonymous::Change(storeId.storeId).c_str());
            return Status::SUCCESS;
        }
    }
    return KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(metaData.tokenId), delay,
        std::bind(&KVDBServiceImpl::DoSyncInOrder, this, metaData, syncInfo, std::placeholders::_1, ACTION_SYNC),
        std::bind(&KVDBServiceImpl::DoComplete, this, metaData, syncInfo, RefCount(), std::placeholders::_1));
}

Status KVDBServiceImpl::SyncExt(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    if (syncInfo.devices.empty()) {
        return Status::INVALID_ARGUMENT;
    }
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    auto device = DMAdapter::GetInstance().ToUUID(syncInfo.devices[0]);
    if (device.empty()) {
        ZLOGE("invalid deviceId, appId:%{public}s storeId:%{public}s deviceId:%{public}s",
            metaData.bundleName.c_str(), Anonymous::Change(metaData.storeId).c_str(),
            Anonymous::Change(syncInfo.devices[0]).c_str());
        return Status::INVALID_ARGUMENT;
    }

    if (!IsRemoteChange(metaData, device)) {
        ZLOGD("no change, do not need sync, appId:%{public}s storeId:%{public}s",
            metaData.bundleName.c_str(), Anonymous::Change(metaData.storeId).c_str());
        DBResult dbResult = { {syncInfo.devices[0], DBStatus::OK} };
        DoComplete(metaData, syncInfo, RefCount(), std::move(dbResult));
        return SUCCESS;
    }
    return KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(metaData.tokenId), 0,
        std::bind(&KVDBServiceImpl::DoSyncInOrder, this, metaData, syncInfo, std::placeholders::_1, ACTION_SYNC),
        std::bind(&KVDBServiceImpl::DoComplete, this, metaData, syncInfo, RefCount(), std::placeholders::_1));
}

Status KVDBServiceImpl::NotifyDataChange(const AppId &appId, const StoreId &storeId)
{
    StoreMetaData meta = GetStoreMetaData(appId, storeId);
    if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta)) {
        ZLOGE("invalid, appId:%{public}s storeId:%{public}s",
            appId.appId.c_str(), Anonymous::Change(storeId.storeId).c_str());
        return Status::INVALID_ARGUMENT;
    }
    if (DeviceMatrix::GetInstance().IsStatics(meta) || DeviceMatrix::GetInstance().IsDynamic(meta)) {
        WaterVersionManager::GetInstance().GenerateWaterVersion(meta.bundleName, meta.storeId);
        DeviceMatrix::GetInstance().OnChanged(meta);
        DoCloudSync(meta, {});
        return SUCCESS;
    }
    DoCloudSync(meta, {});
    TryToSync(meta, true);
    return SUCCESS;
}

void KVDBServiceImpl::TryToSync(const StoreMetaData &metaData, bool force)
{
    auto devices = DMAdapter::ToUUID(DMAdapter::GetInstance().GetRemoteDevices());
    if (devices.empty()) {
        return;
    }
    SyncInfo syncInfo;
    syncInfo.mode = SyncMode::PUSH;
    for (const auto &device : devices) {
        if (!force && (!CommContext::GetInstance().IsSessionReady(device) ||
            !DMAdapter::GetInstance().IsDeviceReady(device))) {
            continue;
        }
        syncInfo.devices = { device };
        KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(metaData.tokenId), 0,
            std::bind(&KVDBServiceImpl::DoSyncInOrder, this, metaData, syncInfo, std::placeholders::_1, ACTION_SYNC),
            std::bind(&KVDBServiceImpl::DoComplete, this, metaData, syncInfo, RefCount(), std::placeholders::_1));
    }
}

Status KVDBServiceImpl::PutSwitch(const AppId &appId, const SwitchData &data)
{
    if (data.value == DeviceMatrix::INVALID_VALUE || data.length == DeviceMatrix::INVALID_LENGTH) {
        return Status::INVALID_ARGUMENT;
    }
    auto deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    SwitchesMetaData oldMeta;
    oldMeta.deviceId = deviceId;
    bool exist = MetaDataManager::GetInstance().LoadMeta(oldMeta.GetKey(), oldMeta, true);
    SwitchesMetaData newMeta;
    newMeta.value = data.value;
    newMeta.length = data.length;
    newMeta.deviceId = deviceId;
    if (!exist || newMeta != oldMeta) {
        bool success = MetaDataManager::GetInstance().SaveMeta(newMeta.GetKey(), newMeta, true);
        if (success) {
            ZLOGI("start broadcast swicthes data");
            DeviceMatrix::DataLevel level = {
                .switches = data.value,
                .switchesLen = data.length,
            };
            DeviceMatrix::GetInstance().Broadcast(level);
        }
    }
    ZLOGI("appId:%{public}s, exist:%{public}d, saved:%{public}d", appId.appId.c_str(), exist, newMeta != oldMeta);
    return Status::SUCCESS;
}

Status KVDBServiceImpl::GetSwitch(const AppId &appId, const std::string &networkId, SwitchData &data)
{
    auto uuid = DMAdapter::GetInstance().ToUUID(networkId);
    if (uuid.empty()) {
        return Status::INVALID_ARGUMENT;
    }
    SwitchesMetaData meta;
    meta.deviceId = uuid;
    if (!MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true)) {
        return Status::NOT_FOUND;
    }
    data.value = meta.value;
    data.length = meta.length;
    return Status::SUCCESS;
}

Status KVDBServiceImpl::RegServiceNotifier(const AppId &appId, sptr<IKVDBNotifier> notifier)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    syncAgents_.Compute(tokenId, [&appId, notifier](const auto &, SyncAgent &value) {
        if (value.pid_ != IPCSkeleton::GetCallingPid()) {
            value.ReInit(IPCSkeleton::GetCallingPid(), appId);
        }
        value.notifier_ = notifier;
        return true;
    });
    if (!DeviceMatrix::GetInstance().IsSupportMatrix()) {
        ZLOGD("not support matrix");
        return Status::SUCCESS;
    }
    StoreMetaData meta;
    meta.appId = appId.appId;
    meta.tokenId = tokenId;
    meta.uid = IPCSkeleton::GetCallingUid();
    meta.bundleName = appId.appId;
    meta.dataType = DataType::TYPE_DYNAMICAL;
    uint16_t code = DeviceMatrix::GetInstance().GetCode(meta);
    if (!DeviceMatrix::GetInstance().IsDynamic(meta) || code == DeviceMatrix::INVALID_MASK) {
        return Status::SUCCESS;
    }
    std::map<std::string, bool> clientMask;
    auto masks = DeviceMatrix::GetInstance().GetRemoteDynamicMask();
    for (const auto &[device, mask] : masks) {
        auto networkId = DMAdapter::GetInstance().ToNetworkID(device);
        if (networkId.empty()) {
            continue;
        }
        bool changed = ((mask & code) == code);
        if (changed) {
            clientMask.insert_or_assign(networkId, changed);
        }
    }
    notifier->OnRemoteChange(std::move(clientMask));
    return Status::SUCCESS;
}

void KVDBServiceImpl::RegisterMatrixChange()
{
    if (!DeviceMatrix::GetInstance().IsSupportMatrix()) {
        ZLOGD("not support matrix");
        return;
    }
    DeviceMatrix::GetInstance().RegRemoteChange([this](const std::string &device, uint16_t mask) {
        auto networkId = DMAdapter::GetInstance().ToNetworkID(device);
        if (networkId.empty()) {
            return;
        }
        syncAgents_.ForEachCopies([networkId, mask](const auto &key, auto &value) {
            StoreMetaData meta;
            meta.appId = value.appId_;
            meta.tokenId = key;
            meta.bundleName = value.appId_;
            meta.dataType = DataType::TYPE_DYNAMICAL;
            if (!DeviceMatrix::GetInstance().IsDynamic(meta)) {
                return false;
            }
            uint16_t code = DeviceMatrix::GetInstance().GetCode(meta);
            std::map<std::string, bool> clientMask;
            bool changed = ((mask & code) == code);
            if ((value.changed_ && changed) || (!value.changed_ && !changed)) {
                return false;
            }
            value.changed_ = changed;
            clientMask.insert_or_assign(networkId, changed);
            if (value.notifier_) {
                value.notifier_->OnRemoteChange(std::move(clientMask));
            }
            return false;
        });
    });
}

Status KVDBServiceImpl::UnregServiceNotifier(const AppId &appId)
{
    syncAgents_.ComputeIfPresent(IPCSkeleton::GetCallingTokenID(), [&appId](const auto &key, SyncAgent &value) {
        if (value.pid_ != IPCSkeleton::GetCallingPid()) {
            ZLOGW("agent already changed! old pid:%{public}d, new pid:%{public}d, appId:%{public}s",
                IPCSkeleton::GetCallingPid(), value.pid_, appId.appId.c_str());
            return true;
        }
        value.notifier_ = nullptr;
        return true;
    });
    return SUCCESS;
}

Status KVDBServiceImpl::SubscribeSwitchData(const AppId &appId)
{
    sptr<IKVDBNotifier> notifier = nullptr;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    syncAgents_.Compute(tokenId, [&appId, &notifier](const auto &, SyncAgent &value) {
        if (value.pid_ != IPCSkeleton::GetCallingPid()) {
            value.ReInit(IPCSkeleton::GetCallingPid(), appId);
        }
        if (value.switchesObserverCount_ == 0) {
            notifier = value.notifier_;
        }
        value.switchesObserverCount_++;
        return true;
    });
    if (notifier == nullptr) {
        return SUCCESS;
    }
    bool success = MetaDataManager::GetInstance().Subscribe(SwitchesMetaData::GetPrefix({}),
        [this, notifier](const std::string &key, const std::string &meta, int32_t action) {
            SwitchesMetaData metaData;
            if (!SwitchesMetaData::Unmarshall(meta, metaData)) {
                ZLOGE("unmarshall matrix meta failed, action:%{public}d", action);
                return true;
            }
            auto networkId = DMAdapter::GetInstance().ToNetworkID(metaData.deviceId);
            SwitchNotification notification;
            notification.deviceId = std::move(networkId);
            notification.data.value = metaData.value;
            notification.data.length = metaData.length;
            notification.state = ConvertAction(static_cast<Action>(action));
            if (notifier != nullptr) {
                notifier->OnSwitchChange(std::move(notification));
            }
            return true;
        }, true);
    ZLOGI("subscribe switch status:%{public}d", success);
    return SUCCESS;
}

Status KVDBServiceImpl::UnsubscribeSwitchData(const AppId &appId)
{
    bool destroyed = false;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    syncAgents_.ComputeIfPresent(tokenId, [&destroyed](auto &key, SyncAgent &value) {
        if (value.switchesObserverCount_ > 0) {
            value.switchesObserverCount_--;
        }
        if (value.switchesObserverCount_ == 0) {
            destroyed = true;
        }
        return true;
    });
    if (destroyed) {
        bool status = MetaDataManager::GetInstance().Unsubscribe(SwitchesMetaData::GetPrefix({}));
        ZLOGI("unsubscribe switch status %{public}d", status);
    }
    return SUCCESS;
}

ProgressDetail KVDBServiceImpl::HandleGenDetails(const GenDetails &details)
{
    ProgressDetail progressDetail;
    if (details.begin() == details.end()) {
        return {};
    }
    auto genDetail = details.begin()->second;
    progressDetail.progress = genDetail.progress;
    progressDetail.code = genDetail.code;
    auto tableDetails = genDetail.details;
    if (tableDetails.begin() == tableDetails.end()) {
        return progressDetail;
    }
    auto genTableDetail = tableDetails.begin()->second;
    auto &tableDetail = progressDetail.details;
    Constant::Copy(&tableDetail, &genTableDetail);
    return progressDetail;
}

Status KVDBServiceImpl::SetSyncParam(const AppId &appId, const StoreId &storeId, const KvSyncParam &syncParam)
{
    if (syncParam.allowedDelayMs > 0 && syncParam.allowedDelayMs < KvStoreSyncManager::SYNC_MIN_DELAY_MS) {
        return Status::INVALID_ARGUMENT;
    }
    if (syncParam.allowedDelayMs > KvStoreSyncManager::SYNC_MAX_DELAY_MS) {
        return Status::INVALID_ARGUMENT;
    }
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    syncAgents_.Compute(tokenId, [&appId, &storeId, &syncParam](auto &key, SyncAgent &value) {
        if (value.pid_ != IPCSkeleton::GetCallingPid()) {
            value.ReInit(IPCSkeleton::GetCallingPid(), appId);
        }
        value.delayTimes_[storeId] = syncParam.allowedDelayMs;
        return true;
    });
    return SUCCESS;
}

Status KVDBServiceImpl::GetSyncParam(const AppId &appId, const StoreId &storeId, KvSyncParam &syncParam)
{
    syncParam.allowedDelayMs = 0;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    syncAgents_.ComputeIfPresent(tokenId, [&appId, &storeId, &syncParam](auto &key, SyncAgent &value) {
        if (value.pid_ != IPCSkeleton::GetCallingPid()) {
            ZLOGW("agent already changed! old pid:%{public}d, new pid:%{public}d, appId:%{public}s",
                IPCSkeleton::GetCallingPid(), value.pid_, appId.appId.c_str());
            return true;
        }

        auto it = value.delayTimes_.find(storeId);
        if (it != value.delayTimes_.end()) {
            syncParam.allowedDelayMs = it->second;
        }
        return true;
    });
    return SUCCESS;
}

Status KVDBServiceImpl::EnableCapability(const AppId &appId, const StoreId &storeId)
{
    StrategyMeta strategyMeta = GetStrategyMeta(appId, storeId);
    if (strategyMeta.instanceId < 0) {
        return ILLEGAL_STATE;
    }
    MetaDataManager::GetInstance().LoadMeta(strategyMeta.GetKey(), strategyMeta);
    strategyMeta.capabilityEnabled = true;
    MetaDataManager::GetInstance().SaveMeta(strategyMeta.GetKey(), strategyMeta);
    return SUCCESS;
}

Status KVDBServiceImpl::DisableCapability(const AppId &appId, const StoreId &storeId)
{
    StrategyMeta strategyMeta = GetStrategyMeta(appId, storeId);
    if (strategyMeta.instanceId < 0) {
        return ILLEGAL_STATE;
    }
    MetaDataManager::GetInstance().LoadMeta(strategyMeta.GetKey(), strategyMeta);
    strategyMeta.capabilityEnabled = false;
    MetaDataManager::GetInstance().SaveMeta(strategyMeta.GetKey(), strategyMeta);
    return SUCCESS;
}

Status KVDBServiceImpl::SetCapability(const AppId &appId, const StoreId &storeId,
    const std::vector<std::string> &local, const std::vector<std::string> &remote)
{
    StrategyMeta strategy = GetStrategyMeta(appId, storeId);
    if (strategy.instanceId < 0) {
        return ILLEGAL_STATE;
    }
    MetaDataManager::GetInstance().LoadMeta(strategy.GetKey(), strategy);
    strategy.capabilityRange.localLabel = local;
    strategy.capabilityRange.remoteLabel = remote;
    MetaDataManager::GetInstance().SaveMeta(strategy.GetKey(), strategy);
    return SUCCESS;
}

Status KVDBServiceImpl::AddSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    auto delay = GetSyncDelayTime(syncInfo.delay, storeId);
    return KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(metaData.tokenId), delay,
        std::bind(&KVDBServiceImpl::DoSyncInOrder, this, metaData, syncInfo, std::placeholders::_1, ACTION_SUBSCRIBE),
        std::bind(&KVDBServiceImpl::DoComplete, this, metaData, syncInfo, RefCount(), std::placeholders::_1));
}

Status KVDBServiceImpl::RmvSubscribeInfo(const AppId &appId, const StoreId &storeId, const SyncInfo &syncInfo)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData);
    auto delay = GetSyncDelayTime(syncInfo.delay, storeId);
    return KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(metaData.tokenId), delay,
        std::bind(
            &KVDBServiceImpl::DoSyncInOrder, this, metaData, syncInfo, std::placeholders::_1, ACTION_UNSUBSCRIBE),
        std::bind(&KVDBServiceImpl::DoComplete, this, metaData, syncInfo, RefCount(), std::placeholders::_1));
}

Status KVDBServiceImpl::Subscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer)
{
    if (observer == nullptr) {
        return INVALID_ARGUMENT;
    }
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    ZLOGI("appId:%{public}s storeId:%{public}s tokenId:0x%{public}x", appId.appId.c_str(),
        Anonymous::Change(storeId.storeId).c_str(), tokenId);
    bool isCreate = false;
    syncAgents_.Compute(tokenId, [&appId, &storeId, &observer, &isCreate](auto &key, SyncAgent &agent) {
        if (agent.pid_ != IPCSkeleton::GetCallingPid()) {
            agent.ReInit(IPCSkeleton::GetCallingPid(), appId);
        }
        if (agent.watcher_ == nullptr) {
            isCreate = true;
            agent.SetWatcher(std::make_shared<KVDBWatcher>());
        }
        agent.SetObserver(iface_cast<KvStoreObserverProxy>(observer->AsObject()));
        agent.count_++;
        return true;
    });
    if (isCreate) {
        AutoCache::GetInstance().SetObserver(tokenId, storeId, GetWatchers(tokenId, storeId));
    }
    return SUCCESS;
}

Status KVDBServiceImpl::Unsubscribe(const AppId &appId, const StoreId &storeId, sptr<IKvStoreObserver> observer)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    ZLOGI("appId:%{public}s storeId:%{public}s tokenId:0x%{public}x", appId.appId.c_str(),
        Anonymous::Change(storeId.storeId).c_str(), tokenId);
    bool destroyed = false;
    syncAgents_.ComputeIfPresent(tokenId, [&appId, &storeId, &observer, &destroyed](auto &key, SyncAgent &agent) {
        if (agent.count_ > 0) {
            agent.count_--;
        }
        if (agent.count_ == 0) {
            destroyed = true;
            agent.SetWatcher(nullptr);
        }
        return true;
    });
    if (destroyed) {
        AutoCache::GetInstance().SetObserver(tokenId, storeId, GetWatchers(tokenId, storeId));
    }
    return SUCCESS;
}

Status KVDBServiceImpl::GetBackupPassword(const AppId &appId, const StoreId &storeId, std::vector<uint8_t> &password)
{
    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    return (BackupManager::GetInstance().GetPassWord(metaData, password)) ? SUCCESS : ERROR;
}

Status KVDBServiceImpl::BeforeCreate(const AppId &appId, const StoreId &storeId, const Options &options)
{
    ZLOGD("appId:%{public}s storeId:%{public}s to export data", appId.appId.c_str(),
        Anonymous::Change(storeId.storeId).c_str());
    StoreMetaData meta = GetStoreMetaData(appId, storeId);
    AddOptions(options, meta);

    StoreMetaData old;
    auto isCreated = MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), old, true);
    if (!isCreated) {
        return SUCCESS;
    }
    if (old.storeType != meta.storeType || Constant::NotEqual(old.isEncrypt, meta.isEncrypt) ||
        old.area != meta.area || !options.persistent || old.dataType != meta.dataType) {
        ZLOGE("meta appId:%{public}s storeId:%{public}s type:%{public}d->%{public}d encrypt:%{public}d->%{public}d "
              "area:%{public}d->%{public}d persistent:%{public}d dataType:%{public}d->%{public}d",
            appId.appId.c_str(), Anonymous::Change(storeId.storeId).c_str(), old.storeType, meta.storeType,
            old.isEncrypt, meta.isEncrypt, old.area, meta.area, options.persistent, old.dataType, options.dataType);
        return Status::STORE_META_CHANGED;
    }
    if (executors_ != nullptr) {
        DistributedData::StoreInfo storeInfo;
        storeInfo.bundleName = appId.appId;
        storeInfo.instanceId = GetInstIndex(storeInfo.tokenId, appId);
        storeInfo.user = std::atoi(meta.user.c_str());
        executors_->Execute([storeInfo]() {
            auto event = std::make_unique<CloudEvent>(CloudEvent::GET_SCHEMA, storeInfo);
            EventCenter::GetInstance().PostEvent(move(event));
        });
    }

    auto dbStatus = DBStatus::OK;
    if (old != meta) {
        dbStatus = Upgrade::GetInstance().ExportStore(old, meta);
    }
    return dbStatus == DBStatus::OK ? SUCCESS : DB_ERROR;
}

Status KVDBServiceImpl::AfterCreate(const AppId &appId, const StoreId &storeId, const Options &options,
    const std::vector<uint8_t> &password)
{
    if (!appId.IsValid() || !storeId.IsValid() || !options.IsValidType()) {
        ZLOGE("failed please check type:%{public}d appId:%{public}s storeId:%{public}s dataType:%{public}d",
            options.kvStoreType, appId.appId.c_str(), Anonymous::Change(storeId.storeId).c_str(), options.dataType);
        return INVALID_ARGUMENT;
    }

    StoreMetaData metaData = GetStoreMetaData(appId, storeId);
    AddOptions(options, metaData);

    StoreMetaData oldMeta;
    auto isCreated = MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), oldMeta, true);
    Status status = SUCCESS;
    if (isCreated && oldMeta != metaData) {
        auto dbStatus = Upgrade::GetInstance().UpdateStore(oldMeta, metaData, password);
        ZLOGI("update status:%{public}d appId:%{public}s storeId:%{public}s inst:%{public}d "
              "type:%{public}d->%{public}d dir:%{public}s dataType:%{public}d->%{public}d",
            dbStatus, appId.appId.c_str(), Anonymous::Change(storeId.storeId).c_str(), metaData.instanceId,
            oldMeta.storeType, metaData.storeType, metaData.dataDir.c_str(), oldMeta.dataType, metaData.dataType);
        if (dbStatus != DBStatus::OK) {
            status = STORE_UPGRADE_FAILED;
        }
    }

    if (!isCreated || oldMeta != metaData) {
        if (!CheckerManager::GetInstance().IsDistrust(Converter::ConvertToStoreInfo(metaData))) {
            MetaDataManager::GetInstance().SaveMeta(metaData.GetKey(), metaData);
        }
        MetaDataManager::GetInstance().SaveMeta(metaData.GetKey(), metaData, true);
    }
    AppIDMetaData appIdMeta;
    appIdMeta.bundleName = metaData.bundleName;
    appIdMeta.appId = metaData.appId;
    MetaDataManager::GetInstance().SaveMeta(appIdMeta.GetKey(), appIdMeta, true);
    SaveLocalMetaData(options, metaData);
    Upgrade::GetInstance().UpdatePassword(metaData, password);
    ZLOGI("appId:%{public}s storeId:%{public}s instanceId:%{public}d type:%{public}d dir:%{public}s "
        "isCreated:%{public}d dataType:%{public}d", appId.appId.c_str(), Anonymous::Change(storeId.storeId).c_str(),
        metaData.instanceId, metaData.storeType, metaData.dataDir.c_str(), isCreated, metaData.dataType);
    return status;
}

int32_t KVDBServiceImpl::OnAppExit(pid_t uid, pid_t pid, uint32_t tokenId, const std::string &appId)
{
    ZLOGI("pid:%{public}d uid:%{public}d appId:%{public}s", pid, uid, appId.c_str());
    syncAgents_.EraseIf([pid](auto &key, SyncAgent &agent) {
        if (agent.pid_ != pid) {
            return false;
        }
        if (agent.watcher_ != nullptr) {
            agent.watcher_->ClearObservers();
            agent.ClearObservers();
        }
        auto stores = AutoCache::GetInstance().GetStoresIfPresent(key);
        for (auto store : stores) {
            if (store != nullptr) {
                store->UnregisterDetailProgressObserver();
            }
        }
        return true;
    });
    return SUCCESS;
}

int32_t KVDBServiceImpl::ResolveAutoLaunch(const std::string &identifier, DBLaunchParam &param)
{
    ZLOGI("user:%{public}s appId:%{public}s storeId:%{public}s identifier:%{public}s", param.userId.c_str(),
        param.appId.c_str(), Anonymous::Change(param.storeId).c_str(), Anonymous::Change(identifier).c_str());
    std::vector<StoreMetaData> metaData;
    auto prefix = StoreMetaData::GetPrefix({ DMAdapter::GetInstance().GetLocalDevice().uuid, param.userId });
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, metaData)) {
        ZLOGE("no store in user:%{public}s", param.userId.c_str());
        return STORE_NOT_FOUND;
    }

    for (const auto &storeMeta : metaData) {
        if (storeMeta.storeType < StoreMetaData::StoreType::STORE_KV_BEGIN ||
            storeMeta.storeType > StoreMetaData::StoreType::STORE_KV_END) {
            continue;
        }
        auto identifierTag = DBManager::GetKvStoreIdentifier("", storeMeta.appId, storeMeta.storeId, true);
        if (identifier != identifierTag) {
            continue;
        }
        auto watchers = GetWatchers(storeMeta.tokenId, storeMeta.storeId);
        AutoCache::GetInstance().GetStore(storeMeta, watchers);

        ZLOGD("user:%{public}s appId:%{public}s storeId:%{public}s", storeMeta.user.c_str(),
            storeMeta.bundleName.c_str(), Anonymous::Change(storeMeta.storeId).c_str());
    }
    return SUCCESS;
}

int32_t KVDBServiceImpl::OnUserChange(uint32_t code, const std::string &user, const std::string &account)
{
    (void)code;
    (void)user;
    (void)account;
    std::vector<int32_t> users;
    AccountDelegate::GetInstance()->QueryUsers(users);
    std::set<int32_t> userIds(users.begin(), users.end());
    AutoCache::GetInstance().CloseExcept(userIds);
    return SUCCESS;
}

int32_t KVDBServiceImpl::OnReady(const std::string &device)
{
    if (device == DeviceManagerAdapter::CLOUD_DEVICE_UUID) {
        return SUCCESS;
    }
    std::vector<StoreMetaData> metaData;
    auto prefix = StoreMetaData::GetPrefix({ DMAdapter::GetInstance().GetLocalDevice().uuid });
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, metaData)) {
        ZLOGE("load meta failed!");
        return STORE_NOT_FOUND;
    }
    for (const auto &data : metaData) {
        if (!data.isAutoSync) {
            continue;
        }
        ZLOGI("[onReady] appId:%{public}s, storeId:%{public}s",
            data.bundleName.c_str(), Anonymous::Change(data.storeId).c_str());
        auto code = DeviceMatrix::GetInstance().GetCode(data);
        if (code == DeviceMatrix::INVALID_MASK) {
            continue;
        }
        std::pair<bool, uint16_t> mask = { false, 0 };
        if (data.dataType == DataType::TYPE_STATICS) {
            mask = DeviceMatrix::GetInstance().GetMask(device, DeviceMatrix::LevelType::STATICS);
        } else {
            mask = DeviceMatrix::GetInstance().GetMask(device);
        }
        if (mask.first && ((mask.second & code) != code)) {
            continue;
        }
        SyncInfo syncInfo;
        syncInfo.mode = SyncMode::PUSH;
        syncInfo.devices = { device };
        auto delay = GetSyncDelayTime(syncInfo.delay, { data.storeId });
        KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(data.tokenId), delay,
            std::bind(&KVDBServiceImpl::DoSync, this, data, syncInfo, std::placeholders::_1, ACTION_SYNC),
            std::bind(&KVDBServiceImpl::DoComplete, this, data, syncInfo, RefCount(), std::placeholders::_1));
    }
    return SUCCESS;
}

int32_t KVDBServiceImpl::OnSessionReady(const std::string &device)
{
    ZLOGI("session ready, device:%{public}s", Anonymous::Change(device).c_str());
    if (!DMAdapter::GetInstance().IsDeviceReady(device)) {
        return SUCCESS;
    }

    SyncOnSessionReady(device);
    auto stores = AutoSyncMatrix::GetInstance().GetChangedStore(device);
    for (const auto &store : stores) {
        ZLOGI("[OnSessionReady] appId:%{public}s, storeId:%{public}s",
            store.bundleName.c_str(), Anonymous::Change(store.storeId).c_str());
        SyncInfo syncInfo;
        syncInfo.mode = SyncMode::PUSH;
        syncInfo.devices = { device };
        KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(store.tokenId), 0,
            std::bind(&KVDBServiceImpl::DoSyncInOrder, this, store, syncInfo, std::placeholders::_1, ACTION_SYNC),
            std::bind(&KVDBServiceImpl::DoComplete, this, store, syncInfo, RefCount(), std::placeholders::_1));
    }
    return SUCCESS;
}

void KVDBServiceImpl::SyncOnSessionReady(const std::string &device)
{
    auto local = DMAdapter::GetInstance().GetLocalDevice().uuid;
    std::vector<StoreMetaData> metas;
    auto prefix = StoreMetaData::GetPrefix({ local });
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, metas)) {
        ZLOGE("load meta failed!");
        return;
    }
    for (const auto &meta : metas) {
        if (!DeviceMatrix::GetInstance().IsStatics(meta) && !DeviceMatrix::GetInstance().IsDynamic(meta)) {
            continue;
        }
        if (!IsRemoteChange(meta, device)) {
            continue;
        }
        SyncInfo syncInfo;
        syncInfo.mode = PULL;
        syncInfo.delay = 0;
        syncInfo.devices = { device };
        ZLOGI("[SyncOnSessionReady] appId:%{public}s, storeId:%{public}s", meta.bundleName.c_str(),
            Anonymous::Change(meta.storeId).c_str());
        KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(meta.tokenId), syncInfo.delay,
            std::bind(&KVDBServiceImpl::DoSyncInOrder, this, meta, syncInfo, std::placeholders::_1, ACTION_SYNC),
            std::bind(&KVDBServiceImpl::DoComplete, this, meta, syncInfo, RefCount(), std::placeholders::_1));
    }
}

bool KVDBServiceImpl::IsRemoteChange(const StoreMetaData &metaData, const std::string &device)
{
    auto code = DeviceMatrix::GetInstance().GetCode(metaData);
    if (code == DeviceMatrix::INVALID_MASK) {
        return true;
    }
    auto [dynamic, statics] = DeviceMatrix::GetInstance().IsConsistent(device);
    if (metaData.dataType == DataType::TYPE_STATICS && statics) {
        return false;
    }
    if (metaData.dataType == DataType::TYPE_DYNAMICAL && dynamic) {
        return false;
    }
    auto [exist, mask] = DeviceMatrix::GetInstance().GetRemoteMask(
        device, static_cast<DeviceMatrix::LevelType>(metaData.dataType));
    return (mask & code) == code;
}

int32_t KVDBServiceImpl::Online(const std::string &device)
{
    AutoSyncMatrix::GetInstance().Online(device);
    return SUCCESS;
}

void KVDBServiceImpl::AddOptions(const Options &options, StoreMetaData &metaData)
{
    metaData.isAutoSync = options.autoSync;
    metaData.isBackup = options.backup;
    metaData.isEncrypt = options.encrypt;
    metaData.storeType = options.kvStoreType;
    metaData.securityLevel = options.securityLevel;
    metaData.area = options.area;
    metaData.appId = CheckerManager::GetInstance().GetAppId(Converter::ConvertToStoreInfo(metaData));
    metaData.appType = "harmony";
    metaData.hapName = options.hapName;
    metaData.dataDir = DirectoryManager::GetInstance().GetStorePath(metaData);
    metaData.schema = options.schema;
    metaData.account = AccountDelegate::GetInstance()->GetCurrentAccountId();
    metaData.isNeedCompress = options.isNeedCompress;
    metaData.dataType = options.dataType;
}

void KVDBServiceImpl::SaveLocalMetaData(const Options &options, const StoreMetaData &metaData)
{
    StoreMetaDataLocal localMetaData;
    localMetaData.isAutoSync = options.autoSync;
    localMetaData.isBackup = options.backup;
    localMetaData.isEncrypt = options.encrypt;
    localMetaData.dataDir = DirectoryManager::GetInstance().GetStorePath(metaData);
    localMetaData.schema = options.schema;
    localMetaData.isPublic = options.isPublic;
    for (auto &policy : options.policies) {
        OHOS::DistributedData::PolicyValue value;
        value.type = policy.type;
        value.index = policy.value.index();
        if (const uint32_t *pval = std::get_if<uint32_t>(&policy.value)) {
            value.valueUint = *pval;
        }
        localMetaData.policies.emplace_back(value);
    }
    MetaDataManager::GetInstance().SaveMeta(metaData.GetKeyLocal(), localMetaData, true);
}

StoreMetaData KVDBServiceImpl::GetStoreMetaData(const AppId &appId, const StoreId &storeId)
{
    StoreMetaData metaData;
    metaData.uid = IPCSkeleton::GetCallingUid();
    metaData.tokenId = IPCSkeleton::GetCallingTokenID();
    metaData.instanceId = GetInstIndex(metaData.tokenId, appId);
    metaData.bundleName = appId.appId;
    metaData.deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.storeId = storeId.storeId;
    auto user = AccountDelegate::GetInstance()->GetUserByToken(metaData.tokenId);
    metaData.user = std::to_string(user);
    return metaData;
}

StrategyMeta KVDBServiceImpl::GetStrategyMeta(const AppId &appId, const StoreId &storeId)
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto userId = AccountDelegate::GetInstance()->GetUserByToken(tokenId);
    auto deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    StrategyMeta strategyMeta(deviceId, std::to_string(userId), appId.appId, storeId.storeId);
    strategyMeta.instanceId = GetInstIndex(tokenId, appId);
    return strategyMeta;
}

int32_t KVDBServiceImpl::GetInstIndex(uint32_t tokenId, const AppId &appId)
{
    if (AccessTokenKit::GetTokenTypeFlag(tokenId) != TOKEN_HAP) {
        return 0;
    }

    HapTokenInfo tokenInfo;
    tokenInfo.instIndex = -1;
    int errCode = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (errCode != RET_SUCCESS) {
        ZLOGE("GetHapTokenInfo error:%{public}d, tokenId:0x%{public}x appId:%{public}s", errCode, tokenId,
            appId.appId.c_str());
        return -1;
    }
    return tokenInfo.instIndex;
}

KVDBServiceImpl::DBResult KVDBServiceImpl::HandleGenBriefDetails(const GenDetails &details)
{
    DBResult dbResults{};
    for (const auto &[id, detail] : details) {
        dbResults[id] = DBStatus(detail.code);
    }
    return dbResults;
}

Status KVDBServiceImpl::DoCloudSync(const StoreMetaData &meta, const SyncInfo &syncInfo)
{
    if (CloudServer::GetInstance() == nullptr || !DMAdapter::GetInstance().IsNetworkAvailable()) {
        return Status::CLOUD_DISABLED;
    }

    DistributedData::StoreInfo storeInfo;
    storeInfo.bundleName = meta.bundleName;
    storeInfo.user = atoi(meta.user.c_str());
    storeInfo.tokenId = meta.tokenId;
    storeInfo.storeName = meta.storeId;
    GenAsync syncCallback = [tokenId = storeInfo.tokenId, seqId = syncInfo.seqId, this](
                             const GenDetails &details) {
        OnAsyncComplete(tokenId, seqId, HandleGenDetails(details));
    };
    auto mixMode = static_cast<int32_t>(GeneralStore::MixMode(GeneralStore::CLOUD_TIME_FIRST,
        meta.isAutoSync ? GeneralStore::AUTO_SYNC_MODE : GeneralStore::MANUAL_SYNC_MODE));
    auto info = ChangeEvent::EventInfo(mixMode, 0, meta.isAutoSync, nullptr, syncCallback);
    auto evt = std::make_unique<ChangeEvent>(std::move(storeInfo), std::move(info));
    EventCenter::GetInstance().PostEvent(std::move(evt));
    return SUCCESS;
}

Status KVDBServiceImpl::DoSync(const StoreMetaData &meta, const SyncInfo &info, const SyncEnd &complete, int32_t type)
{
    ZLOGD("seqId:0x%{public}" PRIx64 " type:%{public}d remote:%{public}zu appId:%{public}s storeId:%{public}s",
        info.seqId, type, info.devices.size(), meta.bundleName.c_str(), Anonymous::Change(meta.storeId).c_str());
    auto uuids = ConvertDevices(info.devices);
    if (uuids.empty()) {
        ZLOGW("no device online seqId:0x%{public}" PRIx64 " remote:%{public}zu appId:%{public}s storeId:%{public}s",
            info.seqId, info.devices.size(), meta.bundleName.c_str(), Anonymous::Change(meta.storeId).c_str());
        return Status::ERROR;
    }

    return DoSyncBegin(uuids, meta, info, complete, type);
}

Status KVDBServiceImpl::DoSyncInOrder(
    const StoreMetaData &meta, const SyncInfo &info, const SyncEnd &complete, int32_t type)
{
    ZLOGD("type:%{public}d seqId:0x%{public}" PRIx64 " remote:%{public}zu appId:%{public}s storeId:%{public}s", type,
        info.seqId, info.devices.size(), meta.bundleName.c_str(), Anonymous::Change(meta.storeId).c_str());
    auto uuids = ConvertDevices(info.devices);
    if (uuids.empty()) {
        ZLOGW("no device seqId:0x%{public}" PRIx64 " remote:%{public}zu appId:%{public}s storeId:%{public}s",
            info.seqId, info.devices.size(), meta.bundleName.c_str(), Anonymous::Change(meta.storeId).c_str());
        return Status::ERROR;
    }
    bool isOnline = false;
    bool isAfterMeta = false;
    for (const auto &uuid : uuids) {
        if (!DMAdapter::GetInstance().IsDeviceReady(uuid)) {
            isOnline = true;
            break;
        }
        auto metaData = meta;
        metaData.deviceId = uuid;
        CapMetaData capMeta;
        auto capKey = CapMetaRow::GetKeyFor(uuid);
        if (!MetaDataManager::GetInstance().LoadMeta(std::string(capKey.begin(), capKey.end()), capMeta)
            || !MetaDataManager::GetInstance().LoadMeta(metaData.GetKey(), metaData)) {
            isAfterMeta = true;
        }
    }
    if (!isOnline && isAfterMeta) {
        auto result = MetaDataManager::GetInstance().Sync(
            uuids, [this, meta, info, complete, type](const auto &results) {
            auto ret = ProcessResult(results);
            if (ret.first.empty()) {
                DoComplete(meta, info, RefCount(), ret.second);
                return;
            }
            auto status = DoSyncBegin(ret.first, meta, info, complete, type);
            ZLOGD("data sync status:%{public}d appId:%{public}s, storeId:%{public}s",
                static_cast<int32_t>(status), meta.bundleName.c_str(), Anonymous::Change(meta.storeId).c_str());
        });
        return result ? Status::SUCCESS : Status::ERROR;
    }
    return DoSyncBegin(uuids, meta, info, complete, type);
}

KVDBServiceImpl::SyncResult KVDBServiceImpl::ProcessResult(const std::map<std::string, int32_t> &results)
{
    std::map<std::string, DBStatus> dbResults;
    std::vector<std::string> devices;
    for (const auto &[uuid, status] : results) {
        dbResults.insert_or_assign(uuid, static_cast<DBStatus>(status));
        if (static_cast<DBStatus>(status) != DBStatus::OK) {
            continue;
        }
        DeviceMatrix::GetInstance().OnExchanged(uuid, DeviceMatrix::META_STORE_MASK);
        devices.emplace_back(uuid);
    }
    ZLOGD("meta sync finish, total size:%{public}zu, success size:%{public}zu", dbResults.size(), devices.size());
    return { devices, dbResults };
}

Status KVDBServiceImpl::DoSyncBegin(const std::vector<std::string> &devices, const StoreMetaData &meta,
    const SyncInfo &info, const SyncEnd &complete, int32_t type)
{
    if (devices.empty()) {
        return Status::INVALID_ARGUMENT;
    }
    auto watcher = GetWatchers(meta.tokenId, meta.storeId);
    auto store = AutoCache::GetInstance().GetStore(meta, watcher);
    if (store == nullptr) {
        ZLOGE("GetStore failed! appId:%{public}s storeId:%{public}s dir:%{public}s", meta.bundleName.c_str(),
            Anonymous::Change(meta.storeId).c_str(), meta.dataDir.c_str());
        return Status::ERROR;
    }
    KVDBQuery query(info.query);
    if (!query.IsValidQuery()) {
        ZLOGE("failed DBQuery:%{public}s", Anonymous::Change(info.query).c_str());
        return Status::INVALID_ARGUMENT;
    }
    auto mode = ConvertGeneralSyncMode(SyncMode(info.mode), SyncAction(type));
    SyncParam syncParam{};
    syncParam.mode = mode;
    auto ret = store->Sync(
        devices, query,
        [this, complete](const GenDetails &result) mutable {
            auto deviceStatus = HandleGenBriefDetails(result);
            complete(deviceStatus);
        },
        syncParam);
    return Status(ret);
}

Status KVDBServiceImpl::DoComplete(const StoreMetaData &meta, const SyncInfo &info, RefCount refCount,
    const DBResult &dbResult)
{
    ZLOGD("seqId:0x%{public}" PRIx64 " tokenId:0x%{public}x remote:%{public}zu", info.seqId, meta.tokenId,
        dbResult.size());
    std::map<std::string, Status> result;
    for (auto &[key, status] : dbResult) {
        result[key] = ConvertDbStatus(status);
    }
    for (const auto &device : info.devices) {
        auto it = result.find(device);
        if (it != result.end() && it->second == SUCCESS) {
            AutoSyncMatrix::GetInstance().OnExchanged(device, meta);
            DeviceMatrix::GetInstance().OnExchanged(device, meta, ConvertType(static_cast<SyncMode>(info.mode)));
        }
    }
    if (info.seqId == std::numeric_limits<uint64_t>::max()) {
        return SUCCESS;
    }
    sptr<IKVDBNotifier> notifier;
    syncAgents_.ComputeIfPresent(meta.tokenId, [&notifier](auto &key, SyncAgent &agent) {
        notifier = agent.notifier_;
        return true;
    });
    if (notifier == nullptr) {
        return SUCCESS;
    }
    notifier->SyncCompleted(result, info.seqId);
    return SUCCESS;
}

uint32_t KVDBServiceImpl::GetSyncDelayTime(uint32_t delay, const StoreId &storeId)
{
    if (delay != 0) {
        return std::min(std::max(delay, KvStoreSyncManager::SYNC_MIN_DELAY_MS), KvStoreSyncManager::SYNC_MAX_DELAY_MS);
    }

    bool isBackground = Constant::IsBackground(IPCSkeleton::GetCallingPid());
    if (!isBackground) {
        return delay;
    }
    delay = KvStoreSyncManager::SYNC_DEFAULT_DELAY_MS;
    syncAgents_.ComputeIfPresent(IPCSkeleton::GetCallingTokenID(), [&delay, &storeId](auto &, SyncAgent &agent) {
        auto it = agent.delayTimes_.find(storeId);
        if (it != agent.delayTimes_.end() && it->second != 0) {
            delay = it->second;
        }
        return true;
    });
    return delay;
}

Status KVDBServiceImpl::ConvertDbStatus(DBStatus status) const
{
    switch (status) {
        case DBStatus::BUSY: // fallthrough
        case DBStatus::DB_ERROR:
            return Status::DB_ERROR;
        case DBStatus::OK:
            return Status::SUCCESS;
        case DBStatus::INVALID_ARGS:
            return Status::INVALID_ARGUMENT;
        case DBStatus::NOT_FOUND:
            return Status::KEY_NOT_FOUND;
        case DBStatus::INVALID_VALUE_FIELDS:
            return Status::INVALID_VALUE_FIELDS;
        case DBStatus::INVALID_FIELD_TYPE:
            return Status::INVALID_FIELD_TYPE;
        case DBStatus::CONSTRAIN_VIOLATION:
            return Status::CONSTRAIN_VIOLATION;
        case DBStatus::INVALID_FORMAT:
            return Status::INVALID_FORMAT;
        case DBStatus::INVALID_QUERY_FORMAT:
            return Status::INVALID_QUERY_FORMAT;
        case DBStatus::INVALID_QUERY_FIELD:
            return Status::INVALID_QUERY_FIELD;
        case DBStatus::NOT_SUPPORT:
            return Status::NOT_SUPPORT;
        case DBStatus::TIME_OUT:
            return Status::TIME_OUT;
        case DBStatus::OVER_MAX_LIMITS:
            return Status::OVER_MAX_LIMITS;
        case DBStatus::EKEYREVOKED_ERROR: // fallthrough
        case DBStatus::SECURITY_OPTION_CHECK_ERROR:
            return Status::SECURITY_LEVEL_ERROR;
        default:
            break;
    }
    return Status::ERROR;
}

KVDBServiceImpl::DBMode KVDBServiceImpl::ConvertDBMode(SyncMode syncMode) const
{
    DBMode dbMode;
    if (syncMode == SyncMode::PUSH) {
        dbMode = DBMode::SYNC_MODE_PUSH_ONLY;
    } else if (syncMode == SyncMode::PULL) {
        dbMode = DBMode::SYNC_MODE_PULL_ONLY;
    } else {
        dbMode = DBMode::SYNC_MODE_PUSH_PULL;
    }
    return dbMode;
}

GeneralStore::SyncMode KVDBServiceImpl::ConvertGeneralSyncMode(SyncMode syncMode, SyncAction syncAction) const
{
    GeneralStore::SyncMode generalSyncMode = GeneralStore::SyncMode::NEARBY_END;
    if (syncAction == SyncAction::ACTION_SUBSCRIBE) {
        generalSyncMode = GeneralStore::SyncMode::NEARBY_SUBSCRIBE_REMOTE;
    } else if (syncAction == SyncAction::ACTION_UNSUBSCRIBE) {
        generalSyncMode = GeneralStore::SyncMode::NEARBY_UNSUBSCRIBE_REMOTE;
    } else if (syncAction == SyncAction::ACTION_SYNC && syncMode == SyncMode::PUSH) {
        generalSyncMode = GeneralStore::SyncMode::NEARBY_PUSH;
    } else if (syncAction == SyncAction::ACTION_SYNC && syncMode == SyncMode::PULL) {
        generalSyncMode = GeneralStore::SyncMode::NEARBY_PULL;
    } else if (syncAction == SyncAction::ACTION_SYNC && syncMode == SyncMode::PUSH_PULL) {
        generalSyncMode = GeneralStore::SyncMode::NEARBY_PULL_PUSH;
    }
    return generalSyncMode;
}

KVDBServiceImpl::ChangeType KVDBServiceImpl::ConvertType(SyncMode syncMode) const
{
    switch (syncMode) {
        case SyncMode::PUSH:
            return ChangeType::CHANGE_LOCAL;
        case SyncMode::PULL:
            return ChangeType::CHANGE_REMOTE;
        case SyncMode::PUSH_PULL:
            return ChangeType::CHANGE_ALL;
        default:
            break;
    }
    return ChangeType::CHANGE_ALL;
}

SwitchState KVDBServiceImpl::ConvertAction(Action action) const
{
    switch (action) {
        case Action::INSERT:
            return SwitchState::INSERT;
        case Action::UPDATE:
            return SwitchState::UPDATE;
        case Action::DELETE:
            return SwitchState::DELETE;
        default:
            break;
    }
    return SwitchState::INSERT;
}

SyncMode KVDBServiceImpl::GetSyncMode(bool local, bool remote) const
{
    if (local && remote) {
        return SyncMode::PUSH_PULL;
    }
    if (local) {
        return SyncMode::PUSH;
    }
    if (remote) {
        return SyncMode::PULL;
    }
    return SyncMode::PUSH_PULL;
}

std::vector<std::string> KVDBServiceImpl::ConvertDevices(const std::vector<std::string> &deviceIds) const
{
    if (deviceIds.empty()) {
        return DMAdapter::ToUUID(DMAdapter::GetInstance().GetRemoteDevices());
    }
    return DMAdapter::ToUUID(deviceIds);
}

AutoCache::Watchers KVDBServiceImpl::GetWatchers(uint32_t tokenId, const std::string &storeId)
{
    auto [success, agent] = syncAgents_.Find(tokenId);
    if (agent.watcher_ == nullptr) {
        return {};
    }
    return { agent.watcher_ };
}

void KVDBServiceImpl::SyncAgent::ReInit(pid_t pid, const AppId &appId)
{
    ZLOGW("pid:%{public}d->%{public}d appId:%{public}s notifier:%{public}d observer:%{public}zu", pid, pid_,
        appId_.appId.c_str(), notifier_ == nullptr, observers_.size());
    pid_ = pid;
    appId_ = appId;
    notifier_ = nullptr;
    delayTimes_.clear();
    count_ = 0;
    watcher_ = nullptr;
    observers_.clear();
}

void KVDBServiceImpl::SyncAgent::SetWatcher(std::shared_ptr<KVDBWatcher> watcher)
{
    if (watcher_ != watcher) {
        watcher_ = watcher;
        if (watcher_ != nullptr) {
            watcher_->SetObservers(observers_);
        }
    }
}

void KVDBServiceImpl::SyncAgent::SetObserver(sptr<KvStoreObserverProxy> observer)
{
    observers_.insert(observer);
    if (watcher_ != nullptr) {
        watcher_->SetObservers(observers_);
    }
}

void KVDBServiceImpl::SyncAgent::ClearObservers()
{
    observers_.clear();
    if (watcher_ != nullptr) {
        watcher_->ClearObservers();
    }
}

int32_t KVDBServiceImpl::OnBind(const BindInfo &bindInfo)
{
    executors_ = bindInfo.executors;
    KvStoreSyncManager::GetInstance()->SetThreadPool(bindInfo.executors);
    DeviceMatrix::GetInstance().SetExecutor(bindInfo.executors);
    return 0;
}

int32_t KVDBServiceImpl::OnInitialize()
{
    RegisterKvServiceInfo();
    RegisterHandler();
    Init();
    RegisterMatrixChange();
    return SUCCESS;
}
} // namespace OHOS::DistributedKv