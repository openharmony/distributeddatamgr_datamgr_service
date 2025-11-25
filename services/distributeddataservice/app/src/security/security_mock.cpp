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

#include "security.h"
#include <regex>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include "device_manager_adapter.h"
#include "log_print.h"
#include "security_label.h"
#include "utils/anonymous.h"

#undef LOG_TAG
#define LOG_TAG "SecurityMock"
namespace OHOS::DistributedKv {
using namespace DistributedDB;
using Anonymous = DistributedData::Anonymous;
const std::string Security::LABEL_VALUES[S4 + 1] = {
    "", "s0", "s1", "s2", "s3", "s4"
};
Security::Security()
{
    ZLOGD("construct");
}

Security::~Security()
{
    ZLOGD("destructor");
}

AppDistributedKv::ChangeLevelType Security::GetChangeLevelType() const
{
    return AppDistributedKv::ChangeLevelType::LOW;
}

DBStatus Security::RegOnAccessControlledEvent(const OnAccessControlledEvent &callback)
{
    ZLOGD("add new lock status observer!");
    return DBStatus::NOT_SUPPORT;
}

bool Security::IsAccessControlled() const
{
    return true;
}

DBStatus Security::SetSecurityOption(const std::string &filePath, const SecurityOption &option)
{
    return OK;
}

DBStatus Security::GetSecurityOption(const std::string &filePath, SecurityOption &option) const
{
    return OK;
}

bool Security::CheckDeviceSecurityAbility(const std::string &deviceId, const SecurityOption &option) const
{
    return true;
}

int Security::Convert2Security(const std::string &name)
{
    return NOT_SET;
}

const std::string Security::Convert2Name(const SecurityOption &option)
{
    return "";
}

bool Security::IsXattrValueValid(const std::string& value) const
{
    return true;
}

bool Security::IsSupportSecurity()
{
    return false;
}

void Security::OnDeviceChanged(const AppDistributedKv::DeviceInfo &info,
                               const AppDistributedKv::DeviceChangeType &type) const
{
    return;
}

bool Security::IsExist(const std::string &file) const
{
    return access(file.c_str(), F_OK) == 0;
}

void Security::InitLocalSecurity()
{
    return;
}

Sensitive Security::GetSensitiveByUuid(const std::string &uuid) const
{
    Sensitive sensitive{};
    return sensitive;
}

bool Security::EraseSensitiveByUuid(const std::string &uuid) const
{
    return true;
}

int32_t Security::GetCurrentUserStatus() const
{
    return NO_PWD;
}

DBStatus Security::SetFileSecurityOption(const std::string &filePath, const SecurityOption &option)
{
    return DistributedDB::DB_ERROR;
}

DBStatus Security::SetDirSecurityOption(const std::string &filePath, const SecurityOption &option)
{
    ZLOGI("the filePath is a directory!");
    (void)filePath;
    (void)option;
    return DBStatus::NOT_SUPPORT;
}

DBStatus Security::GetFileSecurityOption(const std::string &filePath, SecurityOption &option) const
{
    return OK;
}

DBStatus Security::GetDirSecurityOption(const std::string &filePath, SecurityOption &option) const
{
    ZLOGI("the filePath is a directory!");
    (void)filePath;
    (void)option;
    return DBStatus::NOT_SUPPORT;
}
} // namespace OHOS::DistributedKv