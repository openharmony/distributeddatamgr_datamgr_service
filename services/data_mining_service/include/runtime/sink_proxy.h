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

#ifndef OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_RUNTIME_SINK_PROXY_H
#define OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_RUNTIME_SINK_PROXY_H

#include <memory>
#include <mutex>
#include <string>

#include "etl_interfaces.h"
#include "runtime/endpoint_config.h"
#include "runtime/plugin_loader.h"

namespace OHOS::DataMining {
class SinkProxy final {
public:
    SinkProxy(std::string name, EndpointConfig endpoint, std::shared_ptr<PluginLoader> loader,
        std::shared_ptr<Sink> sink = nullptr);
    ~SinkProxy() = default;

    int32_t Save(std::shared_ptr<Context> context, std::shared_ptr<DataValue> data);

private:
    int32_t EnsureSinkLocked();

    std::string name_;
    EndpointConfig endpoint_;
    std::shared_ptr<PluginLoader> loader_;
    std::shared_ptr<Sink> sink_;
    std::mutex mutex_;
};
} // namespace OHOS::DataMining

#endif // OHOS_DISTRIBUTEDDATAMGR_SERVICES_DATA_MINING_SERVICE_RUNTIME_SINK_PROXY_H
