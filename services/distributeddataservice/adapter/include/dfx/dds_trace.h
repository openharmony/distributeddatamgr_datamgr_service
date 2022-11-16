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

#ifndef DDS_TRACE_H
#define DDS_TRACE_H

#include <atomic>
#include <cinttypes>
#include <string>
#include <functional>
#include <chrono>
#include "hitrace/trace.h"
#include "hitrace_meter.h"
namespace OHOS {
namespace DistributedDataDfx {
enum TraceSwitch {
    DEBUG_CLOSE = 0x00,
    BYTRACE_ON = 0x01,
    API_PERFORMANCE_TRACE_ON = 0x02,
    TRACE_CHAIN_ON = 0x04,
};
class __attribute__((visibility("hidden"))) DdsTrace {
public:
    using Action = std::function<void(const std::string &value, uint64_t delta)>;
    using System = std::chrono::system_clock;
    DdsTrace(const std::string &value, unsigned int option = BYTRACE_ON, Action action = nullptr)
    {
        traceSwitch_ = option;
        traceValue_ = value;
        action_ = action;
        static std::atomic_bool enable = false;
        if (!enable.exchange(true)) {
            UpdateTraceLabel();
        }
        Start(value);
    }

    ~DdsTrace()
    {
        Finish(traceValue_);
    }

private:
    void Start(const std::string &value)
    {
        if ((traceSwitch_ & BYTRACE_ON) == BYTRACE_ON) {
            StartTrace(HITRACE_TAG_DISTRIBUTEDDATA, value);
        }
        if ((traceSwitch_ & TRACE_CHAIN_ON) == TRACE_CHAIN_ON) {
            traceId_ = OHOS::HiviewDFX::HiTraceChain::Begin(value, HITRACE_FLAG_DEFAULT);
        }
        if ((traceSwitch_ & API_PERFORMANCE_TRACE_ON) == API_PERFORMANCE_TRACE_ON) {
            lastTime_ = System::now();
        }
    }
    void Finish(const std::string &value)
    {
        if ((traceSwitch_ & BYTRACE_ON) == BYTRACE_ON) {
            FinishTrace(HITRACE_TAG_DISTRIBUTEDDATA);
        }
        if ((traceSwitch_ & TRACE_CHAIN_ON) == TRACE_CHAIN_ON) {
            OHOS::HiviewDFX::HiTraceChain::End(traceId_);
        }
        if ((traceSwitch_ & API_PERFORMANCE_TRACE_ON) == API_PERFORMANCE_TRACE_ON && action_) {
            action_(value, std::chrono::duration_cast<std::chrono::milliseconds>(System::now() - lastTime_).count());
        }
    }
    Action action_;
    std::string traceValue_{ };
    HiviewDFX::HiTraceId traceId_;
    uint32_t traceSwitch_{ 0 };
    System::time_point lastTime_;
};
} // namespace DistributedDataDfx
} // namespace OHOS
#endif
