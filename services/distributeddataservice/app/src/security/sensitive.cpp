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

#include "sensitive.h"
#include <utility>
#include <vector>
#include "log_print.h"
#include "utils/anonymous.h"
#undef LOG_TAG
#define LOG_TAG "Sensitive"

namespace OHOS {
namespace DistributedKv {
using Anonymous = DistributedData::Anonymous;
Sensitive::Sensitive(std::string deviceId)
    : deviceId(std::move(deviceId)), securityLevel(DATA_SEC_LEVEL1)
{
}

Sensitive::Sensitive()
    : deviceId(""), securityLevel(DATA_SEC_LEVEL1)
{
}

uint32_t Sensitive::GetDeviceSecurityLevel()
{
    if (securityLevel > DATA_SEC_LEVEL1) {
        ZLOGI("the device security level had gotten");
        return securityLevel;
    }
    return GetSensitiveLevel(deviceId);
}

bool Sensitive::InitDEVSLQueryParams(DEVSLQueryParams *params, const std::string &udid)
{
    ZLOGI("udid is [%{public}s]", Anonymous::Change(udid).c_str());
    if (params == nullptr || udid.empty()) {
        return false;
    }
    std::vector<uint8_t> vec(udid.begin(), udid.end());
    for (size_t i = 0; i < MAX_UDID_LENGTH && i < vec.size(); i++) {
        params->udid[i] = vec[i];
    }
    params->udidLen = uint32_t(udid.size());
    return true;
}

Sensitive::operator bool() const
{
    return (!deviceId.empty()) && (securityLevel > DATA_SEC_LEVEL1);
}

bool Sensitive::operator >= (const DistributedDB::SecurityOption &option)
{
    if (option.securityLabel == DistributedDB::NOT_SET) {
        return true;
    }
    
    uint32_t level = securityLevel;
    if (level <= DATA_SEC_LEVEL1 && (option.securityLabel - 1) > level) {
        ZLOGI("the device security level hadn't gotten");
        level = GetSensitiveLevel(deviceId);
    }
    return (level >= static_cast<uint32_t>(option.securityLabel - 1));
}

Sensitive::Sensitive(const Sensitive &sensitive)
{
    this->operator=(sensitive);
}

Sensitive &Sensitive::operator=(const Sensitive &sensitive)
{
    if (this == &sensitive) {
        return *this;
    }
    deviceId = sensitive.deviceId;
    securityLevel = sensitive.securityLevel;
    return *this;
}

Sensitive::Sensitive(Sensitive &&sensitive) noexcept
{
    this->operator=(std::move(sensitive));
}

Sensitive &Sensitive::operator=(Sensitive &&sensitive) noexcept
{
    if (this == &sensitive) {
        return *this;
    }
    deviceId = std::move(sensitive.deviceId);
    securityLevel = sensitive.securityLevel;
    return *this;
}

uint32_t Sensitive::GetSensitiveLevel(const std::string &udid)
{
    DEVSLQueryParams query;
    if (!InitDEVSLQueryParams(&query, udid)) {
        ZLOGE("init query params failed! udid:[%{public}s]", Anonymous::Change(udid).c_str());
        return DATA_SEC_LEVEL1;
    }

    uint32_t level = DATA_SEC_LEVEL1;
    int32_t result = DATASL_GetHighestSecLevel(&query, &level);
    if (result != DEVSL_SUCCESS) {
        ZLOGE("get highest level failed(%{public}s)! level: %{public}d, error: %d",
            Anonymous::Change(udid).c_str(), securityLevel, result);
        return DATA_SEC_LEVEL1;
    }
    securityLevel = level;
    ZLOGI("get highest level success(%{public}s)! level: %{public}d, error: %d",
        Anonymous::Change(udid).c_str(), securityLevel, result);
    return securityLevel;
}
} // namespace DistributedKv
} // namespace OHOS
