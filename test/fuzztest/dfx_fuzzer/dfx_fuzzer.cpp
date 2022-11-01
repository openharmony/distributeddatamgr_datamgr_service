/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "dfx_fuzzer.h"

#include <any>
#include <map>
#include <string>
#include <unistd.h>

#include "reporter.h"
#include "value_hash.h"

using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedDataDfx;
namespace OHOS {
void FaultReporterFuzz(FaultReporter *faultReporter, const std::string &strBase, const FaultMsg &faultMsg,
    const CommFaultMsg &commFaultMsg, const DBFaultMsg &dbFaultMsg)
{
    faultReporter->Report(faultMsg);
    faultReporter->Report(commFaultMsg);
    faultReporter->Report(dbFaultMsg);
}

void DatabaseStatisticFuzz(const std::string &strBase, int intBase)
{
    auto databaseStatistic = Reporter::GetInstance()->DatabaseStatistic();
    DbStat ds = {
        .userId = strBase,
        .appId = strBase,
        .storeId = strBase,
        .dbSize = intBase
    };
    databaseStatistic->Report(ds);
}

void VisitStatisticFuzz(const std::string &strBase)
{
    auto visitStatistic = Reporter::GetInstance()->VisitStatistic();
    struct VisitStat visitStat = { strBase, strBase };
    visitStatistic->Report(visitStat);
    std::string myuid = strBase;
    std::string result;
    ValueHash valueHash;
    valueHash.CalcValueHash(myuid, result);
}

void TrafficStatisticFuzz(const std::string &strBase, int intBase)
{
    auto trafficStatistic = Reporter::GetInstance()->TrafficStatistic();
    struct TrafficStat trafficStat = { strBase, strBase, intBase, intBase };
    trafficStatistic->Report(trafficStat);
}

void ApiPerformanceStatisticFuzz(const std::string &strBase, uint64_t uint64Base)
{
    auto apiPerformanceStatistic = Reporter::GetInstance()->ApiPerformanceStatistic();
    struct ApiPerformanceStat apiPerformanceStat = { strBase, uint64Base, uint64Base, uint64Base };
    apiPerformanceStatistic->Report(apiPerformanceStat);
}

void BehaviourReporterFuzz(const std::string &strBase)
{
    auto behaviourReporter = Reporter::GetInstance()->BehaviourReporter();
    struct BehaviourMsg behaviourMsg {
        .userId = strBase,
        .appId = strBase,
        .storeId = strBase,
        .extensionInfo = strBase,
        .behaviourType = BehaviourType::DATABASE_BACKUP
    };
    behaviourReporter->Report(behaviourMsg);
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    std::string fuzzStr(data, data + size);
    int fuzzInt = static_cast<int>(size);
    int32_t fuzzInt32 = static_cast<int32_t>(size);
    uint64_t fuzzUInt64 = static_cast<uint64_t>(size);

    struct FaultMsg faultMsg {
        .faultType = FaultType::SERVICE_FAULT,
        .moduleName = fuzzStr,
        .interfaceName = fuzzStr,
        .errorType = Fault::RF_CLOSE_DB
    };
    struct CommFaultMsg commFaultMsg {
        .userId = fuzzStr,
        .appId = fuzzStr,
        .storeId = fuzzStr,
        .errorCode = { fuzzInt32 }
    };
    struct DBFaultMsg dbFaultMsg {
        .appId = fuzzStr,
        .storeId = fuzzStr,
        .moduleName = fuzzStr,
        .errorType = Fault::DF_DB_DAMAGE
    };

    auto srvFault = Reporter::GetInstance()->ServiceFault();
    OHOS::FaultReporterFuzz(srvFault, fuzzStr, faultMsg, commFaultMsg, dbFaultMsg);

    auto rtFault = Reporter::GetInstance()->RuntimeFault();
    OHOS::FaultReporterFuzz(rtFault, fuzzStr, faultMsg, commFaultMsg, dbFaultMsg);

    auto dbFault = Reporter::GetInstance()->DatabaseFault();
    OHOS::FaultReporterFuzz(dbFault, fuzzStr, faultMsg, commFaultMsg, dbFaultMsg);

    auto comFault = Reporter::GetInstance()->CommunicationFault();
    OHOS::FaultReporterFuzz(comFault, fuzzStr, faultMsg, commFaultMsg, dbFaultMsg);

    OHOS::DatabaseStatisticFuzz(fuzzStr, fuzzInt);
    OHOS::TrafficStatisticFuzz(fuzzStr, fuzzInt);
    OHOS::VisitStatisticFuzz(fuzzStr);
    OHOS::ApiPerformanceStatisticFuzz(fuzzStr, fuzzUInt64);
    OHOS::BehaviourReporterFuzz(fuzzStr);

    return 0;
}
