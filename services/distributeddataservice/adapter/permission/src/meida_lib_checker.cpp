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

#include "meida_lib_checker.h"

#define LOG_TAG "MeidaLibChecker"

#include <memory>
#include "bundlemgr/bundle_mgr_client.h"
#include "log/log_print.h"
#include "utils/crypto.h"
namespace OHOS {
namespace DistributedData {
using namespace AppExecFwk;
MeidaLibChecker MeidaLibChecker::instance_;
constexpr pid_t MeidaLibChecker::SYSTEM_UID;
MeidaLibChecker::MeidaLibChecker() noexcept
{
    CheckerManager::GetInstance().RegisterPlugin(
        "MediaLibraryChecker", [this]() -> auto { return this; });
}

MeidaLibChecker::~MeidaLibChecker()
{}

void MeidaLibChecker::Initialize()
{}

bool MeidaLibChecker::SetTrustInfo(const CheckerManager::Trust &trust)
{
    trusts_[trust.bundleName] = trust.appId;
    return true;
}

std::string MeidaLibChecker::GetAppId(pid_t uid, const std::string &bundleName)
{
    if (!IsValid(uid, bundleName)) {
        return "";
    }
    BundleMgrClient bmsClient;
    std::string orionBundle;
    (void)bmsClient.GetBundleNameForUid(uid, orionBundle);
    auto bundleInfo = std::make_unique<BundleInfo>();
    auto success = bmsClient.GetBundleInfo(bundleName, BundleFlag::GET_BUNDLE_DEFAULT,
                                           *bundleInfo, Constants::ANY_USERID);
    if (!success) {
        return "";
    }
    ZLOGD("orion: %{public}s, uid: %{public}d, bundle: %{public}s appId: %{public}s", orionBundle.c_str(), uid,
        bundleName.c_str(), bundleInfo->appId.c_str());
    return Crypto::Sha256(bundleInfo->appId);
}

bool MeidaLibChecker::IsValid(pid_t uid, const std::string &bundleName)
{
    if (trusts_.find(bundleName) == trusts_.end()) {
        return false;
    }
    if (uid < SYSTEM_UID && uid != CheckerManager::INVALID_UID) {
        return false;
    }

    return true;
}
}
}