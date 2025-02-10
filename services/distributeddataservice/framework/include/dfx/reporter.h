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

#ifndef DISTRIBUTEDDATAMGR_REPORTER_H
#define DISTRIBUTEDDATAMGR_REPORTER_H

#include "behaviour_reporter.h"
#include "dfx_types.h"
#include "executor_pool.h"
#include "fault_reporter.h"
#include "statistic_reporter.h"

namespace OHOS {
namespace DistributedDataDfx {
class Reporter {
public:
    API_EXPORT virtual ~Reporter() = default;
    API_EXPORT static bool RegisterReporterInstance(Reporter *reporter);
    API_EXPORT static Reporter* GetInstance();
    API_EXPORT virtual FaultReporter* ServiceFault() = 0;
    API_EXPORT virtual FaultReporter* RuntimeFault() = 0;
    API_EXPORT virtual FaultReporter* DatabaseFault() = 0;
    API_EXPORT virtual FaultReporter* CommunicationFault() = 0;
    API_EXPORT virtual FaultReporter* CloudSyncFault() = 0;
    API_EXPORT virtual StatisticReporter<DbStat>* DatabaseStatistic() = 0;
    API_EXPORT virtual StatisticReporter<VisitStat>* VisitStatistic() = 0;
    API_EXPORT virtual StatisticReporter<TrafficStat>* TrafficStatistic() = 0;
    API_EXPORT virtual StatisticReporter<ApiPerformanceStat>* ApiPerformanceStatistic() = 0;
    API_EXPORT virtual BehaviourReporter* GetBehaviourReporter() = 0;
    API_EXPORT virtual void SetThreadPool(std::shared_ptr<ExecutorPool> executors) = 0;

private:
    std::shared_ptr<ExecutorPool> executors_;
    static Reporter *instance_;
};
} // namespace DistributedDataDfx
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_REPORTER_H
