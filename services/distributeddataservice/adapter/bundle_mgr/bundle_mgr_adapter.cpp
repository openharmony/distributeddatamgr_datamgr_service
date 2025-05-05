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

#define LOG_TAG "bundle_mgr_adapter"
#include "bundle_mgr/bundle_mgr_adapter.h"
#include "bundle_mgr_client.h"
#include "log_print.h"
using namespace OHOS::AppExecFwk;
namespace OHOS::DistributedData {
BundleFlag Convert(int32_t type)
{
    switch (type) {
        case BundleMgrAdapter::BUNDLE_DEFAULT:
            return GET_BUNDLE_DEFAULT;
        case BundleMgrAdapter::BUNDLE_WITH_ABILITIES:
            return GET_BUNDLE_WITH_ABILITIES;
        case BundleMgrAdapter::BUNDLE_WITH_REQUESTED_PERMISSION:
            return GET_BUNDLE_WITH_REQUESTED_PERMISSION;
        case BundleMgrAdapter::BUNDLE_WITH_EXTENSION_INFO:
            return GET_BUNDLE_WITH_EXTENSION_INFO;
        case BundleMgrAdapter::BUNDLE_WITH_HASH_VALUE:
            return GET_BUNDLE_WITH_HASH_VALUE;
        case BundleMgrAdapter::BUNDLE_WITH_MENU:
            return GET_BUNDLE_WITH_MENU;
        case BundleMgrAdapter::BUNDLE_WITH_ROUTER_MAP:
            return GET_BUNDLE_WITH_ROUTER_MAP;
        case BundleMgrAdapter::BUNDLE_WITH_SKILL:
            return GET_BUNDLE_WITH_SKILL;
    }
    ZLOGW("invalid type:%{public}d", type);
    return GET_BUNDLE_DEFAULT;
}

BundleMgrAdapter::HapModuleInfo Convert(AppExecFwk::HapModuleInfo &&info)
{
    BundleMgrAdapter::HapModuleInfo res;
    res.hapPath = std::move(info.hapPath);
    return res;
}

BundleMgrAdapter::BundleInfo Convert(AppExecFwk::BundleInfo &&info)
{
    BundleMgrAdapter::BundleInfo res;
    res.moduleDirs = info.moduleDirs;
    res.hapModuleInfos.reserve(info.hapModuleInfos.size());
    for (auto &moduleInfo : info.hapModuleInfos) {
        res.hapModuleInfos.push_back(Convert(std::move(moduleInfo)));
    }
    return res;
}

BundleMgrAdapter &BundleMgrAdapter::GetInstance()
{
    static BundleMgrAdapter instance;
    return instance;
}

std::pair<bool, BundleMgrAdapter::BundleInfo> BundleMgrAdapter::GetBundleInfo(const std::string &bundleName,
    BundleType type, int32_t user)
{
    AppExecFwk::BundleInfo info;
    BundleMgrClient client;
    bool res = client.GetBundleInfo(bundleName, Convert(type), info, user == 0 ? Constants::UNSPECIFIED_USERID : user);
    if (!res) {
        return { res, {} };
    }
    BundleInfo bundleInfo = Convert(std::move(info));
    return { res, bundleInfo };
}

std::string BundleMgrAdapter::GetBundleName(int32_t uid)
{
    BundleMgrClient client;
    std::string bundleName;
    auto code = client.GetNameForUid(uid, bundleName);
    if (code != 0) {
        ZLOGE("failed. uid:%{public}d, code:%{public}d", uid, code);
        return "";
    }
    return bundleName;
}

std::string BundleMgrAdapter::GetAppId(int32_t user, const std::string &bundleName)
{
    if (bundleName.empty()) {
        ZLOGE("invalid args. user:%{public}d, bundleName:%{public}s", user, bundleName.c_str());
        return "";
    }
    AppExecFwk::BundleInfo info;
    BundleMgrClient client;
    auto code = client.GetBundleInfo(bundleName, BundleFlag::GET_BUNDLE_DEFAULT, info,
        user == 0 ? Constants::UNSPECIFIED_USERID : user);
    if (code != 0) {
        ZLOGE("failed. user:%{public}d, bundleName:%{public}s", user, bundleName.c_str());
    }
    return info.appId;
}
} // namespace OHOS::DistributedData