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
#define LOG_TAG "RuntimeStore"

#include "runtime_store.h"

#include "data_handler.h"
#include "log_print.h"
#include "ipc_skeleton.h"
#include "unified_data_helper.h"
#include "udmf_radar_reporter.h"
#include "account/account_delegate.h"
#include "metadata/meta_data_manager.h"
#include "metadata/appid_meta_data.h"
#include "device_manager_adapter.h"
#include "bootstrap.h"
#include "directory/directory_manager.h"
#include "utils/anonymous.h"

namespace OHOS {
namespace UDMF {
using namespace RadarReporter;
using namespace DistributedDB;
using Anonymous = OHOS::DistributedData::Anonymous;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

constexpr const char *TEMP_UNIFIED_DATA_FLAG = "temp_udmf_file_flag";
static constexpr size_t TEMP_UDATA_RECORD_SIZE = 1;

RuntimeStore::RuntimeStore(const std::string &storeId) : storeId_(storeId)
{
    UpdateTime();
    ZLOGD("Construct runtimeStore: %{public}s.", Anonymous::Change(storeId_).c_str());
}

RuntimeStore::~RuntimeStore()
{
    ZLOGD("Destruct runtimeStore: %{public}s.", Anonymous::Change(storeId_).c_str());
}

Status RuntimeStore::PutLocal(const std::string &key, const std::string &value)
{
    UpdateTime();
    std::vector<uint8_t> keyBytes = {key.begin(), key.end()};
    std::vector<uint8_t> valueBytes = {value.begin(), value.end()};

    auto status = kvStore_->PutLocal(keyBytes, valueBytes);
    if (status != DBStatus::OK) {
        ZLOGE("KvStore PutLocal failed, status: %{public}d.", status);
        return E_DB_ERROR;
    }
    return E_OK;
}

Status RuntimeStore::GetLocal(const std::string &key, std::string &value)
{
    UpdateTime();
    std::vector<uint8_t> valueBytes;
    std::vector<uint8_t> keyBytes = {key.begin(), key.end()};
    DBStatus status = kvStore_->GetLocal(keyBytes, valueBytes);
    if (status != DBStatus::OK && status != DBStatus::NOT_FOUND) {
        ZLOGE("GetLocal entry failed, key: %{public}s.", key.c_str());
        return E_DB_ERROR;
    }
    if (valueBytes.empty()) {
        ZLOGW("GetLocal entry is empty, key: %{public}s", key.c_str());
        return E_NOT_FOUND;
    }
    value = {valueBytes.begin(), valueBytes.end()};
    return E_OK;
}

Status RuntimeStore::DeleteLocal(const std::string &key)
{
    UpdateTime();
    std::vector<uint8_t> valueBytes;
    std::vector<uint8_t> keyBytes = {key.begin(), key.end()};
    DBStatus status = kvStore_->DeleteLocal(keyBytes);
    if (status != DBStatus::OK && status != DBStatus::NOT_FOUND) {
        ZLOGE("DeleteLocal failed, key: %{public}s.", key.c_str());
        return E_DB_ERROR;
    }
    return E_OK;
}

Status RuntimeStore::Put(const UnifiedData &unifiedData)
{
    UpdateTime();
    std::vector<Entry> entries;
    auto status = DataHandler::MarshalToEntries(unifiedData, entries);
    if (status != E_OK) {
        return status;
    }
    return PutEntries(entries);
}

Status RuntimeStore::Get(const std::string &key, UnifiedData &unifiedData)
{
    UpdateTime();
    std::vector<Entry> entries;
    if (GetEntries(key, entries) != E_OK) {
        ZLOGE("GetEntries failed, dataPrefix: %{public}s.", key.c_str());
        return E_DB_ERROR;
    }
    if (entries.empty()) {
        ZLOGW("entries is empty, dataPrefix: %{public}s", key.c_str());
        return E_NOT_FOUND;
    }
    return DataHandler::UnmarshalEntries(key, entries, unifiedData);
}

bool RuntimeStore::GetDetailsFromUData(UnifiedData &data, UDDetails &details)
{
    auto records = data.GetRecords();
    if (records.size() != TEMP_UDATA_RECORD_SIZE) {
        return false;
    }
    if (records[0] == nullptr || records[0]->GetType() != UDType::FILE) {
        return false;
    }
    auto obj = std::get<std::shared_ptr<Object>>(records[0]->GetOriginValue());
    if (obj == nullptr) {
        ZLOGE("ValueType is not Object!");
        return false;
    }
    std::shared_ptr<Object> detailObj;
    obj->GetValue(DETAILS, detailObj);
        if (detailObj == nullptr) {
            ZLOGE("Not contain details for object!");
            return false;
        }
    auto result = ObjectUtils::ConvertToUDDetails(detailObj);
    if (result.find(TEMP_UNIFIED_DATA_FLAG) == result.end()) {
        return false;
    }
    details = result;
    return true;
}

Status RuntimeStore::GetSummaryFromDetails(const UDDetails &details, Summary &summary)
{
    for (auto &item : details) {
        if (item.first != TEMP_UNIFIED_DATA_FLAG) {
            auto int64Value = std::get_if<int64_t>(&item.second);
            if (int64Value != nullptr) {
                auto size = std::get<int64_t>(item.second);
                summary.summary[item.first] = size;
                summary.totalSize += size;
            }
        }
    }
    return E_OK;
}

Status RuntimeStore::GetSummary(const std::string &key, Summary &summary)
{
    UpdateTime();
    UnifiedData unifiedData;
    if (Get(key, unifiedData) != E_OK) {
        ZLOGE("Get unified data failed, dataPrefix: %{public}s", key.c_str());
        return E_DB_ERROR;
    }

    UDDetails details {};
    if (GetDetailsFromUData(unifiedData, details)) {
        return GetSummaryFromDetails(details, summary);
    }
    UnifiedDataHelper::GetSummary(unifiedData, summary);
    return E_OK;
}

Status RuntimeStore::Update(const UnifiedData &unifiedData)
{
    std::string key = unifiedData.GetRuntime()->key.key;
    if (Delete(key) != E_OK) {
        UpdateTime();
        ZLOGE("Delete unified data failed, dataPrefix: %{public}s.", key.c_str());
        return E_DB_ERROR;
    }
    if (Put(unifiedData) != E_OK) {
        ZLOGE("Update unified data failed, dataPrefix: %{public}s.", key.c_str());
        return E_DB_ERROR;
    }
    return E_OK;
}

Status RuntimeStore::Delete(const std::string &key)
{
    std::vector<Entry> entries;
    if (GetEntries(key, entries) != E_OK) {
        ZLOGE("GetEntries failed, dataPrefix: %{public}s.", key.c_str());
        return E_DB_ERROR;
    }
    if (entries.empty()) {
        ZLOGD("entries is empty.");
        return E_OK;
    }
    std::vector<Key> keys;
    for (const auto &entry : entries) {
        keys.push_back(entry.key);
    }
    return DeleteEntries(keys);
}

Status RuntimeStore::DeleteBatch(const std::vector<std::string> &unifiedKeys)
{
    UpdateTime();
    ZLOGD("called!");
    if (unifiedKeys.empty()) {
        ZLOGD("No need to delete!");
        return E_OK;
    }
    for (const std::string &unifiedKey : unifiedKeys) {
        if (Delete(unifiedKey) != E_OK) {
            ZLOGE("Delete failed, key: %{public}s.", unifiedKey.c_str());
            return E_DB_ERROR;
        }
    }
    return E_OK;
}

Status RuntimeStore::Sync(const std::vector<std::string> &devices)
{
    UpdateTime();
    if (devices.empty()) {
        ZLOGE("devices empty, no need sync.");
        return E_INVALID_PARAMETERS;
    }
    std::vector<std::string> syncDevices = DmAdapter::ToUUID(devices);
    auto onComplete = [this](const std::map<std::string, DBStatus> &devsSyncStatus) {
        DBStatus dbStatus = DBStatus::OK;
        for (const auto &[originDeviceId, status] : devsSyncStatus) {  // only one device.
            if (status != DBStatus::OK) {
                dbStatus = status;
                break;
            }
        }
        if (dbStatus != DBStatus::OK) {
            RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
                BizScene::SYNC_DATA, SyncDataStage::SYNC_END, StageRes::FAILED, dbStatus, BizState::DFX_END);
        } else {
            RadarReporterAdapter::ReportNormal(std::string(__FUNCTION__),
                BizScene::SYNC_DATA, SyncDataStage::SYNC_END, StageRes::SUCCESS, BizState::DFX_END);
        }

        ZLOGI("sync complete, %{public}s, status:%{public}d.", Anonymous::Change(storeId_).c_str(), dbStatus);
    };
    DBStatus status = kvStore_->Sync(syncDevices, SyncMode::SYNC_MODE_PULL_ONLY, onComplete);
    if (status != DBStatus::OK) {
        ZLOGE("Sync kvStore failed, status: %{public}d.", status);
        return E_DB_ERROR;
    }
    return E_OK;
}

Status RuntimeStore::Sync(const std::vector<std::string> &devices, ProcessCallback callback)
{
    UpdateTime();
    if (devices.empty()) {
        ZLOGE("devices empty, no need sync.");
        return E_INVALID_PARAMETERS;
    }
    std::vector<std::string> syncDevices = DmAdapter::ToUUID(devices);
    DevNameMap deviceNameMap;
    for (const auto &device : devices) {
        std::string deviceUuid = DmAdapter::GetInstance().ToUUID(device);
        std::string deviceName = DmAdapter::GetInstance().GetDeviceInfo(device).deviceName;
        deviceNameMap.emplace(deviceUuid, deviceName);
    }
    auto progressCallback = [this, callback, deviceNameMap](const DevSyncProcessMap &processMap) {
        this->NotifySyncProcss(processMap, callback, deviceNameMap);
    };

    DistributedDB::DeviceSyncOption option;
    option.devices = syncDevices;
    option.isWait = false;
    DBStatus status = kvStore_->Sync(option, progressCallback);
    if (status != DBStatus::OK) {
        ZLOGE("Sync kvStore failed, status: %{public}d.", status);
        return E_DB_ERROR;
    }
    return E_OK;
}

void RuntimeStore::NotifySyncProcss(const DevSyncProcessMap &processMap, ProcessCallback callback,
    const DevNameMap &deviceNameMap)
{
    AsyncProcessInfo processInfo;
    for (const auto &[originDeviceId, syncProcess] : processMap) { // only one device
        processInfo.srcDevName = deviceNameMap.at(originDeviceId);
        processInfo.syncId = syncProcess.syncId;
        processInfo.syncFinished = syncProcess.pullInfo.finishedCount;
        processInfo.syncTotal = syncProcess.pullInfo.total;
        if (syncProcess.process != DistributedDB::ProcessStatus::FINISHED) {
            processInfo.syncStatus = ASYNC_RUNNING;
            continue;
        }
        if (syncProcess.errCode == DBStatus::OK) {
            processInfo.syncStatus = ASYNC_SUCCESS;
            RadarReporterAdapter::ReportNormal(std::string(__FUNCTION__),
                BizScene::SYNC_DATA, SyncDataStage::SYNC_END, StageRes::SUCCESS, BizState::DFX_END);
        } else {
            processInfo.syncStatus = ASYNC_FAILURE;
            RadarReporterAdapter::ReportFail(std::string(__FUNCTION__),
                BizScene::SYNC_DATA, SyncDataStage::SYNC_END, StageRes::FAILED, syncProcess.errCode, BizState::DFX_END);
        }
    }
    callback(processInfo);
}

Status RuntimeStore::Clear()
{
    UpdateTime();
    return Delete(DATA_PREFIX);
}

Status RuntimeStore::GetBatchData(const std::string &dataPrefix, std::vector<UnifiedData> &unifiedDataSet)
{
    UpdateTime();
    std::vector<Entry> entries;
    auto status = GetEntries(dataPrefix, entries);
    if (status != E_OK) {
        ZLOGE("GetEntries failed, dataPrefix: %{public}s.", dataPrefix.c_str());
        return E_DB_ERROR;
    }
    if (entries.empty()) {
        ZLOGD("entries is empty.");
        return E_OK;
    }
    std::vector<std::string> keySet;
    for (const auto &entry : entries) {
        std::string keyStr = {entry.key.begin(), entry.key.end()};
        if (std::count(keyStr.begin(), keyStr.end(), '/') == SLASH_COUNT_IN_KEY) {
            keySet.emplace_back(keyStr);
        }
    }

    for (const std::string &key : keySet) {
        UnifiedData data;
        if (DataHandler::UnmarshalEntries(key, entries, data) != E_OK) {
            return E_READ_PARCEL_ERROR;
        }
        unifiedDataSet.emplace_back(data);
    }
    return E_OK;
}

void RuntimeStore::Close()
{
    delegateManager_->CloseKvStore(kvStore_.get());
}

bool RuntimeStore::Init()
{
    if (!SaveMetaData()) {  // get keyinfo about create db fail.
        ZLOGW("Save meta data fail.");
        return false;
    }
    DistributedDB::KvStoreNbDelegate::Option option;
    option.createIfNecessary = true;
    option.isMemoryDb = false;
    option.createDirByStoreIdOnly = true;
    option.isEncryptedDb = false;
    option.isNeedRmCorruptedDb = true;
    option.syncDualTupleMode = true;
    option.secOption = {DistributedKv::SecurityLevel::S1, DistributedDB::ECE};
    DistributedDB::KvStoreNbDelegate *delegate = nullptr;
    DBStatus status = DBStatus::NOT_SUPPORT;
    delegateManager_->GetKvStore(storeId_, option,
                                 [&delegate, &status](DBStatus dbStatus, KvStoreNbDelegate *nbDelegate) {
                                     delegate = nbDelegate;
                                     status = dbStatus;
                                 });
    if (status != DBStatus::OK) {
        ZLOGE("GetKvStore fail, status: %{public}d.", static_cast<int>(status));
        return false;
    }

    auto release = [this](KvStoreNbDelegate *delegate) {
        ZLOGI("Release runtime kvStore.");
        if (delegate == nullptr) {
            return;
        }
        auto retStatus = delegateManager_->CloseKvStore(delegate);
        if (retStatus != DBStatus::OK) {
            ZLOGE("CloseKvStore fail, status: %{public}d.", static_cast<int>(retStatus));
        }
    };
    kvStore_ = std::shared_ptr<KvStoreNbDelegate>(delegate, release);
    uint32_t pragmData = 16 * 1024 * 1024;
    PragmaData input = static_cast<PragmaData>(&pragmData);
    kvStore_->Pragma(SET_MAX_VALUE_SIZE, input);
    return true;
}

bool RuntimeStore::BuildMetaDataParam(DistributedData::StoreMetaData &metaData)
{
    auto localDeviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (localDeviceId.empty()) {
        ZLOGE("failed to get local device id");
        return false;
    }

    uint32_t token = IPCSkeleton::GetSelfTokenID();
    const std::string userId = std::to_string(DistributedData::AccountDelegate::GetInstance()->GetUserByToken(token));
    metaData.appType = "harmony";
    metaData.deviceId = localDeviceId;
    metaData.storeId = storeId_;
    metaData.isAutoSync = false;
    metaData.isBackup = false;
    metaData.isEncrypt = false;
    metaData.bundleName = DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    metaData.appId = DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    metaData.user = userId;
    metaData.account = DistributedData::AccountDelegate::GetInstance()->GetCurrentAccountId();
    metaData.tokenId = token;
    metaData.securityLevel = DistributedKv::SecurityLevel::S1;
    metaData.area = DistributedKv::Area::EL1;
    metaData.uid = static_cast<int32_t>(getuid());
    metaData.storeType = DistributedKv::KvStoreType::SINGLE_VERSION;
    metaData.dataType = DistributedKv::DataType::TYPE_DYNAMICAL;
    metaData.dataDir = DistributedData::DirectoryManager::GetInstance().GetStorePath(metaData);

    return true;
}

bool RuntimeStore::SaveMetaData()
{
    DistributedData::StoreMetaData saveMeta;
    if (!BuildMetaDataParam(saveMeta)) {
        return false;
    }

    int foregroundUserId = 0;
    bool ret = DistributedData::AccountDelegate::GetInstance()->QueryForegroundUserId(foregroundUserId);
    if (!ret) {
        ZLOGE("QueryForegroundUserId failed.");
        return false;
    }

    saveMeta.dataDir.append("/").append(std::to_string(foregroundUserId));
    if (!DistributedData::DirectoryManager::GetInstance().CreateDirectory(saveMeta.dataDir)) {
        ZLOGE("Create directory error");
        return false;
    }

    SetDelegateManager(saveMeta.dataDir, saveMeta.appId, saveMeta.user, std::to_string(foregroundUserId));

    DistributedData::StoreMetaData loadLocal;
    DistributedData::StoreMetaData syncMeta;
    if (DistributedData::MetaDataManager::GetInstance().LoadMeta(saveMeta.GetKey(), loadLocal, true) &&
        DistributedData::MetaDataManager::GetInstance().LoadMeta(saveMeta.GetKey(), syncMeta, false)) {
        ZLOGD("Meta data is already saved.");
        return true;
    }

    auto saved = DistributedData::MetaDataManager::GetInstance().SaveMeta(saveMeta.GetKey(), saveMeta) &&
                 DistributedData::MetaDataManager::GetInstance().SaveMeta(saveMeta.GetKey(), saveMeta, true);
    if (!saved) {
        ZLOGE("SaveMeta failed, saveMeta.key:%{public}s", saveMeta.GetKey().c_str());
        return false;
    }
    DistributedData::AppIDMetaData appIdMeta;
    appIdMeta.bundleName = saveMeta.bundleName;
    appIdMeta.appId = saveMeta.appId;
    saved = DistributedData::MetaDataManager::GetInstance().SaveMeta(appIdMeta.GetKey(), appIdMeta, true);
    if (!saved) {
        ZLOGE("Save appIdMeta failed, appIdMeta.key:%{public}s", appIdMeta.GetKey().c_str());
        return false;
    }
    return true;
}

void RuntimeStore::SetDelegateManager(const std::string &dataDir, const std::string &appId, const std::string &userId,
    const std::string &subUser)
{
    delegateManager_ = std::make_shared<DistributedDB::KvStoreDelegateManager>(appId, userId, subUser);
    DistributedDB::KvStoreConfig kvStoreConfig { dataDir };
    delegateManager_->SetKvStoreConfig(kvStoreConfig);
}

Status RuntimeStore::GetEntries(const std::string &dataPrefix, std::vector<Entry> &entries)
{
    Query dbQuery = Query::Select();
    std::vector<uint8_t> prefix = {dataPrefix.begin(), dataPrefix.end()};
    dbQuery.PrefixKey(prefix);
    dbQuery.OrderByWriteTime(true);
    DBStatus status = kvStore_->GetEntries(dbQuery, entries);
    if (status != DBStatus::OK && status != DBStatus::NOT_FOUND) {
        ZLOGE("KvStore getEntries failed, status: %{public}d.", static_cast<int>(status));
        return E_DB_ERROR;
    }
    return E_OK;
}

Status RuntimeStore::PutEntries(const std::vector<Entry> &entries)
{
    DBStatus status = kvStore_->PutBatch(entries);
    if (status != DBStatus::OK) {
        ZLOGE("putBatch failed, status: %{public}d.", status);
        return E_DB_ERROR;
    }
    return E_OK;
}

Status RuntimeStore::DeleteEntries(const std::vector<Key> &keys)
{
    DBStatus status = kvStore_->DeleteBatch(keys);
    if (status != DBStatus::OK) {
        ZLOGE("deleteBatch failed, status: %{public}d.", status);
        return E_DB_ERROR;
    }
    return E_OK;
}

} // namespace UDMF
} // namespace OHOS