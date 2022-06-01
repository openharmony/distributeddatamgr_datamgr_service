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
    static bool Marshal(MessageParcel &data);
    static bool Unmarshal(MessageParcel &data);

    static bool Marshalling(uint32_t input, MessageParcel &data);
    static bool Unmarshalling(uint32_t &output, MessageParcel &data);

    static bool Marshalling(int32_t input, MessageParcel &data);
    static bool Unmarshalling(int32_t &output, MessageParcel &data);

    static bool Marshalling(const std::string &input, MessageParcel &data);
    static bool Unmarshalling(std::string &output, MessageParcel &data);

    static bool Marshalling(const std::vector<uint8_t> &input, MessageParcel &data);
    static bool Unmarshalling(std::vector<uint8_t> &output, MessageParcel &data);

    static bool Marshalling(const Blob &blob, MessageParcel &data);
    static bool Unmarshalling(Blob &output, MessageParcel &data);

    static bool Marshalling(const Entry &entry, MessageParcel &data);
    static bool Unmarshalling(Entry &output, MessageParcel &data);

    static bool Marshalling(const DeviceInfo &entry, MessageParcel &data);
    static bool Unmarshalling(DeviceInfo &output, MessageParcel &data);

    static bool Marshalling(const ChangeNotification &notification, MessageParcel &parcel);
    static bool Unmarshalling(ChangeNotification &output, MessageParcel &parcel);

    static bool Marshalling(const DistributedRdb::RdbSyncerParam &param, MessageParcel &parcel);
    static bool Unmarshalling(DistributedRdb::RdbSyncerParam &param, MessageParcel &parcel);

    static bool Marshalling(const DistributedRdb::SyncOption &option, MessageParcel &parcel);
    static bool Unmarshalling(DistributedRdb::SyncOption &option, MessageParcel &parcel);

    static bool Marshalling(const DistributedRdb::RdbPredicates &predicates, MessageParcel &parcel);
    static bool Unmarshalling(DistributedRdb::RdbPredicates &predicates, MessageParcel &parcel);

    static bool Marshalling(const Options &input, MessageParcel &data);
    static bool Unmarshalling(Options &output, MessageParcel &data);

    static bool Marshalling(const sptr<IRemoteObject> &input, MessageParcel &data);
    static bool Unmarshalling(sptr<IRemoteObject> &output, MessageParcel &data);

    static int64_t GetTotalSize(const std::vector<Entry> &entries);
    static int64_t GetTotalSize(const std::vector<Key> &entries);

    template<class T> static bool Marshalling(const std::vector<T> &val, MessageParcel &parcel);
    template<class T> static bool Unmarshalling(std::vector<T> &val, MessageParcel &parcel);

    template<class K, class V> static bool Marshalling(const std::map<K, V> &val, MessageParcel &parcel);
    template<class K, class V> static bool Unmarshalling(std::map<K, V> &val, MessageParcel &parcel);

    template<typename T, typename... Types>
    static bool Marshal(MessageParcel &parcel, const T &first, const Types &...others);
    template<typename T, typename... Types>
    static bool Unmarshal(MessageParcel &parcel, T &first, Types &...others);

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

template<class T> bool ITypesUtil::Unmarshalling(std::vector<T> &val, MessageParcel &parcel)
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
        if (!Unmarshalling(v, parcel)) {
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
bool ITypesUtil::Marshal(MessageParcel &parcel, const T &first, const Types &...others)
{
    if (!Marshalling(first, parcel)) {
        return false;
    }
    return Marshal(parcel, others...);
}

template<typename T, typename... Types>
bool ITypesUtil::Unmarshal(MessageParcel &parcel, T &first, Types &...others)
{
    if (!Unmarshalling(first, parcel)) {
        return false;
    }
    return Unmarshal(parcel, others...);
}

template<class K, class V> bool ITypesUtil::Marshalling(const std::map<K, V> &result, MessageParcel &parcel)
{
    if (!parcel.WriteInt32(static_cast<int32_t>(result.size()))) {
        return false;
    }
    for (const auto &entry : result) {
        if (!Marshalling(entry.first, parcel)) {
            return false;
        }
        if (!Marshalling(entry.second, parcel)) {
            return false;
        }
    }
    return true;
}

template<class K, class V> bool ITypesUtil::Unmarshalling(std::map<K, V> &val, MessageParcel &parcel)
{
    int32_t size = 0;
    if (!parcel.ReadInt32(size)) {
        return false;
    }
    if (size < 0) {
        return false;
    }

    size_t readAbleSize = parcel.GetReadableBytes();
    size_t len = static_cast<size_t>(size);
    if ((len > readAbleSize) || len > val.max_size()) {
        return false;
    }

    for (int32_t i = 0; i < size; i++) {
        K key;
        if (!Unmarshalling(key, parcel)) {
            return false;
        }
        V value;
        if (!Unmarshalling(value, parcel)) {
            return false;
        }
        val.insert({ key, value });
    }
    return true;
}
} // namespace OHOS::DistributedKv
#endif
