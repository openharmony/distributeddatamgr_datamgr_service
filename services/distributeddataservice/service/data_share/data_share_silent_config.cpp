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

#define LOG_TAG "DataShareSilentConfig"

#include "data_share_silent_config.h"

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "data_share_service_impl.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "uri_utils.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
const std::string ALL_URI = "*";
bool DataShareSilentConfig::IsSilentProxyEnable(uint32_t callerTokenId, int32_t currentUserId,
    const std::string &bundleName, const std::string &uriStr)
{
    std::string uri = uriStr;
    URIUtils::FormatUri(uri);
    bool isEnable = false;
    if (CheckExistEnableSilentUris(callerTokenId, uri, isEnable) == E_OK) {
        return isEnable;
    }
    std::map<std::string, ProfileInfo> profileInfos;
    DataShareProfileConfig dataShareProfileConfig;
    if (!dataShareProfileConfig.GetProfileInfo(bundleName, currentUserId, profileInfos)) {
        ZLOGE("GetProfileInfo error, No configuration has been set for silent access. uri:%{public}s",
              DistributedData::Anonymous::Change(uri).c_str());
        return true;
    }
    for (const auto& iter : profileInfos) {
        if (iter.first == uri) {
            return iter.second.isSilentProxyEnable;
        }
    }
    return true;
}

bool DataShareSilentConfig::EnableSilentProxy(uint32_t callerTokenId, const std::string &uriStr, bool enable)
{
    std::string uri = uriStr;
    URIUtils::FormatUri(uri);
    if (uri.empty()) {
        enableSilentUris_.Erase(callerTokenId);
        uri = ALL_URI;
    }
    enableSilentUris_.Compute(callerTokenId, [&enable, &uri](const uint32_t &key,
        std::map<std::string, bool> &uris) {
        uris[uri] = enable;
        return !uris.empty();
    });
    return true;
}

int DataShareSilentConfig::CheckExistEnableSilentUris(uint32_t callerTokenId,
    const std::string &uri, bool &isEnable)
{
    int status = ERROR;
    enableSilentUris_.ComputeIfPresent(callerTokenId, [&isEnable, &status, &uri](const uint32_t &key,
        const std::map<std::string, bool> &uris) {
        auto iter = uris.find(uri);
        if (iter != uris.end()) {
            isEnable = iter->second;
            status = E_OK;
            return true;
        }
        iter = uris.find(ALL_URI);
        if (iter != uris.end()) {
            isEnable = iter->second;
            status = E_OK;
            return true;
        }
        return !uris.empty();
    });
    return status;
}
} // namespace OHOS::DataShare
