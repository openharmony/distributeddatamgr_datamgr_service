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

#include "account/account_delegate.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "uri_utils.h"

namespace OHOS::DataShare {
bool LoadConfigCommonStrategy::operator()(std::shared_ptr<Context> context)
{
    context->callerTokenId = IPCSkeleton::GetCallingTokenID();
    context->currentUserId = DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(context->callerTokenId);
    // single app, userId is in uri
    if (context->currentUserId == 0) {
        URIUtils::GetUserIdFromProxyURI(context->uri, context->currentUserId);
        ZLOGI("user uri's userId %{public}d", context->currentUserId);
    }
    FormatUri(context->uri);
    return true;
}

void LoadConfigCommonStrategy::FormatUri(std::string &uri)
{
    auto pos = uri.find('?');
    if (pos == std::string::npos) {
        return;
    }

    uri = uri.substr(0, pos);
}
} // namespace OHOS::DataShare