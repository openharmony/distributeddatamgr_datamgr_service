/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#define LOG_TAG "DataHandler"

#include "data_handler.h"
#include "log_print.h"
#include "tlv_util.h"

namespace OHOS::UDMF {
constexpr const char *UD_KEY_SEPARATOR = "/";
constexpr const char *UD_KEY_ENTRY_SEPARATOR = "#";
constexpr const char *UD_KEY_PROPERTIES_SEPARATOR = "#properties";
constexpr const char *UD_KEY_ACCEPTABLE_INFO_SEPARATOR = "#acceptableInfo";

Status DataHandler::MarshalToEntries(const UnifiedData &unifiedData, std::vector<Entry> &entries)
{
    std::string unifiedKey = unifiedData.GetRuntime()->key.GetUnifiedKey();
    std::vector<uint8_t> runtimeBytes;
    auto runtimeTlv = TLVObject(runtimeBytes);
    if (!TLVUtil::Writing(*unifiedData.GetRuntime(), runtimeTlv, TAG::TAG_RUNTIME)) {
        ZLOGE("Runtime info marshalling failed:%{public}s", unifiedKey.c_str());
        return E_WRITE_PARCEL_ERROR;
    }
    std::vector<uint8_t> udKeyBytes = { unifiedKey.begin(), unifiedKey.end() };
    Entry entry = { udKeyBytes, runtimeBytes };
    entries.emplace_back(entry);
    std::string propsKey = unifiedData.GetRuntime()->key.GetKeyCommonPrefix() + UD_KEY_PROPERTIES_SEPARATOR;
    std::vector<uint8_t> propsBytes;
    auto propsTlv = TLVObject(propsBytes);
    if (!TLVUtil::Writing(*unifiedData.GetProperties(), propsTlv, TAG::TAG_PROPERTIES)) {
        ZLOGE("Properties info marshalling failed:%{public}s", propsKey.c_str());
        return E_WRITE_PARCEL_ERROR;
    }
    std::vector<uint8_t> propsKeyBytes = { propsKey.begin(), propsKey.end() };
    Entry propsEntry = { propsKeyBytes, propsBytes };
    entries.emplace_back(std::move(propsEntry));
    return BuildEntries(unifiedData.GetRecords(), unifiedKey, entries);
}

Status DataHandler::MarshalDataLoadEntries(const DataLoadInfo &info, std::vector<Entry> &entries)
{
    std::string acceptableKey = info.sequenceKey + UD_KEY_ACCEPTABLE_INFO_SEPARATOR;
    std::vector<uint8_t> acceptableBytes;
    auto acceptableTlv = TLVObject(acceptableBytes);
    if (!TLVUtil::Writing(info, acceptableTlv, TAG::TAG_DATA_LOAD_INFO)) {
        ZLOGE("Acceptable info marshalling failed:%{public}s", acceptableKey.c_str());
        return E_WRITE_PARCEL_ERROR;
    }
    std::vector<uint8_t> acceptableKeyBytes = { acceptableKey.begin(), acceptableKey.end() };
    Entry entry = { acceptableKeyBytes, acceptableBytes };
    entries.emplace_back(std::move(entry));
    return E_OK;
}

Status DataHandler::UnmarshalDataLoadEntries(const Entry &entry, DataLoadInfo &info)
{
    std::vector<uint8_t> value = std::move(entry.value);
    auto data = TLVObject(value);
    if (!TLVUtil::ReadTlv(info, data, TAG::TAG_DATA_LOAD_INFO)) {
        ZLOGE("Unmarshall data load info failed.");
        return E_READ_PARCEL_ERROR;
    }
    return E_OK;
}

Status DataHandler::UnmarshalEntries(const std::string &key, const std::vector<Entry> &entries,
    UnifiedData &unifiedData)
{
    std::map<std::string, std::shared_ptr<UnifiedRecord>> records;
    std::map<std::string, std::map<std::string, ValueType>> innerEntries;
    auto status = UnmarshalEntryItem(unifiedData, entries, key, records, innerEntries);
    if (status != E_OK) {
        ZLOGE("UnmarshalEntryItem failed.");
        return status;
    }
    for (auto &[entryKey, entryValue] : innerEntries) {
        auto idx = entryKey.rfind(UD_KEY_ENTRY_SEPARATOR);
        std::string recordKey = entryKey.substr(0, idx);
        std::string entryUtdId = entryKey.substr(idx + 1);
        if (records.find(recordKey) != records.end() && entryValue.find(entryUtdId) != entryValue.end()) {
            records[recordKey]->AddEntry(entryUtdId, std::move(entryValue[entryUtdId]));
        }
    }
    return E_OK;
}

Status DataHandler::UnmarshalEntryItem(UnifiedData &unifiedData, const std::vector<Entry> &entries,
    const std::string &key, std::map<std::string, std::shared_ptr<UnifiedRecord>> &records,
    std::map<std::string, std::map<std::string, ValueType>> &innerEntries)
{
    for (const auto &entry : entries) {
        std::string keyStr = { entry.key.begin(), entry.key.end() };
        std::vector<uint8_t> value = std::move(entry.value);
        auto data = TLVObject(value);
        if (keyStr == key) {
            Runtime runtime;
            if (!TLVUtil::ReadTlv(runtime, data, TAG::TAG_RUNTIME)) {
                ZLOGE("Unmarshall runtime info failed.");
                return E_READ_PARCEL_ERROR;
            }
            unifiedData.SetRuntime(runtime);
            continue;
        }
        auto isStartWithKey = keyStr.find(key) == 0;
        std::string propsKey = UnifiedKey(key).GetKeyCommonPrefix() + UD_KEY_PROPERTIES_SEPARATOR;
        if (!isStartWithKey && (keyStr == propsKey)) {
            std::shared_ptr<UnifiedDataProperties> properties;
            if (!TLVUtil::ReadTlv(properties, data, TAG::TAG_PROPERTIES)) {
                ZLOGE("Unmarshall unified properties failed.");
                return E_READ_PARCEL_ERROR;
            }
            unifiedData.SetProperties(std::move(properties));
            continue;
        }
        auto isEntryItem = keyStr.rfind(UD_KEY_ENTRY_SEPARATOR) != std::string::npos;
        if (isStartWithKey && !isEntryItem) {
            std::shared_ptr<UnifiedRecord> record;
            if (!TLVUtil::ReadTlv(record, data, TAG::TAG_UNIFIED_RECORD)) {
                ZLOGE("Unmarshall unified record failed.");
                return E_READ_PARCEL_ERROR;
            }
            unifiedData.AddRecord(record);
            records.emplace(keyStr, record);
            continue;
        }
        if (isStartWithKey && isEntryItem) {
            std::shared_ptr<std::map<std::string, ValueType>> entryRead =
                std::make_shared<std::map<std::string, ValueType>>();
            if (!TLVUtil::ReadTlv(entryRead, data, TAG::TAG_INNER_ENTRIES)) {
                ZLOGE("Unmarshall inner entry failed.");
                return E_READ_PARCEL_ERROR;
            }
            innerEntries.emplace(keyStr, std::move(*entryRead));
        }
    }
    return E_OK;
}

Status DataHandler::BuildEntries(const std::vector<std::shared_ptr<UnifiedRecord>> &records,
    const std::string &unifiedKey, std::vector<Entry> &entries)
{
    for (const auto &record : records) {
        if (record == nullptr) {
            continue;
        }
        std::string recordKey = unifiedKey + UD_KEY_SEPARATOR + record->GetUid();
        auto recordEntries = record->GetInnerEntries();
        for (auto &recordEntry : *recordEntries) {
            std::string key = recordKey + UD_KEY_ENTRY_SEPARATOR + recordEntry.first;
            std::vector<uint8_t> entryBytes;
            auto entryTlv = TLVObject(entryBytes);
            const std::shared_ptr<std::map<std::string, ValueType>> entryMap =
                std::make_shared<std::map<std::string, ValueType>>();
            entryMap->insert_or_assign(recordEntry.first, recordEntry.second);
            if (!TLVUtil::Writing(entryMap, entryTlv, TAG::TAG_INNER_ENTRIES)) {
                ZLOGI("Marshall inner entry failed.");
                return E_WRITE_PARCEL_ERROR;
            }
            std::vector<uint8_t> keyBytes = { key.begin(), key.end() };
            Entry entry = { keyBytes, entryBytes };
            entries.emplace_back(entry);
        }
        recordEntries->clear();
        std::vector<uint8_t> recordBytes;
        auto recordTlv = TLVObject(recordBytes);
        if (!TLVUtil::Writing(record, recordTlv, TAG::TAG_UNIFIED_RECORD)) {
            ZLOGI("Marshall unified record failed.");
            return E_WRITE_PARCEL_ERROR;
        }
        std::vector<uint8_t> keyBytes = { recordKey.begin(), recordKey.end() };
        Entry entry = { keyBytes, recordBytes };
        entries.emplace_back(entry);
    }
    return E_OK;
}
} // namespace UDMF::OHOS