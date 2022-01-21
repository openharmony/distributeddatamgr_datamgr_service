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

#define LOG_TAG "ITypesUtil"

#include "itypes_util.h"
#include <log_print.h>
#include "autils/constant.h"
#include <climits>

namespace OHOS::DistributedKv {
bool ITypesUtil::Marshalling(const Blob &blob, MessageParcel &data)
{
    return data.WriteUInt8Vector(blob.Data());
}

bool ITypesUtil::Unmarshalling(MessageParcel &data, Blob &output)
{
    std::vector<uint8_t> blob;
    bool result = data.ReadUInt8Vector(&blob);
    output = blob;
    return result;
}

bool ITypesUtil::Marshalling(const std::vector<Blob> &blobs, MessageParcel &data)
{
    return WriteVector(data, blobs, ITypesUtil::GetParcelWriter<Blob>());
}

bool ITypesUtil::Unmarshalling(MessageParcel &data, std::vector<Blob> &output)
{
    return ReadVector(data, output, ITypesUtil::GetParcelReader<Blob>());
}

bool ITypesUtil::Marshalling(const std::vector<Entry> &entry, MessageParcel &data)
{
    return WriteVector(data, entry, ITypesUtil::GetParcelWriter<Entry>());
}

bool ITypesUtil::Unmarshalling(MessageParcel &data, std::vector<Entry> &output)
{
    return ReadVector(data, output, ITypesUtil::GetParcelReader<Entry>());
}

bool ITypesUtil::Marshalling(const Entry &entry, MessageParcel &data)
{
    if (!Marshalling(entry.key, data)) {
        return false;
    }
    return Marshalling(entry.value, data);
}

bool ITypesUtil::Unmarshalling(MessageParcel &data, Entry &output)
{
    if (!Unmarshalling(data, output.key)) {
        return false;
    }
    return Unmarshalling(data, output.value);
}

bool ITypesUtil::Marshalling(const DeviceInfo &entry, MessageParcel &data)
{
    if (!data.WriteString(entry.deviceId)) {
        return false;
    }
    if (!data.WriteString(entry.deviceName)) {
        return false;
    }
    return data.WriteString(entry.deviceType);
}

bool ITypesUtil::Unmarshalling(MessageParcel &data, DeviceInfo &output)
{
    if (!data.ReadString(output.deviceId)) {
        return false;
    }
    if (!data.ReadString(output.deviceName)) {
        return false;
    }
    return data.ReadString(output.deviceType);
}
bool ITypesUtil::Marshalling(const std::vector<DeviceInfo> &input, MessageParcel &data)
{
    return WriteVector(data, input, ITypesUtil::GetParcelWriter<DeviceInfo>());
}

bool ITypesUtil::Unmarshalling(MessageParcel &data, std::vector<DeviceInfo> &output)
{
    return ReadVector(data, output, ITypesUtil::GetParcelReader<DeviceInfo>());
}

bool ITypesUtil::Marshalling(const ChangeNotification &notification, MessageParcel &parcel)
{
    if (!Marshalling(notification.GetInsertEntries(), parcel)) {
        return false;
    }
    
    if (!Marshalling(notification.GetUpdateEntries(), parcel)) {
        return false;
    }
    
    if (!Marshalling(notification.GetDeleteEntries(), parcel)) {
        return false;
    }
    if (!parcel.WriteString(notification.GetDeviceId())) {
        ZLOGE("WriteString deviceId_ failed.");
        return false;
    }
    
    return parcel.WriteBool(notification.IsClear());
}

bool ITypesUtil::Unmarshalling(MessageParcel &parcel, ChangeNotification &output)
{
    std::vector<Entry> insertEntries;
    if (!Unmarshalling(parcel, insertEntries)) {
        return false;
    }
    std::vector<Entry> updateEntries;
    if (!Unmarshalling(parcel, updateEntries)) {
        return false;
    }
    std::vector<Entry> deleteEntries;
    if (!Unmarshalling(parcel, deleteEntries)) {
        return false;
    }
    std::string deviceId;
    if (!parcel.ReadString(deviceId)) {
        ZLOGE("WriteString deviceId_ failed.");
        return false;
    }
    bool isClear;
    if (!parcel.ReadBool(isClear)) {
        ZLOGE("WriteString deviceId_ failed.");
        return false;
    }
    output = ChangeNotification(std::move(insertEntries), std::move(updateEntries), std::move(deleteEntries), deviceId,
                                isClear);
    return true;
}

bool ITypesUtil::Marshalling(const DistributedRdb::RdbSyncerParam& param, MessageParcel& parcel)
{
    if (!parcel.WriteString(param.bundleName_)) {
        ZLOGE("RdbStoreParam write bundle name failed");
        return false;
    }
    if (!parcel.WriteString(param.path_)) {
        ZLOGE("RdbStoreParam write directory failed");
        return false;
    }
    if (!parcel.WriteString(param.storeName_)) {
        ZLOGE("RdbStoreParam write store name failed");
        return false;
    }
    if (!parcel.WriteInt32(param.type_)) {
        ZLOGE("RdbStoreParam write type failed");
        return false;
    }
    if (!parcel.WriteBool(param.isAutoSync_)) {
        ZLOGE("RdbStoreParam write auto sync failed");
        return false;
    }
    return true;
}
bool ITypesUtil::UnMarshalling(MessageParcel& parcel, DistributedRdb::RdbSyncerParam& param)
{
    if (!parcel.ReadString(param.bundleName_)) {
        ZLOGE("RdbStoreParam read bundle name failed");
        return false;
    }
    if (!parcel.ReadString(param.path_)) {
        ZLOGE("RdbStoreParam read directory failed");
        return false;
    }
    if (!parcel.ReadString(param.storeName_)) {
        ZLOGE("RdbStoreParam read store name failed");
        return false;
    }
    if (!parcel.ReadInt32(param.type_)) {
        ZLOGE("RdbStoreParam read type failed");
        return false;
    }
    if (!parcel.ReadBool(param.isAutoSync_)) {
        ZLOGE("RdbStoreParam read auto sync failed");
        return false;
    }
    return true;
}

template<class T>
std::vector<T> ITypesUtil::Convert2Vector(const std::list<T> &entries)
{
    std::vector<T> vector(entries.size());
    int i = 0;
    for (const auto &entry : entries) {
        vector[i++] = entry;
    }
    return vector;
}

template<class T>
std::list<T> ITypesUtil::Convert2List(std::vector<T> &&entries)
{
    std::list<T> result;
    for (auto &entry : entries) {
        result.push_back(std::move(entry));
    }
    return result;
}
int64_t ITypesUtil::GetTotalSize(const std::vector<Entry> &entries)
{
    int64_t bufferSize = 1;
    for (const auto &item : entries) {
        if (item.key.Size() > Constant::MAX_KEY_LENGTH || item.value.Size() > Constant::MAX_VALUE_LENGTH) {
            return -bufferSize;
        }
        bufferSize += item.key.RawSize() + item.value.RawSize();
    }
    return bufferSize - 1;
}
int64_t ITypesUtil::GetTotalSize(const std::vector<Key> &entries)
{
    int64_t bufferSize = 1;
    for (const auto &item : entries) {
        if (item.Size() > Constant::MAX_KEY_LENGTH) {
            return -bufferSize;
        }
        bufferSize += item.RawSize();
    }
    return bufferSize - 1;
}

template<typename T>
bool ITypesUtil::ReadVector(Parcel &parcel, std::vector<T> &val, bool (Parcel::*read)(T &))
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
        if (!(parcel.*read)(v)) {
            return false;
        }
    }
    
    return true;
}

template<typename T>
bool ITypesUtil::WriteVector(Parcel &parcel, const std::vector<T> &val, bool (Parcel::*writer)(const T &))
{
    if (val.size() > INT_MAX) {
        return false;
    }
    
    if (!parcel.WriteInt32(static_cast<int32_t>(val.size()))) {
        return false;
    }
    
    for (auto &v : val) {
        if (!(parcel.*writer)(v)) {
            return false;
        }
    }
    return true;
}
}

