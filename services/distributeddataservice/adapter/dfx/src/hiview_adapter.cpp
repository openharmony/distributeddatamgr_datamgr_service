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

#define LOG_TAG "HiViewAdapter"

#include "hiview_adapter.h"
#include <thread>
#include <unistd.h>
#include "log_print.h"

namespace OHOS {
namespace DistributedDataDfx {
using namespace DistributedKv;
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
constexpr const char *AVERAGE_TIMES = "AVERAGE_TIME";
constexpr const char *WORST_TIMES = "WORST_TIME";
constexpr const char *INTERFACES = "INTERFACES";
constexpr const char *TAG = "TAG";
constexpr const char *POWERSTATS = "PowerStats";
// behaviour key
constexpr const char *BEHAVIOUR_INFO = "BEHAVIOUR_INFO";

const std::map<int, std::string> EVENT_COVERT_TABLE = {
    { DfxCodeConstant::SERVICE_FAULT, "SERVICE_FAULT" },
    { DfxCodeConstant::RUNTIME_FAULT, "RUNTIME_FAULT" },
    { DfxCodeConstant::DATABASE_FAULT, "DATABASE_FAULT" },
    { DfxCodeConstant::COMMUNICATION_FAULT, "COMMUNICATION_FAULT" },
    { DfxCodeConstant::DATABASE_STATISTIC, "DATABASE_STATISTIC}" },
    { DfxCodeConstant::VISIT_STATISTIC, "VISIT_STATISTIC" },
    { DfxCodeConstant::TRAFFIC_STATISTIC, "VISIT_STATISTIC" },
    { DfxCodeConstant::DATABASE_PERFORMANCE_STATISTIC, "DATABASE_PERFORMANCE_STATISTIC" },
    { DfxCodeConstant::API_PERFORMANCE_STATISTIC, "API_PERFORMANCE_STATISTIC" },
    { DfxCodeConstant::API_PERFORMANCE_INTERFACE, "API_PERFORMANCE_STATISTIC" },
    { DfxCodeConstant::DATABASE_SYNC_FAILED, "DATABASE_SYNC_FAILED" },
    { DfxCodeConstant::DATABASE_CORRUPTED_FAILED, "DATABASE_CORRUPTED_FAILED" },
    { DfxCodeConstant::DATABASE_REKEY_FAILED, "DATABASE_REKEY_FAILED" },
    { DfxCodeConstant::DATABASE_BEHAVIOUR, "DATABASE_BEHAVIOUR" },
};
}
using OHOS::HiviewDFX::HiSysEvent;
std::shared_ptr<KvStoreThreadPool> HiViewAdapter::pool_ = KvStoreThreadPool::GetPool(POOL_SIZE, "HiView", true);

std::mutex HiViewAdapter::visitMutex_;
std::map<std::string, StatisticWrap<VisitStat>> HiViewAdapter::visitStat_;

std::mutex HiViewAdapter::trafficMutex_;
std::map<std::string, StatisticWrap<TrafficStat>> HiViewAdapter::trafficStat_;

std::mutex HiViewAdapter::dbMutex_;
std::map<std::string, StatisticWrap<DbStat>> HiViewAdapter::dbStat_;

std::mutex HiViewAdapter::apiPerformanceMutex_;
std::map<std::string, StatisticWrap<ApiPerformanceStat>> HiViewAdapter::apiPerformanceStat_;

bool HiViewAdapter::running_ = false;
TaskScheduler HiViewAdapter::scheduler_ {"HiView"};
std::mutex HiViewAdapter::runMutex_;

void HiViewAdapter::ReportFault(int dfxCode, const FaultMsg &msg)
{
    if (pool_ == nullptr) {
        return;
    }
    KvStoreTask task([dfxCode, msg]() {
        HiSysEvent::Write(HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            CoverEventID(dfxCode),
            HiSysEvent::EventType::FAULT,
            FAULT_TYPE, static_cast<int>(msg.faultType),
            MODULE_NAME, msg.moduleName,
            INTERFACE_NAME, msg.interfaceName,
            ERROR_TYPE, static_cast<int>(msg.errorType));
    });
    pool_->AddTask(std::move(task));
}

void HiViewAdapter::ReportDBFault(int dfxCode, const DBFaultMsg &msg)
{
    if (pool_ == nullptr) {
        return;
    }
    KvStoreTask task([dfxCode, msg]() {
        HiSysEvent::Write(HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            CoverEventID(dfxCode),
            HiSysEvent::EventType::FAULT,
            APP_ID, msg.appId,
            STORE_ID, msg.storeId,
            MODULE_NAME, msg.moduleName,
            ERROR_TYPE, static_cast<int>(msg.errorType));
    });
    pool_->AddTask(std::move(task));
}


void HiViewAdapter::ReportCommFault(int dfxCode, const CommFaultMsg &msg)
{
    if (pool_ == nullptr) {
        return;
    }
    KvStoreTask task([dfxCode, msg]() {
        std::string message;
        for (size_t i = 0; i < msg.deviceId.size(); i++) {
            message.append("No: ").append(std::to_string(i))
            .append(" sync to device: ").append(msg.deviceId[i])
            .append(" has error, errCode:").append(std::to_string(msg.errorCode[i])).append(". ");
        }
        HiSysEvent::Write(HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            CoverEventID(dfxCode),
            HiSysEvent::EventType::FAULT,
            USER_ID, msg.userId,
            APP_ID, msg.appId,
            STORE_ID, msg.storeId,
            SYNC_ERROR_INFO, message);
    });
    pool_->AddTask(std::move(task));
}

void HiViewAdapter::ReportBehaviour(int dfxCode, const BehaviourMsg &msg)
{
    if (pool_ == nullptr) {
        return;
    }
    KvStoreTask task([dfxCode, msg]() {
        std::string message;
        message.append("Behaviour type : ").append(std::to_string(static_cast<int>(msg.behaviourType)))
            .append(" behaviour info : ").append(msg.extensionInfo);
        HiSysEvent::Write(HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            CoverEventID(dfxCode),
            HiSysEvent::EventType::BEHAVIOR,
            USER_ID, msg.userId,
            APP_ID, msg.appId,
            STORE_ID, msg.storeId,
            BEHAVIOUR_INFO, message);
    });
    pool_->AddTask(std::move(task));
}

void HiViewAdapter::ReportDatabaseStatistic(int dfxCode, const DbStat &stat)
{
    if (pool_ == nullptr) {
        return;
    }
    KvStoreTask task([dfxCode, stat]() {
        std::lock_guard<std::mutex> lock(dbMutex_);
        if (!dbStat_.count(stat.GetKey())) {
            dbStat_.insert({stat.GetKey(), {stat, 0, dfxCode}});
        }
    });
    pool_->AddTask(std::move(task));
    StartTimerThread();
}

void HiViewAdapter::ReportDbSize(const StatisticWrap<DbStat> &stat)
{
    uint64_t dbSize;
    if (!stat.val.delegate->GetKvStoreDiskSize(stat.val.storeId, dbSize)) {
        return;
    }

    ValueHash vh;
    std::string userId;
    if (!vh.CalcValueHash(stat.val.userId, userId)) {
        return;
    }

    HiSysEvent::Write(HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
        CoverEventID(stat.code),
        HiSysEvent::EventType::STATISTIC,
        USER_ID, userId, APP_ID, stat.val.appId, STORE_ID, stat.val.storeId, DB_SIZE, dbSize);
}

void HiViewAdapter::InvokeDbSize()
{
    std::lock_guard<std::mutex> lock(dbMutex_);
    for (auto const &kv : dbStat_) {
        if (kv.second.val.delegate == nullptr) {
            continue;
        }
        // device coordinate for single version database
        if (!kv.second.val.storeId.empty()) {
            ReportDbSize(kv.second);
            continue;
        }
        // below codes for multiple version database
        std::vector<StoreInfo> storeInfos;
        kv.second.val.delegate->GetKvStoreKeys(storeInfos);
        if (storeInfos.empty()) {
            continue;
        }
        for (auto const &storeInfo : storeInfos) {
            ReportDbSize({{storeInfo.userId, storeInfo.appId, storeInfo.storeId, 0,
                kv.second.val.delegate}, 0, kv.second.code});
        }
    }
    dbStat_.clear();
}

void HiViewAdapter::ReportTrafficStatistic(int dfxCode, const TrafficStat &stat)
{
    if (pool_ == nullptr) {
        return;
    }
    KvStoreTask task([dfxCode, stat]() {
        std::lock_guard<std::mutex> lock(trafficMutex_);
        auto it = trafficStat_.find(stat.GetKey());
        if (it != trafficStat_.end()) {
            it->second.val.receivedSize += stat.receivedSize;
            it->second.val.sendSize += stat.sendSize;
        } else {
            trafficStat_.insert({stat.GetKey(), {stat, 0, dfxCode}});
        }
    });
    pool_->AddTask(std::move(task));
    StartTimerThread();
}

void HiViewAdapter::InvokeTraffic()
{
    std::lock_guard<std::mutex> lock(trafficMutex_);
    ValueHash vh;
    for (auto const &kv : trafficStat_) {
        std::string deviceId;
        if (!vh.CalcValueHash(kv.second.val.deviceId, deviceId)) {
            continue;
        }

        HiSysEvent::Write(HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            CoverEventID(kv.second.code),
            HiSysEvent::EventType::STATISTIC,
            TAG, POWERSTATS,
            APP_ID, kv.second.val.appId,
            DEVICE_ID, deviceId,
            SEND_SIZE, kv.second.val.sendSize,
            RECEIVED_SIZE, kv.second.val.receivedSize);
    }
    trafficStat_.clear();
}

void HiViewAdapter::ReportVisitStatistic(int dfxCode, const VisitStat &stat)
{
    if (pool_ == nullptr) {
        return;
    }
    KvStoreTask task([dfxCode, stat]() {
        std::lock_guard<std::mutex> lock(visitMutex_);
        auto it = visitStat_.find(stat.GetKey());
        if (it == visitStat_.end()) {
            visitStat_.insert({stat.GetKey(), {stat, 1, dfxCode}});
        } else {
            it->second.times++;
        }
    });
    pool_->AddTask(std::move(task));
    StartTimerThread();
}

void HiViewAdapter::InvokeVisit()
{
    std::lock_guard<std::mutex> lock(visitMutex_);
    for (auto const &kv : visitStat_) {
        HiSysEvent::Write(HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            CoverEventID(kv.second.code),
            HiSysEvent::EventType::STATISTIC,
            TAG, POWERSTATS,
            APP_ID, kv.second.val.appId,
            INTERFACE_NAME, kv.second.val.interfaceName,
            TIMES, kv.second.times);
    }
    visitStat_.clear();
}

void HiViewAdapter::ReportApiPerformanceStatistic(int dfxCode, const ApiPerformanceStat &stat)
{
    if (pool_ == nullptr) {
        return;
    }
    KvStoreTask task([dfxCode, stat]() {
        std::lock_guard<std::mutex> lock(apiPerformanceMutex_);
        auto it = apiPerformanceStat_.find(stat.GetKey());
        if (it == apiPerformanceStat_.end()) {
            apiPerformanceStat_.insert({stat.GetKey(), {stat, 1, dfxCode}}); // the init value of times is 1
        } else {
            it->second.times++;
            it->second.val.costTime = stat.costTime;
            if (it->second.times > 0) {
                it->second.val.averageTime =
                    (it->second.val.averageTime * static_cast<uint64_t>(it->second.times - 1) + stat.costTime)
                    / it->second.times;
            }
            if (stat.costTime > it->second.val.worstTime) {
                it->second.val.worstTime = stat.costTime;
            }
        }
    });
    pool_->AddTask(std::move(task));
    StartTimerThread();
}

void HiViewAdapter::InvokeApiPerformance()
{
    std::string message;
    message.append("[");
    std::lock_guard<std::mutex> lock(apiPerformanceMutex_);
    for (auto const &kv : apiPerformanceStat_) {
        message.append("{\"CODE\":\"").append(std::to_string(kv.second.code)).append("\",")
        .append("\"").append(INTERFACE_NAME).append("\":\"").append(kv.second.val.interfaceName).append("\",")
        .append("\"").append(TIMES).append("\":").append(std::to_string(kv.second.times)).append(",")
        .append("\"").append(AVERAGE_TIMES).append("\":").append(std::to_string(kv.second.val.averageTime)).append(",")
        .append("\"").append(WORST_TIMES).append("\":").append(std::to_string(kv.second.val.worstTime)).append("}");
    }
    message.append("]");
    HiSysEvent::Write(HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
        CoverEventID(DfxCodeConstant::API_PERFORMANCE_STATISTIC),
        HiSysEvent::EventType::STATISTIC,
        INTERFACES, message);
    apiPerformanceStat_.clear();
    ZLOGI("DdsTrace interface: clean");
}

void HiViewAdapter::StartTimerThread()
{
    if (running_) {
        return;
    }
    std::lock_guard<std::mutex> lock(runMutex_);
    if (running_) {
        return;
    }
    running_ = true;
    std::chrono::duration<int> delay(0);
    std::chrono::duration<int> internal(WAIT_TIME);
    auto fun = []() {
        time_t current = time(nullptr);
        tm localTime = { 0 };
        tm *result = localtime_r(&current, &localTime);
        if ((result != nullptr) && (localTime.tm_hour == DAILY_REPORT_TIME)) {
            InvokeDbSize();
            InvokeApiPerformance();
        }
        InvokeTraffic();
        InvokeVisit();
    };
    scheduler_.Every(delay, internal, fun);
}

std::string HiViewAdapter::CoverEventID(int dfxCode)
{
    std::string sysEventID = "";
    auto operatorIter = EVENT_COVERT_TABLE.find(dfxCode);
    if (operatorIter != EVENT_COVERT_TABLE.end()) {
        sysEventID = operatorIter->second;
    }
    return sysEventID;
}
} // namespace DistributedDataDfx
} // namespace OHOS
