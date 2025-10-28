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

#ifndef UDMF_DELAY_DATA_ACQUIRE_CONTAINER_H
#define UDMF_DELAY_DATA_ACQUIRE_CONTAINER_H

#include "block_data.h"
#include "udmf_notifier_proxy.h"
#include "unified_data.h"

namespace OHOS {
namespace UDMF {

class DelayDataAcquireContainer {
public:
    static DelayDataAcquireContainer &GetInstance();
    // delayDataCallback_ part
    void RegisterDelayDataCallback(const std::string &key, const DelayGetDataInfo &info);
    bool HandleDelayDataCallback(const std::string &key, const UnifiedData &unifiedData);
    bool QueryDelayGetDataInfo(const std::string &key, DelayGetDataInfo &info);
    std::vector<std::string> QueryAllDelayKeys();

private:
    DelayDataAcquireContainer() = default;
    ~DelayDataAcquireContainer() = default;
    DelayDataAcquireContainer(const DelayDataAcquireContainer &obj) = delete;
    DelayDataAcquireContainer &operator=(const DelayDataAcquireContainer &obj) = delete;

    std::mutex delayDataMutex_;
    std::map<std::string, DelayGetDataInfo> delayDataCallback_ {};
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_DELAY_DATA_ACQUIRE_CONTAINER_H