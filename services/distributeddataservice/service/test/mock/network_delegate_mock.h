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
#ifndef OHOS_NETWORK_DELEGATE_MOCK_H
#define OHOS_NETWORK_DELEGATE_MOCK_H

#include "network_delegate.h"

namespace OHOS {
namespace DistributedData {
class NetworkDelegateMock : public NetworkDelegate {
public:
    bool IsNetworkAvailable() override;
    NetworkType GetNetworkType(bool retrieve = false) override;
    void RegOnNetworkChange() override;
    bool isNetworkAvailable_ = true;
    virtual ~NetworkDelegateMock() = default;
};
} // namespace DistributedData
} // namespace OHOS
#endif //OHOS_NETWORK_DELEGATE_MOCK_H