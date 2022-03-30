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

#include "virtual_single_ver_sync_db_Interface.h"

#include <algorithm>
#include <thread>

#include "db_common.h"
#include "db_errno.h"
#include "generic_single_ver_kv_entry.h"
#include "intercepted_data_impl.h"
#include "log_print.h"
#include "query_object.h"
#include "securec.h"

namespace DistributedDB {
namespace {
    int GetEntriesFromItems(std::vector<SingleVerKvEntry *> &entries, const std::vector<VirtualDataItem> &dataItems)
    {
        int errCode = E_OK;
        for (auto &item : dataItems) {
            auto entry = new (std::nothrow) GenericSingleVerKvEntry();
            if (entry == nullptr) {
                LOGE("Create entry failed.");
                errCode = -E_OUT_OF_MEMORY;
                break;
            }
            DataItem storageItem;
            storageItem.key = item.key;
            storageItem.value = item.value;
            storageItem.flag = item.flag;
            storageItem.timestamp = item.timestamp;
            storageItem.writeTimestamp = item.writeTimestamp;
            entry->SetEntryData(std::move(storageItem));
            entries.push_back(entry);
        }
        if (errCode != E_OK) {
            for (auto &kvEntry : entries) {
                delete kvEntry;
                kvEntry = nullptr;
            }
            entries.clear();
        }
        return errCode;
    }
}
int VirtualSingleVerSyncDBInterface::GetInterfaceType() const
{
    return SYNC_SVD;
}

void VirtualSingleVerSyncDBInterface::IncRefCount()
{
}

void VirtualSingleVerSyncDBInterface::DecRefCount()
{
}

std::vector<uint8_t> VirtualSingleVerSyncDBInterface::GetIdentifier() const
{
    std::vector<uint8_t> identifier;
    return identifier;
}

int VirtualSingleVerSyncDBInterface::GetMetaData(const Key &key, Value &value) const
{
    auto iter = metadata_.find(key);
    if (iter != metadata_.end()) {
        value = iter->second;
        return E_OK;
    }
    return -E_NOT_FOUND;
}

int VirtualSingleVerSyncDBInterface::PutMetaData(const Key &key, const Value &value)
{
    if (busy_) {
        return -E_BUSY;
    }
    metadata_[key] = value;
    return E_OK;
}

int VirtualSingleVerSyncDBInterface::DeleteMetaData(const std::vector<Key> &keys)
{
    for (const auto &key : keys) {
        (void)metadata_.erase(key);
    }
    return E_OK;
}

int VirtualSingleVerSyncDBInterface::GetAllMetaKeys(std::vector<Key> &keys) const
{
    for (auto iter = metadata_.begin(); iter != metadata_.end(); ++iter) {
        keys.push_back(iter->first);
    }
    LOGD("GetAllMetaKeys size %zu", keys.size());
    return E_OK;
}

int VirtualSingleVerSyncDBInterface::GetSyncData(Timestamp begin, Timestamp end, std::vector<DataItem> &dataItems,
    ContinueToken &continueStmtToken, const DataSizeSpecInfo &dataSizeInfo) const
{
    return -E_NOT_SUPPORT;
}

int VirtualSingleVerSyncDBInterface::GetSyncDataNext(std::vector<DataItem> &dataItems, ContinueToken &continueStmtToken,
    const DataSizeSpecInfo &dataSizeInfo) const
{
    return -E_NOT_SUPPORT;
}

void VirtualSingleVerSyncDBInterface::ReleaseContinueToken(ContinueToken& continueStmtToken) const
{
    return;
}

SchemaObject VirtualSingleVerSyncDBInterface::GetSchemaInfo() const
{
    return schemaObj_;
}

bool VirtualSingleVerSyncDBInterface::CheckCompatible(const std::string& schema, uint8_t type) const
{
    if (schema_.empty() && schema.empty() && ReadSchemaType(type) != SchemaType::UNRECOGNIZED) {
        return true;
    }
    return (schemaObj_.CompareAgainstSchemaString(schema) == -E_SCHEMA_EQUAL_EXACTLY);
}

int VirtualSingleVerSyncDBInterface::PutData(const Key &key, const Value &value, const Timestamp &time, int flag)
{
    VirtualDataItem item;
    item.key = key;
    item.value = value;
    item.timestamp = time;
    item.writeTimestamp = time;
    item.flag = flag;
    item.isLocal = true;
    dbData_.push_back(item);
    return E_OK;
}

void VirtualSingleVerSyncDBInterface::GetMaxTimestamp(Timestamp& stamp) const
{
    for (auto iter = dbData_.begin(); iter != dbData_.end(); ++iter) {
        if (stamp < iter->writeTimestamp) {
            stamp = iter->writeTimestamp;
        }
    }
    LOGD("VirtualSingleVerSyncDBInterface::GetMaxTimestamp time = %" PRIu64, stamp);
}

int VirtualSingleVerSyncDBInterface::RemoveDeviceData(const std::string &deviceName, bool isNeedNotify)
{
    std::lock_guard<std::mutex> autoLock(deviceDataLock_);
    deviceData_.erase(deviceName);
    LOGD("RemoveDeviceData FINISH");
    return E_OK;
}

int VirtualSingleVerSyncDBInterface::GetSyncData(const Key &key, VirtualDataItem &dataItem)
{
    auto iter = std::find_if(dbData_.begin(), dbData_.end(),
        [key](const VirtualDataItem& item) { return item.key == key; });
    if (iter != dbData_.end()) {
        dataItem.key = iter->key;
        dataItem.value = iter->value;
        dataItem.timestamp = iter->timestamp;
        dataItem.writeTimestamp = iter->writeTimestamp;
        dataItem.flag = iter->flag;
        dataItem.isLocal = iter->isLocal;
        return E_OK;
    }
    return -E_NOT_FOUND;
}

int VirtualSingleVerSyncDBInterface::GetSyncData(Timestamp begin, Timestamp end,
    std::vector<SingleVerKvEntry *> &entries, ContinueToken &continueStmtToken,
    const DataSizeSpecInfo &dataSizeInfo) const
{
    std::vector<VirtualDataItem> dataItems;
    int errCode = GetSyncData(begin, end, dataSizeInfo.blockSize, dataItems, continueStmtToken);
    if (errCode != E_OK) {
        LOGE("[VirtualSingleVerSyncDBInterface][GetSyncData] GetSyncData failed err %d", errCode);
        return errCode;
    }
    return GetEntriesFromItems(entries, dataItems);
}

int VirtualSingleVerSyncDBInterface::GetSyncDataNext(std::vector<SingleVerKvEntry *> &entries,
    ContinueToken &continueStmtToken, const DataSizeSpecInfo &dataSizeInfo) const
{
    if (continueStmtToken == nullptr) {
        return -E_NOT_SUPPORT;
    }
    return 0;
}

int VirtualSingleVerSyncDBInterface::GetSyncData(Timestamp begin, Timestamp end, uint32_t blockSize,
    std::vector<VirtualDataItem> &dataItems, ContinueToken &continueStmtToken) const
{
    for (const auto &data : dbData_) {
        if (data.isLocal) {
            if (data.writeTimestamp >= begin && data.writeTimestamp < end) {
                dataItems.push_back(data);
            }
        }
    }
    continueStmtToken = nullptr;
    LOGD("dataItems size %zu", dataItems.size());
    return E_OK;
}

void VirtualSingleVerSyncDBInterface::SetSaveDataDelayTime(uint64_t milliDelayTime)
{
    saveDataDelayTime_ = milliDelayTime;
}

int VirtualSingleVerSyncDBInterface::GetSyncDataNext(std::vector<VirtualDataItem>& dataItems,
    uint32_t blockSize, ContinueToken& continueStmtToken) const
{
    if (continueStmtToken == nullptr) {
        return -E_NOT_SUPPORT;
    }
    return 0;
}

int VirtualSingleVerSyncDBInterface::PutSyncData(std::vector<VirtualDataItem>& dataItems,
    const std::string &deviceName)
{
    for (auto iter = dataItems.begin(); iter != dataItems.end(); ++iter) {
        LOGD("PutSyncData");
        auto dbDataIter = std::find_if(dbData_.begin(), dbData_.end(),
            [iter](VirtualDataItem item) { return item.key == iter->key; });
        if ((dbDataIter != dbData_.end()) && (dbDataIter->writeTimestamp < iter->writeTimestamp)) {
            // if has conflict, compare writeTimestamp
            LOGI("conflict data time local %" PRIu64 ", remote %" PRIu64, dbDataIter->writeTimestamp,
                iter->writeTimestamp);
            dbDataIter->key = iter->key;
            dbDataIter->value = iter->value;
            dbDataIter->timestamp = iter->timestamp;
            dbDataIter->writeTimestamp = iter->writeTimestamp;
            dbDataIter->flag = iter->flag;
            dbDataIter->isLocal = false;
        } else {
            LOGI("PutSyncData, use remote data %" PRIu64, iter->timestamp);
            VirtualDataItem dataItem;
            dataItem.key = iter->key;
            dataItem.value = iter->value;
            dataItem.timestamp = iter->timestamp;
            dataItem.writeTimestamp = iter->writeTimestamp;
            dataItem.flag = iter->flag;
            dataItem.isLocal = false;
            dbData_.push_back(dataItem);
        }
    }
    return E_OK;
}

void VirtualSingleVerSyncDBInterface::SetSchemaInfo(const std::string& schema)
{
    schema_ = schema;
    SchemaObject emptyObj;
    schemaObj_ = emptyObj;
    schemaObj_.ParseFromSchemaString(schema);
}

const KvDBProperties &VirtualSingleVerSyncDBInterface::GetDbProperties() const
{
    return properties_;
}

int VirtualSingleVerSyncDBInterface::GetSecurityOption(SecurityOption &option) const
{
    option = secOption_;
    return E_OK;
}

bool VirtualSingleVerSyncDBInterface::IsReadable() const
{
    return true;
}

void VirtualSingleVerSyncDBInterface::SetSecurityOption(SecurityOption &option)
{
    secOption_ = option;
}

void VirtualSingleVerSyncDBInterface::NotifyRemotePushFinished(const std::string &targetId) const
{
}

int VirtualSingleVerSyncDBInterface::GetDatabaseCreateTimestamp(Timestamp &outTime) const
{
    return E_OK;
}

int VirtualSingleVerSyncDBInterface::GetSyncData(QueryObject &query, const SyncTimeRange &timeRange,
    const DataSizeSpecInfo &dataSizeInfo, ContinueToken &continueStmtToken,
    std::vector<SingleVerKvEntry *> &entries) const
{
    const auto &startKey = query.GetPrefixKey();
    Key endKey = startKey;
    endKey.resize(DBConstant::MAX_KEY_SIZE, UCHAR_MAX);

    std::vector<VirtualDataItem> dataItems;
    for (const auto &data : dbData_) {
        // Only get local data.
        if (!data.isLocal) {
            continue;
        }

        if ((data.flag & VirtualDataItem::DELETE_FLAG) != 0) {
            if (data.timestamp >= timeRange.deleteBeginTime && data.timestamp < timeRange.deleteEndTime) {
                dataItems.push_back(data);
            }
        } else {
            if (data.timestamp >= timeRange.beginTime && data.timestamp < timeRange.endTime &&
                data.key >= startKey && data.key <= endKey) {
                dataItems.push_back(data);
            }
        }
    }

    LOGD("dataItems size %zu", dataItems.size());
    return GetEntriesFromItems(entries, dataItems);
}

int VirtualSingleVerSyncDBInterface::DeleteMetaDataByPrefixKey(const Key &keyPrefix) const
{
    size_t prefixKeySize = keyPrefix.size();
    for (auto iter = metadata_.begin(); iter != metadata_.end();) {
        if (prefixKeySize <= iter->first.size() &&
            keyPrefix == Key(iter->first.begin(), std::next(iter->first.begin(), prefixKeySize))) {
            iter = metadata_.erase(iter);
        } else {
            ++iter;
        }
    }
    return E_OK;
}

int VirtualSingleVerSyncDBInterface::GetCompressionOption(bool &needCompressOnSync, uint8_t &compressionRate) const
{
    return E_OK;
}

int VirtualSingleVerSyncDBInterface::GetCompressionAlgo(std::set<CompressAlgorithm> &algorithmSet) const
{
    return E_OK;
}

int VirtualSingleVerSyncDBInterface::PutSyncData(const DataItem &item)
{
    return E_OK;
}

int VirtualSingleVerSyncDBInterface::CheckAndInitQueryCondition(QueryObject &query) const
{
    return E_OK;
}

int VirtualSingleVerSyncDBInterface::InterceptData(std::vector<SingleVerKvEntry *> &entries,
    const std::string &sourceID, const std::string &targetID) const
{
    return E_OK;
}

int VirtualSingleVerSyncDBInterface::PutSyncDataWithQuery(const QueryObject &query,
    const std::vector<SingleVerKvEntry *> &entries, const std::string &deviceName)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(saveDataDelayTime_));
    std::vector<VirtualDataItem> dataItems;
    for (auto kvEntry : entries) {
        auto genericKvEntry = static_cast<GenericSingleVerKvEntry *>(kvEntry);
        VirtualDataItem item;
        genericKvEntry->GetKey(item.key);
        genericKvEntry->GetValue(item.value);
        item.timestamp = genericKvEntry->GetTimestamp();
        item.writeTimestamp = genericKvEntry->GetWriteTimestamp();
        item.flag = genericKvEntry->GetFlag();
        item.isLocal = false;
        dataItems.push_back(item);
    }
    return PutSyncData(dataItems, deviceName);
}

int VirtualSingleVerSyncDBInterface::AddSubscribe(const std::string &subscribeId, const QueryObject &query,
    bool needCacheSubscribe)
{
    return E_OK;
}

int VirtualSingleVerSyncDBInterface::RemoveSubscribe(const std::string &subscribeId)
{
    return E_OK;
}

int VirtualSingleVerSyncDBInterface::RemoveSubscribe(const std::vector<std::string> &subscribeIds)
{
    return E_OK;
}

void VirtualSingleVerSyncDBInterface::SetBusy(bool busy)
{
    busy_ = busy;
}

void VirtualSingleVerSyncDBInterface::PutDeviceData(const std::string &deviceName, const Key &key, const Value &value)
{
    std::lock_guard<std::mutex> autoLock(deviceDataLock_);
    deviceData_[deviceName][key] = value;
}

void VirtualSingleVerSyncDBInterface::GetDeviceData(const std::string &deviceName, const Key &key, Value &value)
{
    std::lock_guard<std::mutex> autoLock(deviceDataLock_);
    value = deviceData_[deviceName][key];
}
}  // namespace DistributedDB
