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
#include "kv_store_thread_pool.h"
#include "kv_store_task.h"
#include "value_hash.h"

namespace OHOS {
namespace DistributedKv {
template<typename T>
struct StatisticWrap {
    T val;
    int times;
    int code;
};

namespace {
// fault key
constexpr const char *FAULT_TYPE = "FAULT_TYPE";
constexpr const char *MODULE_NAME = "MODULE_NAME";
constexpr const char *INTERFACE_NAME = "INTERFACE_NAME";
constexpr const char *ERROR_TYPE = "ERROR_TYPE";
constexpr const char *SYNC_ERROR_INFO = "SYNC_ERROR_INFO";
// Database statistic
constexpr const char *USER_ID = "ANONYMOUS_UID";
constexpr const char *APP_ID = "APP_ID";
constexpr const char *STORE_ID = "STORE_ID";
constexpr const char *DB_SIZE = "DB_SIZE";
// interface visit statistic
constexpr const char *TIMES = "TIMES";
constexpr const char *DEVICE_ID = "ANONYMOUS_DID";
constexpr const char *SEND_SIZE = "SEND_SIZE";
constexpr const char *RECEIVED_SIZE = "RECEIVED_SIZE";
constexpr const char *COMPLETE_TIME = "COMPLETE_TIME";
constexpr const char *SIZE = "SIZE";
constexpr const char *SRC_DEVICE_ID = "ANONYMOUS_SRC_DID";
constexpr const char *DST_DEVICE_ID = "ANONYMOUS_DST_DID";
constexpr const char *AVERAGE_TIMES = "AVERAGE_TIME";
constexpr const char *WORST_TIMES = "WORST_TIME";
constexpr const char *INTERFACES = "INTERFACES";
constexpr const char *TAG = "TAG";
constexpr const char *POWERSTATS = "PowerStats";
// security key
constexpr const char *SECURITY_INFO = "SECURITY_INFO";
constexpr const char *DEVICE_SENSITIVE_LEVEL = "DEVICE_SENSITIVE_LEVEL";
constexpr const char *OPTION_SENSITIVE_LEVEL = "OPTION_SENSITIVE_LEVEL";
// behaviour key
constexpr const char *BEHAVIOUR_INFO = "BEHAVIOUR_INFO";

const std::map<int, std::string> EVENT_COVERT_TABLE = {
    {DfxCodeConstant::SERVICE_FAULT, "SERVICE_FAULT"},
    {DfxCodeConstant::RUNTIME_FAULT, "RUNTIME_FAULT"},
    {DfxCodeConstant::DATABASE_FAULT, "DATABASE_FAULT"},
    {DfxCodeConstant::COMMUNICATION_FAULT, "DATABASE_FAULT"},
    {DfxCodeConstant::DATABASE_STATISTIC, "COMMUNICATION_FAULT}"},
    {DfxCodeConstant::VISIT_STATISTIC, "VISIT_STATISTIC"},
    {DfxCodeConstant::TRAFFIC_STATISTIC, "VISIT_STATISTIC"},
    {DfxCodeConstant::DATABASE_PERFORMANCE_STATISTIC, "DATABASE_PERFORMANCE_STATISTIC"},
    {DfxCodeConstant::API_PERFORMANCE_STATISTIC, "API_PERFORMANCE_STATISTIC"},
    {DfxCodeConstant::API_PERFORMANCE_INTERFACE, "API_PERFORMANCE_STATISTIC"},
    {DfxCodeConstant::DATABASE_SYNC_FAILED, "DATABASE_SYNC_FAILED"},
    {DfxCodeConstant::DATABASE_RECOVERY_FAILED, "DATABASE_SYNC_FAILED"},
    {DfxCodeConstant::DATABASE_OPEN_FAILED, "DATABASE_RECOVERY_FAILED"},
    {DfxCodeConstant::DATABASE_REKEY_FAILED, "DATABASE_OPEN_FAILED"},
    {DfxCodeConstant::DATABASE_SECURITY, "DATABASE_REKEY_FAILED"},
    {DfxCodeConstant::DATABASE_BEHAVIOUR, "DATABASE_BEHAVIOUR"},
};
}

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
    static void ReportPermissionsSecurity(int dfxCode, const SecurityPermissionsMsg &msg);
    static void ReportSensitiveLevelSecurity(int dfxCode, const SecuritySensitiveLevelMsg &msg);
    static void ReportBehaviour(int dfxCode, const BehaviourMsg &msg);
    static void StartTimerThread();

private:
    static constexpr int POOL_SIZE = 3;
    static std::shared_ptr<KvStoreThreadPool> pool_;

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
    static const inline int EXEC_HOUR_TIME = 23;
    static const inline int EXEC_MIN_TIME = 60;
    static const inline int SIXTY_SEC = 60;

    static const inline int WAIT_TIME = 1 * 60 * 60; // 1 hours
    static const inline int PERIOD_TIME_US = 1 * 1000 * 1000; // 1 s
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_HI_VIEW_ADAPTER_H
