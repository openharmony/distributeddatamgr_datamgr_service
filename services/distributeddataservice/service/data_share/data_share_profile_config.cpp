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

#define LOG_TAG "DataShareProfileConfig"

#include "data_share_profile_config.h"

#include "bundle_mgr_proxy.h"
#include "log_print.h"
#include "uri_utils.h"

namespace OHOS {
namespace DataShare {
bool DataShareProfileConfig::GetProfileInfo(const std::string &calledBundleName, int32_t currentUserId,
    std::map<std::string, ProfileInfo> &profileInfos)
{
    AppExecFwk::BundleInfo bundleInfo;
    if (!BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(calledBundleName, currentUserId, bundleInfo)) {
        ZLOGE("data share GetBundleInfoFromBMS failed! bundleName: %{public}s, currentUserId = %{public}d",
              calledBundleName.c_str(), currentUserId);
        return false;
    }
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type != AppExecFwk::ExtensionAbilityType::DATASHARE) {
            continue;
        }
        std::string info;
        auto ret = RdbBMSAdapter::DataShareProfileInfo::GetResConfigFile(item, info);
        if (!ret) {
            continue;
        }
        ProfileInfo profileInfo;
        if (!profileInfo.Unmarshall(info)) {
            ZLOGE("profileInfo Unmarshall error. infos: %{public}s", info.c_str());
            continue;
        }
        profileInfos[item.uri] = profileInfo;
    }
    return true;
}
} // namespace DataShare
} // namespace OHOS

