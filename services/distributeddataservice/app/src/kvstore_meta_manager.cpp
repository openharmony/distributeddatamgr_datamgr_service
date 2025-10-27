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
#define LOG_TAG "KvStoreMetaManager"

#include "kvstore_meta_manager.h"

#include <ipc_skeleton.h>
#include <thread>
#include <unistd.h>

#include "account/account_delegate.h"
#include "bootstrap.h"
#include "communication_provider.h"
#include "crypto/crypto_manager.h"
#include "device_manager_adapter.h"
#include "device_matrix.h"
#include "directory/directory_manager.h"
#include "eventcenter/event_center.h"
#include "kvstore_data_service.h"
#include "kv_radar_reporter.h"
#include "log_print.h"
#include "matrix_event.h"
#include "metadata/auto_launch_meta_data.h"
#include "metadata/capability_meta_data.h"
#include "metadata/device_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/matrix_meta_data.h"
#include "metadata/strategy_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "metadata/strategy_meta_data.h"
#include "metadata/switches_meta_data.h"
#include "metadata/user_meta_data.h"
#include "metadata/version_meta_data.h"
#include "runtime_config.h"
#include "safe_block_queue.h"
#include "store/general_store.h"
#include "store/store_info.h"
#include "utils/anonymous.h"
#include "utils/block_integer.h"
#include "utils/corrupt_reporter.h"
#include "utils/crypto.h"
#include "utils/ref_count.h"
#include "utils/converter.h"
#include "utils/constant.h"

namespace OHOS {
namespace DistributedKv {
using Commu = AppDistributedKv::CommunicationProvider;
using DmAdapter = DistributedData::DeviceManagerAdapter;
using namespace std::chrono;
using namespace OHOS::DistributedData;
using namespace DistributedDB;
using namespace OHOS::AppDistributedKv;

KvStoreMetaManager::MetaDeviceChangeListenerImpl KvStoreMetaManager::listener_;
KvStoreMetaManager::DBInfoDeviceChangeListenerImpl KvStoreMetaManager::dbInfoListener_;

KvStoreMetaManager::KvStoreMetaManager()
    : metaDelegate_(nullptr), metaDBDirectory_(DirectoryManager::GetInstance().GetMetaStorePath()),
      label_(Bootstrap::GetInstance().GetProcessLabel()),
      delegateManager_(Bootstrap::GetInstance().GetProcessLabel(), "default")
{
    ZLOGI("begin.");
    metaObserver_ = std::make_shared<KvStoreMetaObserver>();
}

KvStoreMetaManager::~KvStoreMetaManager()
{
    if (delaySyncTaskId_ != ExecutorPool::INVALID_TASK_ID) {
        executors_->Remove(delaySyncTaskId_);
        delaySyncTaskId_ = ExecutorPool::INVALID_TASK_ID;
    }
}

KvStoreMetaManager &KvStoreMetaManager::GetInstance()
{
    static KvStoreMetaManager instance;
    return instance;
}

void KvStoreMetaManager::SubscribeMeta(const std::string &keyPrefix, const ChangeObserver &observer)
{
    if (metaObserver_ == nullptr) {
        return;
    }
    metaObserver_->handlerMap_[keyPrefix] = observer;
}

void KvStoreMetaManager::InitMetaListener()
{
    InitMetaData();
    auto status = DmAdapter::GetInstance().StartWatchDeviceChange(&listener_, { "metaMgr" });
    if (status != AppDistributedKv::Status::SUCCESS) {
        ZLOGW("register metaMgr failed: %{public}d.", status);
        return;
    }
    SubscribeMetaKvStore();
    InitBroadcast();
    NotifyAllAutoSyncDBInfo();
}

void KvStoreMetaManager::InitBroadcast()
{
    auto pipe = Bootstrap::GetInstance().GetProcessLabel() + "-" + "default";
    auto result = Commu::GetInstance().ListenBroadcastMsg({ pipe },
        [](const std::string &device, const AppDistributedKv::LevelInfo &levelInfo) {
            DistributedData::DeviceMatrix::DataLevel level;
            level.dynamic = levelInfo.dynamic;
            level.statics = levelInfo.statics;
            level.switches = levelInfo.switches;
            level.switchesLen = levelInfo.switchesLen;
            DeviceMatrix::GetInstance().OnBroadcast(device, std::move(level));
        });

    EventCenter::GetInstance().Subscribe(DeviceMatrix::MATRIX_BROADCAST, [pipe](const Event &event) {
        auto &matrixEvent = static_cast<const MatrixEvent &>(event);
        auto matrixData = matrixEvent.GetMatrixData();
        AppDistributedKv::LevelInfo level;
        level.dynamic = matrixData.dynamic;
        level.statics = matrixData.statics;
        level.switches = matrixData.switches;
        level.switchesLen = matrixData.switchesLen;
        Commu::GetInstance().Broadcast({ pipe }, std::move(level));
    });

    ZLOGI("observer matrix broadcast %{public}d.", result);
}

void KvStoreMetaManager::InitMetaData()
{
    ZLOGI("start.");
    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        ZLOGI("get meta failed.");
        return;
    }

