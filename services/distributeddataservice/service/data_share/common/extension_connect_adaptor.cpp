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
#include "extension_mgr_proxy.h"
#include "log_print.h"
#include "utils/anonymous.h"

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

ExtensionConnectAdaptor::ExtensionConnectAdaptor() : data_(std::make_shared<BlockData<bool>>(1))
{
    callback_ = new (std::nothrow) CallbackImpl(data_);
}

bool ExtensionConnectAdaptor::DoConnect(std::shared_ptr<Context> context)
{
    data_->Clear();
    if (callback_ == nullptr) {
        return false;
    }
    ErrCode ret = ExtensionMgrProxy::GetInstance()->Connect(context->uri, callback_->AsObject(), nullptr);
    if (ret != ERR_OK) {
        ZLOGE("connect ability failed, ret = %{public}d, uri: %{public}s", ret,
            DistributedData::Anonymous::Change(context->uri).c_str());
        return false;
    }
    bool result = data_->GetValue();
    ZLOGI("Do connect, result: %{public}d,  uri: %{public}s", result,
        DistributedData::Anonymous::Change(context->uri).c_str());
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
    data_->Clear();
    ExtensionMgrProxy::GetInstance()->DisConnect(callback_->AsObject());
    bool result = data_->GetValue();
    ZLOGI("Do disconnect, result: %{public}d", result);
    callback_ = nullptr;
}
} // namespace OHOS::DataShare