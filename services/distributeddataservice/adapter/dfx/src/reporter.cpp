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

#include "reporter.h"
#include "fault/communication_fault_impl.h"
#include "fault/database_fault_impl.h"
#include "fault/runtime_fault_impl.h"
#include "fault/service_fault_impl.h"

#include "statistic/traffic_statistic_impl.h"
#include "statistic/visit_statistic_impl.h"
#include "statistic/database_statistic_impl.h"
#include "statistic/api_performance_statistic_impl.h"

#include "behaviour/behaviour_reporter_impl.h"

namespace OHOS {
namespace DistributedDataDfx {
Reporter* Reporter::GetInstance()
{
    static Reporter reporter;
    return &reporter;
}

FaultReporter* Reporter::CommunicationFault()
{
    static CommunicationFaultImpl communicationFault;
    communicationFault.SetThreadPool(executors_);
    return &communicationFault;
}

FaultReporter* Reporter::DatabaseFault()
{
    static DatabaseFaultImpl databaseFault;
    databaseFault.SetThreadPool(executors_);
    return &databaseFault;
}

FaultReporter* Reporter::RuntimeFault()
{
    static RuntimeFaultImpl runtimeFault;
    runtimeFault.SetThreadPool(executors_);
    return &runtimeFault;
}

FaultReporter* Reporter::ServiceFault()
{
    static ServiceFaultImpl serviceFault;
    serviceFault.SetThreadPool(executors_);
    return &serviceFault;
}

StatisticReporter<TrafficStat>* Reporter::TrafficStatistic()
{
    static TrafficStatisticImpl trafficStatistic;
    trafficStatistic.SetThreadPool(executors_);
    return &trafficStatistic;
}

StatisticReporter<struct VisitStat>* Reporter::VisitStatistic()
{
    static VisitStatisticImpl visitStatistic;
    visitStatistic.SetThreadPool(executors_);
    return &visitStatistic;
}

StatisticReporter<struct DbStat>* Reporter::DatabaseStatistic()
{
    static DatabaseStatisticImpl databaseStatistic;
    databaseStatistic.SetThreadPool(executors_);
    return &databaseStatistic;
}

StatisticReporter<ApiPerformanceStat>* Reporter::ApiPerformanceStatistic()
{
    static ApiPerformanceStatisticImpl apiPerformanceStat;
    apiPerformanceStat.SetThreadPool(executors_);
    return &apiPerformanceStat;
}

BehaviourReporter* Reporter::BehaviourReporter()
{
    static BehaviourReporterImpl behaviourReporterImpl;
    behaviourReporterImpl.SetThreadPool(executors_);
    return &behaviourReporterImpl;
}
} // namespace DistributedDataDfx
} // namespace OHOS
