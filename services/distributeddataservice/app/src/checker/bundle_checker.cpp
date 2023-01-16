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
#define LOG_TAG "BundleChecker"

#include "bundle_checker.h"
#include <memory>
#include "accesstoken_kit.h"
#include "hap_token_info.h"
#include "log_print.h"
#include "utils/crypto.h"
namespace OHOS {
namespace DistributedData {
using namespace Security::AccessToken;
__attribute__((used)) BundleChecker BundleChecker::instance_;
BundleChecker::BundleChecker() noexcept
{
    CheckerManager::GetInstance().RegisterPlugin(
        "BundleChecker", [this]() -> auto { return this; });
}

BundleChecker::~BundleChecker()
{
}

void BundleChecker::Initialize()
{
}

bool BundleChecker::SetTrustInfo(const CheckerManager::Trust &trust)
{
    trusts_[trust.bundleName] = trust.appId;
    return true;
}

std::string BundleChecker::GetAppId(const CheckerManager::StoreInfo &info)
{
    if (AccessTokenKit::GetTokenTypeFlag(info.tokenId) != TOKEN_HAP) {
        return "";
    }

    HapTokenInfo tokenInfo;
    if (AccessTokenKit::GetHapTokenInfo(info.tokenId, tokenInfo) != RET_SUCCESS) {
        return "";
    }

    if (!info.bundleName.empty() && tokenInfo.bundleName != info.bundleName) {
        return "";
    }

    auto it = trusts_.find(info.bundleName);
    if (it != trusts_.end() && (it->second == tokenInfo.appID)) {
        return info.bundleName;
    }

    ZLOGD("bundleName:%{public}s, appId:%{public}s", info.bundleName.c_str(), tokenInfo.appID.c_str());
    return Crypto::Sha256(tokenInfo.appID);
}

bool BundleChecker::IsValid(const CheckerManager::StoreInfo &info)
{
    if (AccessTokenKit::GetTokenTypeFlag(info.tokenId) != TOKEN_HAP) {
        return false;
    }

    HapTokenInfo tokenInfo;
    if (AccessTokenKit::GetHapTokenInfo(info.tokenId, tokenInfo) != RET_SUCCESS) {
        return false;
    }

    return tokenInfo.bundleName == info.bundleName;
}
} // namespace DistributedData
} // namespace OHOS