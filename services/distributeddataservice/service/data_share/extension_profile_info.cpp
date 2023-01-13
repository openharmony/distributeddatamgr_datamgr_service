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
 
#define LOG_TAG "ExtensionProfileInfo"
#include "extension_profile_info.h"

#include "bundle_info.h"
#include "bundlemgr/bundle_mgr_client.h"
#include "log_print.h"

namespace OHOS::DataShare {
namespace {
    constexpr const char* METADATA_NAME = "ohos.extension.dataShare";
}
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

bool ExtensionProfileInfo::LoadProfileInfoFromExtension(UriInfo &uriInfo, ProfileInfo &profileInfo,
    bool &isSingleApp, AppExecFwk::BundleInfo &bundleInfo)
{
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
} // namespace OHOS::DataShare