    StoreMetaData data = InitStoreMetaData();
    StoreMetaDataLocal localData;
    localData.isAutoSync = false;
    localData.isBackup = false;
    localData.isEncrypt = false;
    localData.dataDir = metaDBDirectory_;
    localData.schema = "";
    localData.isPublic = true;
    MetaDataManager::GetInstance().SaveMeta(data.GetKeyWithoutPath(), data);
    MetaDataManager::GetInstance().SaveMeta(data.GetKey(), data, true);
    MetaDataManager::GetInstance().SaveMeta(data.GetKeyLocal(), localData, true);
    std::string localUuid;
    DeviceMetaData deviceMeta;
    if (MetaDataManager::GetInstance().LoadMeta(deviceMeta.GetKey(), deviceMeta, true)) {
        localUuid = deviceMeta.newUuid;
    } else {
        localUuid = DmAdapter::GetInstance().GetLocalDevice().uuid;
    }
    if (localUuid.empty()) {
        ZLOGE("get uuid failed");
        return;
    }
    UpdateMetaData(localUuid);
    CheckMetaDeviceId();
    SetCloudSyncer();
    ZLOGI("end.");
}

StoreMetaData KvStoreMetaManager::InitStoreMetaData()
{
    StoreMetaData data;
    auto uid = getuid();
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto userId = AccountDelegate::GetInstance()->GetUserByToken(tokenId);
    data.appId = label_;
    data.appType = "default";
    data.bundleName = label_;
    data.dataDir = metaDBDirectory_;
    data.user = std::to_string(userId);
    data.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    data.isAutoSync = false;
    data.isBackup = false;
    data.isEncrypt = false;
    data.isNeedCompress = true;
    data.storeType = KvStoreType::SINGLE_VERSION;
    data.dataType = DataType::TYPE_DYNAMICAL;
    data.schema = "";
    data.storeId = Bootstrap::GetInstance().GetMetaDBName();
    data.account = AccountDelegate::GetInstance()->GetCurrentAccountId();
    data.uid = static_cast<int32_t>(uid);
    data.version = META_STORE_VERSION;
    data.securityLevel = SecurityLevel::S1;
    data.area = EL1;
    data.tokenId = tokenId;
    return data;
}

void KvStoreMetaManager::UpdateMetaData(const std::string &uuid)
{
    VersionMetaData versionMeta;
    bool isExist = MetaDataManager::GetInstance().LoadMeta(versionMeta.GetKey(), versionMeta, true);
    if (isExist && versionMeta.version == VersionMetaData::CURRENT_VERSION) {
        return;
    }
    bool syncMetaUpdateNeeded = !isExist || versionMeta.version < VersionMetaData::UPDATE_SYNC_META_VERSION;
    bool storeMetaUpgradeNeeded = !isExist || versionMeta.version < VersionMetaData::UPDATE_STORE_META_KEY_VERSION;
    if (syncMetaUpdateNeeded) {
        std::vector<StoreMetaData> metaDataList;
        std::string prefix = StoreMetaData::GetPrefix({ uuid });
        MetaDataManager::GetInstance().LoadMeta(prefix, metaDataList);
        std::vector<std::string> keys;
        for (const auto &metaData : metaDataList) {
            MetaDataManager::GetInstance().SaveMeta(metaData.GetKey(), metaData, true);
            if (CheckerManager::GetInstance().IsDistrust(Converter::ConvertToStoreInfo(metaData)) ||
                (metaData.storeType >= StoreMetaData::StoreType::STORE_RELATIONAL_BEGIN &&
                    metaData.storeType <= StoreMetaData::StoreType::STORE_RELATIONAL_END)) {
                keys.push_back(metaData.GetKey());
            }
        }
        MetaDataManager::GetInstance().DelMeta(keys);
        versionMeta.version = VersionMetaData::UPDATE_SYNC_META_VERSION;
        MetaDataManager::GetInstance().SaveMeta(versionMeta.GetKey(), versionMeta, true);
    }
    if (storeMetaUpgradeNeeded && UpgradeStoreMeta(uuid)) {
        versionMeta.version = VersionMetaData::UPDATE_STORE_META_KEY_VERSION;
        MetaDataManager::GetInstance().SaveMeta(versionMeta.GetKey(), versionMeta, true);
    }
}

// The metadata database is upgraded. The suffix is added to the path.
bool KvStoreMetaManager::UpgradeStoreMeta(const std::string &uuid)
{
    std::vector<StoreMetaData> metaDataList;
    std::string prefix = StoreMetaData::GetPrefix({ uuid });
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, metaDataList, true)) {
        ZLOGE("load meta failed");
        return false;
    }
    size_t batchSize = 100; // The batch quantity is 100.
    return Constant::BatchProcess(metaDataList, batchSize, [](const auto &begin, const auto &end) {
        std::vector<MetaDataManager::Entry> entries;
        std::vector<std::string> deleteKeys;
        auto size = static_cast<size_t>(std::distance(begin, end));
        entries.reserve(size * 3); // Expand the entries size by 3 times.
        deleteKeys.reserve(size * 4); // Expand the deleteKeys size by 4 times.
        auto it = begin;
        do {
            auto &meta = *it;
            StoreMetaDataLocal localMeta;
            bool isExist = MetaDataManager::GetInstance().LoadMeta(meta.GetKeyLocalWithoutPath(), localMeta, true);
            if (isExist) {
                deleteKeys.push_back(meta.GetKeyLocalWithoutPath());
                entries.push_back({ meta.GetKeyLocal(), Serializable::Marshall(localMeta) });
            }
            SecretKeyMetaData secretKeyMetaData;
            isExist = MetaDataManager::GetInstance().LoadMeta(meta.GetSecretKeyWithoutPath(), secretKeyMetaData, true);
            if (isExist) {
                deleteKeys.push_back(meta.GetSecretKeyWithoutPath());
                entries.push_back({ meta.GetSecretKey(), Serializable::Marshall(secretKeyMetaData) });
            }
            deleteKeys.push_back(meta.GetKeyWithoutPath());
            deleteKeys.push_back(meta.GetDebugInfoKeyWithoutPath());
            deleteKeys.push_back(meta.GetDfxInfoKeyWithoutPath());
            StoreMetaMapping storeMetaMapping(meta);
            auto key = storeMetaMapping.GetKey();
            entries.push_back({ std::move(key), Serializable::Marshall(storeMetaMapping) });
            entries.push_back({ meta.GetKey(), Serializable::Marshall(meta) });
        } while (++it != end);

        if (MetaDataManager::GetInstance().SaveMeta(entries, true)) {
            MetaDataManager::GetInstance().DelMeta(deleteKeys, true);
            return true;
        }
        return false;
    });
}

