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

std::atomic_uint DdsTrace::indexCount_ = 0; // the value is changed by different thread
std::atomic_bool DdsTrace::isSetBytraceEnabled_ = false;

DdsTrace::DdsTrace(const std::string& value, unsigned int option)
    : traceSwitch_(option)
{
    traceValue_ = value;
    traceCount_ = ++indexCount_;
    SetBytraceEnable();
    Start(value);
}

DdsTrace::~DdsTrace()
{
    Finish(traceValue_);
}

void DdsTrace::SetMiddleTrace(const std::string& beforeValue, const std::string& afterValue)
{
    traceValue_ = afterValue;
    Middle(beforeValue, afterValue);
}

void DdsTrace::Start(const std::string& value)
{
    if (traceSwitch_ == DEBUG_CLOSE) {
        return;
    }
    if ((traceSwitch_ & BYTRACE_ON) == BYTRACE_ON) {
        StartTrace(BYTRACE_LABEL, value);
    }
    if ((traceSwitch_ & TRACE_CHAIN_ON) == TRACE_CHAIN_ON) {
        traceId_ = HiTrace::Begin(value, HITRACE_FLAG_DEFAULT);
    }
    if ((traceSwitch_ & API_PERFORMANCE_TRACE_ON) == API_PERFORMANCE_TRACE_ON) {
        lastTime_ = TimeUtils::CurrentTimeMicros();
    }
    ZLOGD("DdsTrace-Start: Trace[%{public}u] %{public}s In", traceCount_, value.c_str());
}

void DdsTrace::Middle(const std::string& beforeValue, const std::string& afterValue)
{
    if (traceSwitch_ == DEBUG_CLOSE) {
        return;
    }
    if ((traceSwitch_ & BYTRACE_ON) == BYTRACE_ON) {
        MiddleTrace(BYTRACE_LABEL, beforeValue, afterValue);
    }
    ZLOGD("DdsTrace-Middle: Trace[%{public}u] %{public}s --- %{public}s", traceCount_,
        beforeValue.c_str(), afterValue.c_str());
}

void DdsTrace::Finish(const std::string& value)
{
    uint64_t delta = 0;
    if (traceSwitch_ == DEBUG_CLOSE) {
        return;
    }
    if ((traceSwitch_ & BYTRACE_ON) == BYTRACE_ON) {
        FinishTrace(BYTRACE_LABEL);
    }
    if ((traceSwitch_ & TRACE_CHAIN_ON) == TRACE_CHAIN_ON) {
        HiTrace::End(traceId_);
    }
    if ((traceSwitch_ & API_PERFORMANCE_TRACE_ON) == API_PERFORMANCE_TRACE_ON) {
        delta = TimeUtils::CurrentTimeMicros() - lastTime_;
        Reporter::GetInstance()->ApiPerformanceStatistic()->Report({value, delta, delta, delta});
    }
    ZLOGD("DdsTrace-Finish: Trace[%u] %{public}s Out: %{public}" PRIu64"us.", traceCount_, value.c_str(), delta);
}

bool DdsTrace::SetBytraceEnable()
{
    if (isSetBytraceEnabled_.exchange(true)) {
        return true;
    }
    UpdateTraceLabel();
    ZLOGD("success, current tag is true");
    return true;
}
} // namespace DistributedKv
} // namespace OHOS
