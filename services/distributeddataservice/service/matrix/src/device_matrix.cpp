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

#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "communication_provider.h"
#include "device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "log_print.h"
#include "metadata/matrix_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/switches_meta_data.h"
#include "types.h"
#include "utils/anonymous.h"
namespace OHOS::DistributedData {
using DMAdapter = DeviceManagerAdapter;
using Commu = AppDistributedKv::CommunicationProvider;
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
            if (!MatrixMetaData::Unmarshall(meta, metaData)) {
                ZLOGE("unmarshall meta failed, action:%{public}d", action);
                return true;
            }
            auto deviceId = std::move(metaData.deviceId);
            ZLOGI("Matrix ver:%{public}u origin:%{public}d device:%{public}s",
                metaData.version, metaData.origin, Anonymous::Change(deviceId).c_str());
            if (metaData.origin == MatrixMetaData::Origin::REMOTE_CONSISTENT) {
                return true;
            }
            matrixs_.Set(deviceId, metaData);
            return true;
        }, true);
    MetaDataManager::GetInstance().Subscribe(MatrixMetaData::GetPrefix({}),
        [this](const std::string &, const std::string &meta, int32_t action) {
            if (action != MetaDataManager::INSERT && action != MetaDataManager::UPDATE) {
                return true;
            }
            MatrixMetaData metaData;
            MatrixMetaData::Unmarshall(meta, metaData);
            auto deviceId = std::move(metaData.deviceId);
            versions_.Set(deviceId, metaData);
            ZLOGI("Matrix ver:%{public}u device:%{public}s",
                metaData.version, Anonymous::Change(deviceId).c_str());
            return true;
        });
}

DeviceMatrix::~DeviceMatrix()
{
    MetaDataManager::GetInstance().Unsubscribe(MatrixMetaData::GetPrefix({}));
}

bool DeviceMatrix::Initialize(uint32_t token, std::string storeId)
{
    auto stores = CheckerManager::GetInstance().GetDynamicStores();
    dynamicApps_.clear();
    for (auto &store : stores) {
        dynamicApps_.push_back(std::move(store.bundleName));
    }
    stores = CheckerManager::GetInstance().GetStaticStores();
    staticsApps_.clear();
    for (auto &store : stores) {
        staticsApps_.push_back(std::move(store.bundleName));
    }
    auto pipe = Bootstrap::GetInstance().GetProcessLabel() + "-" + "default";
    auto status = Commu::GetInstance().Broadcast(
        { pipe }, { INVALID_LEVEL, INVALID_LEVEL, INVALID_VALUE, INVALID_LENGTH });
    isSupportBroadcast_ = (status != DistributedKv::Status::NOT_SUPPORT_BROADCAST);
    ZLOGI("Is support broadcast:%{public}d", isSupportBroadcast_);
    tokenId_ = token;
    storeId_ = std::move(storeId);
    MatrixMetaData oldMeta;
    MatrixMetaData newMeta;
    newMeta.version = CURRENT_VERSION;
    newMeta.dynamic = CURRENT_DYNAMIC_MASK | META_STORE_MASK;
    newMeta.statics = CURRENT_STATICS_MASK;
    newMeta.dynamicInfo = dynamicApps_;
    newMeta.staticsInfo = staticsApps_;
    newMeta.deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    auto key = newMeta.GetKey();
    auto loaded = MetaDataManager::GetInstance().LoadMeta(key, oldMeta);
    if (loaded && newMeta.version == oldMeta.version) {
        return true;
    }
    if (loaded && newMeta.version != oldMeta.version) {
        newMeta.dynamic = Merge(oldMeta.dynamic, newMeta.dynamic);
        newMeta.statics = Merge(oldMeta.statics, newMeta.statics);
    }
    ZLOGI("Save Matrix ver:%{public}u -> %{public}u ", oldMeta.version, newMeta.version);
    MetaDataManager::GetInstance().SaveMeta(newMeta.GetKey(), newMeta, true);
    return MetaDataManager::GetInstance().SaveMeta(newMeta.GetKey(), newMeta);
}

