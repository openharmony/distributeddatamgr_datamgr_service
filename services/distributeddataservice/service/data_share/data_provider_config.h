/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef DATASHARESERVICE_DATA_PROVIDER_CONFIG_H
#define DATASHARESERVICE_DATA_PROVIDER_CONFIG_H

#include <map>
#include <string>

#include "account/account_delegate.h"
#include "bundle_mgr_proxy.h"
#include "data_share_profile_config.h"
#include "hap_module_info.h"
#include "uri_utils.h"

namespace OHOS::DataShare {
using ExtensionAbility = OHOS::AppExecFwk::ExtensionAbilityInfo;
class DataProviderConfig {
public:
    DataProviderConfig(const std::string &uri, uint32_t callerTokenId);
    
    struct ProviderInfo {
        std::string uri;
        int32_t currentUserId = -1;
        int32_t visitedUserId = -1;
        int32_t appIndex = 0;   // appIndex is in [1, 1000], and original app's index is 0
        std::string bundleName;
        std::string moduleName;
        std::string storeName;
        std::string tableName;
        std::string readPermission;
        std::string writePermission;
        std::string acrossAccountsPermission = "ohos.permission.INTERACT_ACROSS_LOCAL_ACCOUNTS";
        std::string type = "rdb";
        std::string backup;
        std::string extensionUri;
        bool isFromExtension = false;
        bool singleton = false;
        bool hasExtension = false;
        bool allowEmptyPermission = false;
        bool storeMetaDataFromUri = false;
        AccessCrossMode accessCrossMode = AccessCrossMode::USER_UNDEFINED;
        std::vector<AllowList> allowLists;
    };

    std::pair<int, ProviderInfo> GetProviderInfo();
private:
    bool GetFromUriPath();
    int GetFromProxyData();
    int GetFromExtension();
    int GetFromDataProperties(const ProfileInfo &profileInfo, const std::string &moduleName);
    int GetFromExtensionProperties(const ProfileInfo &profileInfo, const std::string &moduleName);
    void GetMetaDataFromUri();
    std::pair<int, BundleConfig> GetBundleInfo();
    bool IsInExtList(const std::string &bundleName);
    enum class PATH_PARAM : int32_t {
        BUNDLE_NAME = 0,
        MODULE_NAME,
        STORE_NAME,
        TABLE_NAME,
        PARAM_SIZE
    };
    ProviderInfo providerInfo_;
    UriConfig uriConfig_;
    static constexpr const char *MODULE_SCOPE = "module";
    static constexpr const char *DATA_SHARE_EXTENSION_META = "ohos.extension.dataShare";
    static constexpr const char *DATA_SHARE_PROPERTIES_META = "dataProperties";
};
} // namespace OHOS::DataShare
#endif
