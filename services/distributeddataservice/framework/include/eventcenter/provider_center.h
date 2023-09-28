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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_EVENTCENTER_PROVIDER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_EVENTCENTER_PROVIDER_H
#include <cstdint>
#include <functional>
#include <memory>

#include "concurrent_map.h"
#include "message.h"
#include "visibility.h"
namespace OHOS {
namespace DistributedData {
class ProviderCenter {
public:
    enum : int32_t {
        PROVIDER_CENTER_NO_PROVIDER = 0,
        PROVIDER_CENTER_INVALID_ARGS,
        PROVIDER_CENTER_ERROR,
        PROVIDER_CENTER_OK,
    };
    using Provider = std::function<int(const Message&, Message&)>;
    API_EXPORT static ProviderCenter& GetInstance();
    API_EXPORT bool Subscribe(int32_t msgId, const Provider& provider);
    API_EXPORT bool Unsubscribe(int32_t msgId);
    API_EXPORT int32_t Send(const Message& data, Message& reply) const;

private:
    ProviderCenter();
    ProviderCenter(const ProviderCenter&) = delete;
    ProviderCenter(ProviderCenter&&) = delete;
    ProviderCenter& operator=(const ProviderCenter&) = delete;
    ProviderCenter& operator=(ProviderCenter&&) = delete;
    ConcurrentMap<int32_t, Provider> providers_;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_EVENTCENTER_PROVIDER_H
