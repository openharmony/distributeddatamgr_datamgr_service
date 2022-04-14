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

#ifndef DISTRIBUTED_DATA_FRAMEWORKS_COMMON_ITYPES_UTIL_H
#define DISTRIBUTED_DATA_FRAMEWORKS_COMMON_ITYPES_UTIL_H
#include <climits>
#include <memory>

#include "change_notification.h"
#include "message_parcel.h"
#include "rdb_types.h"
#include "types.h"

namespace OHOS::DistributedKv {
class ITypesUtil final {
public:
    static bool Marshalling(MessageParcel &data);
    static bool Unmarshalling(MessageParcel &data);

    static bool Marshalling(const std::string &input, MessageParcel &data);
    static bool Unmarshalling(MessageParcel &data, std::string &output);

    static bool Marshalling(const Blob &blob, MessageParcel &data);
    static bool Unmarshalling(MessageParcel &data, Blob &output);

    static bool Marshalling(const Entry &entry, MessageParcel &data);
    static bool Unmarshalling(MessageParcel &data, Entry &output);

    static bool Marshalling(const DeviceInfo &entry, MessageParcel &data);
    static bool Unmarshalling(MessageParcel &data, DeviceInfo &output);

    static bool Marshalling(const ChangeNotification &notification, MessageParcel &parcel);
    static bool Unmarshalling(MessageParcel &parcel, ChangeNotification &output);

    static bool Marshalling(const DistributedRdb::RdbSyncerParam &param, MessageParcel &parcel);
    static bool Unmarshalling(MessageParcel &parcel, DistributedRdb::RdbSyncerParam &param);

    static bool Marshalling(const DistributedRdb::SyncResult &result, MessageParcel &parcel);
    static bool Unmarshalling(MessageParcel &parcel, DistributedRdb::SyncResult &result);

    static bool Marshalling(const DistributedRdb::SyncOption &option, MessageParcel &parcel);
    static bool Unmarshalling(MessageParcel &parcel, DistributedRdb::SyncOption &option);

    static bool Marshalling(const DistributedRdb::RdbPredicates &predicates, MessageParcel &parcel);
    static bool Unmarshalling(MessageParcel &parcel, DistributedRdb::RdbPredicates &predicates);

    static bool Marshalling(const Options &input, MessageParcel &data);
    static bool Unmarshalling(MessageParcel &data, Options &output);

    static int64_t GetTotalSize(const std::vector<Entry> &entries);
    static int64_t GetTotalSize(const std::vector<Key> &entries);

    template<class T> static bool Marshalling(const std::vector<T> &val, MessageParcel &parcel);
    template<class T> static bool Unmarshalling(MessageParcel &parcel, std::vector<T> &val);

    template<typename T, typename... Types>
    static bool Marshalling(MessageParcel &parcel, const T &first, const Types &...others);
    template<typename T, typename... Types>
    static bool Unmarshalling(MessageParcel &parcel, T &first, Types &...others);

    template<typename T> static Status MarshalToBuffer(const T &input, int size, MessageParcel &data);

    template<typename T> static Status MarshalToBuffer(const std::vector<T> &input, int size, MessageParcel &data);

