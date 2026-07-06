/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#define LOG_TAG "LoadConfigFromDataProxyNodeStrategy"

#include "load_config_from_data_proxy_node_strategy.h"

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "bundle_mgr_proxy.h"
#include "common/utils.h"
#include "data_share_profile_config.h"
#include "datashare_errno.h"
#include "log_print.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
using namespace OHOS::DistributedData;
bool LoadConfigFromDataProxyNodeStrategy::operator()(std::shared_ptr<Context> context)
{
    if (!LoadConfigFromUri(context)) {
        return false;
    }
    context->type = PUBLISHED_DATA_TYPE;
#ifdef ACCOUNT_ISOLATION_ENABLED
    ResolveProviderAppIndex(context);
#endif
    if (BundleMgrProxy::GetInstance()->GetBundleInfoFromBMSWithCheck(
        context->calledBundleName, context->visitedUserId, context->bundleInfo) != E_OK) {
        ZLOGE("GetBundleInfoFromBMSWithCheck failed! bundleName: %{public}s", context->calledBundleName.c_str());
        context->errCode = E_BUNDLE_NAME_NOT_EXIST;
        return false;
    }
    if (context->uri.empty()) {
        context->permission = "reject";
        return true;
    }
    std::string uri = context->uri;
    URIUtils::FormatUri(uri);
    for (auto const &hapModuleInfo : context->bundleInfo.hapModuleInfos) {
        auto proxyDatas = hapModuleInfo.proxyDatas;
        for (auto const &proxyData : proxyDatas) {
            if (proxyData.uri != uri) {
                continue;
            }
            context->permission = context->isRead ? proxyData.requiredReadPermission
                                                  : proxyData.requiredWritePermission;
            if (context->permission.empty()) {
                context->permission = "reject";
            }
            auto properties = proxyData.profileInfo;
            if (properties.resultCode == ERROR || properties.resultCode == NOT_FOUND) {
                return true;
            }
            GetContextInfoFromDataProperties(properties.profile, hapModuleInfo.moduleName, context);
            return true;
        }
    }
    if (context->callerBundleName == context->calledBundleName) {
        return true;
    }
    // cross permission can only cross uri like weather,can not cross like datashareproxy://weather
    if (context->isAllowCrossPer && !URIUtils::IsDataProxyURI(context->uri)) {
        ZLOGI("access has white permission, go");
        return true;
    }
    context->errCode = E_URI_NOT_EXIST;
    ZLOGI("not find DataProperties! %{public}s is private", URIUtils::Anonymous(context->uri).c_str());
    return false;
}

bool LoadConfigFromDataProxyNodeStrategy::GetContextInfoFromDataProperties(const ProfileInfo &properties,
    const std::string &moduleName, std::shared_ptr<Context> context)
{
    if (properties.scope == MODULE_SCOPE) {
        // module scope
        context->calledModuleName = moduleName;
    }
    context->calledStoreName = properties.storeName;
    context->calledTableName = properties.tableName;
    context->type = properties.type;
    context->accountIsolation = properties.accountIsolation;
    return true;
}

bool LoadConfigFromDataProxyNodeStrategy::LoadConfigFromUri(std::shared_ptr<Context> context)
{
    if (!URIUtils::GetBundleNameFromProxyURI(context->uri, context->calledBundleName)) {
        return false;
    }
    return true;
}

void LoadConfigFromDataProxyNodeStrategy::ResolveProviderAppIndex(std::shared_ptr<Context> context)
{
    if (context->accountId <= 0) {
        return;
    }
    auto *delegate = AccountDelegate::GetInstance();
    if (delegate == nullptr) {
        ZLOGE("AccountDelegate is null, provider identity skipped");
        return;
    }
    std::vector<int32_t> cloneAppIndexes;
    auto bmsErr = BundleMgrProxy::GetInstance()->GetCloneAppIndexes(
        context->calledBundleName, cloneAppIndexes, context->visitedUserId);
    if (bmsErr != 0) {
        ZLOGW("GetCloneAppIndexes failed, err:%{public}d, bundle:%{public}s, treat as no clone app", bmsErr,
            context->calledBundleName.c_str());
    }
    if (!cloneAppIndexes.empty()) {
        int32_t providerAppIndex =
            delegate->GetAppIndexBySubProfileId(context->visitedUserId, context->accountId);
        if (providerAppIndex >= 0) {
            ZLOGI("account isolation: accountId %{public}d -> appIndex %{public}d, userId %{public}d",
                context->accountId, providerAppIndex, context->visitedUserId);
            context->appIndex = providerAppIndex;
        } else {
            ZLOGE("accountId to appIndex failed, accountId:%{public}d, userId:%{public}d", context->accountId,
                context->visitedUserId);
        }
    }
}
} // namespace OHOS::DataShare