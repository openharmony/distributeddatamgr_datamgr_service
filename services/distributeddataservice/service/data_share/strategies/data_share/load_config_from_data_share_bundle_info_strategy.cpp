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
#define LOG_TAG "LoadConfigFromDataShareBundleInfoStrategy"

#include "load_config_from_data_share_bundle_info_strategy.h"

#include "bundle_mgr_proxy.h"
#include "data_share_profile_config.h"
#include "log_print.h"
#include "uri_utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
struct ConfigData {
    constexpr static int8_t TABLE_MATCH_PRIORITY = 3;
    constexpr static int8_t STORE_MATCH_PRIORITY = 2;
    constexpr static int8_t COMMON_MATCH_PRIORITY = 1;
    constexpr static int8_t UNDEFINED_PRIORITY = -1;
    ConfigData() : crossMode_(AccessSystemMode::UNDEFINED, UNDEFINED_PRIORITY) {}
    void SetCrossUserMode(uint8_t priority, uint8_t crossMode)
    {
        if (crossMode_.second < priority && crossMode > AccessSystemMode::UNDEFINED &&
            crossMode < AccessSystemMode::MAX) {
            crossMode_.first = static_cast<AccessSystemMode>(crossMode);
            crossMode_.second = priority;
        }
    }
    void FillIntoContext(std::shared_ptr<Context> context)
    {
        if (crossMode_.second != ConfigData::UNDEFINED_PRIORITY) {
            context->accessSystemMode = crossMode_.first;
        }
    }

private:
    std::pair<AccessSystemMode, int8_t> crossMode_;
};

bool LoadConfigFromDataShareBundleInfoStrategy::LoadConfigFromProfile(
    const ProfileInfo &profileInfo, std::shared_ptr<Context> context)
{
    std::string storeUri = URIUtils::DATA_SHARE_SCHEMA + context->calledBundleName + "/" + context->calledModuleName +
        "/" + context->calledStoreName;
    std::string tableUri = storeUri + "/" + context->calledTableName;
    ConfigData result;
    for (auto const &item : profileInfo.tableConfig) {
        if (item.uri == tableUri) {
            result.SetCrossUserMode(ConfigData::TABLE_MATCH_PRIORITY, item.crossUserMode);
            continue;
        }
        if (item.uri == storeUri) {
            result.SetCrossUserMode(ConfigData::STORE_MATCH_PRIORITY, item.crossUserMode);
            continue;
        }
        if (item.uri == "*") {
            result.SetCrossUserMode(ConfigData::COMMON_MATCH_PRIORITY, item.crossUserMode);
            continue;
        }
    }
    result.FillIntoContext(context);
    return true;
}

bool LoadConfigFromDataShareBundleInfoStrategy::operator()(std::shared_ptr<Context> context)
{
    if (!LoadConfigFromUri(context)) {
        ZLOGE("LoadConfigFromUri failed! bundleName: %{public}s", context->calledBundleName.c_str());
        return false;
    }
    if (!BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(
        context->calledBundleName, context->currentUserId, context->bundleInfo)) {
        ZLOGE("GetBundleInfoFromBMS failed! bundleName: %{public}s", context->calledBundleName.c_str());
        return false;
    }
    for (auto &item : context->bundleInfo.extensionInfos) {
        if (item.type == AppExecFwk::ExtensionAbilityType::DATASHARE) {
            context->permission = context->isRead ? item.readPermission : item.writePermission;

            auto [ret, profileInfo] = DataShareProfileConfig::GetDataProperties(item.metadata,
                item.resourcePath, item.hapPath, DATA_SHARE_EXTENSION_META);
            if (ret == NOT_FOUND) {
                return true; // optional meta data config
            }
            if (ret == ERROR) {
                ZLOGE("parse failed! %{public}s", context->calledBundleName.c_str());
                return false;
            }
            LoadConfigFromProfile(profileInfo, context);
            return true;
        }
    }
    return false;
}
bool LoadConfigFromDataShareBundleInfoStrategy::LoadConfigFromUri(std::shared_ptr<Context> context)
{
    UriInfo uriInfo;
    if (!URIUtils::GetInfoFromURI(context->uri, uriInfo)) {
        return false;
    }
    context->calledBundleName = std::move(uriInfo.bundleName);
    context->calledModuleName = std::move(uriInfo.moduleName);
    context->calledStoreName = std::move(uriInfo.storeName);
    context->calledTableName = std::move(uriInfo.tableName);
    return true;
}
} // namespace OHOS::DataShare