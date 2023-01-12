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
 
#define LOG_TAG "ProfileInfoUtils"
#include "profile_info_utils.h"

#include "bundle_info.h"
#include "bundlemgr/bundle_mgr_client.h"
#include "log_print.h"

namespace OHOS::DataShare {
BundleMgrProxy ProfileInfoUtils::bmsProxy_;
bool Config::Marshal(json &node) const
{
    SetValue(node[GET_NAME(scope)], scope);
    SetValue(node[GET_NAME(crossUserMode)], crossUserMode);
    SetValue(node[GET_NAME(readPermission)], readPermission);
    SetValue(node[GET_NAME(writePermission)], writePermission);
    return true;
}

bool Config::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(scope), scope);
    GetValue(node, GET_NAME(crossUserMode), crossUserMode);
    GetValue(node, GET_NAME(readPermission), readPermission);
    GetValue(node, GET_NAME(writePermission), writePermission);
    return true;
}


bool ProfileInfo::Marshal(json &node) const
{
    SetValue(node[GET_NAME(tablesConfig)], tablesConfig);
    return true;
}

bool ProfileInfo::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(tablesConfig), tablesConfig);
    return true;
}

bool ProfileInfoUtils::LoadProfileInfoFromExtension(UriInfo &uriInfo, uint32_t tokenId,
    ProfileInfo &profileInfo, bool &isSingleApp)
{
    AppExecFwk::BundleInfo bundleInfo;
    if (!bmsProxy_.GetBundleInfoFromBMS(uriInfo.bundleName, tokenId, bundleInfo)) {
        ZLOGE("GetBundleInfoFromBMS failed!");
        return false;
    }
    isSingleApp = bundleInfo.singleton;
    // non singleApp don't need get profileInfo
    if (!isSingleApp) {
        return true;
    }

    AppExecFwk::BundleMgrClient client;
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type == AppExecFwk::ExtensionAbilityType::DATASHARE) {
            std::vector<std::string> infos;
            auto ret = client.GetResConfigFile(item, METADATA_NAME, infos);
            if (!ret) {
                ZLOGE("GetProfileFromExtension failed!");
                return false;
            }
            return profileInfo.Unmarshall(infos[0]);
        }
    }
    ZLOGE("not find datashare extension!");
    return false;
}

bool ProfileInfoUtils::CheckCrossUserMode(ProfileInfo &profileInfo, UriInfo &uriInfo, int32_t userId,
    const bool isSingleApp)
{
    if (!isSingleApp) {
        return true;
    }

    int crossUserMode = 0;
    for (auto &item : profileInfo.tablesConfig) {
        if (item.scope == "*") {
            crossUserMode = item.crossUserMode;
        }
    }

    for (auto &item : profileInfo.tablesConfig) {
        if (item.scope == uriInfo.storeName) {
            crossUserMode = item.crossUserMode;
        }
    }

    std::string tableKey = uriInfo.storeName + "/" + uriInfo.tableName;
    for (auto &item : profileInfo.tablesConfig) {
        if (item.scope == tableKey) {
            crossUserMode = item.crossUserMode;
        }
    }

    if (crossUserMode != USERMODE_SHARED && crossUserMode != USERMODE_UNIQUE) {
        return false;
    }
    if (crossUserMode == USERMODE_UNIQUE) {
        uriInfo.tableName.append("_").append(std::to_string(userId));
    }
    return true;
}
} // namespace OHOS::DataShare