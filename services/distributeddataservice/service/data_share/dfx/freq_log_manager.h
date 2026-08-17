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

#ifndef DATASHARESERVICE_FREQ_LOG_MANAGER_H
#define DATASHARESERVICE_FREQ_LOG_MANAGER_H

#include <atomic>
#include <chrono>
#include <cinttypes>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "concurrent_map.h"
#include "executor_pool.h"

namespace OHOS {
namespace DataShare {

class FreqLogManager {
public:
    static FreqLogManager &GetInstance();
    void SetThreadPool(std::shared_ptr<ExecutorPool> executors);
    void ReportCall(uint32_t code, uint64_t tokenId, uint64_t uid, uint64_t pid,
        uint64_t costMs, const std::string &uri);

private:
    struct CodeStats {
        uint64_t count = 0;
        uint64_t totalCostMs = 0;
        uint64_t maxCostMs = 0;
    };

    struct CallerStats {
        uint64_t uid = 0;
        uint64_t pid = 0;
        std::string callerBundleName;
        std::map<uint32_t, CodeStats> codeStats;
        std::set<std::string> targetBundleNames;
    };

    void StartTimer();
    void PrintAndReset();
    std::map<uint32_t, std::vector<CallerStats>> CollectAndSort();
    void PrintCodeGroup(uint32_t code, std::vector<CallerStats> &callers);
    std::string FormatTargets(const std::set<std::string> &targets);

    static constexpr std::chrono::seconds REPORT_INTERVAL{60};
    static constexpr uint32_t MAX_TARGET_PRINT = 10;
    static constexpr const char *CODE_NAME_QUERY = "Query";
    static constexpr const char *CODE_NAME_SILENT_PROXY = "GetSilentProxyStatus";

    ConcurrentMap<uint64_t, CallerStats> callers_;
    std::shared_ptr<ExecutorPool> executors_;
    std::atomic<bool> running_{false};
};

} // namespace DataShare
} // namespace OHOS
#endif // DATASHARESERVICE_FREQ_LOG_MANAGER_H