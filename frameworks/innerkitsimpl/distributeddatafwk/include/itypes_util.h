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

#include <memory>
#include "types.h"
#include "change_notification.h"
#include "message_parcel.h"
#include "rdb_types.h"

namespace OHOS::DistributedKv {
class ITypesUtil final {
public:
    static bool Marshalling(const Blob &blob, MessageParcel &data);
    static bool Unmarshalling(MessageParcel &data, Blob &output);

    static bool Marshalling(const std::vector<Blob> &blobs, MessageParcel &data);
    static bool Unmarshalling(MessageParcel &data, std::vector<Blob> &output);

    static bool Marshalling(const Entry &entry, MessageParcel &data);
    static bool Unmarshalling(MessageParcel &data, Entry &output);

    static bool Marshalling(const std::vector<Entry> &entry, MessageParcel &data);
    static bool Unmarshalling(MessageParcel &data, std::vector<Entry> &output);

    static bool Marshalling(const DeviceInfo &entry, MessageParcel &data);
    static bool Unmarshalling(MessageParcel &data, DeviceInfo &output);

    static bool Marshalling(const std::vector<DeviceInfo> &input, MessageParcel &data);
    static bool Unmarshalling(MessageParcel &data, std::vector<DeviceInfo> &output);

    static bool Marshalling(const ChangeNotification &notification, MessageParcel &parcel);
    static bool Unmarshalling(MessageParcel &parcel, ChangeNotification &output);

    static bool Marshalling(const DistributedRdb::RdbSyncerParam& param, MessageParcel& parcel);
    static bool UnMarshalling(MessageParcel& parcel, DistributedRdb::RdbSyncerParam& param);
    
    static bool Marshalling(const DistributedRdb::SyncResult& result, MessageParcel& parcel);
    static bool UnMarshalling(MessageParcel& parcel, DistributedRdb::SyncResult& result);
    
    static bool Marshalling(const DistributedRdb::SyncOption& option, MessageParcel& parcel);
    static bool UnMarshalling(MessageParcel& parcel, DistributedRdb::SyncOption& option);
    
    static bool Marshalling(const DistributedRdb::RdbPredicates& predicates, MessageParcel& parcel);
    static bool UnMarshalling(MessageParcel& parcel, DistributedRdb::RdbPredicates& predicates);
    
    static int64_t GetTotalSize(const std::vector<Entry> &entries);
    static int64_t GetTotalSize(const std::vector<Key> &entries);

    template<typename T>
    static Status MarshalToBuffer(const T &input, int size, MessageParcel &data)
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
        return data.WriteRawData(buffer.get(), size) ? Status::SUCCESS :  Status::IPC_ERROR;
    }

    template<typename T>
    static Status MarshalToBuffer(const std::vector<T> &input, int size, MessageParcel &data)
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
        return data.WriteRawData(buffer.get(), size) ? Status::SUCCESS :  Status::IPC_ERROR;
    }

    template<typename T>
    static Status UnmarshalFromBuffer(MessageParcel &data, int size, T &output)
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
        return output.ReadFromBuffer(buffer, size) ? Status::SUCCESS :  Status::IPC_ERROR;
    }

    template<typename T>
    static Status UnmarshalFromBuffer(MessageParcel &data, int size, std::vector<T> &output)
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

private:
    template<typename T>
    class VectorParcel : public MessageParcel {
    public:
        bool Writer(const T &entry) { return ITypesUtil::Marshalling(entry, *this); }
        bool Reader(T &entry) { return ITypesUtil::Unmarshalling(*this, entry); }
    };

    template <typename T>
    static bool ReadVector(Parcel &parcel, std::vector<T> &val, bool (Parcel::*read)(T &));
    template <typename T>
    static bool WriteVector(Parcel &parcel, const std::vector<T> &val, bool (Parcel::*writer)(const T &));
    template<typename T> using Reader = bool (Parcel::*)(T &);
    template<typename T> using Writer = bool (Parcel::*)(const T &);
    template<typename T>
    static Writer<T> GetParcelWriter()
    {
        return static_cast<Writer<T>>(&VectorParcel<T>::Writer);
    }
    template<typename T>
    static Reader<T> GetParcelReader()
    {
        return static_cast<Reader<T>>(&VectorParcel<T>::Reader);
    }
    template<typename T>
    static std::vector<T> Convert2Vector(const std::list<T> &entries);
    template<typename T>
    static std::list<T> Convert2List(std::vector<T> &&entries);
};
}
#endif

