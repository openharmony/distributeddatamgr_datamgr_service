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
#define LOG_TAG "ExtensionConnectAdaptor"

#include "extension_connect_adaptor.h"

#include <thread>

#include "app_connect_manager.h"
#include "callback_impl.h"
#include "extension_ability_manager.h"
#include "extension_ability_info.h"
#include "extension_mgr_proxy.h"
#include "log_print.h"
#include "uri_utils.h"

namespace OHOS::DataShare {
ExtensionConnectAdaptor::ExtensionConnectAdaptor() : data_(std::make_shared<BlockData<bool>>(1))
{
    callback_ = new (std::nothrow) CallbackImpl(data_);
}

bool ExtensionConnectAdaptor::DoConnect(const std::string &uri, const std::string &bundleName)
{
    data_->Clear();
    if (callback_ == nullptr) {
        return false;
    }
    ErrCode ret = ExtensionAbilityManager::GetInstance().ConnectExtension(uri, bundleName, callback_->AsObject());
    if (ret != ERR_OK) {
        ZLOGE("connect ability failed, ret = %{public}d, uri: %{public}s", ret,
            URIUtils::Anonymous(uri).c_str());
        return false;
    }
    bool result = data_->GetValue();
    ZLOGI("Do connect, result: %{public}d,  uri: %{public}s", result,
        URIUtils::Anonymous(uri).c_str());
    return result;
}

bool ExtensionConnectAdaptor::TryAndWait(const std::string &uri, const std::string &bundleName,
    int maxWaitTime)
{
    ExtensionConnectAdaptor strategy;
    return AppConnectManager::Wait(
        bundleName, maxWaitTime,
        [&uri, &bundleName, &strategy]() {
            return strategy.DoConnect(uri, bundleName);
        },
        [&strategy]() {
            strategy.Disconnect();
            return true;
        });
}

void ExtensionConnectAdaptor::Disconnect()
{
    if (callback_ == nullptr) {
        ZLOGE("callback null");
        return;
    }
    data_->Clear();
    bool result = data_->GetValue();
    ZLOGI("Do disconnect, result: %{public}d", result);
    callback_ = nullptr;
}
} // namespace OHOS::DataShare