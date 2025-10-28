/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#define LOG_TAG "CommonUtils"
#include "common_utils.h"

#include "accesstoken_kit.h"
#include "config_factory.h"
#include "log_print.h"
#include "tokenid_kit.h"
#include "hiview_fault_adapter.h"

namespace OHOS::DataShare {

bool& DataShareThreadLocal::GetFromSystemApp()
{
    static thread_local bool isFromSystemApp = true;
    return isFromSystemApp;
}

void DataShareThreadLocal::SetFromSystemApp(bool isFromSystemApp)
{
    GetFromSystemApp() = isFromSystemApp;
}

bool DataShareThreadLocal::IsFromSystemApp()
{
    return GetFromSystemApp();
}

void DataShareThreadLocal::CleanFromSystemApp()
{
    SetFromSystemApp(true);
}

bool CheckSystemAbility(uint32_t tokenId)
{
    Security::AccessToken::ATokenTypeEnum tokenType =
        Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId);
    return (tokenType == Security::AccessToken::ATokenTypeEnum::TOKEN_NATIVE ||
        tokenType == Security::AccessToken::ATokenTypeEnum::TOKEN_SHELL);
}

// GetTokenType use tokenId, and IsSystemApp use fullTokenId, these are different
bool CheckSystemCallingPermission(uint32_t tokenId, uint64_t fullTokenId)
{
    if (CheckSystemAbility(tokenId)) {
        return true;
    }
    // IsSystemAppByFullTokenID here is not IPC
    return Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(fullTokenId);
}

// Check if given provider appIdentifier is in providerAllowList
bool ProviderInAllowList(const std::string &appIdentifier)
{
    DistributedData::DataShareConfig *config = DistributedData::ConfigFactory::GetInstance().GetDataShareConfig();
    if (config == nullptr) {
        ZLOGE("allowlist is null");
        return true;
    }
    std::vector<std::string>& identifiersList = config->providerIdentifiers;
    return std::find(identifiersList.begin(), identifiersList.end(), appIdentifier) != identifiersList.end();
}

// Check if caller is system app. If not, check if given provider appIdentifier is in providerAllowList
bool VerifyProvider(const DataProviderConfig::ProviderInfo &providerInfo, const pid_t calllingPid)
{
    // In data_share_service_stub, when OnRemoteRequest receive IPC call, caller has already been
    // checked if it is system app. If it is system app then no need to check if provider is in allowList
    if (DataShareThreadLocal::IsFromSystemApp()) {
        return true;
    }
    if (!ProviderInAllowList(providerInfo.appIdentifier)) {
        // No need to print since app not in AppGallery do not have appIdentifier.
        DataShareFaultInfo faultInfo{HiViewFaultAdapter::unapprovedProvider, providerInfo.bundleName.c_str(),
            providerInfo.moduleName.c_str(), "", __FUNCTION__, -1, "Silent"};
        HiViewFaultAdapter::ReportDataFault(faultInfo);
        // Provider not in allowlist
        return false;
    }
    // Provider in allowlist
    return true;
}
} // namespace OHOS::DataShare