void KvStoreMetaManager::InitMetaParameter()
{
    ZLOGI("start.");
    executors_->Execute(GetTask(0));
    DistributedDB::KvStoreConfig kvStoreConfig{ metaDBDirectory_ };
    delegateManager_.SetKvStoreConfig(kvStoreConfig);
}

ExecutorPool::Task KvStoreMetaManager::GetTask(uint32_t retry)
{
    return [this, retry] {
        auto status = CryptoManager::GetInstance().CheckRootKey();
        if (status == CryptoManager::ErrCode::SUCCESS) {
            ZLOGI("root key exist.");
            return;
        }
        if (status == CryptoManager::ErrCode::NOT_EXIST &&
            CryptoManager::GetInstance().GenerateRootKey() == CryptoManager::ErrCode::SUCCESS) {
            ZLOGI("GenerateRootKey success.");
            return;
        }
        ZLOGW("GenerateRootKey failed, retry times:%{public}d.", static_cast<int>(retry));
        if (retry + 1 > RETRY_MAX_TIMES) {
            ZLOGE("fail to register subscriber!");
            return;
        }
        executors_->Schedule(std::chrono::seconds(RETRY_INTERVAL), GetTask(retry + 1));
    };
}

KvStoreMetaManager::NbDelegate KvStoreMetaManager::GetMetaKvStore()
{
    if (metaDelegate_ != nullptr) {
        return metaDelegate_;
    }
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (metaDelegate_ != nullptr) {
        return metaDelegate_;
    }

    auto metaDbName = Bootstrap::GetInstance().GetMetaDBName();
    if (CorruptReporter::HasCorruptedFlag(metaDBDirectory_, metaDbName)) {
        DistributedDB::DBStatus dbStatus = delegateManager_.DeleteKvStore(metaDbName);
        if (dbStatus != DistributedDB::DBStatus::OK) {
            ZLOGE("delete meta store failed! dbStatus: %{public}d", dbStatus);
        }
        metaDelegate_ = CreateMetaKvStore(true);
        if (metaDelegate_ != nullptr) {
            CorruptReporter::DeleteCorruptedFlag(metaDBDirectory_, metaDbName);
        }
    } else {
        metaDelegate_ = CreateMetaKvStore();
    }
    auto fullName = GetBackupPath();
    auto backup = [executors = executors_, queue = std::make_shared<SafeBlockQueue<Backup>>(MAX_TASK_COUNT), fullName](
                      const auto &store) -> int32_t {
        auto result = queue->PushNoWait([fullName](const auto &store) -> int32_t {
            auto dbStatus = store->CheckIntegrity();
            if (dbStatus != DistributedDB::DBStatus::OK) {
                ZLOGE("meta store check integrity fail, dbStatus:%{public}d", dbStatus);
                return dbStatus;
            }
            return store->Export(fullName, {}, true);
        });
        if (!result) {
            return OK;
        }
        executors->Schedule(std::chrono::hours(RETRY_INTERVAL), GetBackupTask(queue, executors, store));
        return OK;
    };
    MetaDataManager::GetInstance().Initialize(metaDelegate_, backup, metaDbName);
    return metaDelegate_;
}