    template<typename T> static Status UnmarshalFromBuffer(MessageParcel &data, int size, T &output);
    template<typename T> static Status UnmarshalFromBuffer(MessageParcel &data, int size, std::vector<T> &output);
};

template<class T> bool ITypesUtil::Marshalling(const std::vector<T> &val, MessageParcel &parcel)
{
    if (val.size() > INT_MAX) {
        return false;
    }

    if (!parcel.WriteInt32(static_cast<int32_t>(val.size()))) {
        return false;
    }

    for (auto &v : val) {
        if (!Marshalling(v, parcel)) {
            return false;
        }
    }
    return true;
}

template<class T> bool ITypesUtil::Unmarshalling(MessageParcel &parcel, std::vector<T> &val)
{
    int32_t len = parcel.ReadInt32();
    if (len < 0) {
        return false;
    }

    size_t readAbleSize = parcel.GetReadableBytes();
    size_t size = static_cast<size_t>(len);
    if ((size > readAbleSize) || (size > val.max_size())) {
        return false;
    }

    val.resize(size);
    if (val.size() < size) {
        return false;
    }

    for (auto &v : val) {
        if (!Unmarshalling(parcel, v)) {
            return false;
        }
    }

    return true;
}

template<typename T> Status ITypesUtil::MarshalToBuffer(const T &input, int size, MessageParcel &data)
{
    std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(size);
    if (!data.WriteBool(buffer != nullptr)) {
        return Status::IPC_ERROR;
    }
    if (buffer == nullptr) {
        return Status::ILLEGAL_STATE;
    }
    uint8_t *cursor = buffer.get();
    if (!input.WriteToBuffer(cursor, size)) {
        return Status::IPC_ERROR;
    }
    return data.WriteRawData(buffer.get(), size) ? Status::SUCCESS : Status::IPC_ERROR;
}

template<typename T> Status ITypesUtil::MarshalToBuffer(const std::vector<T> &input, int size, MessageParcel &data)
{
    std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(size);
    if (!data.WriteBool(buffer != nullptr)) {
        return Status::IPC_ERROR;
    }
    if (buffer == nullptr) {
        return Status::ILLEGAL_STATE;
    }
    uint8_t *cursor = buffer.get();
    for (const auto &entry : input) {
        if (!entry.WriteToBuffer(cursor, size)) {
            return Status::IPC_ERROR;
        }
    }
    if (!data.WriteInt32(input.size())) {
        return Status::IPC_ERROR;
    }
    return data.WriteRawData(buffer.get(), size) ? Status::SUCCESS : Status::IPC_ERROR;
}

template<typename T> Status ITypesUtil::UnmarshalFromBuffer(MessageParcel &data, int size, T &output)
{
    if (size < 0) {
        return Status::INVALID_ARGUMENT;
    }
    if (!data.ReadBool()) {
        return Status::ILLEGAL_STATE;
    }
    const uint8_t *buffer = reinterpret_cast<const uint8_t *>(data.ReadRawData(size));
    if (buffer == nullptr) {
        return Status::INVALID_ARGUMENT;
    }
    return output.ReadFromBuffer(buffer, size) ? Status::SUCCESS : Status::IPC_ERROR;
}

template<typename T> Status ITypesUtil::UnmarshalFromBuffer(MessageParcel &data, int size, std::vector<T> &output)
{
    if (size < 0) {
        return Status::INVALID_ARGUMENT;
    }
    if (!data.ReadBool()) {
        return Status::ILLEGAL_STATE;
    }
    int count = data.ReadInt32();
    const uint8_t *buffer = reinterpret_cast<const uint8_t *>(data.ReadRawData(size));
    if (count < 0 || buffer == nullptr) {
        return Status::INVALID_ARGUMENT;
    }

    output.resize(count);
    for (auto &entry : output) {
        if (!entry.ReadFromBuffer(buffer, size)) {
            output.clear();
            return Status::IPC_ERROR;
        }
    }
    return Status::SUCCESS;
}

template<typename T, typename... Types>
bool ITypesUtil::Marshalling(MessageParcel &parcel, const T &first, const Types &...others)
{
    if (sizeof...(others) == 0) {
        return true;
    }

    if (!Marshalling(first, parcel)) {
        return false;
    }

    return Marshalling(parcel, others...);
}

template<typename T, typename... Types>
bool ITypesUtil::Unmarshalling(MessageParcel &parcel, T &first, Types &...others)
{
    if (sizeof...(others) == 0) {
        return true;
    }

    if (!Unmarshalling(parcel, first)) {
        return false;
    }
    return Unmarshalling(parcel, others...);
}
} // namespace OHOS::DistributedKv
#endif
