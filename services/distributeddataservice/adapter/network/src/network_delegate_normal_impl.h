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

#ifndef OHOS_DISTRIBUTED_DATA_NETWORK_NORMAL_DELEGATE_IMPL_H
#define OHOS_DISTRIBUTED_DATA_NETWORK_NORMAL_DELEGATE_IMPL_H

#include <chrono>
#include <cstdint>

#include "dm_device_info.h"
#include "network/network_delegate.h"

namespace OHOS::DistributedData {
class NetworkDelegateNormalImpl : public NetworkDelegate {
public:
    using DmDeviceInfo = OHOS::DistributedHardware::DmDeviceInfo;
    static bool Init();
    bool IsNetworkAvailable() override;
    NetworkType GetNetworkType(bool retrieve = false) override;
    void RegOnNetworkChange() override;
    void BindExecutor(std::shared_ptr<ExecutorPool> executors) override;
    friend class NetConnCallbackObserver;
private:
    NetworkDelegateNormalImpl();
    ~NetworkDelegateNormalImpl();
    const DmDeviceInfo cloudDmInfo_;
    NetworkType SetNet(NetworkType netWorkType);
    NetworkType RefreshNet();
    static inline uint64_t GetTimeStamp()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();
    }
    ExecutorPool::Task GetTask(uint32_t retry);
    static constexpr int32_t EFFECTIVE_DURATION = 30 * 1000; // ms
    static constexpr int32_t NET_LOST_DURATION = 10 * 1000;  // ms
    static constexpr int32_t MAX_RETRY_TIME_S = 3;
    static constexpr int32_t RETRY_WAIT_TIME_S = 1;
    NetworkType defaultNetwork_ = NONE;
    uint64_t expireTime_ = 0;
    uint64_t netLostTime_ = 0;
    std::shared_ptr<ExecutorPool> executors_;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_NETWORK_NORMAL_DELEGATE_IMPL_H