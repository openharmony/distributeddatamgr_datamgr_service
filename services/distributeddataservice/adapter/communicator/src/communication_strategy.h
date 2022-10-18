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

#ifndef DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_COMMUNICATION_STRAGETY_H
#define DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_COMMUNICATION_STRAGETY_H

#include <vector>
#include "concurrent_map.h"
#include "app_device_change_listener.h"
#include "session.h"

namespace OHOS {
namespace AppDistributedKv {
class CommunicationStrategy : public AppDistributedKv::AppDeviceChangeListener {
public:
    static CommunicationStrategy &GetInstance();
    Status Init();
    void OnDeviceChanged(const AppDistributedKv::DeviceInfo &info,
                         const AppDistributedKv::DeviceChangeType &type) const override;
    bool GetStrategy(const std::string &deviceId, int32_t dataLen, std::vector<LinkType> &linkTypes);
private:
    CommunicationStrategy() = default;
    ~CommunicationStrategy() = default;
    CommunicationStrategy(CommunicationStrategy const &) = delete;
    void operator=(CommunicationStrategy const &) = delete;
    CommunicationStrategy(CommunicationStrategy &&) = delete;
    CommunicationStrategy &operator=(CommunicationStrategy &&) = delete;
    void UpdateCommunicationStrategy(const AppDistributedKv::DeviceInfo &info,
                                     const AppDistributedKv::DeviceChangeType &type) const;
    mutable ConcurrentMap<std::string, bool> strategys_;
    static constexpr uint32_t SWITCH_CONNECTION_THRESHOLD = 75 * 1024;
};
}  // namespace AppDistributedKv
}  // namespace OHOS

#endif // DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_COMMUNICATION_STRAGETY_H
