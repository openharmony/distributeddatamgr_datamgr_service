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
#define LOG_TAG "LoadConfigCommonStrategy"
#include "load_config_common_strategy.h"

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "uri_utils.h"

namespace OHOS::DataShare {
constexpr const char USER_PARAM[] = "user";
constexpr const char TOKEN_ID_PARAM[] = "srcToken";
constexpr const char DST_BUNDLE_NAME_PARAM[] = "dstBundleName";
bool LoadConfigCommonStrategy::operator()(std::shared_ptr<Context> context)
{
    if (context->callerTokenId == 0) {
        context->callerTokenId = IPCSkeleton::GetCallingTokenID();
    }
    context->currentUserId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(context->callerTokenId);
    // sa, userId is in uri, caller token id is from first caller tokenId
    if (context->currentUserId == 0) {
        GetInfoFromProxyURI(
            context->uri, context->currentUserId, context->callerTokenId, context->calledBundleName);
        URIUtils::FormatUri(context->uri);
    }
    if (context->needAutoLoadCallerBundleName && context->callerBundleName.empty()) {
        Security::AccessToken::HapTokenInfo tokenInfo;
        auto result = Security::AccessToken::AccessTokenKit::GetHapTokenInfo(context->callerTokenId, tokenInfo);
        if (result != Security::AccessToken::RET_SUCCESS) {
            ZLOGE("token:0x%{public}x, result:%{public}d", context->callerTokenId, result);
            return false;
        }
        context->callerBundleName = tokenInfo.bundleName;
    }
    return true;
}

bool LoadConfigCommonStrategy::GetInfoFromProxyURI(
    const std::string &uri, int32_t &user, uint32_t &callerTokenId, std::string &calledBundleName)
{
    auto queryParams = URIUtils::GetQueryParams(uri);
    if (!queryParams[USER_PARAM].empty()) {
        auto [success, data] = URIUtils::Strtoul(queryParams[USER_PARAM]);
        if (!success) {
            return false;
        }
        user = static_cast<int32_t>(std::move(data));
    }
    if (!queryParams[TOKEN_ID_PARAM].empty()) {
        auto [success, data] = URIUtils::Strtoul(queryParams[TOKEN_ID_PARAM]);
        if (!success) {
            return false;
        }
        callerTokenId = std::move(data);
    }
    if (!queryParams[DST_BUNDLE_NAME_PARAM].empty()) {
        calledBundleName = queryParams[DST_BUNDLE_NAME_PARAM];
    }
    return true;
}
} // namespace OHOS::DataShare