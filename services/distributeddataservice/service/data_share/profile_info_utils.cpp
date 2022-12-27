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
#include "bundle_mgr_proxy.h"
#include "bundlemgr/bundle_mgr_client.h"
#include "log_print.h"
#include "nlohmann/json.hpp"

namespace OHOS::DataShare {
namespace {
    const std::string KEY_GLOBAL_CONFIG = "*";
    const std::string METADATA_NAME = "ohos.extension.dataShare";
    const std::string CROSS_USER_MODE = "crossUserMode";
}

class JsonUtils {
public:
    static bool GetUserModeFromJson(const nlohmann::json &json, ProfileInfo &profileInfo, const std::string &key)
    {
        if (json.find(key) == json.end()) {
            ZLOGW("not find key: %{public}s.", key.c_str());
            return true;
        }

        if (!json.at(key).is_object()) {
            ZLOGE("json[%{public}s] must be object!", key.c_str());
            return false;
        }

        if (json[key].find(CROSS_USER_MODE) != json[key].end()) {
            if (!json[key].at(CROSS_USER_MODE).is_number_integer()) {
                ZLOGE("the value of crossUserMode must be integer!");
                return false;
            }
            profileInfo.crossUserMode = json[key].at(CROSS_USER_MODE).get<int>();
        }
        return true;
    }

    static bool ParseProfileInfoFromJson(std::string &jsonStr, UriInfo &uriInfo, ProfileInfo &profileInfo)
    {
        nlohmann::json sourceJson = nlohmann::json::parse(jsonStr, nullptr, false);
        if (sourceJson.is_discarded()) {
            ZLOGE("json::parse error!");
            return false;
        }

        if (!GetUserModeFromJson(sourceJson, profileInfo, KEY_GLOBAL_CONFIG)) {
            ZLOGE("json::parse global config error!");
            return false;
        }

        std::string storeConfigKey = uriInfo.storeName;
        if (!GetUserModeFromJson(sourceJson, profileInfo, storeConfigKey)) {
            ZLOGE("json::parse store config error!");
            return false;
        }

        std::string tableConfigKey = uriInfo.storeName + "/" + uriInfo.tableName;
        if (!GetUserModeFromJson(sourceJson, profileInfo, tableConfigKey)) {
            ZLOGE("json::parse table config error!");
            return false;
        }
        return true;
    }
};

BundleMgrProxy PermissionProxy::bmsProxy_;
bool ProfileInfoUtils::LoadProfileInfoFromExtension(UriInfo &uriInfo, uint32_t tokenId,
    ProfileInfo &profileInfo)
{
    AppExecFwk::BundleInfo bundleInfo;
    if (!bmsProxy_.GetBundleInfoFromBMS(uriInfo.bundleName, tokenId, bundleInfo)) {
        ZLOGE("GetBundleInfoFromBMS failed!");
        return false;
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
            return JsonUtils::ParseProfileInfoFromJson(infos[0], uriInfo, profileInfo);
        }
    }
    ZLOGE("not find datashare extension!");
    return false;
}
} // namespace OHOS::DataShare