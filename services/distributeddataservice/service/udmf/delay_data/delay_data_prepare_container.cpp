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

#define LOG_TAG "DelayDataPrepareContainer"
#include "delay_data_prepare_container.h"
#include "log_print.h"

namespace OHOS {
namespace UDMF {
constexpr uint32_t WAIT_TIME = 800;

DelayDataPrepareContainer &DelayDataPrepareContainer::GetInstance()
{
    static DelayDataPrepareContainer instance;
    return instance;
}

bool DelayDataPrepareContainer::HandleDelayLoad(const QueryOption &query, UnifiedData &unifiedData)
{
    if (query.key.empty()) {
        ZLOGE("HandleDelayLoad failed, key is empty");
        return false;
    }
    sptr<UdmfNotifierProxy> callback = nullptr;
    std::shared_ptr<BlockData<std::optional<UnifiedData>, std::chrono::milliseconds>> blockData;
    {
        std::lock_guard<std::mutex> lock(dataLoadMutex_);
        auto dataLoadIter = dataLoadCallback_.find(query.key);
        if (dataLoadIter == dataLoadCallback_.end()) {
            return false;
        }
        auto blockDataIter = blockDelayDataCache_.find(query.key);
        if (blockDataIter != blockDelayDataCache_.end()) {
            blockData = blockDataIter->second.blockData;
        } else {
            blockData = std::make_shared<BlockData<std::optional<UnifiedData>, std::chrono::milliseconds>>(WAIT_TIME);
            blockDelayDataCache_.insert_or_assign(query.key, BlockDelayData{query.tokenId, blockData});
            callback = dataLoadIter->second;
        }
    }
    if (callback != nullptr) {
        callback->HandleDelayObserver(query.key, DataLoadInfo());
    }
    ZLOGI("Start waiting for data, key:%{public}s", query.key.c_str());
    if (blockData == nullptr) {
        ZLOGE("Block data is null");
        return false;
    }
    auto dataOpt = blockData->GetValue();
    if (dataOpt.has_value()) {
        unifiedData = *dataOpt;
        std::lock_guard<std::mutex> lock(dataLoadMutex_);
        blockDelayDataCache_.erase(query.key);
        dataLoadCallback_.erase(query.key);
    }
    return true;
}

void DelayDataPrepareContainer::RegisterDataLoadCallback(const std::string &key, sptr<UdmfNotifierProxy> callback)
{
    std::lock_guard<std::mutex> lock(dataLoadMutex_);
    if (key.empty() || callback == nullptr) {
        ZLOGE("RegisterDataLoadCallback failed, key is empty or callback is null");
        return;
    }
    dataLoadCallback_.insert_or_assign(key, callback);
}

int DelayDataPrepareContainer::QueryDataLoadCallbackSize()
{
    std::lock_guard<std::mutex> lock(dataLoadMutex_);
    return static_cast<int>(dataLoadCallback_.size());
}

bool DelayDataPrepareContainer::ExecDataLoadCallback(const std::string &key, const DataLoadInfo &info)
{
    sptr<UdmfNotifierProxy> callback = nullptr;
    {
        std::lock_guard<std::mutex> lock(dataLoadMutex_);
        auto it = dataLoadCallback_.find(key);
        if (it == dataLoadCallback_.end()) {
            return false;
        }
        callback = it->second;
        dataLoadCallback_.erase(key);
    }
    if (callback == nullptr) {
        ZLOGE("Data load callback is null, key:%{public}s", key.c_str());
        return false;
    }
    callback->HandleDelayObserver(key, info);
    return true;
}

void DelayDataPrepareContainer::ExecAllDataLoadCallback()
{
    std::map<std::string, sptr<UdmfNotifierProxy>> callbacks;
    {
        std::lock_guard<std::mutex> lock(dataLoadMutex_);
        callbacks = dataLoadCallback_;
        dataLoadCallback_.clear();
    }
    for (const auto &[key, callback] : callbacks) {
        DataLoadInfo info;
        if (callback != nullptr) {
            callback->HandleDelayObserver(key, info);
        }
    }
}

bool DelayDataPrepareContainer::QueryBlockDelayData(const std::string &key, BlockDelayData &blockData)
{
    std::lock_guard<std::mutex> lock(dataLoadMutex_);
    auto it = blockDelayDataCache_.find(key);
    if (it == blockDelayDataCache_.end()) {
        return false;
    }
    blockData = it->second;
    return true;
}

} // namespace UDMF
} // namespace OHOS