void DeviceMatrix::Online(const std::string &device, RefCount refCount)
{
    Mask mask;
    EventCenter::Defer defer;
    {
        std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
        auto it = offLines_.find(device);
        if (it != offLines_.end()) {
            mask = it->second;
            offLines_.erase(it);
        } else {
            UpdateMask(mask);
        }
        onLines_.insert_or_assign(device, mask);
    }
    auto [dynamic, statics] = IsConsistent(device);
    if ((dynamic && statics) || (Low(mask.statics) == 0 && Low(mask.dynamic) == 0)) {
        ZLOGI("dynamic:%{public}d statics:%{public}d dynamic mask:0x%{public}08x statics mask:0x%{public}08x",
            dynamic, statics, Low(mask.dynamic), Low(mask.statics));
        return;
    }
    if (dynamic) {
        mask.dynamic = ResetMask(mask.dynamic);
    }
    if (statics) {
        mask.statics = ResetMask(mask.statics);
    }
    MatrixEvent::MatrixData matrixData;
    matrixData.statics = mask.statics;
    matrixData.dynamic = mask.dynamic;
    auto evt = std::make_unique<MatrixEvent>(MATRIX_ONLINE, device, matrixData);
    evt->SetRefCount(std::move(refCount));
    EventCenter::GetInstance().PostEvent(std::move(evt));
}

std::pair<bool, bool> DeviceMatrix::IsConsistent(const std::string &device)
{
    std::pair<bool, bool> isConsistent = { false, false };
    auto [dynamicSaved, dynamicRecvLevel] = GetRecvLevel(device);
    auto [dynamicExist, dynamicConsLevel] = GetConsLevel(device);
    isConsistent.first = (dynamicSaved && dynamicExist &&
        (dynamicRecvLevel <= dynamicConsLevel) && (dynamicRecvLevel != INVALID_MASK));
    auto [staticsSaved, staticsRecvLevel] = GetRecvLevel(device, LevelType::STATICS);
    auto [staticsExist, staticsConsLevel] = GetConsLevel(device, LevelType::STATICS);
    isConsistent.second = (staticsSaved && staticsExist &&
        (staticsRecvLevel <= staticsConsLevel) && (staticsRecvLevel != INVALID_MASK));
    return isConsistent;
}

void DeviceMatrix::Offline(const std::string &device)
{
    Mask mask;
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    auto it = onLines_.find(device);
    if (it != onLines_.end()) {
        mask = it->second;
        onLines_.erase(it);
    } else {
        UpdateMask(mask);
    }
    offLines_.insert_or_assign(device, mask);
}

void DeviceMatrix::UpdateMask(Mask &mask)
{
    auto device = DMAdapter::GetInstance().GetLocalDevice().uuid;
    auto [exist, meta] = GetMatrixMeta(device);
    mask.dynamic = Merge(meta.dynamic, mask.dynamic);
    mask.statics = Merge(meta.statics, mask.statics);
}

std::pair<uint16_t, uint16_t> DeviceMatrix::OnBroadcast(const std::string &device, const DataLevel &dataLevel)
{
    if (device.empty() || !dataLevel.IsValid()) {
        ZLOGE("invalid args, device:%{public}s, dataLevel valid:%{public}d",
            Anonymous::Change(device).c_str(), dataLevel.IsValid());
        return { INVALID_LEVEL, INVALID_LEVEL };
    }
    SaveSwitches(device, dataLevel);
    auto result = ConvertMask(device, dataLevel);
    Mask mask;
    if (dataLevel.dynamic != INVALID_LEVEL) {
        mask.dynamic &= Low(result.first);
        if (High(dataLevel.dynamic) != INVALID_HIGH) {
            mask.dynamic |= High(dataLevel.dynamic);
        }
    } else {
        mask.dynamic &= ~dataLevel.dynamic;
    }
    if (dataLevel.statics != INVALID_LEVEL) {
        mask.statics &= Low(result.second);
        if (High(dataLevel.statics) != INVALID_HIGH) {
            mask.statics |= High(dataLevel.statics);
        }
    } else {
        mask.statics &= ~dataLevel.statics;
    }
    UpdateRemoteMeta(device, mask);
    {
        std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
        auto it = remotes_.find(device);
        if (it != remotes_.end()) {
            mask.dynamic |= Low(it->second.dynamic);
            mask.statics |= Low(it->second.statics);
        }
        remotes_.insert_or_assign(device, mask);
    }
    if (observer_) {
        observer_(device, mask.dynamic);
    }
    return { mask.dynamic, mask.statics };
}

