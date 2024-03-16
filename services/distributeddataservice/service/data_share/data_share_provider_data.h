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
#ifndef DATASHARESERVICE_DATA_SHARE_PROVIDER_DATA_H
#define DATASHARESERVICE_DATA_SHARE_PROVIDER_DATA_H

#include <map>
#include <string>

#include "account/account_delegate.h"
#include "bundle_mgr_proxy.h"
#include "data_share_profile_info.h"
#include "hap_module_info.h"
#include "uri_utils.h"

namespace OHOS::DataShare {
using namespace OHOS::RdbBMSAdapter;
using BundleInfo = OHOS::AppExecFwk::BundleInfo;
using ExtensionAbility = OHOS::AppExecFwk::ExtensionAbilityInfo;
constexpr const char PROXY_URI_SCHEMA[] = "datashareproxy";
class ProviderInfo {
public:
    ProviderInfo() = default;
    ProviderInfo(const std::string &silentUri, uint32_t &callerTokenId)
    {
        uri = silentUri;
        currentUserId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(callerTokenId);
        if (currentUserId == 0) {
            URIUtils::FormatUri(uri);
        }
        isProxyUri = std::string(PROXY_URI_SCHEMA) == Uri(uri).GetScheme();
    }
    virtual ~ProviderInfo() = default;
    std::string uri;
    int32_t currentUserId = -1;
    std::string bundleName;
    std::string moduleName;
    std::string storeName;
    std::string tableName;
    std::string readPermission;
    std::string writePermission;
    std::string hapPath;
    std::string resourcePath;
    bool isProxyUri = false;
};

class DataShareProvider {
public:
    int GetProviderInfo(ProviderInfo &providerInfo);
private:
    bool GetProviderInfoFromUriPath(ProviderInfo &providerInfo);

    int GetProviderInfoFromProxyData(ProviderInfo &providerInfo);
    
    int GetProviderInfoFromExtension(ProviderInfo &providerInfo);
    enum PATH_PARAM : int32_t {
        BUNDLE_NAME = 0,
        MODULE_NAME,
        STORE_NAME,
        TABLE_NAME,
        PARAM_SIZE
    };
};
} // namespace OHOS::DataShare
#endif