ExecutorPool::Task KvStoreMetaManager::GetBackupTask(
    TaskQueue queue, std::shared_ptr<ExecutorPool> executors, const NbDelegate store)
{
    return [queue, executors, store]() {
        Backup backupTask;
        if (!queue->PopNotWait(backupTask)) {
            return;
        }
        if (backupTask(store) != OK && queue->PushNoWait(backupTask)) {
            executors->Schedule(std::chrono::hours(RETRY_INTERVAL), GetBackupTask(queue, executors, store));
        }
    };
}

KvStoreMetaManager::NbDelegate KvStoreMetaManager::CreateMetaKvStore(bool isRestore)
{
    DistributedDB::DBStatus dbStatusTmp = DistributedDB::DBStatus::NOT_SUPPORT;
    auto option = InitDBOption();
    DistributedDB::KvStoreNbDelegate *delegate = nullptr;
    if (!isRestore) {
        delegateManager_.GetKvStore(Bootstrap::GetInstance().GetMetaDBName(), option,
            [&delegate, &dbStatusTmp](DistributedDB::DBStatus dbStatus, DistributedDB::KvStoreNbDelegate *nbDelegate) {
                delegate = nbDelegate;
                dbStatusTmp = dbStatus;
            });
    }
    if (dbStatusTmp == DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB || isRestore) {
        ZLOGE("meta data corrupted!");
        option.isNeedRmCorruptedDb = true;
        auto fullName = GetBackupPath();
        delegateManager_.GetKvStore(Bootstrap::GetInstance().GetMetaDBName(), option,
            [&delegate, &dbStatusTmp, &fullName](DistributedDB::DBStatus dbStatus,
                DistributedDB::KvStoreNbDelegate *nbDelegate) {
                delegate = nbDelegate;
                dbStatusTmp = dbStatus;
                if (dbStatusTmp == DistributedDB::DBStatus::OK && delegate != nullptr) {
                    ZLOGI("start import meta data");
                    DistributedDB::CipherPassword password;
                    delegate->Import(fullName, password, true);
                }
            });
    }
    if (dbStatusTmp != DistributedDB::DBStatus::OK || delegate == nullptr) {
        ZLOGE("GetKvStore return error status: %{public}d or delegate is nullptr", static_cast<int>(dbStatusTmp));
        return nullptr;
    }
    delegate->SetRemotePushFinishedNotify([](const RemotePushNotifyInfo &info) {
        DeviceMatrix::GetInstance().OnExchanged(info.deviceId, DeviceMatrix::META_STORE_MASK,
            DeviceMatrix::LevelType::DYNAMIC, DeviceMatrix::ChangeType::CHANGE_REMOTE);
    });
    bool param = true;
    auto data = static_cast<DistributedDB::PragmaData>(&param);
    delegate->Pragma(DistributedDB::SET_SYNC_RETRY, data);
    auto release = [this](DistributedDB::KvStoreNbDelegate *delegate) {
        ZLOGI("release meta data kv store");
        if (delegate == nullptr) {
            return;
        }
        auto result = delegateManager_.CloseKvStore(delegate);
        if (result != DistributedDB::DBStatus::OK) {
            ZLOGE("CloseMetaKvStore return error status: %{public}d", static_cast<int>(result));
        }
    };
    return NbDelegate(delegate, release);
}

