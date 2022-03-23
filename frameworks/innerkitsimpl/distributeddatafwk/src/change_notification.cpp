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

#define LOG_TAG "ChangeNotification"

#include "change_notification.h"
#include "constant.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
ChangeNotification::ChangeNotification(std::vector<Entry> &&insertEntries, std::vector<Entry> &&updateEntries,
                                       std::vector<Entry> &&deleteEntries, const std::string &deviceId, bool isClear)
    : insertEntries_(std::move(insertEntries)), updateEntries_(std::move(updateEntries)),
    deleteEntries_(std::move(deleteEntries)), deviceId_(deviceId), isClear_(isClear)
{}

ChangeNotification::~ChangeNotification()
{}

const std::vector<Entry> &ChangeNotification::GetInsertEntries() const
{
    return insertEntries_;
}

const std::vector<Entry> &ChangeNotification::GetUpdateEntries() const
{
    return updateEntries_;
}

const std::vector<Entry> &ChangeNotification::GetDeleteEntries() const
{
    return deleteEntries_;
}

const std::string &ChangeNotification::GetDeviceId() const
{
    return deviceId_;
}

bool ChangeNotification::IsClear() const
{
    return isClear_;
}

bool ChangeNotification::Marshalling(Parcel &parcel) const
{
    if (!parcel.SetMaxCapacity(Constant::MAX_IPC_CAPACITY)) {
        return false;
    }
    int32_t lenInsert = static_cast<int32_t>(insertEntries_.size());
    if (!parcel.WriteInt32(lenInsert)) {
        return false;
    }

    for (const auto &entry : insertEntries_) {
        if (!parcel.WriteParcelable(&entry)) {
            return false;
        }
    }

    int32_t lenUpdate = static_cast<int32_t>(updateEntries_.size());
    if (!parcel.WriteInt32(lenUpdate)) {
        return false;
    }
    for (const auto &entry : updateEntries_) {
        if (!parcel.WriteParcelable(&entry)) {
            return false;
        }
    }

    int32_t lenDelete =  static_cast<int32_t>(deleteEntries_.size());
    if (!parcel.WriteInt32(lenDelete)) {
        return false;
    }
    for (const auto &entry : deleteEntries_) {
        if (!parcel.WriteParcelable(&entry)) {
            return false;
        }
    }
    if (!parcel.WriteString(deviceId_)) {
        ZLOGE("WriteString deviceId_ failed.");
        return false;
    }

    return parcel.WriteBool(isClear_);
}

ChangeNotification *ChangeNotification::Unmarshalling(Parcel &parcel)
{
    std::vector<Entry> insertEntries;
    std::vector<Entry> updateEntries;
    std::vector<Entry> deleteEntries;
    int lenInsert = parcel.ReadInt32();
    if (lenInsert < 0) {
        ZLOGE("lenInsert is %d", lenInsert);
        return nullptr;
    }
    for (int i = 0; i < lenInsert; i++) {
        sptr<Entry> entryTmp = parcel.ReadParcelable<Entry>();
        if (entryTmp != nullptr) {
            insertEntries.push_back(*entryTmp);
        } else {
            ZLOGE("insertEntries get nullptr");
            return nullptr;
        }
    }
    int lenUpdate = parcel.ReadInt32();
    if (lenUpdate < 0) {
        ZLOGE("lenUpdate is %d", lenUpdate);
        return nullptr;
    }
    for (int i = 0; i < lenUpdate; i++) {
        sptr<Entry> entryTmp = parcel.ReadParcelable<Entry>();
        if (entryTmp != nullptr) {
            updateEntries.push_back(*entryTmp);
        } else {
            ZLOGE("updateEntries get nullptr");
            return nullptr;
        }
    }
    int lenDelete = parcel.ReadInt32();
    if (lenDelete < 0) {
        ZLOGE("lenDelete is %d", lenDelete);
        return nullptr;
    }
    for (int i = 0; i < lenDelete; i++) {
        sptr<Entry> entryTmp = parcel.ReadParcelable<Entry>();
        if (entryTmp != nullptr) {
            deleteEntries.push_back(*entryTmp);
        } else {
            ZLOGE("deleteEntries get nullptr");
            return nullptr;
        }
    }
    std::string deviceId = parcel.ReadString();
    bool isClear = parcel.ReadBool();
    return new(std::nothrow) ChangeNotification(std::move(insertEntries), std::move(updateEntries),
        std::move(deleteEntries), deviceId, isClear);
}
}  // namespace DistributedKv
}  // namespace OHOS
