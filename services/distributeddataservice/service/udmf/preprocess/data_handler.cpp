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
constexpr const char *SLICE_SEPARATOR = "-";
constexpr const char *UD_KEY_SEPARATOR = "/";
static constexpr size_t MAX_KV_DATA_SIZE = 4 * 1024 * 1024;

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

    return Split(unifiedData.GetRecords(), unifiedKey, entries);
}

Status DataHandler::UnmarshalEntries(const std::string &key, const std::vector<Entry> &entries,
    UnifiedData &unifiedData)
{
    std::map<std::string, std::vector<std::vector<uint8_t>>> sliceMap;
    for (const auto &entry : entries) {
        std::string keyStr = { entry.key.begin(), entry.key.end() };
        if (keyStr == key) {
            Runtime runtime;
            auto runtimeTlv = TLVObject(const_cast<std::vector<uint8_t> &>(entry.value));
            if (!TLVUtil::ReadTlv(runtime, runtimeTlv, TAG::TAG_RUNTIME)) {
                ZLOGE("Unmarshall runtime info failed.");
                return E_READ_PARCEL_ERROR;
            }
            unifiedData.SetRuntime(runtime);
            continue;
        }
        if (keyStr.find(key) == 0) {
            auto status = Assembler(entry, keyStr, sliceMap, unifiedData);
            if (status != E_OK) {
                return status;
            }
        }
    }
    for (auto [uid, slice] : sliceMap) {
        std::vector<uint8_t> rawData;
        for (auto sliceData : slice) {
            rawData.insert(rawData.end(), sliceData.begin(), sliceData.end());
        }
        std::shared_ptr<UnifiedRecord> record;
        auto recordTlv = TLVObject(rawData);
        if (!TLVUtil::ReadTlv(record, recordTlv, TAG::TAG_UNIFIED_RECORD)) {
            ZLOGE("Unmarshall unified record failed for slice data.");
            return E_READ_PARCEL_ERROR;
        }
        unifiedData.AddRecord(std::move(record));
    }
    return E_OK;
}

Status DataHandler::Split(const std::vector<std::shared_ptr<UnifiedRecord>> &records, const std::string &unifiedKey,
    std::vector<Entry> &entries)
{
    for (const auto &record : records) {
        std::vector<uint8_t> recordBytes;
        auto recordTlv = TLVObject(recordBytes);
        if (!TLVUtil::Writing(record, recordTlv, TAG::TAG_UNIFIED_RECORD)) {
            ZLOGI("Marshall unified record failed.");
            return E_WRITE_PARCEL_ERROR;
        }
        std::string recordKey = unifiedKey + UD_KEY_SEPARATOR + record->GetUid();
        auto recordSize = recordBytes.size();
        if (recordSize <= MAX_KV_DATA_SIZE) {
            std::vector<uint8_t> keyBytes = { recordKey.begin(), recordKey.end() };
            Entry entry = { keyBytes, recordBytes };
            entries.emplace_back(entry);
            continue;
        }
        size_t offset = 0;
        int sliceNum = 1;
        while (offset < recordSize) {
            auto chunkSize = std::min(MAX_KV_DATA_SIZE, recordSize - offset);
            std::vector<uint8_t> chunk(recordBytes.begin() + offset, recordBytes.begin() + offset + chunkSize);
            std::string recordKeySplitSeq = recordKey + SLICE_SEPARATOR + std::to_string(sliceNum);
            std::vector<uint8_t> keyBytes = { recordKeySplitSeq.begin(), recordKeySplitSeq.end() };
            Entry entry = { keyBytes, chunk };
            entries.emplace_back(entry);
            sliceNum++;
            offset += chunkSize;
        }
    }
    return E_OK;
}

Status DataHandler::Assembler(const Entry &entry, const std::string &keyStr,
    std::map<std::string, std::vector<std::vector<uint8_t>>> &sliceMap, UnifiedData &unifiedData)
{
    auto suffixKey = keyStr.substr(keyStr.rfind(UD_KEY_SEPARATOR) + 1);
    if (suffixKey.rfind(SLICE_SEPARATOR) == std::string::npos) {
        std::shared_ptr<UnifiedRecord> record;
        auto recordTlv = TLVObject(const_cast<std::vector<uint8_t> &>(entry.value));
        if (!TLVUtil::ReadTlv(record, recordTlv, TAG::TAG_UNIFIED_RECORD)) {
            ZLOGE("Unmarshall unified record failed.");
            return E_READ_PARCEL_ERROR;
        }
        unifiedData.AddRecord(std::move(record));
        return E_OK;
    }
    auto separatorPos = suffixKey.rfind(SLICE_SEPARATOR);
    size_t sliceNum = std::stoull(suffixKey.substr(separatorPos + 1));
    std::string uid = suffixKey.substr(0, separatorPos);
    if (sliceMap[uid].size() < sliceNum) {
        sliceMap[uid].resize(sliceNum);
    }
    sliceMap[uid][sliceNum - 1] = std::move(entry.value);
    return E_OK;
}
} // namespace UDMF::OHOS