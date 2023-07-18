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
#define LOG_TAG "GetDataStrategy"

#include "get_data_strategy.h"

#include "accesstoken_kit.h"
#include "data_proxy/load_config_from_data_proxy_node_strategy.h"
#include "general/load_config_common_strategy.h"
#include "log_print.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
Data GetDataStrategy::Execute(std::shared_ptr<Context> context, int &errorCode)
{
    auto &preProcess = GetStrategy();
    if (preProcess.IsEmpty()) {
        ZLOGE("get strategy fail, maybe memory not enough");
        errorCode = E_ERROR;
        return Data();
    }
    if (!preProcess(context)) {
        ZLOGE("pre process fail, uri: %{public}s", DistributedData::Anonymous::Change(context->uri).c_str());
        errorCode = context->errCode;
        return Data();
    }
    auto result = PublishedData::Query(context->calledBundleName, context->currentUserId);
    Data data;
    for (auto &item : result) {
        if (!CheckPermission(context, item.value.key)) {
            ZLOGI("uri: %{private}s not allowed", context->uri.c_str());
            continue;
        }
        if (item.GetVersion() > data.version_) {
            data.version_ = item.GetVersion();
        }
        data.datas_.emplace_back(
            item.value.key, item.value.subscriberId, PublishedDataNode::MoveTo(item.value.value));
    }
    return data;
}

SeqStrategy &GetDataStrategy::GetStrategy()
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (!strategies_.IsEmpty()) {
        return strategies_;
    }
    std::initializer_list<Strategy *> list = {
        new (std::nothrow) LoadConfigCommonStrategy(),
        new (std::nothrow) LoadConfigFromDataProxyNodeStrategy()
    };
    auto ret = strategies_.Init(list);
    if (!ret) {
        std::for_each(list.begin(), list.end(), [](Strategy *item) {
            delete item;
        });
        return strategies_;
    }
    return strategies_;
}

bool GetDataStrategy::CheckPermission(std::shared_ptr<Context> context, const std::string &key)
{
    if (context->callerBundleName == context->calledBundleName) {
        ZLOGI("access private data, caller and called is same, go");
        return true;
    }
    for (const auto &moduleInfo : context->bundleInfo.hapModuleInfos) {
        auto proxyDatas = moduleInfo.proxyDatas;
        for (auto &proxyData : proxyDatas) {
            if (proxyData.uri != key) {
                continue;
            }
            if (proxyData.requiredReadPermission.empty()) {
                ZLOGE("no read permission");
                return false;
            }
            int status = Security::AccessToken::AccessTokenKit::VerifyAccessToken(
                context->callerTokenId, proxyData.requiredReadPermission);
            if (status != Security::AccessToken::PermissionState::PERMISSION_GRANTED) {
                ZLOGE("Verify permission denied! callerTokenId:%{public}u permission:%{public}s",
                    context->callerTokenId, proxyData.requiredReadPermission.c_str());
                return false;
            }
            return true;
        }
    }
    return false;
}
} // namespace OHOS::DataShare