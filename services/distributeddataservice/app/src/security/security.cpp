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

#include "security.h"
#include <regex>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include "dev_slinfo_mgr.h"
#include "device_manager_adapter.h"
#include "log_print.h"
#include "security_label.h"
#include "utils/anonymous.h"

#undef LOG_TAG
#define LOG_TAG "Security"
namespace OHOS::DistributedKv {
namespace {
    constexpr const char *SECURITY_VALUE_XATTR_PARRERN = "s([01234])";
}
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
    auto curStatus = GetCurrentUserStatus();
    return !(curStatus == UNLOCK || curStatus == NO_PWD);
}

DBStatus Security::SetSecurityOption(const std::string &filePath, const SecurityOption &option)
{
    if (filePath.empty()) {
        return INVALID_ARGS;
    }

    struct stat curStat;
    stat(filePath.c_str(), &curStat);
    if (S_ISDIR(curStat.st_mode)) {
        return SetDirSecurityOption(filePath, option);
    } else {
        return SetFileSecurityOption(filePath, option);
    }
}

DBStatus Security::GetSecurityOption(const std::string &filePath, SecurityOption &option) const
{
    if (filePath.empty()) {
        return INVALID_ARGS;
    }

    struct stat curStat;
    stat(filePath.c_str(), &curStat);
    if (S_ISDIR(curStat.st_mode)) {
        return GetDirSecurityOption(filePath, option);
    } else {
        return GetFileSecurityOption(filePath, option);
    }
}

bool Security::CheckDeviceSecurityAbility(const std::string &deviceId, const SecurityOption &option) const
{
    ZLOGD("The kvstore security level: label:%d", option.securityLabel);
    Sensitive sensitive = GetSensitiveByUuid(deviceId);
    return (sensitive >= option);
}

int Security::Convert2Security(const std::string &name)
{
    for (int i = 0; i <= S4; i++) {
        if (name == LABEL_VALUES[i]) {
            return i;
        }
    }
    return NOT_SET;
}

const std::string Security::Convert2Name(const SecurityOption &option)
{
    if (option.securityLabel <= NOT_SET || option.securityLabel > S4) {
        return "";
    }

    return LABEL_VALUES[option.securityLabel];
}

bool Security::IsXattrValueValid(const std::string& value) const
{
    if (value.empty()) {
        ZLOGD("value is empty");
        return false;
    }

    return std::regex_match(value, std::regex(SECURITY_VALUE_XATTR_PARRERN));
}

bool Security::IsSupportSecurity()
{
    return false;
}

void Security::OnDeviceChanged(const AppDistributedKv::DeviceInfo &info,
                               const AppDistributedKv::DeviceChangeType &type) const
{
    if (info.networkId.empty()) {
        ZLOGD("deviceId is empty");
        return;
    }

    if (info.uuid == DistributedData::DeviceManagerAdapter::CLOUD_DEVICE_UUID) {
        ZLOGD("This is network change");
        return;
    }

    switch (type) {
        case AppDistributedKv::DeviceChangeType::DEVICE_OFFLINE:
            ZLOGD("device is offline, deviceId:%{public}s", Anonymous::Change(info.uuid).c_str());
            EraseSensitiveByUuid(info.uuid);
            break;
        case AppDistributedKv::DeviceChangeType::DEVICE_ONLINE:
            ZLOGD("device is online, deviceId:%{public}s", Anonymous::Change(info.uuid).c_str());
            (void)GetSensitiveByUuid(info.uuid);
            break;
        default:
            break;
    }
}

bool Security::IsExits(const std::string &file) const
{
    return access(file.c_str(), F_OK) == 0;
}

Sensitive Security::GetSensitiveByUuid(const std::string &uuid) const
{
    auto it = devicesUdid_.Find(uuid);
    if (!it.first) {
        executors_->Execute([this, uuid]() {
            auto iter = devicesUdid_.Find(uuid);
            if (iter.first) {
                return;
            }
            auto udid = DistributedData::DeviceManagerAdapter::GetInstance().ToUDID(uuid);
            if (udid.empty()) {
                return;
            }
            Sensitive sensitive(udid);
            auto level = sensitive.GetDeviceSecurityLevel();
            ZLOGI("udid:%{public}s, uuid:%{public}s, security level:%{public}d",
                  Anonymous::Change(udid).c_str(), Anonymous::Change(uuid).c_str(), level);
            devicesUdid_.Insert(uuid, sensitive);
        });
    }
    return it.second;
}

bool Security::EraseSensitiveByUuid(const std::string &uuid) const
{
    devicesUdid_.Erase(uuid);
    return true;
}

int32_t Security::GetCurrentUserStatus() const
{
    return NO_PWD;
}

DBStatus Security::SetFileSecurityOption(const std::string &filePath, const SecurityOption &option)
{
    if (!IsExits(filePath)) {
        ZLOGE("option:%{public}d file:%{public}s not exits", option.securityLabel, filePath.c_str());
        return INVALID_ARGS;
    }
    if (option.securityLabel == NOT_SET) {
        return OK;
    }
    auto dataLevel = Convert2Name(option);
    if (dataLevel.empty()) {
        ZLOGE("Invalid args! label:%{public}d path:%{public}s", option.securityLabel, filePath.c_str());
        return INVALID_ARGS;
    }

    bool result = OHOS::FileManagement::ModuleSecurityLabel::SecurityLabel::SetSecurityLabel(filePath, dataLevel);
    if (result) {
        return OK;
    }

    auto error = errno;
    std::string current = OHOS::FileManagement::ModuleSecurityLabel::SecurityLabel::GetSecurityLabel(filePath);
    ZLOGE("failed! error:%{public}d current:%{public}s label:%{public}s file:%{public}s", error, current.c_str(),
        dataLevel.c_str(), filePath.c_str());
    if (current == dataLevel) {
        return OK;
    }
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
    if (!IsExits(filePath)) {
        option = {NOT_SET, ECE};
        return OK;
    }

    std::string value = OHOS::FileManagement::ModuleSecurityLabel::SecurityLabel::GetSecurityLabel(filePath);
    if (!IsXattrValueValid(value)) {
        option = {NOT_SET, ECE};
        return OK;
    }

    ZLOGI("get security option %{public}s", value.c_str());
    if (value == "s3") {
        option = { Convert2Security(value), SECE };
    } else {
        option = { Convert2Security(value), ECE };
    }
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
