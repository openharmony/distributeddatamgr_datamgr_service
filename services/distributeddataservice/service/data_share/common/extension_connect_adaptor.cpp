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
#include "log_print.h"

namespace OHOS::DataShare {
bool ExtensionConnectAdaptor::Connect(std::shared_ptr<Context> context)
{
    for (auto const &extension : context->bundleInfo.extensionInfos) {
        if (extension.type == AppExecFwk::ExtensionAbilityType::DATASHARE) {
            return DoConnect(context);
        }
    }
    return false;
}

ExtensionConnectAdaptor::ExtensionConnectAdaptor() : data_(1) {}

bool ExtensionConnectAdaptor::DoConnect(std::shared_ptr<Context> context)
{
    AAFwk::Want want;
    want.SetUri(context->uri);
    data_.Clear();
    callback_ = new (std::nothrow) CallbackImpl(data_);
    if (callback_ == nullptr) {
        ZLOGE("new failed");
        return false;
    }
    ZLOGI("Start connect %{public}s", context->uri.c_str());
    ErrCode ret = AAFwk::AbilityManagerClient::GetInstance()->ConnectAbility(want, callback_, nullptr);
    if (ret != ERR_OK) {
        ZLOGE("connect ability failed, ret = %{public}d", ret);
        return false;
    }
    bool result = data_.GetValue();
    if (result) {
        ZLOGI("connect ability ended successfully");
    }
    return result;
}

bool ExtensionConnectAdaptor::TryAndWait(std::shared_ptr<Context> context, int maxWaitTime)
{
    ExtensionConnectAdaptor strategy;
    return AppConnectManager::Wait(
        context->calledBundleName, maxWaitTime,
        [&context, &strategy]() {
            return strategy.Connect(context);
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
    data_.Clear();
    ZLOGI("Start disconnect");
    AAFwk::AbilityManagerClient::GetInstance()->DisconnectAbility(callback_);
    if (!data_.GetValue()) {
        ZLOGI("disconnect ability ended successfully");
    }
    callback_ = nullptr;
}
} // namespace OHOS::DataShare