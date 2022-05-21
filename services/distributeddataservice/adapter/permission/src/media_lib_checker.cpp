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
#define LOG_TAG "MediaLibChecker"

#include "media_lib_checker.h"
#include <memory>
#include "accesstoken_kit.h"
#include "bundlemgr/bundle_mgr_client.h"
#include "hap_token_info.h"
#include "log_print.h"
#include "utils/crypto.h"
namespace OHOS {
namespace DistributedData {
using namespace AppExecFwk;
using namespace Security::AccessToken;
__attribute__((used)) MediaLibChecker MediaLibChecker::instance_;
MediaLibChecker::MediaLibChecker() noexcept
{
    CheckerManager::GetInstance().RegisterPlugin(
        "MediaLibraryChecker", [this]() -> auto { return this; });
}

MediaLibChecker::~MediaLibChecker()
{}

void MediaLibChecker::Initialize()
{}

bool MediaLibChecker::SetTrustInfo(const CheckerManager::Trust &trust)
{
    trusts_[trust.bundleName] = trust.appId;
    return true;
}

std::string MediaLibChecker::GetAppId(const CheckerManager::StoreInfo &info)
{
    if (!IsValid(info)) {
        return "";
    }
    BundleMgrClient bmsClient;
    std::string orionBundle;
    (void)bmsClient.GetBundleNameForUid(info.uid, orionBundle);
    auto bundleInfo = std::make_unique<BundleInfo>();
    auto success =
        bmsClient.GetBundleInfo(info.bundleName, BundleFlag::GET_BUNDLE_DEFAULT, *bundleInfo, Constants::ANY_USERID);
    if (!success) {
        return "";
    }
    ZLOGD("orion: %{public}s, bundle: %{public}s appId: %{public}s", orionBundle.c_str(),
        info.bundleName.c_str(), bundleInfo->appId.c_str());
    return Crypto::Sha256(bundleInfo->appId);
}

bool MediaLibChecker::IsValid(const CheckerManager::StoreInfo &info)
{
    if (trusts_.find(info.bundleName) == trusts_.end()) {
        return false;
    }
    return (AccessTokenKit::GetTokenTypeFlag(info.tokenId) == TOKEN_HAP);
}
} // namespace DistributedData
} // namespace OHOS