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
#define LOG_TAG "ConnectExtensionStrategy"

#include "connect_extension_strategy.h"

#include <thread>

#include "ability_connect_callback_stub.h"
#include "ability_manager_client.h"
#include "log_print.h"

namespace OHOS::DataShare {
bool ConnectExtensionStrategy::operator()(std::shared_ptr<Context> context)
{
    for (auto &extension: context->bundleInfo.extensionInfos) {
        if (extension.type == AppExecFwk::ExtensionAbilityType::DATASHARE) {
            return Connect(context);
        }
    }
    return false;
}

ConnectExtensionStrategy::ConnectExtensionStrategy() : data_(1) {}
class CallBackImpl : public AAFwk::AbilityConnectionStub {
public:
    CallBackImpl(BlockData<bool> &data) : data_(data) {}
    void OnAbilityConnectDone(
        const AppExecFwk::ElementName &element, const sptr<IRemoteObject> &remoteObject, int resultCode) override
    {
        bool result = true;
        data_.SetValue(result);
    }
    void OnAbilityDisconnectDone(const AppExecFwk::ElementName &element, int resultCode) override
    {
        bool result = false;
        data_.SetValue(result);
    }

private:
    BlockData<bool> &data_;
};

bool ConnectExtensionStrategy::Connect(std::shared_ptr<Context> context)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    AAFwk::Want want;
    want.SetUri(context->uri);
    data_.Clear();
    sptr<AAFwk::IAbilityConnection> callback = new CallBackImpl(data_);
    ZLOGI("Start connect %{public}s", context->uri.c_str());
    ErrCode ret = AAFwk::AbilityManagerClient::GetInstance()->ConnectAbility(want, callback, nullptr);
    if (ret != ERR_OK) {
        ZLOGE("connect ability failed, ret = %{public}d", ret);
        return false;
    }
    bool result = data_.GetValue();
    if (result) {
        ZLOGI("connect ability ended successfully");
    }
    data_.Clear();
    AAFwk::AbilityManagerClient::GetInstance()->DisconnectAbility(callback);
    if (!data_.GetValue()) {
        ZLOGI("disconnect ability ended successfully");
    }
    return result;
}

bool ConnectExtensionStrategy::Execute(
    std::shared_ptr<Context> context, std::function<bool()> isFinished, int maxWaitTimeMs)
{
    ConnectExtensionStrategy strategy;
    if (!strategy(context)) {
        return false;
    }
    if (isFinished == nullptr) {
        return true;
    }
    int waitTime = 0;
    static constexpr int RETRY_TIME = 500;
    while (!isFinished()) {
        if (waitTime > maxWaitTimeMs) {
            ZLOGE("cannot finish work");
            return false;
        }
        ZLOGI("has wait %{public}d ms", waitTime);
        std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_TIME));
        waitTime += RETRY_TIME;
    }
    return true;
}
} // namespace OHOS::DataShare