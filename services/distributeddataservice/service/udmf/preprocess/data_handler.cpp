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
#include "tlv_object.h"
#include "tlv_util.h"

namespace OHOS::UDMF {
constexpr const char *UD_KEY_SEPARATOR = "/";
constexpr const char *UD_KEY_ENTRY_SEPARATOR = "#";

Status DataHandler::MarshalToEntries(const UnifiedData &unifiedData, std::vector<Entry> &entries)
{
    std::string unifiedKey = unifiedData.GetRuntime()->key.GetUnifiedKey();
    std::vector<uint8_t> runtimeBytes;
    auto runtimeTlv = TLVObject(runtimeBytes);
    if (!TLVUtil::Writing(*unifiedData.GetRuntime(), runtimeTlv, TAG::TAG_RUNTIME)) {
        ZLOGE("Marshall runtime info failed, dataPrefix: %{public}s.", unifiedKey.c_str());
        return E_WRITE_PARCEL_ERROR;
    }
    std::vector<uint8_t> udKeyBytes = { unifiedKey.begin(), unifiedKey.end() };
    Entry entry = { udKeyBytes, runtimeBytes };
    entries.emplace_back(entry);
    return BuildEntries(unifiedData.GetRecords(), unifiedKey, entries);
}

Status DataHandler::UnmarshalEntries(const std::string &key, const std::vector<Entry> &entries,
    UnifiedData &unifiedData)
{
    std::map<std::string, std::shared_ptr<UnifiedRecord>> recordsMap;
    std::map<std::string, std::map<std::string, ValueType>> innerEntries;
    for (const auto &entry : entries) {
        std::string keyStr = { entry.key.begin(), entry.key.end() };
        auto data = TLVObject(const_cast<std::vector<uint8_t> &>(entry.value));
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
        if (!isStartWithKey) {
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
            recordsMap.emplace(keyStr, record);
            continue;
        }
        if (isStartWithKey && isEntryItem) {
            std::shared_ptr<std::map<std::string, ValueType>> entryContainer = std::make_shared<std::map<std::string, ValueType>>();
            if (!TLVUtil::ReadTlv(entryContainer, data, TAG::TAG_INNER_ENTRIES)) {
                ZLOGE("Unmarshall inner entry failed.");
                return E_READ_PARCEL_ERROR;
            }
            innerEntries.emplace(keyStr, *entryContainer);
        }
    }
    for (auto &[key, entryValue] : innerEntries) {
        auto idx = key.rfind(UD_KEY_ENTRY_SEPARATOR);
        std::string recordKey = key.substr(0, idx);
        std::string entryUtdId = key.substr(idx + 1);
        if (recordsMap.find(recordKey) != recordsMap.end() && entryValue.find(entryUtdId) != entryValue.end()) {
            recordsMap[recordKey]->AddEntry(entryUtdId, std::move(entryValue[entryUtdId]));
        }
    }
    return E_OK;
}

Status DataHandler::BuildEntries(const std::vector<std::shared_ptr<UnifiedRecord>> &records,
    const std::string &unifiedKey, std::vector<Entry> &entries)
{
    ZLOGI("BuildEntries  1111");
    for (const auto &record : records) {
        std::string recordKey = unifiedKey + UD_KEY_SEPARATOR + record->GetUid();
        auto recordEntries = record->GetInnerEntries();
        for (auto &recordEntry : *recordEntries) {
            std::string key = recordKey + UD_KEY_ENTRY_SEPARATOR + recordEntry.first;
            std::vector<uint8_t> entryBytes;
            auto entryTlv = TLVObject(entryBytes);
            const std::shared_ptr<std::map<std::string, ValueType>> entryContainer = std::make_shared<std::map<std::string, ValueType>>();
            entryContainer->insert_or_assign(recordEntry.first, recordEntry.second);
            if (!TLVUtil::Writing(entryContainer, entryTlv, TAG::TAG_INNER_ENTRIES)) {
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