std::pair<uint16_t, uint16_t> DeviceMatrix::ConvertMask(const std::string &device, const DataLevel &dataLevel)
{
    Mask mask;
    auto meta = GetMatrixInfo(device);
    if (meta.version == CURRENT_VERSION) {
        return { mask.dynamic & dataLevel.dynamic, mask.statics & dataLevel.statics };
    }
    return { ConvertDynamic(meta, dataLevel.dynamic), ConvertStatics(meta, dataLevel.statics) };
}

uint16_t DeviceMatrix::ConvertDynamic(const MatrixMetaData &meta, uint16_t mask)
{
    uint16_t result = Low(mask) & META_STORE_MASK;
    uint16_t code = Low(mask) & (~META_STORE_MASK);
    while (code != 0) {
        uint16_t index = ConvertIndex(code);
        if (index >= meta.dynamicInfo.size()) {
            return result;
        }
        const auto &app = meta.dynamicInfo[index];
        for (size_t i = 0; i < dynamicApps_.size(); i++) {
            if (dynamicApps_[i] == app) {
                result |= SetMask(i + 1);
                break;
            }
        }
        code &= code - 1;
    }
    return result;
}

uint16_t DeviceMatrix::ConvertStatics(const MatrixMetaData &meta, uint16_t mask)
{
    uint16_t result = 0;
    uint16_t code = Low(mask);
    while (code != 0) {
        uint16_t index = ConvertIndex(code);
        if (index >= meta.staticsInfo.size()) {
            return result;
        }
        const auto &app = meta.staticsInfo[index];
        for (size_t i = 0; i < staticsApps_.size(); i++) {
            if (staticsApps_[i] == app) {
                result |= SetMask(i);
                break;
            }
        }
        code &= code - 1;
    }
    return result;
}

uint16_t DeviceMatrix::ConvertIndex(uint16_t code)
{
    uint16_t index = (~code) & (code - 1);
    // 0xAAAA: 1010101010101010  0x5555: 0101010101010101 1: move the high(0xAAAA) to low(0x5555) bits
    index = ((index & 0xAAAA) >> 1) + (index & 0x5555);
    // 0xCCCC: 1100110011001100  0x3333: 0011001100110011 2: the count save at 2 bits
    index = ((index & 0xCCCC) >> 2) + (index & 0x3333);
    // 0xF0F0: 1111000011110000  0x0F0F: 0000111100001111 4: the count save at 4 bits
    index = ((index & 0xF0F0) >> 4) + (index & 0x0F0F);
    // 0xFF00: 1111000011110000  0x00FF: 0000111100001111 8: the count save at 8 bits
    index = ((index & 0xFF00) >> 8) + (index & 0x00FF);
    index--;
    return index;
}

