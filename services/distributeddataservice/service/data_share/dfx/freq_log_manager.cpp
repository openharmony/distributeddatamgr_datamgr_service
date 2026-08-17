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
#define LOG_TAG "FreqLogManager"

#include "freq_log_manager.h"

#include <algorithm>
#include "hiview_fault_adapter.h"
#include "idata_share_service.h"
#include "log_print.h"
#include "utils.h"

namespace OHOS {
namespace DataShare {

FreqLogManager &FreqLogManager::GetInstance()
{
    static FreqLogManager instance;
    return instance;
}

void FreqLogManager::SetThreadPool(std::shared_ptr<ExecutorPool> executors)
{
    executors_ = executors;
    StartTimer();
}

void FreqLogManager::StartTimer()
{
    if (executors_ == nullptr) {
        return;
    }
    if (running_.exchange(true)) {
        return;
    }
    auto interval = std::chrono::seconds(REPORT_INTERVAL);
    auto fun = [this]() { PrintAndReset(); };
    executors_->Schedule(fun, interval);
}

void FreqLogManager::ReportCall(uint32_t code, uint64_t tokenId, uint64_t uid, uint64_t pid,
    uint64_t costMs, const std::string &uri)
{
    callers_.Compute(tokenId, [code, uid, pid, costMs, &uri](const uint64_t &key, CallerStats &stats) {
        if (stats.uid == 0) {
            stats.uid = uid;
            stats.pid = pid;
            stats.callerBundleName = HiViewFaultAdapter::GetCallingName(static_cast<uint32_t>(key)).first;
        }
        auto &codeStat = stats.codeStats[code];
        codeStat.count++;
        codeStat.totalCostMs += costMs;
        if (costMs > codeStat.maxCostMs) {
            codeStat.maxCostMs = costMs;
        }
        UriInfo uriInfo;
        if (URIUtils::GetInfoFromURI(uri, uriInfo)) {
            stats.targetBundleNames.insert(uriInfo.bundleName);
        }
        return true;
    });
}

void FreqLogManager::PrintAndReset()
{
    auto codeGroups = CollectAndSort();
    for (auto &codeGroup : codeGroups) {
        PrintCodeGroup(codeGroup.first, codeGroup.second);
    }
    callers_.Clear();
}

std::map<uint32_t, std::vector<FreqLogManager::CallerStats>> FreqLogManager::CollectAndSort()
{
    std::map<uint32_t, std::vector<CallerStats>> codeGroups;
    callers_.ForEach([&codeGroups](const uint64_t &key, CallerStats &stats) {
        for (const auto &codeEntry : stats.codeStats) {
            codeGroups[codeEntry.first].push_back(stats);
        }
        return false;
    });
    for (auto &codeGroup : codeGroups) {
        uint32_t code = codeGroup.first;
        auto &callers = codeGroup.second;
        std::sort(callers.begin(), callers.end(),
            [code](const CallerStats &a, const CallerStats &b) {
                auto itA = a.codeStats.find(code);
                auto itB = b.codeStats.find(code);
                uint64_t countA = (itA != a.codeStats.end()) ? itA->second.count : 0;
                uint64_t countB = (itB != b.codeStats.end()) ? itB->second.count : 0;
                return countA > countB;
            });
    }
    return codeGroups;
}

void FreqLogManager::PrintCodeGroup(uint32_t code, std::vector<CallerStats> &callers)
{
    uint64_t totalCalls = 0;
    for (const auto &c : callers) {
        auto it = c.codeStats.find(code);
        if (it != c.codeStats.end()) {
            totalCalls += it->second.count;
        }
    }
    const char *codeName = (code == IDataShareService::DATA_SHARE_SERVICE_CMD_QUERY) ?
        CODE_NAME_QUERY : CODE_NAME_SILENT_PROXY;
    ZLOGI("FreqStats[%{public}llds] code:%{public}u(%{public}s) totalCalls:%{public}" PRIu64
          ", callers:%{public}zu",
          static_cast<long long>(REPORT_INTERVAL.count()), code, codeName, totalCalls, callers.size());
    for (size_t i = 0; i < callers.size(); i++) {
        const auto &c = callers[i];
        auto it = c.codeStats.find(code);
        uint64_t count = (it != c.codeStats.end()) ? it->second.count : 0;
        uint64_t avgCost = (count > 0) ? (it->second.totalCostMs / count) : 0;
        uint64_t maxCost = (it != c.codeStats.end()) ? it->second.maxCostMs : 0;
        ZLOGI("  caller[%{public}zu]:%{public}s(pid=%{public}" PRIu64 "), calls=%{public}" PRIu64
              ", avgCost=%{public}" PRIu64 "ms, maxCost=%{public}" PRIu64 "ms, targets=[%{public}s]",
              i, c.callerBundleName.c_str(), c.pid, count, avgCost, maxCost,
              FormatTargets(c.targetBundleNames).c_str());
    }
}

std::string FreqLogManager::FormatTargets(const std::set<std::string> &targets)
{
    std::string result;
    uint32_t count = 0;
    for (const auto &t : targets) {
        if (count >= MAX_TARGET_PRINT) {
            result += ",...";
            break;
        }
        if (count > 0) {
            result += ", ";
        }
        result += t;
        count++;
    }
    return result;
}
} // namespace DataShare
} // namespace OHOS