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
#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include "unified_data.h"
#include "tlv_util.h"
#include "types_export.h"

namespace OHOS::UDMF {
using namespace DistributedDB;

class DataHandler {
public:
    static Status MarshalToEntries(const UnifiedData &unifiedData, std::vector<Entry> &entries);
    static Status UnmarshalEntries(const std::string &key, const std::vector<Entry> &entries,
        UnifiedData &unifiedData);
    template <typename T>
    static Status MarshalToEntries(const T &data, Value &value, TAG tag);
    template <typename T>
    static Status UnmarshalEntries(const Value &value, T &data, TAG tag);
    static Status MarshallDataLoadEntries(const DataLoadInfo &info, std::vector<Entry> &entries);
    static Status UnmarshalDataLoadEntries(const Entry &entry, DataLoadInfo &info);

private:
    static Status BuildEntries(const std::vector<std::shared_ptr<UnifiedRecord>> &records,
        const std::string &unifiedKey, std::vector<Entry> &entries);
    static Status UnmarshalEntryItem(UnifiedData &unifiedData, const std::vector<Entry> &entries,
        const std::string &key, std::map<std::string, std::shared_ptr<UnifiedRecord>> &records,
        std::map<std::string, std::map<std::string, ValueType>> &innerEntries);
};

template <typename T>
Status DataHandler::MarshalToEntries(const T &data, Value &value, TAG tag)
{
    auto tlvObject = TLVObject(value);
    if (!TLVUtil::Writing(data, tlvObject, tag)) {
        return E_WRITE_PARCEL_ERROR;
    }
    return E_OK;
}

template <typename T>
Status DataHandler::UnmarshalEntries(const Value &value, T &data, TAG tag)
{
    auto tlvObject = TLVObject(const_cast<std::vector<uint8_t> &>(value));
    if (!TLVUtil::ReadTlv(data, tlvObject, tag)) {
        return E_READ_PARCEL_ERROR;
    }
    return E_OK;
}
} // namespace UDMF::OHOS
#endif // DATA_HANDLER_H