void DeviceMatrix::UpdateRemoteMeta(const std::string &device, Mask &mask)
{
    MatrixMetaData newMeta;
    newMeta.dynamic = mask.dynamic;
    newMeta.statics = mask.statics;
    newMeta.deviceId = device;
    newMeta.origin = Origin::REMOTE_RECEIVED;
    newMeta.dynamicInfo = dynamicApps_;
    newMeta.staticsInfo = staticsApps_;
    auto [exist, oldMeta] = GetMatrixMeta(device);
    if (exist && oldMeta == newMeta) {
        return;
    }
    if (exist) {
        if (High(newMeta.dynamic) < High(oldMeta.dynamic)) {
            newMeta.dynamic &= 0x000F;
            newMeta.dynamic |= High(oldMeta.dynamic);
        }
        if (High(newMeta.statics) < High(oldMeta.statics)) {
            newMeta.statics &= 0x000F;
            newMeta.statics |= High(oldMeta.statics);
        }
    }
    MetaDataManager::GetInstance().SaveMeta(newMeta.GetKey(), newMeta, true);
}

void DeviceMatrix::SaveSwitches(const std::string &device, const DataLevel &dataLevel)
{
    if (device.empty() || dataLevel.switches == INVALID_VALUE ||
        dataLevel.switchesLen == INVALID_LENGTH) {
        ZLOGW("switches data invalid, device:%{public}s", Anonymous::Change(device).c_str());
        return;
    }
    SwitchesMetaData newMeta;
    newMeta.deviceId = device;
    newMeta.value = dataLevel.switches;
    newMeta.length = dataLevel.switchesLen;
    SwitchesMetaData oldMeta;
    auto saved = MetaDataManager::GetInstance().LoadMeta(newMeta.GetKey(), oldMeta, true);
    if (!saved || newMeta != oldMeta) {
        MetaDataManager::GetInstance().SaveMeta(newMeta.GetKey(), newMeta, true);
    }
    ZLOGI("exist:%{public}d, changed:%{public}d", saved, newMeta != oldMeta);
}

void DeviceMatrix::Broadcast(const DataLevel &dataLevel)
{
    EventCenter::Defer defer;
    MatrixEvent::MatrixData matrix = {
        .dynamic = ResetMask(dataLevel.dynamic),
        .statics = ResetMask(dataLevel.statics),
        .switches = dataLevel.switches,
        .switchesLen = dataLevel.switchesLen,
    };
    std::lock_guard<decltype(taskMutex_)> lock(taskMutex_);
    if (!lastest_.IsValid()) {
        EventCenter::GetInstance().PostEvent(std::make_unique<MatrixEvent>(MATRIX_BROADCAST, "", matrix));
        return;
    }
    matrix.dynamic |= Low(lastest_.dynamic);
    matrix.statics |= Low(lastest_.statics);
    if (High(matrix.dynamic) != INVALID_HIGH &&  High(lastest_.dynamic)!= INVALID_HIGH &&
        High(matrix.dynamic) < High(lastest_.dynamic)) {
            matrix.dynamic &= 0x000F;
            matrix.dynamic |= High(matrix.dynamic);
    }
    if (High(matrix.statics) != INVALID_HIGH &&  High(lastest_.statics)!= INVALID_HIGH &&
        High(matrix.statics) < High(lastest_.statics)) {
            matrix.statics &= 0x000F;
            matrix.statics |= High(matrix.statics);
    }
    EventCenter::GetInstance().PostEvent(std::make_unique<MatrixEvent>(MATRIX_BROADCAST, "", matrix));
    lastest_ = matrix;
    auto executor = executors_;
    if (executor != nullptr && task_ != ExecutorPool::INVALID_TASK_ID) {
        task_ = executor->Reset(task_, std::chrono::minutes(RESET_MASK_DELAY));
    }
}

