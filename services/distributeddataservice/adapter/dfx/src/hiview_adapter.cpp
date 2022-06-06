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
namespace DistributedKv {
using OHOS::HiviewDFX::HiSysEvent;
std::shared_ptr<KvStoreThreadPool> HiViewAdapter::pool_ = KvStoreThreadPool::GetPool(POOL_SIZE, true);

std::mutex HiViewAdapter::visitMutex_;
std::map<std::string, StatisticWrap<VisitStat>> HiViewAdapter::visitStat_;

std::mutex HiViewAdapter::trafficMutex_;
std::map<std::string, StatisticWrap<TrafficStat>> HiViewAdapter::trafficStat_;

std::mutex HiViewAdapter::dbMutex_;
std::map<std::string, StatisticWrap<DbStat>> HiViewAdapter::dbStat_;

std::mutex HiViewAdapter::apiPerformanceMutex_;
std::map<std::string, StatisticWrap<ApiPerformanceStat>> HiViewAdapter::apiPerformanceStat_;

bool HiViewAdapter::running_ = false;
std::mutex HiViewAdapter::runMutex_;

void HiViewAdapter::ReportFault(int dfxCode, const FaultMsg &msg)
{
    KvStoreTask task([dfxCode, msg]() {
        HiSysEvent::Write(HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            std::to_string(dfxCode),
            HiSysEvent::EventType::FAULT,
            FAULT_TYPE, static_cast<int>(msg.faultType),
            MODULE_NAME, msg.moduleName,
            INTERFACE_NAME, msg.interfaceName,
            ERROR_TYPE, static_cast<int>(msg.errorType));
    });
    if (pool_ != nullptr) {
        pool_->AddTask(std::move(task));
    }
}

void HiViewAdapter::ReportDBFault(int dfxCode, const DBFaultMsg &msg)
{
    KvStoreTask task([dfxCode, msg]() {
        HiSysEvent::Write(HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            std::to_string(dfxCode),
            HiSysEvent::EventType::FAULT,
            APP_ID, msg.appId,
            STORE_ID, msg.storeId,
            MODULE_NAME, msg.moduleName,
            ERROR_TYPE, static_cast<int>(msg.errorType));
    });
    if (pool_ != nullptr) {
        pool_->AddTask(std::move(task));
    }
}


void HiViewAdapter::ReportCommFault(int dfxCode, const CommFaultMsg &msg)
{
    KvStoreTask task([dfxCode, msg]() {
        std::string message;

        for (int i = 0; i < msg.deviceId.size(); i++) {
            message.append("sync to device: ").append(msg.deviceId[i])
            .append(" error:").append(std::to_string(msg.errorCode[i]));
        }
        HiSysEvent::Write(HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            std::to_string(dfxCode),
            HiSysEvent::EventType::FAULT,
            USER_ID, msg.userId,
            APP_ID, msg.appId,
            STORE_ID, msg.storeId,
            SYNC_ERROR_INFO, message);
    });
    if (pool_ != nullptr) {
        pool_->AddTask(std::move(task));
    }
}

void HiViewAdapter::ReportPermissionsSecurity(int dfxCode, const SecurityPermissionsMsg &msg)
{
    KvStoreTask task([dfxCode, msg]() {
        HiSysEvent::Write(HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            std::to_string(dfxCode),
            HiSysEvent::EventType::SECURITY,
            USER_ID, msg.appId,
            APP_ID, msg.appId,
            STORE_ID, msg.storeId,
            DEVICE_ID, msg.deviceId,
            SECURITY_INFO, static_cast<int>(msg.securityInfo));
    });
    if (pool_ != nullptr) {
        pool_->AddTask(std::move(task));
    }
}

void HiViewAdapter::ReportSensitiveLevelSecurity(int dfxCode, const SecuritySensitiveLevelMsg &msg)
{
    KvStoreTask task([dfxCode, msg]() {
        HiSysEvent::Write(HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            std::to_string(dfxCode),
            HiSysEvent::EventType::SECURITY,
            DEVICE_ID, msg.deviceId,
            DEVICE_SENSITIVE_LEVEL, msg.deviceSensitiveLevel,
            OPTION_SENSITIVE_LEVEL, msg.optionSensitiveLevel,
            SECURITY_INFO, static_cast<int>(msg.securityInfo));
    });
    if (pool_ != nullptr) {
        pool_->AddTask(std::move(task));
    }
}

void HiViewAdapter::ReportBehaviour(int dfxCode, const BehaviourMsg &msg)
{
    KvStoreTask task([dfxCode, msg]() {
        HiSysEvent::Write(HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            std::to_string(dfxCode),
            HiSysEvent::EventType::BEHAVIOR,
            USER_ID, msg.appId,
            APP_ID, msg.appId,
            STORE_ID, msg.storeId,
            BEHAVIOUR_INFO, static_cast<int>(msg.behaviourType));
    });
    if (pool_ != nullptr) {
        pool_->AddTask(std::move(task));
    }
}

void HiViewAdapter::ReportDatabaseStatistic(int dfxCode, const DbStat &stat)
{
    KvStoreTask task([dfxCode, stat]() {
        std::lock_guard<std::mutex> lock(dbMutex_);
        if (!dbStat_.count(stat.GetKey())) {
            dbStat_.insert({stat.GetKey(), {stat, 0, dfxCode}});
        }
    });
    if (pool_ != nullptr) {
        pool_->AddTask(std::move(task));
    }
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
        std::to_string(stat.code),
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
    if (pool_ != nullptr) {
        pool_->AddTask(std::move(task));
    }
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
            std::to_string(kv.second.code),
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
    KvStoreTask task([dfxCode, stat]() {
        std::lock_guard<std::mutex> lock(visitMutex_);
        auto it = visitStat_.find(stat.GetKey());
        if (it == visitStat_.end()) {
            visitStat_.insert({stat.GetKey(), {stat, 1, dfxCode}});
        } else {
            it->second.times++;
        }
    });
    if (pool_ != nullptr) {
        pool_->AddTask(std::move(task));
    }
    StartTimerThread();
}

void HiViewAdapter::InvokeVisit()
{
    std::lock_guard<std::mutex> lock(visitMutex_);
    for (auto const &kv : visitStat_) {
        HiSysEvent::Write(HiSysEvent::Domain::DISTRIBUTED_DATAMGR,
            std::to_string(kv.second.code),
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
    KvStoreTask task([dfxCode, stat]() {
        std::lock_guard<std::mutex> lock(apiPerformanceMutex_);
        auto it = apiPerformanceStat_.find(stat.GetKey());
        if (it == apiPerformanceStat_.end()) {
            apiPerformanceStat_.insert({stat.GetKey(), {stat, 1, dfxCode}}); // the init value of times is 1
        } else {
            it->second.times++;
            it->second.val.costTime = stat.costTime;
            if (it->second.times > 0) {
                it->second.val.averageTime = (it->second.val.averageTime * (it->second.times - 1) + stat.costTime)
                    / it->second.times;
            }
            if (stat.costTime > it->second.val.worstTime) {
                it->second.val.worstTime = stat.costTime;
            }
        }
    });
    if (pool_ != nullptr) {
        pool_->AddTask(std::move(task));
    }
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
        std::to_string(DfxCodeConstant::API_PERFORMANCE_STATISTIC),
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
    auto fun = []() {
        while (true) {
            time_t current = time(nullptr);
            tm localTime = { 0 };
            tm *result = localtime_r(&current, &localTime);
            if (result == nullptr) {
                continue;
            }
            int currentHour = localTime.tm_hour;
            int currentMin = localTime.tm_min;
            if ((EXEC_MIN_TIME - currentMin) != EXEC_MIN_TIME) {
                sleep((EXEC_MIN_TIME - currentMin) * SIXTY_SEC);
                InvokeTraffic();
                InvokeVisit();
                if (currentHour == EXEC_HOUR_TIME) {
                    InvokeDbSize();
                    InvokeApiPerformance();
                }
            } else {
                sleep(WAIT_TIME);
            }
        }
    };
    std::thread th = std::thread(fun);
    th.detach();
}
} // namespace DistributedKv
} // namespace OHOS
