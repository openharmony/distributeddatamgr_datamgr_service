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

#include "eventcenter/provider_center.h"
namespace OHOS {
namespace DistributedData {
ProviderCenter::ProviderCenter() {}
ProviderCenter& ProviderCenter::GetInstance()
{
    static ProviderCenter providerCenter;
    return providerCenter;
}

bool ProviderCenter::Subscribe(int32_t msgId, const ProviderCenter::Provider& provider)
{
    if (provider == nullptr) {
        return false;
    }
    return providers_.ComputeIfAbsent(msgId, [&provider](const auto&) {
        return provider;
    });
}

bool ProviderCenter::Unsubscribe(int32_t msgId)
{
    return providers_.ComputeIfPresent(msgId, [](const auto&, const auto&) {
        return false;
    });
}

int32_t ProviderCenter::Send(const Message& data, Message& reply) const
{
    auto [success, provider] = providers_.Find(data.GetMessageId());
    if (!success || !provider) {
        return PROVIDER_CENTER_NO_PROVIDER;
    }
    return provider(data, reply);
}

} // namespace DistributedData
} // namespace OHOS
