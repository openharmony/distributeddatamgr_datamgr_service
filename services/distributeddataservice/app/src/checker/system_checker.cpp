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
#include "system_checker.h"
#include "accesstoken_kit.h"
#include "log_print.h"
namespace OHOS {
namespace DistributedData {
using namespace Security::AccessToken;
__attribute__((used)) SystemChecker SystemChecker::instance_;
SystemChecker::SystemChecker() noexcept
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

std::string SystemChecker::GetAppId(const CheckerManager::StoreInfo &info)
{
    if (!IsValid(info)) {
        return "";
    }
    std::string appId = (trusts_.find(info.bundleName) != trusts_.end()) ? trusts_[info.bundleName] : info.bundleName;
    ZLOGD("bundleName:%{public}s, appId:%{public}s", info.bundleName.c_str(), appId.c_str());
    return appId;
}

bool SystemChecker::IsValid(const CheckerManager::StoreInfo &info)
{
    auto type = AccessTokenKit::GetTokenTypeFlag(info.tokenId);
    return (type == TOKEN_NATIVE || type == TOKEN_SHELL || info.uid == CheckerManager::ROOT_UID);
}
} // namespace DistributedData
} // namespace OHOS