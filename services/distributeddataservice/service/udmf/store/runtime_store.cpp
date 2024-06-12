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

#include <algorithm>
#include <vector>
#include<unistd.h>

#include "log_print.h"
#include "ipc_skeleton.h"
#include "tlv_util.h"
#include "account/account_delegate.h"
#include "metadata/store_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/appid_meta_data.h"
#include "device_manager_adapter.h"
#include "bootstrap.h"
#include "directory/directory_manager.h"

namespace OHOS {
namespace UDMF {
using namespace DistributedDB;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

const std::string TEMP_UNIFIED_DATA_FLAG = "temp_udmf_file_flag";
static constexpr size_t TEMP_UDATA_RECORD_SIZE = 1;

RuntimeStore::RuntimeStore(const std::string &storeId) : storeId_(storeId)
{
    UpdateTime();
    ZLOGD("Construct runtimeStore: %{public}s.", storeId_.c_str());
}

RuntimeStore::~RuntimeStore()
{
    ZLOGD("Destruct runtimeStore: %{public}s.", storeId_.c_str());
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
    std::string unifiedKey = unifiedData.GetRuntime()->key.GetUnifiedKey();
    // add runtime info
    std::vector<uint8_t> runtimeBytes;
    auto runtimeTlv = TLVObject(runtimeBytes);
    if (!TLVUtil::Writing(*unifiedData.GetRuntime(), runtimeTlv)) {
        ZLOGE("Marshall runtime info failed, dataPrefix: %{public}s.", unifiedKey.c_str());
        return E_WRITE_PARCEL_ERROR;
    }
    std::vector<uint8_t> udKeyBytes = {unifiedKey.begin(), unifiedKey.end()};
    Entry entry = {udKeyBytes, runtimeBytes};
    entries.push_back(entry);

    // add unified record
    for (const auto &record : unifiedData.GetRecords()) {
        std::vector<uint8_t> recordBytes;
        auto recordTlv = TLVObject(recordBytes);
        if (!TLVUtil::Writing(record, recordTlv)) {
            ZLOGI("Marshall unified record failed.");
            return E_WRITE_PARCEL_ERROR;
        }
        std::string recordKey = unifiedKey + "/" + record->GetUid();
        std::vector<uint8_t> keyBytes = {recordKey.begin(), recordKey.end() };
        Entry entry = { keyBytes, recordBytes };
        entries.push_back(entry);
    }
    auto status = PutEntries(entries);
    return status;
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
    return UnmarshalEntries(key, entries, unifiedData);
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
    auto file = static_cast<File*>(records[0].get());
    if (file == nullptr) {
        return false;
    }
    auto result = file->GetDetails();
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
    for (const auto &record : unifiedData.GetRecords()) {
        int64_t recordSize = record->GetSize();
        auto udType = UD_TYPE_MAP.at(record->GetType());
        auto it = summary.summary.find(udType);
        if (it == summary.summary.end()) {
            summary.summary[udType] = recordSize;
        } else {
            summary.summary[udType] += recordSize;
        }
        summary.totalSize += recordSize;
    }
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
    auto onComplete = [this](const std::map<std::string, DBStatus> &) {
        ZLOGI("sync complete, %{public}s.", storeId_.c_str());
    };
    DBStatus status = kvStore_->Sync(syncDevices, SyncMode::SYNC_MODE_PULL_ONLY, onComplete);
    if (status != DBStatus::OK) {
        ZLOGE("Sync kvStore failed, status: %{public}d.", status);
        return E_DB_ERROR;
    }
    return E_OK;
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
        if (UnmarshalEntries(key, entries, data) != E_OK) {
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
    return true;
}

bool RuntimeStore::SaveMetaData()
{
    auto localDeviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (localDeviceId.empty()) {
        ZLOGE("failed to get local device id");
        return false;
    }

    uint32_t token = IPCSkeleton::GetSelfTokenID();
    const std::string userId = std::to_string(DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(token));
    DistributedData::StoreMetaData saveMeta;
    saveMeta.appType = "harmony";
    saveMeta.deviceId = localDeviceId;
    saveMeta.storeId = storeId_;
    saveMeta.isAutoSync = false;
    saveMeta.isBackup = false;
    saveMeta.isEncrypt = false;
    saveMeta.bundleName = DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    saveMeta.appId = DistributedData::Bootstrap::GetInstance().GetProcessLabel();
    saveMeta.user =  userId;
    saveMeta.account = DistributedKv::AccountDelegate::GetInstance()->GetCurrentAccountId();
    saveMeta.tokenId = token;
    saveMeta.securityLevel = DistributedKv::SecurityLevel::S1;
    saveMeta.area = DistributedKv::Area::EL1;
    saveMeta.uid = static_cast<int32_t>(getuid());
    saveMeta.storeType = DistributedKv::KvStoreType::SINGLE_VERSION;
    saveMeta.dataType = DistributedKv::DataType::TYPE_DYNAMICAL;
    saveMeta.dataDir = DistributedData::DirectoryManager::GetInstance().GetStorePath(saveMeta);

    SetDelegateManager(saveMeta.dataDir, saveMeta.appId, userId);

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

void RuntimeStore::SetDelegateManager(const std::string &dataDir, const std::string &appId, const std::string &userId)
{
    delegateManager_ = std::make_shared<DistributedDB::KvStoreDelegateManager>(appId, userId);
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
    DBStatus status = kvStore_->StartTransaction();
    if (status != DBStatus::OK) {
        ZLOGE("start transaction failed, status: %{public}d.", status);
        return E_DB_ERROR;
    }
    status = kvStore_->PutBatch(entries);
    if (status != DBStatus::OK) {
        ZLOGE("putBatch failed, status: %{public}d.", status);
        status = kvStore_->Rollback();
        if (status != DBStatus::OK) {
            ZLOGE("rollback failed, status: %{public}d.", status);
        }
        return E_DB_ERROR;
    }
    status = kvStore_->Commit();
    if (status != DBStatus::OK) {
        ZLOGE("commit failed, status: %{public}d.", status);
        return E_DB_ERROR;
    }
    return E_OK;
}

Status RuntimeStore::DeleteEntries(const std::vector<Key> &keys)
{
    DBStatus status = kvStore_->StartTransaction();
    if (status != DBStatus::OK) {
        ZLOGE("start transaction failed, status: %{public}d.", status);
        return E_DB_ERROR;
    }
    status = kvStore_->DeleteBatch(keys);
    if (status != DBStatus::OK) {
        ZLOGE("deleteBatch failed, status: %{public}d.", status);
        status = kvStore_->Rollback();
        if (status != DBStatus::OK) {
            ZLOGE("rollback failed, status: %{public}d.", status);
        }
        return E_DB_ERROR;
    }
    status = kvStore_->Commit();
    if (status != DBStatus::OK) {
        ZLOGE("commit failed, status: %{public}d.", status);
        return E_DB_ERROR;
    }
    return E_OK;
}

Status RuntimeStore::UnmarshalEntries(const std::string &key, std::vector<Entry> &entries, UnifiedData &unifiedData)
{
    for (const auto &entry : entries) {
        std::string keyStr = {entry.key.begin(), entry.key.end()};
        if (keyStr == key) {
            Runtime runtime;
            auto runtimeTlv = TLVObject(const_cast<std::vector<uint8_t> &>(entry.value));
            if (!TLVUtil::Reading(runtime, runtimeTlv)) {
                ZLOGE("Unmarshall runtime info failed.");
                return E_READ_PARCEL_ERROR;
            }
            unifiedData.SetRuntime(runtime);
        } else if (keyStr.find(key) == 0) {
            std::shared_ptr<UnifiedRecord> record;
            auto recordTlv = TLVObject(const_cast<std::vector<uint8_t> &>(entry.value));
            if (!TLVUtil::Reading(record, recordTlv)) {
                ZLOGE("Unmarshall unified record failed.");
                return E_READ_PARCEL_ERROR;
            }
            unifiedData.AddRecord(record);
        }
    }
    return E_OK;
}
} // namespace UDMF
} // namespace OHOS