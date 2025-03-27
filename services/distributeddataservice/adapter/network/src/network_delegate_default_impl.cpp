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
#define LOG_TAG "NetworkDelegateDefaultImpl"
#include "network_delegate_default_impl.h"

namespace OHOS::DistributedData {
__attribute__((used)) static bool g_isInit = NetworkDelegateDefaultImpl::Init();
NetworkDelegateDefaultImpl::NetworkDelegateDefaultImpl()
{
}

NetworkDelegateDefaultImpl::~NetworkDelegateDefaultImpl()
{
}

bool NetworkDelegateDefaultImpl::IsNetworkAvailable()
{
    return false;
}

void NetworkDelegateDefaultImpl::RegOnNetworkChange()
{
    return;
}

NetworkDelegate::NetworkType NetworkDelegateDefaultImpl::GetNetworkType(bool retrieve)
{
    return NetworkType::NONE;
}

void NetworkDelegateDefaultImpl::BindExecutor(std::shared_ptr<ExecutorPool> executors)
{
}

bool NetworkDelegateDefaultImpl::Init()
{
    static NetworkDelegateDefaultImpl delegate;
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&]() { NetworkDelegate::RegisterNetworkInstance(&delegate); });
    return true;
}
} // namespace OHOS::DistributedData