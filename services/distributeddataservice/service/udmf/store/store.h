/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef UDMF_STORE_H
#define UDMF_STORE_H

#include <string>
#include <shared_mutex>
#include <mutex>

#include "unified_data.h"

namespace OHOS {
namespace UDMF {
using ProcessCallback = std::function<void(AsyncProcessInfo &syncInfo)>;
class Store {
public:
    using Time = std::chrono::steady_clock::time_point;
    virtual Status Put(const UnifiedData &unifiedData) = 0;
    virtual Status Get(const std::string &key, UnifiedData &unifiedData) = 0;
    virtual Status GetSummary(UnifiedKey &key, Summary &summary) = 0;
    virtual Status Update(const UnifiedData &unifiedData) = 0;
    virtual Status Delete(const std::string &key) = 0;
    virtual Status DeleteBatch(const std::vector<std::string> &unifiedKeys) = 0;
    virtual Status Sync(const std::vector<std::string> &devices) = 0;
    virtual Status Sync(const std::vector<std::string> &devices, ProcessCallback callback) = 0;
    virtual Status Clear() = 0;
    virtual Status GetBatchData(const std::string &dataPrefix, std::vector<UnifiedData> &unifiedDataSet) = 0;
    virtual Status PutLocal(const std::string &key, const std::string &value) = 0;
    virtual Status GetLocal(const std::string &key, std::string &value) = 0;
    virtual Status DeleteLocal(const std::string &key) = 0;
    virtual Status PutRuntime(const std::string &key, const Runtime &runtime) = 0;
    virtual Status GetRuntime(const std::string &key, Runtime &runtime) = 0;
    virtual Status PutSummary(UnifiedKey &key, const Summary &summary) = 0;
    virtual Status SetRemotePullStartNotify() = 0;
    virtual Status RegisterDataChangedObserver(const std::string &key, uint32_t type) = 0;
    virtual bool UnRegisterDataChangedObserver(const std::string &key) = 0;
    virtual Status PutDelayData(const UnifiedData &unifiedData) = 0;
    virtual Status PutDataLoadInfo(const DataLoadInfo &info) = 0;
    virtual Status PushDelayData(const std::vector<std::string> &devices) = 0;
    virtual bool Init() = 0;

    bool operator<(const Time &time) const
    {
        std::shared_lock<decltype(timeMutex_)> lock(timeMutex_);
        return time_ < time;
    }

    void UpdateTime()
    {
        std::unique_lock<std::shared_mutex> lock(timeMutex_);
        time_ = std::chrono::steady_clock::now() + std::chrono::minutes(INTERVAL);
    }
private:
    static constexpr int64_t INTERVAL = 1; // 1 min
    mutable Time time_;
    mutable std::shared_mutex timeMutex_;
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_STORE_H
