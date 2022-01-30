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
#include "communication_provider.h"
#include "constant.h"
#include "sensitive.h"
#include "log_print.h"
#include "block_integer.h"
#include "ohos_account_kits.h"

#undef LOG_TAG
#define LOG_TAG "SecurityAdapter"
namespace OHOS::DistributedKv {
using namespace DistributedDB;
std::atomic_bool Security::isInitialized_ = true;
const char * const Security::LABEL_VALUES[S4 + 1] = {};
const char * const Security::DATA_DE[] = { nullptr };
const char * const Security::DATA_CE[] = { nullptr };

Security::Security(const std::string &appId, const std::string &userId, const std::string &dir)
{
    ZLOGD("constructor kvStore_ is %s", dir.c_str());
}

Security::~Security()
{
    ZLOGD("destructor kvStore_");
}

DBStatus Security::RegOnAccessControlledEvent(const OnAccessControlledEvent &callback)
{
    ZLOGD("add new lock status observer! current size: %d", static_cast<int32_t>(observers_.size()));
    if (callback == nullptr) {
        return INVALID_ARGS;
    }

    if (!IsSupportSecurity()) {
        ZLOGI("Not support lock status!");
        return NOT_SUPPORT;
    }

    if (observers_.empty()) {
        observers_.insert(std::pair<int32_t, OnAccessControlledEvent>(GetCurrentUserId(), callback));
        std::thread th = std::thread([this] {
            ZLOGI("Start to subscribe lock status!");
            bool result = false;
            std::function<int32_t(int32_t, int32_t)> observer = [this](int32_t userId, int32_t state) -> int32_t {
                auto observer = observers_.find(userId);
                if (observer == observers_.end() || observer->second == nullptr) {
                    return DB_ERROR;
                }
                observer->second(!(state == UNLOCK || state == NO_PWD));
                return OK;
            };
            // retry after 10 second, 10 * 1000 * 1000 mains 1 second
            BlockInteger retry(10 * 1000 * 1000);
            for (; retry < RETRY_MAX_TIMES && !result; ++retry) {
                result = SubscribeUserStatus(observer);
            }

            ZLOGI("Subscribe lock status! retry:%d, result: %d", static_cast<int>(retry), result);
        });
        th.detach();
    } else {
        observers_.insert(std::pair<int32_t, OnAccessControlledEvent>(GetCurrentUserId(), callback));
    }
    return OK;
}

bool Security::IsAccessControlled() const
{
    int curStatus = GetCurrentUserStatus();
    return !(curStatus == UNLOCK || curStatus == NO_PWD);
}

DBStatus Security::SetSecurityOption(const std::string &filePath, const SecurityOption &option)
{
    if (filePath.empty()) {
        return INVALID_ARGS;
    }

    if (!InPathsBox(filePath, DATA_DE) && !InPathsBox(filePath, DATA_CE)) {
        return NOT_SUPPORT;
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

    if (!InPathsBox(filePath, DATA_DE) && !InPathsBox(filePath, DATA_CE)) {
        return NOT_SUPPORT;
    }

    struct stat curStat;
    stat(filePath.c_str(), &curStat);
    if (S_ISDIR(curStat.st_mode)) {
        return GetDirSecurityOption(filePath, option);
    } else {
        return GetFileSecurityOption(filePath, option);
    }
}

DBStatus Security::GetDirSecurityOption(const std::string &filePath, SecurityOption &option) const
{
    return NOT_SUPPORT;
}

DBStatus Security::GetFileSecurityOption(const std::string &filePath, SecurityOption &option) const
{
    return NOT_SUPPORT;
}

DBStatus Security::SetDirSecurityOption(const std::string &filePath, const SecurityOption &option)
{
    return NOT_SUPPORT;
}

DBStatus Security::SetFileSecurityOption(const std::string &filePath, const SecurityOption &option)
{
    if (option.securityLabel == NOT_SET) {
        return OK;
    }
    return NOT_SUPPORT;
}

bool Security::CheckDeviceSecurityAbility(const std::string &devId, const SecurityOption &option) const
{
    ZLOGD("The kv store is null, label:%d", option.securityLabel);
    return GetDeviceNodeByUuid(devId, nullptr) >= option;
}

int32_t Security::GetCurrentUserId() const
{
    std::int32_t uid = getuid();
    return AccountSA::OhosAccountKits::GetInstance().GetDeviceAccountIdByUID(uid);
}

int32_t Security::GetCurrentUserStatus() const
{
    if (!IsSupportSecurity()) {
        return NO_PWD;
    }
    return NO_PWD;
}

bool Security::SubscribeUserStatus(std::function<int32_t(int32_t, int32_t)> &observer) const
{
    return false;
}

const char *Security::Convert2Name(const SecurityOption &option, bool isCE)
{
    if (option.securityLabel <= NOT_SET || option.securityLabel > S4) {
        return nullptr;
    }

    return nullptr;
}

int Security::Convert2Security(const std::string &name)
{
    return NOT_SET;
}

bool Security::IsSupportSecurity()
{
    return false;
}

bool Security::IsFirstInit()
{
    return isInitialized_.exchange(false);
}

bool Security::IsExits(const std::string &file) const
{
    return access(file.c_str(), F_OK) == 0;
}

bool Security::InPathsBox(const std::string &file, const char * const pathsBox[]) const
{
    auto curPath = pathsBox;
    if (curPath == nullptr) {
        return false;
    }
    while ((*curPath) != nullptr) {
        if (file.find(*curPath) == 0) {
            return true;
        }
        curPath++;
    }
    return false;
}

Sensitive Security::GetDeviceNodeByUuid(const std::string &uuid,
                                        const std::function<std::vector<uint8_t>(void)> &getValue)
{
    static std::mutex mutex;
    static std::map<std::string, Sensitive> devicesUdid;
    std::lock_guard<std::mutex> guard(mutex);
    auto it = devicesUdid.find(uuid);
    if (devicesUdid.find(uuid) != devicesUdid.end()) {
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

        Sensitive sensitive(network.GetUdidByNodeId(device.deviceId), 0);
        if (getValue == nullptr) {
            devicesUdid.insert({uuid, std::move(sensitive)});
            return devicesUdid[uuid];
        }

        auto value = getValue();
        sensitive.Unmarshal(value);
        if (!value.empty()) {
            devicesUdid.insert({uuid, std::move(sensitive)});
            return devicesUdid[uuid];
        }

        return sensitive;
    }

    return Sensitive();
}
}
