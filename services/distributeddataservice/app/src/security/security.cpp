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
#include <unistd.h>
#include <thread>
#include <algorithm>
#include <regex>
#include "constant.h"
#include "log_print.h"
#include "kvstore_utils.h"
#include "communication_provider.h"
#include "dev_slinfo_mgr.h"
#include "security_label.h"

#undef LOG_TAG
#define LOG_TAG "Security"
namespace OHOS::DistributedKv {
namespace {
    const std::string SECURITY_VALUE_XATTR_PARRERN = "s([01234])";
    const std::string EMPTY_STRING = "";
}
using namespace DistributedDB;
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

DBStatus Security::RegOnAccessControlledEvent(const OnAccessControlledEvent &callback)
{
    ZLOGD("add new lock status observer!");
    return NOT_SUPPORT;
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
    Sensitive sensitive = GetDeviceNodeByUuid(deviceId, true, nullptr);
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
        return EMPTY_STRING;
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
    if (info.deviceId.empty()) {
        ZLOGD("deviceId is empty");
        return;
    }

    bool isOnline = type == AppDistributedKv::DeviceChangeType::DEVICE_ONLINE;
    Sensitive sensitive = GetDeviceNodeByUuid(info.deviceId, isOnline, nullptr);
    ZLOGD("device is online:%d, deviceId:%{public}s", isOnline, KvStoreUtils::ToBeAnonymous(info.deviceId).c_str());
    if (isOnline) {
        auto secuiryLevel = sensitive.GetDeviceSecurityLevel();
        ZLOGI("device is online, secuiry Level:%d", secuiryLevel);
    }
}

bool Security::IsExits(const std::string &file) const
{
    return access(file.c_str(), F_OK) == 0;
}

Sensitive Security::GetDeviceNodeByUuid(const std::string &uuid, bool isOnline,
                                        const std::function<std::vector<uint8_t>(void)> &getValue)
{
    static std::mutex mutex;
    static std::map<std::string, Sensitive> devicesUdid;
    std::lock_guard<std::mutex> guard(mutex);
    auto it = devicesUdid.find(uuid);
    if (!isOnline) {
        if (it != devicesUdid.end()) {
            devicesUdid.erase(uuid);
        }
        return Sensitive();
    }
    if (it != devicesUdid.end()) {
        return it->second;
    }

    auto &network = AppDistributedKv::CommunicationProvider::GetInstance();
    auto devices = network.GetRemoteNodesBasicInfo();
    devices.push_back(network.GetLocalBasicInfo());
    for (auto &device : devices) {
        auto deviceUuid = network.GetUuidByNodeId(device.deviceId);
        ZLOGD("GetDeviceNodeByUuid(%.10s) peer device is %.10s", uuid.c_str(), deviceUuid.c_str());
        if (uuid != deviceUuid) {
            continue;
        }

        Sensitive sensitive(network.GetUdidByNodeId(device.deviceId));
        if (getValue == nullptr) {
            devicesUdid.insert(std::pair<std::string, Sensitive>(uuid, std::move(sensitive)));
            return devicesUdid[uuid];
        }

        auto value = getValue();
        ZLOGI("getValue is not nullptr!");
        return sensitive;
    }

    return Sensitive();
}

int32_t Security::GetCurrentUserStatus() const
{
    if (!IsSupportSecurity()) {
        return NO_PWD;
    }
    return NO_PWD;
}

DBStatus Security::SetFileSecurityOption(const std::string &filePath, const SecurityOption &option)
{
    ZLOGI("set security option %d", option.securityLabel);
    if (!IsExits(filePath)) {
        return INVALID_ARGS;
    }
    if (option.securityLabel == NOT_SET) {
        return OK;
    }
    auto dataLevel = Convert2Name(option);
    if (dataLevel.empty()) {
        ZLOGE("Invalid label args! label:%d, flag:%d path:%s",
              option.securityLabel, option.securityFlag, filePath.c_str());
        return INVALID_ARGS;
    }

    bool result = OHOS::DistributedFS::ModuleSecurityLabel::SecurityLabel::SetSecurityLabel(filePath, dataLevel);
    if (!result) {
        ZLOGE("set security label failed!, result:%d, datalevel:%s", result, dataLevel.c_str());
        return DB_ERROR;
    }

    return OK;
}

DBStatus Security::SetDirSecurityOption(const std::string &filePath, const SecurityOption &option)
{
    ZLOGI("the filePath is a directory!");
    return NOT_SUPPORT;
}

DBStatus Security::GetFileSecurityOption(const std::string &filePath, SecurityOption &option) const
{
    if (!IsExits(filePath)) {
        option = {NOT_SET, ECE};
        return OK;
    }

    std::string value = OHOS::DistributedFS::ModuleSecurityLabel::SecurityLabel::GetSecurityLabel(filePath);
    if (!IsXattrValueValid(value)) {
        option = {NOT_SET, ECE};
        return OK;
    }
    ZLOGI("get security option %s", value.c_str());
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
    return NOT_SUPPORT;
}
}