DistributedDB::KvStoreNbDelegate::Option KvStoreMetaManager::InitDBOption()
{
    DistributedDB::KvStoreNbDelegate::Option option;
    option.createIfNecessary = true;
    option.isMemoryDb = false;
    option.createDirByStoreIdOnly = true;
    option.isEncryptedDb = false;
    option.isNeedIntegrityCheck = true;
    option.isNeedRmCorruptedDb = false;
    option.isNeedCompressOnSync = true;
    option.compressionRate = COMPRESS_RATE;
    option.secOption = { DistributedDB::S1, DistributedDB::ECE };
    return option;
}

void KvStoreMetaManager::SetCloudSyncer()
{
    auto cloudSyncer = [this]() {
        std::lock_guard<decltype(mutex_)> lock(mutex_);
        if (delaySyncTaskId_ == Executor::INVALID_TASK_ID) {
            delaySyncTaskId_ = executors_->Schedule(std::chrono::milliseconds(DELAY_SYNC), CloudSyncTask());
        } else {
            delaySyncTaskId_ = executors_->Reset(delaySyncTaskId_, std::chrono::milliseconds(DELAY_SYNC));
        }
    };
    MetaDataManager::GetInstance().SetCloudSyncer(cloudSyncer);
}

std::function<void()> KvStoreMetaManager::CloudSyncTask()
{
    return [this]() {
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);
            delaySyncTaskId_ = ExecutorPool::INVALID_TASK_ID;
        }
        DeviceMatrix::GetInstance().OnChanged(DeviceMatrix::META_STORE_MASK);
    };
}

void KvStoreMetaManager::SubscribeMetaKvStore()
{
    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        ZLOGW("register meta observer failed.");
        return;
    }

    int mode = DistributedDB::OBSERVER_CHANGES_NATIVE | DistributedDB::OBSERVER_CHANGES_FOREIGN;
    if (metaObserver_ == nullptr) {
        return;
    }
    auto dbStatus = metaDelegate->RegisterObserver(DistributedDB::Key(), mode, metaObserver_);
    if (dbStatus != DistributedDB::DBStatus::OK) {
        ZLOGW("register meta observer failed :%{public}d.", dbStatus);
    }
}

KvStoreMetaManager::KvStoreMetaObserver::~KvStoreMetaObserver()
{
    ZLOGW("meta observer destruct.");
}

void KvStoreMetaManager::KvStoreMetaObserver::OnChange(const DistributedDB::KvStoreChangedData &data)
{
    ZLOGD("on data change.");
    HandleChanges(CHANGE_FLAG::INSERT, data.GetEntriesInserted());
    HandleChanges(CHANGE_FLAG::UPDATE, data.GetEntriesUpdated());
    HandleChanges(CHANGE_FLAG::DELETE, data.GetEntriesDeleted());
    KvStoreMetaManager::GetInstance().OnDataChange(CHANGE_FLAG::INSERT, data.GetEntriesInserted());
    KvStoreMetaManager::GetInstance().OnDataChange(CHANGE_FLAG::UPDATE, data.GetEntriesUpdated());
    KvStoreMetaManager::GetInstance().OnDataChange(CHANGE_FLAG::DELETE, data.GetEntriesDeleted());
}

void KvStoreMetaManager::KvStoreMetaObserver::HandleChanges(CHANGE_FLAG flag,
    const std::list<DistributedDB::Entry> &entries)
{
    for (const auto &entry : entries) {
        std::string key(entry.key.begin(), entry.key.end());
        for (const auto &item : handlerMap_) {
            ZLOGI("flag:%{public}d, key:%{public}s", flag, Anonymous::Change(key).c_str());
            if (key.find(item.first) == 0) {
                item.second(entry.key, entry.value, flag);
            }
        }
    }
}

