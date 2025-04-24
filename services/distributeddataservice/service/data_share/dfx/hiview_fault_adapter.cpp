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

#define LOG_TAG "HiViewFaultAdapter"

#include "hiview_fault_adapter.h"

#include <iomanip>
#include <sstream>
#include "log_print.h"

namespace OHOS {
namespace DataShare {
namespace {
    constexpr char DOMAIN[] = "DISTDATAMGR";
    constexpr const char *EVENT_NAME = "DISTRIBUTED_DATA_SHARE_FAULT";
    constexpr const size_t PARAMS_SIZE = 8;
    // digits for milliseconds
    constexpr uint8_t MILLSECOND_DIGIT = 3;
}

void HiViewFaultAdapter::ReportDataFault(const DataShareFaultInfo &faultInfo)
{
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%F %T");
    // get milliseconds
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    ss << '.' << std::setfill('0') << std::setw(MILLSECOND_DIGIT) << ms.count();
    auto time = ss.str();
    HiSysEventParam faultTime = { .name = "FAULT_TIME", .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char*>(time.c_str()) }, .arraySize = 0 };
    HiSysEventParam faultType = { .name = "FAULT_TYPE", .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char*>(faultInfo.faultType.c_str()) }, .arraySize = 0 };
    HiSysEventParam bundleName = { .name = "BUNDLE_NAME", .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char*>(faultInfo.bundleName.c_str()) }, .arraySize = 0 };
    HiSysEventParam moduleName = { .name = "MODULE_NAME", .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char*>(faultInfo.moduleName.c_str()) }, .arraySize = 0 };
    HiSysEventParam storeName = { .name = "STORE_NAME", .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char*>(faultInfo.storeName.c_str()) }, .arraySize = 0 };
    HiSysEventParam businessType = { .name = "BUSINESS_TYPE", .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char*>(faultInfo.businessType.c_str()) }, .arraySize = 0 };
    HiSysEventParam errorCode = { .name = "ERROR_CODE", .t = HISYSEVENT_INT32,
        .v = { .i32 = faultInfo.errorCode }, .arraySize = 0 };
    HiSysEventParam appendix = { .name = "APPENDIX", .t = HISYSEVENT_STRING,
        .v = { .s = const_cast<char*>(faultInfo.appendix.c_str()) }, .arraySize = 0 };
    HiSysEventParam params[] = { faultTime, faultType, bundleName, moduleName,
        storeName, businessType, errorCode, appendix };
    int res = OH_HiSysEvent_Write(DOMAIN, EVENT_NAME, HISYSEVENT_FAULT, params, PARAMS_SIZE);
    ZLOGI("OH_HiSysEvent_Write, res = %{public}d", res);
}

std::pair<std::string, int> HiViewFaultAdapter::GetCallingName(uint32_t callingTokenid)
{
    std::string callingName;
    auto tokenType = Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(callingTokenid);
    int result = -1;
    if (tokenType == Security::AccessToken::TOKEN_HAP) {
        Security::AccessToken::HapTokenInfo tokenInfo;
        result = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(callingTokenid, tokenInfo);
        if (result == Security::AccessToken::RET_SUCCESS) {
            callingName = tokenInfo.bundleName;
        }
    } else if (tokenType == Security::AccessToken::TOKEN_NATIVE || tokenType == Security::AccessToken::TOKEN_SHELL) {
        Security::AccessToken::NativeTokenInfo tokenInfo;
        result = Security::AccessToken::AccessTokenKit::GetNativeTokenInfo(callingTokenid, tokenInfo);
        if (result == Security::AccessToken::RET_SUCCESS) {
            callingName = tokenInfo.processName;
        }
    } else {
        ZLOGE("tokenType is invalid, tokenType:%{public}d, Tokenid:%{public}x", tokenType, callingTokenid);
    }
    return std::make_pair(callingName, result);
}

void TimeoutReport::Report(const std::string &timeoutAppendix, const std::chrono::milliseconds timeoutms)
{
    auto end = std::chrono::steady_clock::now();
    std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // Used to report DFX timeout faults
    if (needFaultReport && duration > HiViewFaultAdapter::dfxTimeOutMs) {
        DFXReport(duration);
    }
    // Used to report log timeout
    if (duration > timeoutms) {
        int64_t milliseconds = duration.count();
        ZLOGE("over time when doing %{public}s, %{public}s, cost:%{public}" PRIi64 "ms",
            dfxInfo.businessType.c_str(), timeoutAppendix.c_str(), milliseconds);
    }
}

void TimeoutReport::Report(const std::string &user, uint32_t callingPid, int32_t appIndex, int32_t instanceId)
{
    auto end = std::chrono::steady_clock::now();
    std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    // Used to report DFX timeout faults
    if (needFaultReport && duration > HiViewFaultAdapter::dfxTimeOutMs) {
        DFXReport(duration);
    }
    // Used to report log timeout
    if (duration > HiViewFaultAdapter::timeOutMs) {
        int64_t milliseconds = duration.count();
        std::string timeoutAppendix = "bundleName: " + dfxInfo.bundleName + ", user: " + user + ", callingPid: " +
            std::to_string(callingPid);
        if (!dfxInfo.storeName.empty()) {
            timeoutAppendix += ", storeName: " + dfxInfo.storeName;
        }
        if (appIndex != -1) {
            timeoutAppendix += ", appIndex: " + std::to_string(appIndex);
        }
        if (instanceId != -1) {
            timeoutAppendix += ", instanceId: " + std::to_string(instanceId);
        }
        ZLOGE("over time when doing %{public}s, %{public}s, cost:%{public}" PRIi64 "ms",
            dfxInfo.businessType.c_str(), timeoutAppendix.c_str(), milliseconds);
    }
}

void TimeoutReport::DFXReport(const std::chrono::milliseconds &duration)
{
    int64_t milliseconds = duration.count();
    std::string appendix = "callingName:" + HiViewFaultAdapter::GetCallingName(dfxInfo.callingTokenId).first;
    appendix += ",cost:" + std::to_string(milliseconds) + "ms";
    DataShareFaultInfo faultInfo{HiViewFaultAdapter::timeOut, dfxInfo.bundleName, dfxInfo.moduleName,
        dfxInfo.storeName, dfxInfo.businessType, errorCode, appendix};
    HiViewFaultAdapter::ReportDataFault(faultInfo);
}
}
}