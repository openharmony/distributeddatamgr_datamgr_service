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

#ifndef DISTRIBUTEDDATAMGR_HI_VIEW_ADAPTER_H
#define DISTRIBUTEDDATAMGR_HI_VIEW_ADAPTER_H

#include <map>
#include <mutex>
#include "dfx_types.h"
#include "dfx_code_constant.h"
#include "hisysevent.h"
#include "task_scheduler.h"
#include "kv_store_thread_pool.h"
#include "kv_store_task.h"
#include "value_hash.h"

namespace OHOS {
namespace DistributedDataDfx {
template<typename T>
struct StatisticWrap {
    T val;
    int times;
    int code;
};

class HiViewAdapter {
public:
    ~HiViewAdapter();
    static void ReportFault(int dfxCode, const FaultMsg &msg);
    static void ReportDBFault(int dfxCode, const DBFaultMsg &msg);
    static void ReportCommFault(int dfxCode, const CommFaultMsg &msg);
    static void ReportVisitStatistic(int dfxCode, const VisitStat &stat);
    static void ReportTrafficStatistic(int dfxCode, const TrafficStat &stat);
    static void ReportDatabaseStatistic(int dfxCode, const DbStat &stat);
    static void ReportApiPerformanceStatistic(int dfxCode, const ApiPerformanceStat &stat);
    static void ReportBehaviour(int dfxCode, const BehaviourMsg &msg);
    static void StartTimerThread();

private:
    static constexpr int POOL_SIZE = 3;
    static std::shared_ptr<DistributedKv::KvStoreThreadPool> pool_;

    static std::mutex visitMutex_;
    static std::map<std::string, StatisticWrap<VisitStat>> visitStat_;
    static void InvokeVisit();

    static std::mutex trafficMutex_;
    static std::map<std::string, StatisticWrap<TrafficStat>> trafficStat_;
    static void InvokeTraffic();

    static std::mutex dbMutex_;
    static std::map<std::string, StatisticWrap<DbStat>> dbStat_;
    static void InvokeDbSize();
    static void ReportDbSize(const StatisticWrap<DbStat> &stat);

    static std::mutex apiPerformanceMutex_;
    static std::map<std::string, StatisticWrap<ApiPerformanceStat>> apiPerformanceStat_;
    static void InvokeApiPerformance();

    static std::string CoverEventID(int dfxCode);
private:
    static std::mutex runMutex_;
    static bool running_;
    static TaskScheduler scheduler_;
    static const inline int DAILY_REPORT_TIME = 23;
    static const inline int WAIT_TIME = 1 * 60 * 60; // 1 hours
};
}  // namespace DistributedDataDfx
}  // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_HI_VIEW_ADAPTER_H
