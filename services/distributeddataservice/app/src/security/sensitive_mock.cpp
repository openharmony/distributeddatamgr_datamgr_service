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

#include "sensitive.h"
#include <utility>
#include <vector>
#include "log_print.h"
#include "utils/anonymous.h"
#undef LOG_TAG
#define LOG_TAG "SensitiveMock"

namespace OHOS {
namespace DistributedKv {
using Anonymous = DistributedData::Anonymous;
static constexpr uint32_t DATA_SEC_LEVEL1 = 1;
static constexpr uint32_t MAX_UDID_LENGTH = 64;

struct DEVSLQueryParams {
    uint8_t udid[MAX_UDID_LENGTH];
    uint32_t udidLen;
};
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
    return 0;
}

bool InitDEVSLQueryParams(DEVSLQueryParams *params, const std::string &udid)
{
    return true;
}

Sensitive::operator bool() const
{
    return true;
}

bool Sensitive::operator >= (const DistributedDB::SecurityOption &option)
{
    return true;
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
    return 0;
}
} // namespace DistributedKv
} // namespace OHOS