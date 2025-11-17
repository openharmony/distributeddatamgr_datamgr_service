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
#ifndef UDMF_LIFECYCLE_MANAGER_H
#define UDMF_LIFECYCLE_MANAGER_H

#include "lifecycle_policy.h"

#include <unordered_set>
#include "concurrent_map.h"

namespace OHOS {
namespace UDMF {
class LifeCycleManager {
public:
    static LifeCycleManager &GetInstance();
    Status OnGot(UnifiedKey &key, const uint32_t tokenId, bool isNeedPush = true);
    Status OnStart();
    Status StartLifeCycleTimer();
    void SetThreadPool(std::shared_ptr<ExecutorPool> executors);
    bool InsertUdKey(uint32_t tokenId, const std::string &udKey);
    void EraseUdkey(const std::string &udKey, uint32_t tokenId);
    Status OnAppUninstall(uint32_t tokenId);

private:
    ExecutorPool::Task GetTask();
    static std::unordered_map<std::string, std::shared_ptr<LifeCyclePolicy>> intentionPolicy_;
    static constexpr const char *DATA_PREFIX = "udmf://";
    std::shared_ptr<ExecutorPool> executors_;
    ConcurrentMap<uint32_t, std::unordered_set<std::string>> udKeys_;

    void ClearUdKeys();
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_LIFECYCLE_MANAGER_H
