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

#include "datashare_errno.h"

#include "bundle_mgr_proxy.h"
#include "common/uri_utils.h"
#include "log_print.h"

namespace OHOS::DataShare {
bool LoadConfigFromDataProxyNodeStrategy::operator()(std::shared_ptr<Context> context)
{
    if (!LoadConfigFromUri(context)) {
        return false;
    }
    context->type = DataProperties::PUBLISHED_DATA_TYPE;
    ZLOGE("hanlu enter %{public}s %{public}d", context->calledBundleName.c_str(), context->callerTokenId);
    if (!BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(
            context->calledBundleName, context->currentUserId, context->bundleInfo)) {
        ZLOGE("GetBundleInfoFromBMS failed! bundleName: %{public}s", context->calledBundleName.c_str());
        return false;
    }
    for (auto &hapModuleInfo : context->bundleInfo.hapModuleInfos) {
        ZLOGE("hanlu uri: %{public}d", hapModuleInfo.proxyDatas.size());
        auto proxyDatas = hapModuleInfo.proxyDatas;
        for (auto &proxyData : proxyDatas) {
            ZLOGE("hanlu uri: %{public}s inputuri:%{public}s", proxyData.uri.c_str(), context->uri.c_str());
            if (proxyData.uri != context->uri) {
                continue;
            }
            context->permission = context->isRead ? proxyData.requiredReadPermission
                                                  : proxyData.requiredWritePermission;
            if (context->permission.empty()) {
                context->permission = "reject";
            }
            DataProperties properties;
            bool isCompressed = !hapModuleInfo.hapPath.empty();
            std::string resourcePath = isCompressed ? hapModuleInfo.hapPath : hapModuleInfo.resourcePath;
            if (!DataShareProfileInfo::GetDataPropertiesFromProxyDatas(
                    proxyData, resourcePath, isCompressed, properties)) {
                return true;
            }
            GetContextInfoFromDataProperties(properties, hapModuleInfo.moduleName, context);
            ZLOGE("hanlu calledStoreName: %{public}s %{public}s", context->calledStoreName.c_str(),
                context->calledTableName.c_str());
            return true;
        }
    }
    if (context->callerBundleName == context->calledBundleName) {
        ZLOGI("access private data, caller and called is same, go");
        return true;
    }
    context->errCode = E_URI_NOT_EXIST;
    ZLOGI("not find DataProperties! %{private}s is private", context->uri.c_str());
    return false;
}

bool LoadConfigFromDataProxyNodeStrategy::GetContextInfoFromDataProperties(const DataProperties &properties,
    const std::string &moduleName, std::shared_ptr<Context> context)
{
    if (properties.scope == DataProperties::MODULE_SCOPE) {
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
    ZLOGE("hanlu enter %{publis}s", context->calledBundleName.c_str());
    return true;
}
} // namespace OHOS::DataShare