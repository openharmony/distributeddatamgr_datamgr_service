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

#ifndef DISTRIBUTEDDATAMGR_REPORTER_IMPL_H
#define DISTRIBUTEDDATAMGR_REPORTER_IMPL_H

#include <memory>
#include <mutex>

#include "dfx/behaviour_reporter.h"
#include "dfx/dfx_types.h"
#include "executor_pool.h"
#include "dfx/fault_reporter.h"
#include "dfx/reporter.h"
#include "dfx/statistic_reporter.h"

namespace OHOS {
namespace DistributedDataDfx {
class ReporterImpl : public Reporter {
public:
    static bool Init();
    FaultReporter* ServiceFault() override;
    FaultReporter* RuntimeFault() override;
    FaultReporter* DatabaseFault() override;
    FaultReporter* CommunicationFault() override;
    FaultReporter* CloudSyncFault() override;

    StatisticReporter<DbStat>* DatabaseStatistic() override;
    StatisticReporter<VisitStat>* VisitStatistic() override;
    StatisticReporter<TrafficStat>* TrafficStatistic() override;
    StatisticReporter<ApiPerformanceStat>* ApiPerformanceStatistic() override;

    BehaviourReporter* GetBehaviourReporter() override;
    void SetThreadPool(std::shared_ptr<ExecutorPool> executors) override;

private:
    ~ReporterImpl() override = default;
    std::shared_ptr<ExecutorPool> executors_;
};
} // namespace DistributedDataDfx
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_REPORTER_IMPL_H
