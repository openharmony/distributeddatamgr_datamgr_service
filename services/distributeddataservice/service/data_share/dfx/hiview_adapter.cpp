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
#define LOG_TAG "HiViewAdapter"

#include "hiview_adapter.h"

#include <algorithm>
#include "hiview_fault_adapter.h"
#include "log_print.h"

namespace OHOS {
namespace DataShare {
namespace {
constexpr char DOMAIN[] = "DISTDATAMGR";
constexpr const char *EVENT_NAME = "DATA_SHARE_STATISTIC";
constexpr const uint32_t MAX_COLLECT_COUNT = 20;
constexpr const uint32_t MIN_CALL_COUNT = 1000;
constexpr const size_t PARAMS_SIZE = 7;
}

using namespace std::chrono;

HiViewAdapter& HiViewAdapter::GetInstance()
{
    static HiViewAdapter instance;
    return instance;
}

void HiViewAdapter::ReportDataStatistic(const CallerInfo &callerInfo)
{
    if (executors_ == nullptr) {
        ZLOGE("executors is nullptr, collect failed");
        return;
    }

    callers_.Compute(callerInfo.callerTokenId, [&callerInfo, this](const uint64_t &key, CallerTotalInfo &value) {
        if (value.totalCount == 0) {
            value.callerUid = callerInfo.callerUid;
            std::string callingName = HiViewFaultAdapter::GetCallingName(key).first;
            value.callerName = callingName.empty() ? std::to_string(callerInfo.callerPid) : callingName;
        }
        value.totalCount += 1;
        value.maxCostTime = std::max(callerInfo.costTime, value.maxCostTime);
        value.totalCostTime += callerInfo.costTime;
        value.slowRequestCount = value.slowRequestCount + (callerInfo.isSlowRequest ? 1 : 0);
        auto it = funcIdMap_.find(callerInfo.funcId);
        if (it != funcIdMap_.end()) {
            auto funcName = it->second;
            value.funcCounts[funcName] = value.funcCounts[funcName] + 1;
        }
        return true;
    });
}

void HiViewAdapter::InvokeData()
{
    uint32_t mapSize = callers_.Size();
    if (mapSize == 0) { return; }
    std::vector<CallerTotalInfo> callerTotalInfos;
    GetCallerTotalInfoVec(callerTotalInfos);
    uint32_t count = std::min(static_cast<uint32_t>(callerTotalInfos.size()), MAX_COLLECT_COUNT);
    if (count == 0) { return; }
    int64_t callerUids[count];
    char* callerNames[count];
    int64_t totalCounts[count];
    int64_t slowRequestCounts[count];
    int64_t maxCostTimes[count];
    int64_t totalCostTimes[count];
    char* funcCounts[count];
    for (uint32_t i = 0; i < count; i++) {
        CallerTotalInfo &callerTotalInfo = callerTotalInfos[i];
        callerUids[i] = callerTotalInfo.callerUid;
        callerNames[i] = const_cast<char *>(callerTotalInfo.callerName.c_str());
        totalCounts[i] = callerTotalInfo.totalCount;
        slowRequestCounts[i] = callerTotalInfo.slowRequestCount;
        maxCostTimes[i] = callerTotalInfo.maxCostTime;
        totalCostTimes[i] = callerTotalInfo.totalCostTime;
        std::string funcCount = "{";
        for (const auto &it : callerTotalInfo.funcCounts) {
            funcCount += it.first + ":" + std::to_string(it.second) + ",";
        }
        funcCount = funcCount.substr(0, funcCount.size() - 1) + "}";
        funcCounts[i] = const_cast<char *>(funcCount.c_str());
    }

    HiSysEventParam callerUid = { .name = "COLLIE_UID", .t = HISYSEVENT_INT64_ARRAY,
        .v = { .array = callerUids }, .arraySize = count };
    HiSysEventParam callerName = { .name = "CALLER_NAME", .t = HISYSEVENT_STRING_ARRAY,
        .v = { .array = callerNames }, .arraySize = count };
    HiSysEventParam totalCount = { .name = "TOTAL_COUNT", .t = HISYSEVENT_INT64_ARRAY,
        .v = { .array = totalCounts }, .arraySize = count };
    HiSysEventParam slowRequestCount = { .name = "SLOW_RQUEST_COUNT", .t = HISYSEVENT_INT64_ARRAY,
        .v = { .array = slowRequestCounts }, .arraySize = count };
    HiSysEventParam maxCostTime = { .name = "MAX_COST_TIME", .t = HISYSEVENT_INT64_ARRAY,
        .v = { .array = maxCostTimes }, .arraySize = count };
    HiSysEventParam totalCostTime = { .name = "TOTAL_COST_TIME", .t = HISYSEVENT_INT64_ARRAY,
        .v = { .array = totalCostTimes }, .arraySize = count };
    HiSysEventParam funcCount = { .name = "FUNC_COUNTS", .t = HISYSEVENT_STRING_ARRAY,
        .v = { .array = funcCounts }, .arraySize = count };
    HiSysEventParam params[] = {callerUid, callerName, totalCount, slowRequestCount,
        maxCostTime, totalCostTime, funcCount};
    int res = OH_HiSysEvent_Write(DOMAIN, EVENT_NAME, HISYSEVENT_STATISTIC, params, PARAMS_SIZE);
    ZLOGI("OH_HiSysEvent_Write, res = %{public}d", res);
}

void HiViewAdapter::GetCallerTotalInfoVec(std::vector<CallerTotalInfo> &callerTotalInfos)
{
    callers_.EraseIf([&callerTotalInfos](const uint64_t key, CallerTotalInfo &callerTotalInfo) {
        if (callerTotalInfo.totalCount >= MIN_CALL_COUNT)
            callerTotalInfos.push_back(callerTotalInfo);
        return true;
    });
    std::sort(callerTotalInfos.begin(), callerTotalInfos.end(),
        [](const CallerTotalInfo &first, const CallerTotalInfo &second) {
        return first.totalCount > second.totalCount;
    });
}
} // namespace DataShare
} // namespace OHOS