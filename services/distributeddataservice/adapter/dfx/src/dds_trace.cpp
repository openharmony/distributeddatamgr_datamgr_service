/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#define LOG_TAG "DdsTrace"

#include "dds_trace.h"
#include <atomic>
#include <cinttypes>
#include "hitrace_meter.h"
#include "log_print.h"
#include "time_utils.h"
#include "reporter.h"

namespace OHOS {
namespace DistributedKv {
static constexpr uint64_t BYTRACE_LABEL = HITRACE_TAG_DISTRIBUTEDDATA;
using OHOS::HiviewDFX::HiTrace;

std::atomic_uint DdsTrace::indexCount = 0; // the value is changed by different thread
std::atomic_bool DdsTrace::isSetBytraceEnabled = false;

DdsTrace::DdsTrace(const std::string& value, unsigned int option)
{
    traceValue = value;
    traceCount = ++indexCount;
    SetBytraceEnable();
    SetOptionSwitch(option);
    Start(value);
}

DdsTrace::~DdsTrace()
{
    Finish(traceValue);
}

void DdsTrace::SetMiddleTrace(const std::string& beforeValue, const std::string& afterValue)
{
    traceValue = afterValue;
    Middle(beforeValue, afterValue);
}

void DdsTrace::Start(const std::string& value)
{
    if (switchOption == DEBUG_CLOSE) {
        return;
    }
    if ((switchOption & BYTRACE_ON) == BYTRACE_ON) {
        StartTrace(BYTRACE_LABEL, value);
    }
    if ((switchOption & TRACE_CHAIN_ON) == TRACE_CHAIN_ON) {
        traceId = HiTrace::Begin(value, HITRACE_FLAG_DEFAULT);
    }
    if ((switchOption & API_PERFORMANCE_TRACE_ON) == API_PERFORMANCE_TRACE_ON) {
        lastTime = TimeUtils::CurrentTimeMicros();
    }
    ZLOGD("DdsTrace-Start: Trace[%{public}u] %{public}s In", traceCount, value.c_str());
}

void DdsTrace::Middle(const std::string& beforeValue, const std::string& afterValue)
{
    if (switchOption == DEBUG_CLOSE) {
        return;
    }
    if (switchOption & BYTRACE_ON) {
        MiddleTrace(BYTRACE_LABEL, beforeValue, afterValue);
    }
    ZLOGD("DdsTrace-Middle: Trace[%{public}u] %{public}s --- %{public}s", traceCount,
        beforeValue.c_str(), afterValue.c_str());
}

void DdsTrace::Finish(const std::string& value)
{
    uint64_t delta = 0;
    if (switchOption == DEBUG_CLOSE) {
        return;
    }
    if (switchOption & BYTRACE_ON) {
        FinishTrace(BYTRACE_LABEL);
    }
    if ((switchOption & TRACE_CHAIN_ON) == TRACE_CHAIN_ON) {
        HiTrace::End(traceId);
    }
    if (switchOption & API_PERFORMANCE_TRACE_ON) {
        delta = TimeUtils::CurrentTimeMicros() - lastTime;
        Reporter::GetInstance()->ApiPerformanceStatistic()->Report({value, delta, delta, delta});
    }
    ZLOGD("DdsTrace-Finish: Trace[%u] %{public}s Out: %{public}" PRIu64"us.", traceCount, value.c_str(), delta);
}

bool DdsTrace::SetBytraceEnable()
{
    if (isSetBytraceEnabled) {
        return true;
    }

    UpdateTraceLabel();
    isSetBytraceEnabled = true;

    ZLOGD("success, current tag is true");
    return true;
}

void DdsTrace::SetOptionSwitch(unsigned int option)
{
    if ((option & BYTRACE_ON) == BYTRACE_ON) {
        switchOption |= BYTRACE_ON;
    }
    if ((option & TRACE_CHAIN_ON) == TRACE_CHAIN_ON) {
        switchOption |= TRACE_CHAIN_ON;
    }
    if ((option & API_PERFORMANCE_TRACE_ON) == API_PERFORMANCE_TRACE_ON) {
        switchOption |= API_PERFORMANCE_TRACE_ON;
    }
}
} // namespace DistributedKv
} // namespace OHOS