void DeviceMatrix::OnChanged(uint16_t code, LevelType type)
{
    if (type < LevelType::STATICS || type > LevelType::DYNAMIC) {
        return;
    }
    uint16_t low = Low(code);
    uint16_t codes[LevelType::BUTT] = { 0 };
    codes[type] |= low;
    EventCenter::Defer defer;
    {
        std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
        for (auto &[key, mask] : onLines_) {
            mask.statics |= codes[LevelType::STATICS];
            mask.dynamic |= codes[LevelType::DYNAMIC];
        }
        for (auto &[key, mask] : offLines_) {
            mask.statics |= codes[LevelType::STATICS];
            mask.dynamic |= codes[LevelType::DYNAMIC];
        }
    }
    if (low == 0) {
        return;
    }
    Mask mask;
    UpdateMask(mask);
    mask.SetStaticsMask(codes[LevelType::STATICS]);
    mask.SetDynamicMask(codes[LevelType::DYNAMIC]);
    MatrixEvent::MatrixData matrixData = {
        .dynamic = mask.dynamic,
        .statics = mask.statics,
    };
    SwitchesMetaData switchesData;
    switchesData.deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    if (MetaDataManager::GetInstance().LoadMeta(switchesData.GetKey(), switchesData, true)) {
        matrixData.switches = switchesData.value;
        matrixData.switchesLen = switchesData.length;
    }
    EventCenter::GetInstance().PostEvent(std::make_unique<MatrixEvent>(MATRIX_BROADCAST, "", matrixData));
    std::lock_guard<decltype(taskMutex_)> lock(taskMutex_);
    lastest_ = matrixData;
    auto executor = executors_;
    if (executor != nullptr) {
        if (task_ != ExecutorPool::INVALID_TASK_ID) {
            task_ = executor->Reset(task_, std::chrono::minutes(RESET_MASK_DELAY));
        } else {
            task_ = executor->Schedule(std::chrono::minutes(RESET_MASK_DELAY), GenResetTask());
        }
    }
}

void DeviceMatrix::Mask::SetDynamicMask(uint16_t mask)
{
    dynamic = ResetMask(dynamic);
    dynamic |= Low(mask);
}

void DeviceMatrix::Mask::SetStaticsMask(uint16_t mask)
{
    statics = ResetMask(statics);
    statics |= Low(mask);
}

void DeviceMatrix::OnChanged(const StoreMetaData &metaData)
{
    if (metaData.dataType < LevelType::STATICS || metaData.dataType > LevelType::DYNAMIC) {
        return;
    }
    auto code = GetCode(metaData);
    if (code != 0) {
        OnChanged(code, static_cast<LevelType>(metaData.dataType));
    }
}

void DeviceMatrix::OnExchanged(const std::string &device, uint16_t code, LevelType type, ChangeType changeType)
{
    if (type < LevelType::STATICS || type > LevelType::DYNAMIC) {
        return;
    }
    uint16_t low = Low(code);
    uint16_t codes[LevelType::BUTT] = { 0 };
    codes[type] |= low;
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    if (changeType == ChangeType::CHANGE_LOCAL || changeType == ChangeType::CHANGE_ALL) {
        auto it = onLines_.find(device);
        if (it != onLines_.end()) {
            it->second.statics &= ~codes[LevelType::STATICS];
            it->second.dynamic &= ~codes[LevelType::DYNAMIC];
        }
        it = offLines_.find(device);
        if (it != offLines_.end()) {
            it->second.statics &= ~codes[LevelType::STATICS];
            it->second.dynamic &= ~codes[LevelType::DYNAMIC];
        }
    }
    if (changeType == ChangeType::CHANGE_LOCAL) {
        return;
    }
    auto it = remotes_.find(device);
    if (it != remotes_.end()) {
        it->second.statics &= ~codes[LevelType::STATICS];
        it->second.dynamic &= ~codes[LevelType::DYNAMIC];
    }
    UpdateConsistentMeta(device, it->second);
    if (observer_) {
        observer_(device, it->second.dynamic);
    }
}

