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

#ifndef DATASHARESERVICE_HIVIEW_FAULT_ADAPTER_H
#define DATASHARESERVICE_HIVIEW_FAULT_ADAPTER_H

#include <chrono>
#include <cinttypes>
#include <string>
#include "accesstoken_kit.h"
#include "hisysevent_c.h"

namespace OHOS {
namespace DataShare {
struct DataShareFaultInfo {
    std::string faultType;
    std::string bundleName;
    std::string moduleName;
    std::string storeName;
    std::string businessType;
    int32_t errorCode;
    std::string appendix;
};

class HiViewFaultAdapter {
public:
    static constexpr const std::chrono::milliseconds timeOutMs = std::chrono::milliseconds(300);
    static constexpr const std::chrono::milliseconds dfxTimeOutMs = std::chrono::milliseconds(2000);
    static constexpr const char* timeOut = "TIME_OUT";
    static constexpr const char* resultsetFull = "RESULTSET_FULL";
    static constexpr const char* curdFailed = "CURD_FAILED";
    static constexpr const char* kvDBCorrupt = "KVDB_CORRUPT";
    static constexpr const char* unapprovedProvider = "UNAPPROVED_PROVIDER";
    static constexpr const char* invalidPredicates = "INVALID_PREDICATES";
    static void ReportDataFault(const DataShareFaultInfo &faultInfo);
    static std::pair<std::string, int> GetCallingName(uint32_t callingTokenid);
};

// TimeoutReport is used for recording the time usage of multiple interfaces;
// It can set up a timeout threshold, and when the time usage exceeds this threshold, will print log;
// If want to record DFX fault report, need to set needFaultReport to true (default is false);
// In the future, it can be separeted to diffent function in order to achieve these functions.
struct TimeoutReport {
    struct DfxInfo {
        std::string bundleName;
        std::string moduleName;
        std::string storeName;
        std::string businessType;
        uint64_t callingTokenId;
    };

    DfxInfo dfxInfo;
    int32_t errorCode = 0;
    bool needFaultReport;
    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
    explicit TimeoutReport(const DfxInfo &dfxInfo, bool needFaultReport = false)
        : dfxInfo(dfxInfo), needFaultReport(needFaultReport)
    {}
    void Report(const std::string &timeoutAppendix = "",
        const std::chrono::milliseconds timeoutms = HiViewFaultAdapter::timeOutMs);
    void Report(const std::string &user, uint32_t callingPid, int32_t appIndex = -1, int32_t instanceId = -1);
    void DFXReport(const std::chrono::milliseconds &duration);
    ~TimeoutReport() = default;
};
}
}
#endif