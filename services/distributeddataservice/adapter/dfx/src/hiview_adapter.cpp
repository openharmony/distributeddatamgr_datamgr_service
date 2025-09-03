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
#include "utils/anonymous.h"
#include "utils/time_utils.h"
namespace OHOS {
namespace DistributedDataDfx {
using namespace DistributedKv;
using Anonymous = DistributedData::Anonymous;
namespace {
constexpr const char *DATAMGR_DOMAIN = "DISTDATAMGR";
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
constexpr const char *CHANNEL = "CHANNEL";
constexpr const char *DATA_SIZE = "DATA_SIZE";
constexpr const char *DATA_TYPE = "DATA_TYPE";
constexpr const char *OPERATION = "OPERATION";
constexpr const char *RESULT = "RESULT";

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
    { DfxCodeConstant::UDMF_DATA_BEHAVIOR, "UDMF_DATA_BEHAVIOR" },
    { DfxCodeConstant::ARKDATA_CLOUD_SYNC_FAULT, "ARKDATA_CLOUD_SYNC_FAULT" },
};
}
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

void HiViewAdapter::ReportFault(int dfxCode, const FaultMsg &msg, std::shared_ptr<ExecutorPool> executors)
{
    if (executors == nullptr) {
        ZLOGW("executors is nullptr!");
        return;
    }
    ExecutorPool::Task task([dfxCode, msg]() {
        struct HiSysEventParam params[] = {
            { .name = { *FAULT_TYPE },
                .t = HISYSEVENT_INT32,
                .v = { .i32 = static_cast<int32_t>(msg.faultType) },
                .arraySize = 0 },
            { .name = { *MODULE_NAME },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(msg.moduleName.c_str()) },
                .arraySize = 0 },
            { .name = { *INTERFACE_NAME },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(msg.interfaceName.c_str()) },
                .arraySize = 0 },
            { .name = { *ERROR_TYPE },
                .t = HISYSEVENT_INT32,
                .v = { .i32 = static_cast<int32_t>(msg.errorType) },
                .arraySize = 0 },
        };
        OH_HiSysEvent_Write(
            DATAMGR_DOMAIN,
            CoverEventID(dfxCode).c_str(),
            HISYSEVENT_FAULT,
            params,
            sizeof(params) / sizeof(params[0])
        );
    });
    executors->Execute(std::move(task));
}

void HiViewAdapter::ReportDBFault(int dfxCode, const DBFaultMsg &msg, std::shared_ptr<ExecutorPool> executors)
{
    if (executors == nullptr) {
        ZLOGW("executors is nullptr!");
        return;
    }
    ExecutorPool::Task task([dfxCode, msg]() {
        std::string storeId = Anonymous::Change(msg.storeId);
        struct HiSysEventParam params[] = {
            { .name = { *APP_ID },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(msg.appId.c_str()) },
                .arraySize = 0 },
            { .name = { *STORE_ID },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(storeId.c_str()) },
                .arraySize = 0 },
            { .name = { *MODULE_NAME },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(msg.moduleName.c_str()) },
                .arraySize = 0 },
            { .name = { *ERROR_TYPE },
                .t = HISYSEVENT_INT32,
                .v = { .i32 = static_cast<int32_t>(msg.errorType) },
                .arraySize = 0 },
        };
        OH_HiSysEvent_Write(
            DATAMGR_DOMAIN,
            CoverEventID(dfxCode).c_str(),
            HISYSEVENT_FAULT,
            params,
            sizeof(params) / sizeof(params[0])
        );
    });
    executors->Execute(std::move(task));
}

void HiViewAdapter::ReportCommFault(int dfxCode, const CommFaultMsg &msg, std::shared_ptr<ExecutorPool> executors)
{
    if (executors == nullptr) {
        ZLOGW("executors is nullptr!");
        return;
    }
    ExecutorPool ::Task task([dfxCode, msg]() {
        std::string message;
        std::string storeId = Anonymous::Change(msg.storeId);
        for (size_t i = 0; i < msg.deviceId.size(); i++) {
            message.append("No: ").append(std::to_string(i))
                .append(" sync to device: ").append(msg.deviceId[i])
                .append(" has error, errCode:").append(std::to_string(msg.errorCode[i])).append(". ");
        }
        struct HiSysEventParam params[] = {
            { .name = { *USER_ID },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(msg.userId.c_str()) },
                .arraySize = 0 },
            { .name = { *APP_ID },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(msg.appId.c_str()) },
                .arraySize = 0 },
            { .name = { *STORE_ID },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(storeId.c_str()) },
                .arraySize = 0 },
            { .name = { *SYNC_ERROR_INFO },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(message.c_str()) },
                .arraySize = 0 },
        };
        OH_HiSysEvent_Write(
            DATAMGR_DOMAIN,
            CoverEventID(dfxCode).c_str(),
            HISYSEVENT_FAULT,
            params,
            sizeof(params) / sizeof(params[0])
        );
    });
    executors->Execute(std::move(task));
}

