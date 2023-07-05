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

#ifndef UDMF_SAMEPROCESSIPCGUARD_H
#define UDMF_SAMEPROCESSIPCGUARD_H

#include <string>

#include "ipc_skeleton.h"

namespace OHOS {
namespace UDMF {
class SameProcessIpcGuard {
public:
    SameProcessIpcGuard()
    {
        identity = IPCSkeleton::ResetCallingIdentity();
    }

    ~SameProcessIpcGuard()
    {
        IPCSkeleton::SetCallingIdentity(identity);
    }

private:
    std::string identity;
};
} // namespace UDMF
} // namespace OHOS

#endif // UDMF_SAMEPROCESSIPCGUARD_H