void KvStoreMetaManager::MetaDeviceChangeListenerImpl::OnDeviceChanged(const AppDistributedKv::DeviceInfo &info,
    const AppDistributedKv::DeviceChangeType &type) const
{
    if (info.uuid == DmAdapter::CLOUD_DEVICE_UUID) {
        return;
    }
    switch (type) {
        case AppDistributedKv::DeviceChangeType::DEVICE_OFFLINE:
            DeviceMatrix::GetInstance().Offline(info.uuid);
            break;
        case AppDistributedKv::DeviceChangeType::DEVICE_ONLINE:
            DeviceMatrix::GetInstance().Online(info.uuid, RefCount([deviceId = info.uuid]() {
                DmAdapter::GetInstance().NotifyReadyEvent(deviceId);
            }));
            break;
        default:
            ZLOGI("flag:%{public}d", type);
            break;
    }
}

AppDistributedKv::ChangeLevelType KvStoreMetaManager::MetaDeviceChangeListenerImpl::GetChangeLevelType() const
{
    return AppDistributedKv::ChangeLevelType::LOW;
}

std::string KvStoreMetaManager::GetBackupPath() const
{
    return (DirectoryManager::GetInstance().GetMetaBackupPath() + "/" +
            Crypto::Sha256(label_ + "_" + Bootstrap::GetInstance().GetMetaDBName()));
}

void KvStoreMetaManager::BindExecutor(std::shared_ptr<ExecutorPool> executors)
{
    executors_ = executors;
}

void KvStoreMetaManager::OnDataChange(CHANGE_FLAG flag, const std::list<DistributedDB::Entry>& changedData)
{
    for (const auto& entry : changedData) {
        std::string key(entry.key.begin(), entry.key.end());
        if (key.find(StoreMetaData::GetKey({})) != 0) {
            continue;
        }
        StoreMetaData metaData;
        metaData.Unmarshall({ entry.value.begin(), entry.value.end() });
        if (!metaData.isAutoSync) {
            continue;
        }
        std::vector<DistributedDB::DBInfo> dbInfos;
        AddDbInfo(metaData, dbInfos, flag == CHANGE_FLAG::DELETE);
        DistributedDB::RuntimeConfig::NotifyDBInfos({ metaData.deviceId }, dbInfos);
    }
}

void KvStoreMetaManager::GetDbInfosByDeviceId(const std::string& deviceId, std::vector<DistributedDB::DBInfo>& dbInfos)
{
    std::vector<StoreMetaData> metaData;
    if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ deviceId }), metaData)) {
        ZLOGW("load meta failed, deviceId:%{public}s", Anonymous::Change(deviceId).c_str());
        return;
    }
    for (auto const& data : metaData) {
        if (data.isAutoSync) {
            AddDbInfo(data, dbInfos);
        }
    }
}

void KvStoreMetaManager::AddDbInfo(const StoreMetaData& metaData, std::vector<DistributedDB::DBInfo>& dbInfos,
    bool isDeleted)
{
    DistributedDB::DBInfo dbInfo;
    dbInfo.appId = metaData.deviceId;
    dbInfo.userId = metaData.user;
    dbInfo.storeId = metaData.storeId;
    dbInfo.isNeedSync = !isDeleted;
    dbInfo.syncDualTupleMode = true;
    dbInfos.push_back(dbInfo);
}

void KvStoreMetaManager::OnDeviceChange(const std::string& deviceId)
{
    std::vector<DistributedDB::DBInfo> dbInfos;
    GetDbInfosByDeviceId(deviceId, dbInfos);
    DistributedDB::RuntimeConfig::NotifyDBInfos({ deviceId }, dbInfos);
}

void KvStoreMetaManager::NotifyAllAutoSyncDBInfo()
{
    auto deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (deviceId.empty()) {
        ZLOGE("local deviceId empty");
        return;
    }
    std::vector<StoreMetaData> metaData;
    if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ deviceId }), metaData)) {
        ZLOGE("load meta failed, deviceId:%{public}s", Anonymous::Change(deviceId).c_str());
        return;
    }
    std::vector<DistributedDB::DBInfo> dbInfos;
    for (auto const& data : metaData) {
        if (!data.isAutoSync) {
            continue;
        }
        AddDbInfo(data, dbInfos);
    }
    if (!dbInfos.empty()) {
        DistributedDB::RuntimeConfig::NotifyDBInfos({ deviceId }, dbInfos);
    }
}