void HiViewAdapter::ReportArkDataFault(int dfxCode, const ArkDataFaultMsg &msg, std::shared_ptr<ExecutorPool> executors)
{
    if (executors == nullptr) {
        ZLOGW("executors is nullptr!");
        return;
    }
    ExecutorPool::Task task([dfxCode, msg]() {
        std::string occurTime = DistributedData::TimeUtils::GetCurSysTimeWithMs();
        std::string bundleName = msg.bundleName;
        std::string moduleName = msg.moduleName;
        std::string storeName = msg.storeName;
        std::string businessType = msg.businessType;
        std::string appendix = msg.appendixMsg;
        std::string faultType = msg.faultType;
        struct HiSysEventParam params[] = {
            { .name = { "FAULT_TIME" }, .t = HISYSEVENT_STRING, .v = { .s = occurTime.data() }, .arraySize = 0 },
            { .name = { "FAULT_TYPE" }, .t = HISYSEVENT_STRING, .v = { .s = faultType.data() }, .arraySize = 0 },
            { .name = { "BUNDLE_NAME" }, .t = HISYSEVENT_STRING, .v = { .s = bundleName.data() }, .arraySize = 0 },
            { .name = { "MODULE_NAME" }, .t = HISYSEVENT_STRING, .v = { .s = moduleName.data() }, .arraySize = 0 },
            { .name = { "STORE_NAME" }, .t = HISYSEVENT_STRING, .v = { .s = storeName.data() }, .arraySize = 0 },
            { .name = { "BUSINESS_TYPE" }, .t = HISYSEVENT_STRING, .v = { .s = businessType.data() }, .arraySize = 0 },
            { .name = { "ERROR_CODE" }, .t = HISYSEVENT_INT32, .v = { .i32 = msg.errorType }, .arraySize = 0 },
            { .name = { "APPENDIX" }, .t = HISYSEVENT_STRING, .v = { .s = appendix.data() }, .arraySize = 0 },
        };
        OH_HiSysEvent_Write(DATAMGR_DOMAIN, CoverEventID(dfxCode).c_str(), HISYSEVENT_FAULT, params,
            sizeof(params) / sizeof(params[0]));
    });
    executors->Execute(std::move(task));
}

void HiViewAdapter::ReportBehaviour(int dfxCode, const BehaviourMsg &msg, std::shared_ptr<ExecutorPool> executors)
{
    if (executors == nullptr) {
        ZLOGW("executors is nullptr!");
        return;
    }
    ExecutorPool::Task task([dfxCode, msg]() {
        std::string message;
        std::string storeId = Anonymous::Change(msg.storeId);
        message.append("Behaviour type : ").append(std::to_string(static_cast<int>(msg.behaviourType)))
            .append(" behaviour info : ").append(msg.extensionInfo);
        struct HiSysEventParam params[] = {
            { .name = { *USER_ID },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(msg.userId.c_str()) },
                .arraySize = 0 },
            { .name = { *APP_ID },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(msg.appId.c_str()) },
                .arraySize = 0 },
            { .name = { *STORE_ID },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(storeId.c_str()) },
                .arraySize = 0 },
            { .name = { *BEHAVIOUR_INFO },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(message.c_str()) },
                .arraySize = 0 },
        };
        OH_HiSysEvent_Write(
            DATAMGR_DOMAIN,
            CoverEventID(dfxCode).c_str(),
            HISYSEVENT_BEHAVIOR,
            params,
            sizeof(params) / sizeof(params[0])
        );
    });
    executors->Execute(std::move(task));
}

