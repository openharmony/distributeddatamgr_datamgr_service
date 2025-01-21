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

struct RdbTimeCostInfo {
    std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
    std::string bundleName;
    std::string moduleName;
    std::string storeName;
    std::string businessType;
    int32_t errorCode = 0;
    uint64_t callingTokenId;

    explicit RdbTimeCostInfo(const std::string &bundleName, const std::string &moduleName,
        const std::string &storeName, const std::string &businessType,
        uint64_t tokenId) : bundleName(bundleName), moduleName(moduleName),
        storeName(storeName), businessType(businessType), callingTokenId(tokenId)
    {}
    ~RdbTimeCostInfo();
};


class HiViewFaultAdapter {
public:
    static void ReportDataFault(const DataShareFaultInfo &faultInfo);
    static std::pair<std::string, int> GetCallingName(uint32_t callingTokenid);
};

inline const char* TIME_OUT = "TIME_OUT";
inline const char* RESULTSET_FULL = "RESULTSET_FULL";
inline const char* CURD_FAILED = "CURD_FAILED";
inline const std::chrono::milliseconds TIME_OUT_MS = std::chrono::milliseconds(300);
}
}
#endif