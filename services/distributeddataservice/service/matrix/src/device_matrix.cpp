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
#define LOG_TAG "DeviceMatrix"
#include "device_matrix.h"

#include "device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "log_print.h"
#include "matrix_event.h"
#include "metadata/matrix_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "utils/anonymous.h"
namespace OHOS::DistributedData {
using DMAdapter = DeviceManagerAdapter;
DeviceMatrix &DeviceMatrix::GetInstance()
{
    static DeviceMatrix instance;
    return instance;
}

DeviceMatrix::DeviceMatrix()
{
    MetaDataManager::GetInstance().Subscribe(MatrixMetaData::GetPrefix({}),
        [this](const std::string &, const std::string &meta, int32_t action) {
            if (action != MetaDataManager::INSERT && action != MetaDataManager::UPDATE) {
                return true;
            }
            MatrixMetaData metaData;
            MatrixMetaData::Unmarshall(meta, metaData);
            auto deviceId = std::move(metaData.deviceId);
            versions_.Set(deviceId, metaData);
            ZLOGI("Matrix ver:%{public}u mask:0x%{public}x device:%{public}s", metaData.version, metaData.mask,
                Anonymous::Change(deviceId).c_str());
            return true;
        });
}

DeviceMatrix::~DeviceMatrix()
{
    MetaDataManager::GetInstance().Unsubscribe(MatrixMetaData::GetPrefix({}));
}

bool DeviceMatrix::Initialize(uint32_t token, std::string storeId)
{
    tokenId_ = token;
    storeId_ = std::move(storeId);
    MatrixMetaData oldMeta;
    MatrixMetaData newMeta;
    newMeta.version = CURRENT_VERSION;
    newMeta.mask = META_STORE_MASK | CURRENT_MASK;
    newMeta.deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    newMeta.maskInfo = maskApps_;
    auto key = newMeta.GetKey();
    auto loaded = MetaDataManager::GetInstance().LoadMeta(key, oldMeta);
    if (loaded && newMeta.version == oldMeta.version) {
        return true;
    }
    ZLOGI("Save Matrix ver:%{public}u -> %{public}u mask:0x%{public}x -> 0x%{public}x", oldMeta.version,
        newMeta.version, oldMeta.mask, newMeta.mask);
    return MetaDataManager::GetInstance().SaveMeta(newMeta.GetKey(), newMeta);
}

void DeviceMatrix::Online(const std::string &device)
{
    Mask mask;
    EventCenter::Defer defer;
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    auto it = offLines_.find(device);
    if (it != offLines_.end()) {
        mask = it->second;
        offLines_.erase(it);
    }
    onLines_.insert_or_assign(device, mask);
    if (mask.bitset != 0) {
        EventCenter::GetInstance().PostEvent(std::make_unique<MatrixEvent>(MATRIX_ONLINE, device, mask.bitset));
    }
}

void DeviceMatrix::Offline(const std::string &device)
{
    Mask mask;
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    auto it = onLines_.find(device);
    if (it != onLines_.end()) {
        mask = it->second;
        onLines_.erase(it);
    }
    offLines_.insert_or_assign(device, mask);
}

void DeviceMatrix::OnBroadcast(const std::string &device, uint16_t code)
{
    Mask mask{ .bitset = 0 };
    uint16_t rightCode = ConvertMask(device, code);
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    auto it = remotes_.find(device);
    if (it != remotes_.end()) {
        mask = it->second;
    }
    mask.bitset |= rightCode;
    remotes_.insert_or_assign(device, mask);
}

void DeviceMatrix::OnChanged(uint16_t code)
{
    EventCenter::Defer defer;
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    for (auto &[key, mask] : onLines_) {
        mask.bitset |= code;
    }

    for (auto &[key, mask] : offLines_) {
        mask.bitset |= code;
    }

    if (code != 0) {
        EventCenter::GetInstance().PostEvent(std::make_unique<MatrixEvent>(MATRIX_BROADCAST, "", code));
    }
}

void DeviceMatrix::OnExchanged(const std::string &device, uint16_t code, bool isRemote)
{
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    if (!isRemote) {
        auto it = onLines_.find(device);
        if (it != onLines_.end()) {
            it->second.bitset &= ~code;
        }
        it = offLines_.find(device);
        if (it != offLines_.end()) {
            it->second.bitset &= ~code;
        }
    }

    auto it = remotes_.find(device);
    if (it != remotes_.end()) {
        it->second.bitset &= ~code;
    }
}

uint16_t DeviceMatrix::GetCode(const StoreMetaData &metaData)
{
    if (metaData.tokenId == tokenId_ && metaData.storeId == storeId_) {
        return META_STORE_MASK;
    }

    for (size_t i = 0; i < maskApps_.size(); i++) {
        if (maskApps_[i] == metaData.appId) {
            return SetMask(i);
        }
    }
    return 0;
}

uint16_t DeviceMatrix::ConvertMask(const std::string &device, uint16_t code)
{
    Mask mask;
    MatrixMetaData meta = GetMatrixMeta(device);
    if (meta.version == CURRENT_VERSION) {
        return (mask.bitset & code);
    }

    uint16_t result = code & META_STORE_MASK;
    code &= ~META_STORE_MASK;
    while (code != 0) {
        uint16_t index = (~code) & (code - 1);
        // 0x6666: 1010101010101010  0x5555: 0101010101010101 1: move the high(0x6666) to low(0x5555) bits
        index = ((index & 0xAAAA) >> 1) + (index & 0x5555);
        // 0xCCCC: 1100110011001100  0x3333: 0011001100110011 2: the count save at 2 bits
        index = ((index & 0xCCCC) >> 2) + (index & 0x3333);
        // 0xF0F0: 1111000011110000  0x0F0F: 0000111100001111 4: the count save at 4 bits
        index = ((index & 0xF0F0) >> 4) + (index & 0x0F0F);
        // 0xFF00: 1111000011110000  0x00FF: 0000111100001111 8: the count save at 8 bits
        index = ((index & 0xFF00) >> 8) + (index & 0x00FF);
        index--;
        if (index >= meta.maskInfo.size()) {
            return result;
        }
        auto &app = meta.maskInfo[index];
        for (size_t i = 0; i < maskApps_.size(); i++) {
            if (maskApps_[i] == app) {
                result |= SetMask(i);
                break;
            }
        }
        code &= code - 1;
    }
    return result;
}

MatrixMetaData DeviceMatrix::GetMatrixMeta(const std::string &device)
{
    MatrixMetaData meta;
    auto success = versions_.Get(device, meta);
    if (success) {
        return meta;
    }

    meta.deviceId = device;
    success = MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta);
    if (success) {
        meta.deviceId = "";
        versions_.Set(device, meta);
    }
    return meta;
}
} // namespace OHOS::DistributedData