void HiViewAdapter::ReportDatabaseStatistic(int dfxCode, const DbStat &stat, std::shared_ptr<ExecutorPool> executors)
{
    if (executors == nullptr) {
        ZLOGW("executors is nullptr!");
        return;
    }
    ExecutorPool::Task task([dfxCode, stat]() {
        std::lock_guard<std::mutex> lock(dbMutex_);
        if (!dbStat_.count(stat.GetKey())) {
            dbStat_.insert({stat.GetKey(), {stat, 0, dfxCode}});
        }
    });
    executors->Execute(std::move(task));
    StartTimerThread(executors);
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
    std::string storeId = Anonymous::Change(stat.val.storeId);
    struct HiSysEventParam params[] = {
        { .name = { *USER_ID },
            .t = HISYSEVENT_STRING,
            .v = { .s = const_cast<char *>(userId.c_str()) },
            .arraySize = 0 },
        { .name = { *APP_ID },
            .t = HISYSEVENT_STRING,
            .v = { .s = const_cast<char *>(stat.val.appId.c_str()) },
            .arraySize = 0 },
        { .name = { *STORE_ID },
            .t = HISYSEVENT_STRING,
            .v = { .s = const_cast<char *>(storeId.c_str()) },
            .arraySize = 0 },
        { .name = { *DB_SIZE }, .t = HISYSEVENT_UINT64, .v = { .ui64 = dbSize }, .arraySize = 0 },
    };
    OH_HiSysEvent_Write(
        DATAMGR_DOMAIN,
        CoverEventID(stat.code).c_str(),
        HISYSEVENT_STATISTIC,
        params,
        sizeof(params) / sizeof(params[0])
    );
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

void HiViewAdapter::ReportTrafficStatistic(int dfxCode, const TrafficStat &stat,
    std::shared_ptr<ExecutorPool> executors)
{
    if (executors == nullptr) {
        ZLOGW("executors is nullptr!");
        return;
    }
    ExecutorPool::Task task([dfxCode, stat]() {
        std::lock_guard<std::mutex> lock(trafficMutex_);
        auto it = trafficStat_.find(stat.GetKey());
        if (it != trafficStat_.end()) {
            it->second.val.receivedSize += stat.receivedSize;
            it->second.val.sendSize += stat.sendSize;
        } else {
            trafficStat_.insert({stat.GetKey(), {stat, 0, dfxCode}});
        }
    });
    executors->Execute(std::move(task));
    StartTimerThread(executors);
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
        std::string devId  = Anonymous::Change(deviceId);
        struct HiSysEventParam params[] = {
            { .name = { *TAG }, .t = HISYSEVENT_STRING, .v = { .s = const_cast<char *>(POWERSTATS) }, .arraySize = 0 },
            { .name = { *APP_ID },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(kv.second.val.appId.c_str()) },
                .arraySize = 0 },
            { .name = { *DEVICE_ID },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(devId.c_str()) },
                .arraySize = 0 },
            { .name = { *SEND_SIZE },
                .t = HISYSEVENT_INT64,
                .v = { .i64 = static_cast<int64_t>(kv.second.val.sendSize) },
                .arraySize = 0 },
            { .name = { *RECEIVED_SIZE },
                .t = HISYSEVENT_INT64,
                .v = { .i64 = static_cast<int64_t>(kv.second.val.receivedSize) },
                .arraySize = 0 },
        };
        OH_HiSysEvent_Write(
            DATAMGR_DOMAIN,
            CoverEventID(kv.second.code).c_str(),
            HISYSEVENT_STATISTIC,
            params,
            sizeof(params) / sizeof(params[0])
        );
    }
    trafficStat_.clear();
}

void HiViewAdapter::ReportVisitStatistic(int dfxCode, const VisitStat &stat, std::shared_ptr<ExecutorPool> executors)
{
    if (executors == nullptr) {
        ZLOGW("executors is nullptr!");
        return;
    }
    ExecutorPool::Task task([dfxCode, stat]() {
        std::lock_guard<std::mutex> lock(visitMutex_);
        auto it = visitStat_.find(stat.GetKey());
        if (it == visitStat_.end()) {
            visitStat_.insert({stat.GetKey(), {stat, 1, dfxCode}});
        } else {
            it->second.times++;
        }
    });
    executors->Execute(std::move(task));
    StartTimerThread(executors);
}

