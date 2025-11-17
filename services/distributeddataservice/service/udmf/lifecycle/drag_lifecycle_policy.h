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
#ifndef UDMF_DRAG_LIFECYCLE_POLICY_H
#define UDMF_DRAG_LIFECYCLE_POLICY_H
#include "lifecycle_policy.h"

namespace OHOS {
namespace UDMF {
class DragLifeCyclePolicy : public LifeCyclePolicy {
public:
    Status OnTimeout(const std::string &intention) override;
private:
    Status OnGotToRemote(std::shared_ptr<Store> store) override;
    Status GetTimeoutRuntime(const std::shared_ptr<Store> &store,
        std::unordered_map<std::string, Runtime> &timeoutRuntimes);
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_DRAG_LIFECYCLE_POLICY_H
