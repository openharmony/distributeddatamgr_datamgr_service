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

#include "sensitive.h"
#include <utility>
#include "iprocess_system_api_adapter.h"
#include "log_print.h"
#undef LOG_TAG
#define LOG_TAG "Sensitive"

namespace OHOS::DistributedKv {
Sensitive::Sensitive(std::string deviceId, uint32_t type)
    : deviceId(std::move(deviceId)), securityLevel(0), deviceType(type)
{
}

Sensitive::Sensitive(const std::vector<uint8_t> &value)
    : securityLevel(0), deviceType(0)
{
    Unmarshal(value);
}

std::vector<uint8_t> Sensitive::Marshal() const
{
    return {};
}

void Sensitive::Unmarshal(const std::vector<uint8_t> &value)
{
}

uint32_t Sensitive::GetSensitiveLevel()
{
    return securityLevel;
}

bool Sensitive::operator >= (const DistributedDB::SecurityOption &option)
{
    return true;
}

bool Sensitive::LoadData()
{
    return true;
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
    dataBase64 = std::move(sensitive.dataBase64);
    securityLevel = sensitive.securityLevel;
    deviceType = sensitive.deviceType;
    return *this;
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
    dataBase64 = sensitive.dataBase64;
    securityLevel = sensitive.securityLevel;
    deviceType = sensitive.deviceType;
    return *this;
}

uint32_t Sensitive::GetDevslDeviceType() const
{
    return deviceType;
}
}
