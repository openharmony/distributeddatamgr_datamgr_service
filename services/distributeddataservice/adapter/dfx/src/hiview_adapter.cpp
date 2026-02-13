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
// Database statistic
constexpr const char *USER_ID = "ANONYMOUS_UID";
constexpr const char *APP_ID = "APP_ID";
constexpr const char *STORE_ID = "STORE_ID";
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
        std::string storeName = Anonymous::Change(msg.storeName);
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