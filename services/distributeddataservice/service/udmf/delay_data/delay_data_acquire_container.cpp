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

#define LOG_TAG "DelayDataAcquireContainer"

#include "delay_data_acquire_container.h"

#include "log_print.h"
#include "udmf_notifier_proxy.h"

namespace OHOS {
namespace UDMF {

DelayDataAcquireContainer &DelayDataAcquireContainer::GetInstance()
{
    static DelayDataAcquireContainer instance;
    return instance;
}

void DelayDataAcquireContainer::RegisterDelayDataCallback(const std::string &key, const DelayGetDataInfo &info)
{
    std::lock_guard<std::mutex> lock(delayDataMutex_);
    if (key.empty()) {
        ZLOGE("RegisterDelayDataCallback failed, key is empty");
        return;
    }
    delayDataCallback_.insert_or_assign(key, info);
}

bool DelayDataAcquireContainer::HandleDelayDataCallback(const std::string &key, const UnifiedData &unifiedData)
{
    sptr<DelayDataCallbackProxy> callback = nullptr;
    {
        std::lock_guard<std::mutex> lock(delayDataMutex_);
        auto it = delayDataCallback_.find(key);
        if (it == delayDataCallback_.end()) {
            ZLOGE("Can not find delay data callback, key:%{public}s", key.c_str());
            return false;
        }
        callback = iface_cast<DelayDataCallbackProxy>(it->second.dataCallback);
        delayDataCallback_.erase(key);
    }
    if (callback == nullptr) {
        ZLOGE("Delay data callback is null, key:%{public}s", key.c_str());
        return false;
    }
    callback->DelayDataCallback(key, unifiedData);
    return true;
}

bool DelayDataAcquireContainer::QueryDelayGetDataInfo(const std::string &key, DelayGetDataInfo &getDataInfo)
{
    std::lock_guard<std::mutex> lock(delayDataMutex_);
    auto it = delayDataCallback_.find(key);
    if (it == delayDataCallback_.end()) {
        return false;
    }
    getDataInfo = it->second;
    return true;
}

std::vector<std::string> DelayDataAcquireContainer::QueryAllDelayKeys()
{
    std::lock_guard<std::mutex> lock(delayDataMutex_);
    std::vector<std::string> keys;
    for (const auto &[key, _] : delayDataCallback_) {
        if (key.empty()) {
            continue;
        }
        keys.emplace_back(key);
    }
    return keys;
}

bool DelayDataAcquireContainer::IsContainDelayData(const std::string &key)
{
    std::lock_guard<std::mutex> lock(delayDataMutex_);
    return delayDataCallback_.find(key) != delayDataCallback_.end();
}

} // namespace UDMF
} // namespace OHOS