void HiViewAdapter::InvokeVisit()
{
    std::lock_guard<std::mutex> lock(visitMutex_);
    for (auto const &kv : visitStat_) {
        struct HiSysEventParam params[] = {
            { .name = { *TAG },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(POWERSTATS) },
                .arraySize = 0 },
            { .name = { *APP_ID },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(kv.second.val.appId.c_str()) },
                .arraySize = 0 },
            { .name = { *INTERFACE_NAME },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(kv.second.val.interfaceName.c_str()) },
                .arraySize = 0 },
            { .name = { *BEHAVIOUR_INFO },
                .t = HISYSEVENT_INT64,
                .v = { .i64 = static_cast<int64_t>(kv.second.times) },
                .arraySize = 0 },
        };
        OH_HiSysEvent_Write(
            DATAMGR_DOMAIN,
            CoverEventID(kv.second.code).c_str(),
            HISYSEVENT_STATISTIC,
            params,
            sizeof(params) / sizeof(params[0])
        );
    }
    visitStat_.clear();
}

void HiViewAdapter::ReportApiPerformanceStatistic(int dfxCode, const ApiPerformanceStat &stat,
    std::shared_ptr<ExecutorPool> executors)
{
    if (executors == nullptr) {
        ZLOGW("executors is nullptr!");
        return;
    }
    ExecutorPool::Task task([dfxCode, stat]() {
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

    executors->Execute(std::move(task));
    StartTimerThread(executors);
}

void HiViewAdapter::ReportUdmfBehaviour(
    int dfxCode, const UdmfBehaviourMsg &msg, std::shared_ptr<ExecutorPool> executors)
{
    if (executors == nullptr) {
        ZLOGI("report udmf behavior failed");
        return;
    }
    ExecutorPool::Task task([dfxCode, msg]() {
        struct HiSysEventParam params[] = {
            { .name = { *APP_ID },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(msg.appId.c_str()) },
                .arraySize = 0 },
            { .name = { *CHANNEL },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(msg.channel.c_str()) },
                .arraySize = 0 },
            { .name = { *DATA_SIZE },
                .t = HISYSEVENT_INT64,
                .v = { .i64 = msg.dataSize },
                .arraySize = 0 },
            { .name = { *DATA_TYPE },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(msg.dataType.c_str()) },
                .arraySize = 0 },
            { .name = { *OPERATION },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(msg.operation.c_str()) },
                .arraySize = 0 },
            { .name = { *RESULT },
                .t = HISYSEVENT_STRING,
                .v = { .s = const_cast<char *>(msg.result.c_str()) },
                .arraySize = 0 },
        };
        OH_HiSysEvent_Write(
            DATAMGR_DOMAIN,
            CoverEventID(dfxCode).c_str(),
            HISYSEVENT_BEHAVIOR,
            params,
            sizeof(params) / sizeof(params[0])
        );
    });
    executors->Execute(std::move(task));
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
            .append("\"").append(AVERAGE_TIMES).append("\":").append(std::to_string(kv.second.val.averageTime))
            .append(",").append("\"").append(WORST_TIMES).append("\":").append(std::to_string(kv.second.val.worstTime))
            .append("}");
    }
    message.append("]");
    struct HiSysEventParam params[] = {
        { .name = { *INTERFACES },
            .t = HISYSEVENT_STRING,
            .v = { .s = const_cast<char *>(message.c_str()) },
            .arraySize = 0 },
    };
    OH_HiSysEvent_Write(
        DATAMGR_DOMAIN,
        CoverEventID(DfxCodeConstant::API_PERFORMANCE_STATISTIC).c_str(),
        HISYSEVENT_STATISTIC,
        params,
        sizeof(params) / sizeof(params[0])
    );
    apiPerformanceStat_.clear();
    ZLOGI("DdsTrace interface: clean");
}

void HiViewAdapter::StartTimerThread(std::shared_ptr<ExecutorPool> executors)
{
    if (running_) {
        return;
    }
    std::lock_guard<std::mutex> lock(runMutex_);
    if (running_) {
        return;
    }
    running_ = true;
    auto interval = std::chrono::seconds(WAIT_TIME);
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
    executors->Schedule(fun, interval);
}

std::string HiViewAdapter::CoverEventID(int dfxCode)
{
    std::string sysEventID;
    auto operatorIter = EVENT_COVERT_TABLE.find(dfxCode);
    if (operatorIter != EVENT_COVERT_TABLE.end()) {
        sysEventID = operatorIter->second;
    }
    return sysEventID;
}
} // namespace DistributedDataDfx
} // namespace OHOS