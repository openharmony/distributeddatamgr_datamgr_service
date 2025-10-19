/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define LOG_TAG "DelayDataContainer"
#include "delay_data_container.h"
#include "log_print.h"

namespace OHOS {
namespace UDMF {
constexpr uint32_t WAIT_TIME = 800;

DelayDataContainer &DelayDataContainer::GetInstance()
{
    static DelayDataContainer instance;
    return instance;
}

bool DelayDataContainer::HandleDelayLoad(const QueryOptions &query, UnifiedData &unifiedData, int32_t &res)
{
    return dataLoadCallback_.ComputeIfPresent(query.key, [&](const auto &key, auto &callback) {
        std::shared_ptr<BlockData<std::optional<UnifiedData>, std::chrono::milliseconds>> blockData;
        auto [found, cache] = blockDelayDataCache_.Find(key);
        if (found) {
            blockData = cache.blockData;
        } else {
            blockData = std::make_shared<BlockData<std::optional<UnifiedData>, std::chrono::milliseconds>>(WAIT_TIME);
            blockDelayDataCache_.Insert(key, BlockDelayData{query.tokenId, blockData});
            callback->HandleDelayObserver(key, DataLoadInfo());
        }
        ZLOGI("Start waiting for data, key:%{public}s", key.c_str());
        auto dataOpt = blockData->GetValue();
        if (dataOpt.has_value()) {
            unifiedData = *dataOpt;
            blockDelayDataCache_.Erase(key);
            return false;
        }
        res = E_NOT_FOUND;
        return true;
    });
}

void DelayDataContainer::RegisterDataLoadCallback(const std::string &key, sptr<UdmfNotifierProxy> callback)
{
    dataLoadCallback_.InsertOrAssign(key, callback);
}

bool DelayDataContainer::ExecDataLoadCallback(const std::string &key, const DataLoadInfo &info)
{
    auto it = dataLoadCallback_.Find(key);
    if (!it.first) {
        return false;
    }
    dataLoadCallback_.Erase(key);
    ZLOGI("Execute data load callback, key:%{public}s", key.c_str());
    it.second->HandleDelayObserver(key, info);
    return true;
}

void DelayDataContainer::ExecAllDataLoadCallback()
{
    dataLoadCallback_.ForEach([](const auto &key, auto &callback) {
        ZLOGI("Execute data load callback, key:%{public}s", key.c_str());
        callback->HandleDelayObserver(key, DataLoadInfo());
        return true;
    });
    dataLoadCallback_.Clear();
}

void DelayDataContainer::RegisterDelayDataCallback(const std::string &key, const DelayGetDataInfo &info)
{
    delayDataCallback_.InsertOrAssign(key, info);
}

bool DelayDataContainer::HandleDelayDataCallback(const std::string &key, const UnifiedData &unifiedData)
{
    return delayDataCallback_.ComputeIfPresent(key, [&](const auto &key, auto &delayGetDataInfo) {
        auto callback = iface_cast<DelayDataCallbackProxy>(delayGetDataInfo.dataCallback);
        if (callback == nullptr) {
            ZLOGE("Delay data callback is null, key:%{public}s", key.c_str());
            return false;
        }
        callback->DelayDataCallback(key, unifiedData);
        delayDataCallback_.Erase(key);
        return false;
    });
}

bool DelayDataContainer::QueryDelayGetDataInfo(const std::string &key, DelayGetDataInfo &getDataInfo)
{
    auto [found, callback] = delayDataCallback_.Find(key);
    if (!found) {
        return false;
    }
    getDataInfo = callback;
    return true;
}

bool DelayDataContainer::QueryBlockDelayData(const std::string &key, BlockDelayData &blockData)
{
    auto [found, data] = blockDelayDataCache_.Find(key);
    if (!found) {
        return false;
    }
    blockData = data;
    return true;
}

void DelayDataContainer::SaveDelayDragDeviceInfo(std::string &deviceId)
{
    delayDragDeviceInfo_.emplace_back(deviceId);
}

std::vector<std::string> DelayDataContainer::QueryDelayDragDeviceInfo()
{
    auto devices = delayDragDeviceInfo_;
    delayDragDeviceInfo_.clear();
    return devices;
}

void DelayDataContainer::ClearDelayDragDeviceInfo()
{
    delayDragDeviceInfo_.clear();
}

void DelayDataContainer::SaveDelayAcceptableInfo(const std::string &key, const DataLoadInfo &info)
{
    delayAcceptableInfos_.InsertOrAssign(key, info);
}

bool DelayDataContainer::QueryDelayAcceptableInfo(const std::string &key, DataLoadInfo &info)
{
    auto [found, dataLoadInfo] = delayAcceptableInfos_.Find(key);
    if (!found) {
        return false;
    }
    info = dataLoadInfo;
    delayAcceptableInfos_.Erase(key);
    return true;
}
} // namespace UDMF
} // namespace OHOS