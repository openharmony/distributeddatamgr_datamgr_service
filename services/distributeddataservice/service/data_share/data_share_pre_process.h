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
#ifndef DATASHARESERVICE_DATA_SHARE_PRE_PROCESS_H
#define DATASHARESERVICE_DATA_SHARE_PRE_PROCESS_H

#include <map>
#include <string>

#include "bundle_mgr_proxy.h"
#include "context.h"
#include "data_share_profile_info.h"
#include "hap_module_info.h"

namespace OHOS::DataShare {
using namespace OHOS::RdbBMSAdapter;
using BundleInfo = OHOS::AppExecFwk::BundleInfo;
using ExtensionAbility = OHOS::AppExecFwk::ExtensionAbilityInfo;
using Metadata = OHOS::AppExecFwk::Metadata;
class DataShareProvider {
public:
    std::string uri;
    std::string readPermission;
    std::string writePermission;
    std::string hapPath;
    std::string resourcePath;
    std::string moduleName;
};

class DataSharePreProcess {
public:
    int RegisterObserverProcess(std::shared_ptr<CalledInfo> calledInfo);
private:
    bool GetCalledInfoFromUri(std::shared_ptr<CalledInfo> calledInfo);

    void GetCallerInfoFromUri(uint32_t &callerTokenId, std::shared_ptr<CalledInfo> calledInfo);

    int GetProviderInfoFromProxyData(std::shared_ptr<CalledInfo> calledInfo, DataShareProvider &providerInfo);
    
    int GetProviderInfoFromExtension(std::shared_ptr<CalledInfo> calledInfo, DataShareProvider &providerInfo);

    int GetProviderInfoFromUri(std::shared_ptr<CalledInfo> calledInfo, DataShareProvider &providerInfo);

    bool VerifyPermission(const std::string &permission, const uint32_t callerTokenId,
        std::shared_ptr<CalledInfo> calledInfo);
};
} // namespace OHOS::DataShare
#endif
