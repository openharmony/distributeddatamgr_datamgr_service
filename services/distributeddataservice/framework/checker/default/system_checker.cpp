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
#define LOG_TAG "SystemChecker"
#include "checker/default/system_checker.h"
#include "log/log_print.h"
namespace OHOS {
namespace DistributedData {
SystemChecker SystemChecker::instance_;
constexpr pid_t SystemChecker::SYSTEM_UID;
SystemChecker::SystemChecker()
{
    CheckerManager::GetInstance().RegisterPlugin(
        "SystemChecker", [this]() -> auto { return this; });
}

SystemChecker::~SystemChecker()
{
}

void SystemChecker::Initialize()
{
}

bool SystemChecker::SetTrustInfo(const CheckerManager::Trust &trust)
{
    trusts_[trust.bundleName] = trust.appId;
    return true;
}

std::string SystemChecker::GetAppId(pid_t uid, const std::string &bundleName)
{
    if (trusts_.find(bundleName) == trusts_.end() || uid >= SYSTEM_UID || uid == CheckerManager::INVALID_UID) {
        return "";
    }
    ZLOGD("bundleName:%{public}s, uid:%{public}d, appId:%{public}s", bundleName.c_str(), uid,
        trusts_[bundleName].c_str());
    return trusts_[bundleName];
}

bool SystemChecker::IsValid(pid_t uid, const std::string &bundleName)
{
    if (trusts_.find(bundleName) == trusts_.end() || uid >= SYSTEM_UID || uid == CheckerManager::INVALID_UID) {
        return false;
    }
    // todo get appid
    return true;
}
}
}