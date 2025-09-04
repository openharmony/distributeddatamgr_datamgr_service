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

#include "bundle_mgr_proxy.h"
#include "common/utils.h"
#include "data_share_profile_config.h"
#include "datashare_errno.h"
#include "log_print.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
bool LoadConfigFromDataProxyNodeStrategy::operator()(std::shared_ptr<Context> context)
{
    if (!LoadConfigFromUri(context)) {
        return false;
    }
    context->type = PUBLISHED_DATA_TYPE;
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
    return true;
}

bool LoadConfigFromDataProxyNodeStrategy::LoadConfigFromUri(std::shared_ptr<Context> context)
{
    if (!URIUtils::GetBundleNameFromProxyURI(context->uri, context->calledBundleName)) {
        return false;
    }
    return true;
}
} // namespace OHOS::DataShare