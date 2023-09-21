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
#ifndef UDMF_LIFECYCLE_POLICY_H
#define UDMF_LIFECYCLE_POLICY_H

#include <algorithm>
#include <chrono>
#include <memory>

#include "store_cache.h"
#include "unified_key.h"

namespace OHOS {
namespace UDMF {
class LifeCyclePolicy {
public:
    using Duration = std::chrono::steady_clock::duration;
    static constexpr Duration INTERVAL = std::chrono::milliseconds(60 * 60 * 1000);
    virtual ~LifeCyclePolicy() = default;
    virtual Status OnGot(const UnifiedKey &key);
    virtual Status OnStart(const std::string &intention);
    virtual Status OnTimeout(const std::string &intention);
    virtual Status GetTimeoutKeys(
        const std::shared_ptr<Store> &store, Duration interval, std::vector<std::string> &timeoutKeys);

private:
    static constexpr const char *DATA_PREFIX = "udmf://";
};
} // namespace UDMF
} // namespace OHOS

#endif // UDMF_LIFECYCLE_POLICY_H