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
#define LOG_TAG "RdbHiviewAdapter"
 
#include "rdb_hiview_adapter.h"
#include "log_print.h"
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <sstream>
  
namespace OHOS::DistributedRdb {
static constexpr const char *FAULT_EVENT = "DISTRIBUTED_DATA_RDB_FAULT";
static constexpr const char *DISTRIBUTED_DATAMGR = "DISTDATAMGR";
static constexpr const char *STATISTIC_EVENT = "RDB_STATISTIC";
constexpr int32_t WAIT_TIME = 60 * 60 * 24; // 24h
  
RdbHiViewAdapter& RdbHiViewAdapter::GetInstance()
{
    static RdbHiViewAdapter instance;
    return instance;
}
  
void RdbHiViewAdapter::ReportStatistic(const RdbStatEvent &stat)
{
    if (executors_ == nullptr) {
        return;
    }
    statEvents_.Compute(stat, [&stat](const RdbStatEvent &key, uint32_t &value) {
        value++;
        return true;
    });
}
  
void RdbHiViewAdapter::InvokeSync()
{
    auto statEvents = std::move(statEvents_);
    uint32_t count = statEvents.Size();
    if (count == 0) {
        return;
    }

    uint32_t statTypes[count];
    const char* bundleNames[count];
    const char* storeNames[count];
    uint32_t subTypes[count];
    uint32_t costTimes[count];
    uint32_t occurTimes[count];
    uint32_t index = 0;

    statEvents.ForEach([&statTypes, &bundleNames, &storeNames, &subTypes, &costTimes, &occurTimes, &index](
        const RdbStatEvent &key, uint32_t &value) {
        statTypes[index] = key.statType;
        bundleNames[index] = key.bundleName.c_str();
        storeNames[index] = key.storeName.c_str();
        subTypes[index] = key.subType;
        costTimes[index] = key.costTime;
        occurTimes[index] = value;
        index++;
        return false;
    });

    HiSysEventParam params[] = {
        { .name = "TYPE", .t = HISYSEVENT_UINT32_ARRAY, .v = { .array = statTypes }, .arraySize = count },
        { .name = "BUNDLE_NAME", .t = HISYSEVENT_STRING_ARRAY, .v = { .array = bundleNames }, .arraySize = count },
        { .name = "STORE_NAME", .t = HISYSEVENT_STRING_ARRAY, .v = { .array = storeNames }, .arraySize = count },
        { .name = "PARAM_TYPE1", .t = HISYSEVENT_UINT32_ARRAY, .v = { .array = subTypes }, .arraySize = count },
        { .name = "PARAM_TYPE2", .t = HISYSEVENT_UINT32_ARRAY, .v = { .array = costTimes }, .arraySize = count },
        { .name = "TIMES", .t = HISYSEVENT_UINT32_ARRAY, .v = { .array = occurTimes }, .arraySize = count },
    };
    auto size = sizeof(params) / sizeof(params[0]);
    OH_HiSysEvent_Write(DISTRIBUTED_DATAMGR, STATISTIC_EVENT, HISYSEVENT_STATISTIC, params, size);
}
  
void RdbHiViewAdapter::StartTimerThread()
{
    if (executors_ == nullptr) {
        return;
    }
    if (running_.exchange(true)) {
        return;
    }
    auto interval = std::chrono::seconds(WAIT_TIME);
    auto fun = [this]() {
        InvokeSync();
    };
    executors_->Schedule(fun, interval);
}
  
void RdbHiViewAdapter::SetThreadPool(std::shared_ptr<ExecutorPool> executors)
{
    executors_ = executors;
    StartTimerThread();
}

void RdbHiViewAdapter::ReportRdbFault(const RdbFaultEvent &rdbFaultEvent)
{
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::chrono::nanoseconds nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch());
    std::stringstream oss;
    char buffer[MAX_TIME_BUF_LEN] = {0};
    std::tm local_time;
    localtime_r(&time, &local_time);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_time);
    oss << buffer << '.' << std::setfill('0') << std::setw(MILLISECONDS_LEN)
        << (nsec.count() / NANO_TO_MILLI) % MILLI_PRE_SEC;
    std::string occurTime = oss.str();
    std::string bundleName = rdbFaultEvent.bundleName;
    std::string faultType = rdbFaultEvent.faultType;
    std::string appendInfo = rdbFaultEvent.custLog;
    if (bundleName.empty()) {
        return;
    }
    HiSysEventParam params[] = {
        {.name = "FAULT_TIME", .t = HISYSEVENT_STRING, .v = {.s = occurTime.data()}, .arraySize = 0},
        {.name = "FAULT_TYPE", .t = HISYSEVENT_STRING, .v = {.s = faultType.data()}, .arraySize = 0},
        {.name = "BUNDLE_NAME", .t = HISYSEVENT_STRING, .v = {.s = bundleName.data()}, .arraySize = 0},
        {.name = "ERROR_CODE",
            .t = HISYSEVENT_INT32,
            .v = {.ui32 = static_cast<uint32_t>(rdbFaultEvent.errorCode)},
            .arraySize = 0},
        {.name = "APPENDIX", .t = HISYSEVENT_STRING, .v = {.s = appendInfo.data()}, .arraySize = 0},
    };
    auto size = sizeof(params) / sizeof(params[0]);
    OH_HiSysEvent_Write(DISTRIBUTED_DATAMGR, FAULT_EVENT, HISYSEVENT_FAULT, params, size);
}
} // namespace OHOS::DistributedRdb