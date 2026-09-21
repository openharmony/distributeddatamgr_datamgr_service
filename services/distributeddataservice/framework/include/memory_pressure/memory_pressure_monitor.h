/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_MEMORY_PRESSURE_MONITOR_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_MEMORY_PRESSURE_MONITOR_H

#include <cstdint>
#include <functional>
#include <string>

#include "visibility.h"

namespace OHOS::DistributedData {
class MemoryPressureMonitor {
public:
    using Observer = std::function<void(int32_t)>;

    API_EXPORT static MemoryPressureMonitor *GetInstance();
    API_EXPORT static bool RegisterInstance(MemoryPressureMonitor *instance);

    virtual ~MemoryPressureMonitor() = default;
    virtual int32_t Subscribe(const std::string &name, Observer observer) = 0;
    virtual int32_t Unsubscribe(const std::string &name) = 0;

protected:
    MemoryPressureMonitor() = default;

private:
    static MemoryPressureMonitor *instance_;
};
} // namespace OHOS::DistributedData

#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_MEMORY_PRESSURE_MONITOR_H