void DeviceMatrix::UpdateConsistentMeta(const std::string &device, const Mask &remote)
{
    uint16_t statics = Low(remote.statics);
    uint16_t dynamic = Low(remote.dynamic);
    if (statics != 0 && dynamic != 0) {
        return;
    }
    auto [exist, meta] = GetMatrixMeta(device);
    if (!exist) {
        meta.dynamic = remote.dynamic;
        meta.statics = remote.statics;
        meta.origin = Origin::REMOTE_RECEIVED;
        meta.dynamicInfo = dynamicApps_;
        meta.staticsInfo = staticsApps_;
        meta.deviceId = device;
        MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true);
    }
    auto [saved, metaData] = GetMatrixMeta(device, true);
    if (!saved) {
        meta.origin = Origin::REMOTE_CONSISTENT;
        meta.deviceId = device;
        MetaDataManager::GetInstance().SaveMeta(meta.GetConsistentKey(), meta, true);
        return;
    }
    if (statics == 0 && High(meta.statics) > High(metaData.statics)) {
        metaData.statics = meta.statics;
    }
    if (dynamic == 0 && High(meta.dynamic) > High(metaData.dynamic)) {
        metaData.dynamic = meta.dynamic;
    }
    MetaDataManager::GetInstance().SaveMeta(metaData.GetConsistentKey(), metaData, true);
}

void DeviceMatrix::OnExchanged(const std::string &device, const StoreMetaData &metaData, ChangeType type)
{
    if (metaData.dataType < LevelType::STATICS || metaData.dataType > LevelType::DYNAMIC) {
        return;
    }
    auto code = GetCode(metaData);
    if (code != 0) {
        OnExchanged(device, code, static_cast<LevelType>(metaData.dataType), type);
    }
}

uint16_t DeviceMatrix::GetCode(const StoreMetaData &metaData)
{
    if (metaData.tokenId == tokenId_ && metaData.storeId == storeId_) {
        return META_STORE_MASK;
    }
    if (metaData.dataType == LevelType::STATICS) {
        for (size_t i = 0; i < staticsApps_.size(); i++) {
            if (staticsApps_[i] == metaData.appId) {
                return SetMask(i);
            }
        }
    }
    if (metaData.dataType == LevelType::DYNAMIC) {
        for (size_t i = 0; i < dynamicApps_.size(); i++) {
            if (dynamicApps_[i] == metaData.appId) {
                return SetMask(i + 1);
            }
        }
    }
    return 0;
}

std::pair<bool, uint16_t> DeviceMatrix::GetMask(const std::string &device, LevelType type)
{
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    auto it = onLines_.find(device);
    if (it == onLines_.end()) {
        return { false, 0 };
    }
    switch (type) {
        case LevelType::STATICS:
            return { true, Low(it->second.statics) };
        case LevelType::DYNAMIC:
            return { true, Low(it->second.dynamic) };
        default:
            break;
    }
    return { false, 0 };
}

std::pair<bool, uint16_t> DeviceMatrix::GetRemoteMask(const std::string &device, LevelType type)
{
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    auto it = remotes_.find(device);
    if (it == remotes_.end()) {
        return { false, 0 };
    }
    switch (type) {
        case LevelType::STATICS:
            return { true, Low(it->second.statics) };
        case LevelType::DYNAMIC:
            return { true, Low(it->second.dynamic) };
        default:
            break;
    }
    return { false, 0 };
}

std::map<std::string, uint16_t> DeviceMatrix::GetRemoteDynamicMask()
{
    std::map<std::string, uint16_t> masks;
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    for (const auto &[device, mask] : remotes_) {
        masks.insert_or_assign(device, Low(mask.dynamic));
    }
    return masks;
}

std::pair<bool, uint16_t> DeviceMatrix::GetRecvLevel(const std::string &device, LevelType type)
{
    auto [exist, meta] = GetMatrixMeta(device);
    if (!exist) {
        return { false, High(INVALID_LEVEL) };
    }
    switch (type) {
        case LevelType::STATICS:
            return { true, High(meta.statics) };
        case LevelType::DYNAMIC:
            return { true, High(meta.dynamic) };
        default:
            break;
    }
    return { false, High(INVALID_LEVEL) };
}

std::pair<bool, uint16_t> DeviceMatrix::GetConsLevel(const std::string &device, LevelType type)
{
    auto [exist, meta] = GetMatrixMeta(device, true);
    if (!exist) {
        return { false, High(INVALID_LEVEL) };
    }
    switch (type) {
        case LevelType::STATICS:
            return { true, High(meta.statics) };
        case LevelType::DYNAMIC:
            return { true, High(meta.dynamic) };
        default:
            break;
    }
    return { false, High(INVALID_LEVEL) };
}