void KvStoreMetaManager::DBInfoDeviceChangeListenerImpl::OnDeviceChanged(const AppDistributedKv::DeviceInfo& info,
    const DeviceChangeType& type) const
{
    if (type != DeviceChangeType::DEVICE_ONLINE) {
        ZLOGD("offline or onReady ignore, type:%{public}d, deviceId:%{public}s", type,
            Anonymous::Change(info.uuid).c_str());
        return;
    }
    if (info.uuid == DistributedData::DeviceManagerAdapter::CLOUD_DEVICE_UUID) {
        ZLOGD("Network change, ignore");
        return;
    }
    KvStoreMetaManager::GetInstance().OnDeviceChange(info.uuid);
}

AppDistributedKv::ChangeLevelType KvStoreMetaManager::DBInfoDeviceChangeListenerImpl::GetChangeLevelType() const
{
    return AppDistributedKv::ChangeLevelType::MIN;
}

void KvStoreMetaManager::CheckMetaDeviceId()
{
    DeviceMetaData deviceMeta;
    auto localUuid = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (localUuid.empty()) {
        ZLOGW("get uuid failed");
        return;
    }
    if (!MetaDataManager::GetInstance().LoadMeta(deviceMeta.GetKey(), deviceMeta, true)) {
        deviceMeta.newUuid = localUuid;
        MetaDataManager::GetInstance().SaveMeta(deviceMeta.GetKey(), deviceMeta, true);
        return;
    }
    if (deviceMeta.newUuid != localUuid) {
        UpdateStoreMetaData(localUuid, deviceMeta.newUuid);
        UpdateMetaDatas(localUuid, deviceMeta.newUuid);
        ZLOGI("meta changed! cur uuid:%{public}s, old uuid:%{public}s", Anonymous::Change(localUuid).c_str(),
            Anonymous::Change(deviceMeta.newUuid).c_str());
        deviceMeta.newUuid = localUuid;
        MetaDataManager::GetInstance().SaveMeta(deviceMeta.GetKey(), deviceMeta, true);
    }
}

void KvStoreMetaManager::UpdateStoreMetaData(const std::string &newUuid, const std::string &oldUuid)
{
    std::vector<StoreMetaData> storeMetas;
    MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ oldUuid }), storeMetas, true);
    for (auto &storeMeta : storeMetas) {
        auto oldMeta = storeMeta;
        storeMeta.isNeedUpdateDeviceId = true;
        storeMeta.deviceId = newUuid;
        MetaDataManager::GetInstance().SaveMeta(storeMeta.GetKey(), storeMeta, true);
        MetaDataManager::GetInstance().DelMeta(oldMeta.GetKey(), true);

        StoreMetaData syncStoreMeta;
        if (MetaDataManager::GetInstance().LoadMeta(oldMeta.GetKeyWithoutPath(), syncStoreMeta)) {
            syncStoreMeta.deviceId = newUuid;
            MetaDataManager::GetInstance().SaveMeta(storeMeta.GetKeyWithoutPath(), syncStoreMeta);
            MetaDataManager::GetInstance().DelMeta(oldMeta.GetKeyWithoutPath());
        }

        StrategyMeta strategyMeta;
        if (MetaDataManager::GetInstance().LoadMeta(oldMeta.GetStrategyKey(), strategyMeta)) {
            strategyMeta.devId = newUuid;
            MetaDataManager::GetInstance().SaveMeta(storeMeta.GetStrategyKey(), strategyMeta);
            MetaDataManager::GetInstance().DelMeta(oldMeta.GetStrategyKey());
        }

        StoreMetaDataLocal metaDataLocal;
        if (MetaDataManager::GetInstance().LoadMeta(oldMeta.GetKeyLocal(), metaDataLocal, true)) {
            MetaDataManager::GetInstance().SaveMeta(storeMeta.GetKeyLocal(), metaDataLocal, true);
            MetaDataManager::GetInstance().DelMeta(oldMeta.GetKeyLocal(), true);
        }

        AutoLaunchMetaData autoLaunchMetaData;
        bool isExist = MetaDataManager::GetInstance().LoadMeta(oldMeta.GetAutoLaunchKey(), autoLaunchMetaData, true);
        if (!isExist) {
            oldMeta.storeId = "";
            isExist = MetaDataManager::GetInstance().LoadMeta(oldMeta.GetAutoLaunchKey(), autoLaunchMetaData, true);
        }
        if (isExist) {
            MetaDataManager::GetInstance().DelMeta(oldMeta.GetAutoLaunchKey(), true);
            oldMeta.deviceId = newUuid;
            MetaDataManager::GetInstance().SaveMeta(oldMeta.GetAutoLaunchKey(), autoLaunchMetaData, true);
        }
        if (storeMeta.isEncrypt) {
            MetaDataManager::GetInstance().DelMeta(storeMeta.GetSecretKey(), true);
            MetaDataManager::GetInstance().DelMeta(storeMeta.GetCloneSecretKey(), true);
        }
        MetaDataManager::GetInstance().DelMeta(oldMeta.GetDebugInfoKey(), true);
        MetaDataManager::GetInstance().DelMeta(oldMeta.GetDfxInfoKey(), true);
    }
    UpdateStoreMetaMapping(newUuid, oldUuid);
}

