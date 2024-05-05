/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "AutoSyncMatrix"

#include "auto_sync_matrix.h"
#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "device_matrix.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "utils/anonymous.h"
#include "utils/converter.h"

namespace OHOS::DistributedData {
AutoSyncMatrix &AutoSyncMatrix::GetInstance()
{
    static AutoSyncMatrix instance;
    return instance;
}

AutoSyncMatrix::AutoSyncMatrix()
{
    MetaDataManager::GetInstance().Subscribe(StoreMetaData::GetPrefix({}),
        [this](const std::string &, const std::string &meta, int32_t action) -> bool {
            StoreMetaData metaData;
            if (!StoreMetaData::Unmarshall(meta, metaData)) {
                return true;
            }
            if (!IsAutoSync(metaData)) {
                return true;
            }
            ZLOGI("bundleName:%{public}s storeId:%{public}s action:%{public}d",
                metaData.bundleName.c_str(), Anonymous::Change(metaData.storeId).c_str(), action);
            auto act = static_cast<MetaDataManager::Action>(action);
            switch (act) {
                case MetaDataManager::INSERT:
                    AddStore(metaData);
                    break;
                case MetaDataManager::DELETE:
                    DelStore(metaData);
                    break;
                case MetaDataManager::UPDATE:
                    UpdateStore(metaData);
                    break;
                default:
                    break;
            }
            return true;
    });
}

AutoSyncMatrix::~AutoSyncMatrix()
{
}

void AutoSyncMatrix::Initialize()
{
    std::vector<StoreMetaData> metas;
    if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({}), metas)) {
        ZLOGE("load meta failed.");
        return;
    }
    for (const auto &meta : metas) {
        if (!IsAutoSync(meta)) {
            continue;
        }
        AddStore(meta);
    }
}

bool AutoSyncMatrix::IsAutoSync(const StoreMetaData &meta)
{
    if (meta.bundleName == Bootstrap::GetInstance().GetProcessLabel()) {
        return false;
    }
    if (meta.dataType != DataType::DYNAMICAL || DeviceMatrix::GetInstance().IsDynamic(meta)) {
        return false;
    }
    return meta.isAutoSync;
}

void AutoSyncMatrix::AddStore(const StoreMetaData &meta)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    metas_.emplace_back(std::move(meta));
    size_t pos = metas_.size() - 1;
    for (auto &[device, mask] : onlines_) {
        mask.Set(pos);
    }
    for (auto &[device, mask] : offlines_) {
        mask.Set(pos);
    }
}

void AutoSyncMatrix::DelStore(const StoreMetaData &meta)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    auto it = metas_.begin();
    for (; it < metas_.end(); it++) {
        if ((*it) != meta) {
            continue;
        }
        metas_.erase(it);
        break;
    }
    size_t pos = it - metas_.begin();
    if (pos == metas_.size()) {
        return;
    }
    for (auto &[device, mask] : onlines_) {
        mask.Delete(pos);
    }
    for (auto &[device, mask] : offlines_) {
        mask.Delete(pos);
    }
}

void AutoSyncMatrix::Mask::Delete(size_t pos)
{
    std::bitset<MAX_SIZE> tmp;
    for (size_t i = 0; i < pos; i++) {
        tmp.set(i);
    }
    tmp = tmp & data;
    data >>= pos + 1;
    data <<= pos;
    data = data | tmp;
}

void AutoSyncMatrix::Mask::Set(size_t pos)
{
    data.set(pos);
}

void AutoSyncMatrix::Mask::Init(size_t size)
{
    for (size_t i = 0; i < size; i++) {
        data.set(i);
    }
}

void AutoSyncMatrix::Mask::Reset(size_t pos)
{
    data.reset(pos);
}

void AutoSyncMatrix::UpdateStore(const StoreMetaData &meta)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    for (auto &store : metas_) {
        if (store.GetKey() != meta.GetKey()) {
            continue;
        }
        store = meta;
        break;
    }
}

void AutoSyncMatrix::Online(const std::string &device)
{
    if (device.empty()) {
        return;
    }
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    Mask mask;
    mask.Init(metas_.size());
    auto it = offlines_.find(device);
    if (it != offlines_.end()) {
        mask = it->second;
        offlines_.erase(it);
    }
    onlines_.insert_or_assign(device, mask);
}
    
void AutoSyncMatrix::Offline(const std::string &device)
{
    if (device.empty()) {
        return;
    }
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    Mask mask;
    mask.Init(metas_.size());
    auto it = onlines_.find(device);
    if (it != onlines_.end()) {
        mask = it->second;
        onlines_.erase(it);
    }
    offlines_.insert_or_assign(device, mask);
}

void AutoSyncMatrix::OnChanged(const StoreMetaData &metaData)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    auto it = std::find(metas_.begin(), metas_.end(), metaData);
    if (it == metas_.end()) {
        return;
    }
    size_t pos = it - metas_.begin();
    for (auto &[device, mask] : onlines_) {
        mask.Set(pos);
    }
    for (auto &[device, mask] : offlines_) {
        mask.Set(pos);
    }
}
    
void AutoSyncMatrix::OnExchanged(const std::string &device, const StoreMetaData &metaData)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    auto it = std::find(metas_.begin(), metas_.end(), metaData);
    if (it == metas_.end()) {
        return;
    }
    size_t pos = it - metas_.begin();
    auto iter = onlines_.find(device);
    if (iter != onlines_.end()) {
        iter->second.Reset(pos);
    }
    iter = offlines_.find(device);
    if (iter != offlines_.end()) {
        iter->second.Reset(pos);
    }
}

std::vector<StoreMetaData> AutoSyncMatrix::GetChangedStore(const std::string &device)
{
    if (device.empty()) {
        return {};
    }
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    auto it = onlines_.find(device);
    if (it == onlines_.end()) {
        return {};
    }
    auto mask = it->second;
    size_t size = metas_.size();
    Mask tmp;
    tmp.Set(0);
    std::vector<StoreMetaData> result;
    for (size_t i = 0; i < size; i++) {
        if (tmp.data == (mask.data & tmp.data)) {
            result.emplace_back(metas_[i]);
        }
        mask.data >>= 1;
    }
    return result;
}
} // namespace OHOS::DistributedData
