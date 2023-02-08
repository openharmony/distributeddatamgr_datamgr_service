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
 
#define LOG_TAG "DataShareProfileInfo"
#include "data_share_profile_info.h"

#include "bundle_info.h"
#include "bundlemgr/bundle_mgr_client.h"
#include "log_print.h"

namespace OHOS::DataShare {
namespace {
    constexpr const char* METADATA_NAME = "ohos.extension.dataShare";
}
bool Config::Marshal(json &node) const
{
    SetValue(node[GET_NAME(uri)], uri);
    SetValue(node[GET_NAME(crossUserMode)], crossUserMode);
    SetValue(node[GET_NAME(readPermission)], readPermission);
    SetValue(node[GET_NAME(writePermission)], writePermission);
    return true;
}

bool Config::Unmarshal(const json &node)
{
    bool ret = true;
    ret = GetValue(node, GET_NAME(uri), uri) && ret;
    GetValue(node, GET_NAME(crossUserMode), crossUserMode);
    GetValue(node, GET_NAME(readPermission), readPermission);
    GetValue(node, GET_NAME(writePermission), writePermission);
    return ret;
}

bool ProfileInfo::Marshal(json &node) const
{
    SetValue(node[GET_NAME(tableConfig)], tableConfig);
    return true;
}

bool ProfileInfo::Unmarshal(const json &node)
{
    bool ret = true;
    GetValue(node, GET_NAME(tableConfig), tableConfig) && ret;
    return ret;
}

bool DataShareProfileInfo::LoadProfileInfoFromExtension(const AppExecFwk::BundleInfo &bundleInfo,
    ProfileInfo &profileInfo, bool &isSingleApp)
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