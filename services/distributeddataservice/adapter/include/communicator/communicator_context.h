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

#ifndef DISTRIBUTEDDATAMGR_COMMUNICATOR_CONTEXT_H
#define DISTRIBUTEDDATAMGR_COMMUNICATOR_CONTEXT_H

#include "app_device_change_listener.h"
#include "executor_pool.h"
#include "commu_types.h"
#include "concurrent_map.h"
#include "visibility.h"
#include "iprocess_communicator.h"

namespace OHOS::DistributedData {
class API_EXPORT CommunicatorContext {
public:
    using Status = OHOS::DistributedKv::Status;
    using DevChangeListener = OHOS::AppDistributedKv::AppDeviceChangeListener;
    using DeviceInfo = OHOS::AppDistributedKv::DeviceInfo;
    using OnCloseAble = std::function<void(const std::string &deviceId)>;

    API_EXPORT static CommunicatorContext &GetInstance();
    API_EXPORT void SetThreadPool(std::shared_ptr<ExecutorPool> executors);
    std::shared_ptr<ExecutorPool> GetThreadPool();
    Status RegSessionListener(const DevChangeListener *observer);
    Status UnRegSessionListener(const DevChangeListener *observer);
    void NotifySessionReady(const std::string &deviceId);
    void NotifySessionClose(const std::string &deviceId);
    void SetSessionListener(const OnCloseAble &closeAbleCallback);
    bool IsSessionReady(const std::string &deviceId);

private:
    CommunicatorContext() = default;
    ~CommunicatorContext() = default;
    CommunicatorContext(CommunicatorContext const &) = delete;
    void operator=(CommunicatorContext const &) = delete;
    CommunicatorContext(CommunicatorContext &&) = delete;
    CommunicatorContext &operator=(CommunicatorContext &&) = delete;

    mutable std::mutex sessionMutex_;
    OnCloseAble closeListener_;
    std::shared_ptr<ExecutorPool> executors_;
    std::mutex mutex_;
    std::vector<const DevChangeListener *> observers_;
    ConcurrentMap<const std::string, const std::string> devices_ {};
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTEDDATAMGR_COMMUNICATOR_CONTEXT_H