void DeviceMatrix::Clear()
{
    matrixs_.ResetCapacity(0);
    matrixs_.ResetCapacity(MAX_DEVICES);
    versions_.ResetCapacity(0);
    versions_.ResetCapacity(MAX_DEVICES);
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    onLines_.clear();
    offLines_.clear();
    remotes_.clear();
}

std::pair<bool, MatrixMetaData> DeviceMatrix::GetMatrixMeta(const std::string &device, bool IsConsistent)
{
    MatrixMetaData meta;
    if (!IsConsistent && matrixs_.Get(device, meta)) {
        return { true, meta };
    }
    meta.deviceId = device;
    std::string key;
    if (IsConsistent) {
        key = meta.GetConsistentKey();
    } else {
        key = meta.GetKey();
    }
    auto success = MetaDataManager::GetInstance().LoadMeta(key, meta, true);
    if (success && !IsConsistent) {
        meta.deviceId = "";
        matrixs_.Set(device, meta);
    }
    return { success, meta };
}

void DeviceMatrix::SetMatrixMeta(const MatrixMetaData &meta, bool IsConsistent)
{
    if (!IsConsistent) {
        matrixs_.Set(meta.deviceId, meta);
    }
    MetaDataManager::GetInstance().SaveMeta(IsConsistent ? meta.GetConsistentKey() : meta.GetKey(), meta, true);
}

MatrixMetaData DeviceMatrix::GetMatrixInfo(const std::string &device)
{
    MatrixMetaData meta;
    if (versions_.Get(device, meta)) {
        return meta;
    }
    meta.deviceId = device;
    if (MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta)) {
        meta.deviceId = "";
        versions_.Set(device, meta);
    }
    return meta;
}

void DeviceMatrix::RegRemoteChange(std::function<void(const std::string &, uint16_t)> observer)
{
    if (observer_) {
        ZLOGD("duplicate register");
        return;
    }
    observer_ = std::move(observer);
}

bool DeviceMatrix::IsDynamic(const StoreMetaData &metaData)
{
    if (metaData.dataType != LevelType::DYNAMIC) {
        return false;
    }
    if (metaData.tokenId == tokenId_ && metaData.storeId == storeId_) {
        return true;
    }
    return std::find(dynamicApps_.begin(), dynamicApps_.end(), metaData.appId) != dynamicApps_.end();
}

bool DeviceMatrix::IsStatics(const StoreMetaData &metaData)
{
    if (metaData.dataType != LevelType::STATICS) {
        return false;
    }
    return std::find(staticsApps_.begin(), staticsApps_.end(), metaData.appId) != staticsApps_.end();
}

void DeviceMatrix::SetExecutor(std::shared_ptr<ExecutorPool> executors)
{
    executors_ = executors;
}

DeviceMatrix::Task DeviceMatrix::GenResetTask()
{
    return [this]() {
        MatrixEvent::MatrixData matrixData;
        {
            std::lock_guard<decltype(taskMutex_)> lock(taskMutex_);
            matrixData = lastest_;
            task_ = ExecutorPool::INVALID_TASK_ID;
        }
        matrixData.dynamic = ResetMask(matrixData.dynamic);
        matrixData.statics = ResetMask(matrixData.statics);
        EventCenter::GetInstance().PostEvent(std::make_unique<MatrixEvent>(MATRIX_BROADCAST, "", matrixData));
    };
}

bool DeviceMatrix::DataLevel::IsValid() const
{
    return !(dynamic == INVALID_LEVEL && statics == INVALID_LEVEL &&
        switches == INVALID_VALUE && switchesLen == INVALID_LENGTH);
}

bool DeviceMatrix::IsSupportMatrix()
{
    return isSupportBroadcast_;
}
} // namespace OHOS::DistributedData