void KvStoreMetaManager::UpdateStoreMetaMapping(const std::string &newUuid, const std::string &oldUuid)
{
    std::vector<StoreMetaMapping> storeMetaMappings;
    MetaDataManager::GetInstance().LoadMeta(StoreMetaMapping::GetPrefix({ oldUuid }), storeMetaMappings, true);
    for (auto &meta : storeMetaMappings) {
        auto oldKey = meta.GetKey();
        meta.deviceId = newUuid;
        MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true);
        MetaDataManager::GetInstance().DelMeta(oldKey, true);
    }
}

void KvStoreMetaManager::UpdateMetaDatas(const std::string &newUuid, const std::string &oldUuid)
{
    MatrixMetaData matrixMeta;
    if (MetaDataManager::GetInstance().LoadMeta(MatrixMetaData::GetPrefix({ oldUuid }), matrixMeta, true)) {
        MetaDataManager::GetInstance().DelMeta(MatrixMetaData::GetPrefix({ oldUuid }), true);
        matrixMeta.deviceId = newUuid;
        MetaDataManager::GetInstance().SaveMeta(MatrixMetaData::GetPrefix({ newUuid }), matrixMeta, true);
    }

    if (MetaDataManager::GetInstance().LoadMeta(MatrixMetaData::GetPrefix({ oldUuid }), matrixMeta)) {
        MetaDataManager::GetInstance().DelMeta(MatrixMetaData::GetPrefix({ oldUuid }));
        matrixMeta.deviceId = newUuid;
        MetaDataManager::GetInstance().SaveMeta(MatrixMetaData::GetPrefix({ newUuid }), matrixMeta);
    }

    SwitchesMetaData switchesMetaData;
    if (MetaDataManager::GetInstance().LoadMeta(SwitchesMetaData::GetPrefix({ oldUuid }),
        switchesMetaData, true)) {
        MetaDataManager::GetInstance().DelMeta(SwitchesMetaData::GetPrefix({ oldUuid }), true);
        switchesMetaData.deviceId = newUuid;
        MetaDataManager::GetInstance().SaveMeta(SwitchesMetaData::GetPrefix({ newUuid }),
            switchesMetaData, true);
    }

    UserMetaData userMeta;
    if (MetaDataManager::GetInstance().LoadMeta(UserMetaRow::GetKeyFor(oldUuid), userMeta)) {
        MetaDataManager::GetInstance().DelMeta(UserMetaRow::GetKeyFor(oldUuid));
        userMeta.deviceId = newUuid;
        MetaDataManager::GetInstance().SaveMeta(UserMetaRow::GetKeyFor(newUuid), userMeta);
    }
        
    CapMetaData capMetaData;
    auto capKey = CapMetaRow::GetKeyFor(oldUuid);
    if (MetaDataManager::GetInstance().LoadMeta(std::string(capKey.begin(), capKey.end()), capMetaData)) {
        auto newCapKey = CapMetaRow::GetKeyFor(newUuid);
        MetaDataManager::GetInstance().DelMeta(std::string(capKey.begin(), capKey.end()));
        MetaDataManager::GetInstance().SaveMeta(std::string(newCapKey.begin(), newCapKey.end()), capMetaData);
    }
}
} // namespace DistributedKv
} // namespace OHOS