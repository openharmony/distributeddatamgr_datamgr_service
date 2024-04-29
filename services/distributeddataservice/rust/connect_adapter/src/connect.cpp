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
#define LOG_TAG "Connect"

#include "connect.h"
#include <thread>
#include "iostream"

const int RESULT_OK = 0;

using namespace OHOS;

namespace {
    constexpr const char *BUNDLE_NAME = "com.example.cloudsync";
    constexpr const char *ABILITY_NAME = "ServiceExtAbility";
    static OHOS::sptr<Connect> g_connect = nullptr;
    static std::mutex mutex_;
}

void Connect::OnAbilityConnectDone(const OHOS::AppExecFwk::ElementName &element,
                                   const OHOS::sptr<OHOS::IRemoteObject> &remoteObject,
                                   int resultCode)
{
    if (resultCode != RESULT_OK) {
        ZLOGE("ability connect failed, error code:%{public}d", resultCode);
    }

    ZLOGD("ability connect success, ability name:%{public}s", element.GetAbilityName().c_str());
    remoteObject_ = remoteObject;
    std::unique_lock<std::mutex> lock(mtx_);
    flag_ = true;
    cv_.notify_one();
    lock.unlock();
}

void Connect::OnAbilityDisconnectDone(const OHOS::AppExecFwk::ElementName &element, int resultCode)
{
    if (resultCode != RESULT_OK) {
        ZLOGE("ability disconnect failed, error code:%{public}d", resultCode);
    }
    remoteObject_ = nullptr;
    std::unique_lock<std::mutex> lock(mtx_);
    flag_ = true;
    cv_.notify_one();
    lock.unlock();
}

bool Connect::IsConnect()
{
    return remoteObject_ != nullptr;
}

void Connect::WaitConnect()
{
    std::unique_lock<std::mutex> lock(g_connect->mtx_);
    cv_.wait_for(lock, std::chrono::seconds(CONNECT_TIMEOUT), [this] {
        return flag_;
    });
    cv_.notify_one();
}

namespace ConnectInner {
void DoConnect(int userId)
{
    ZLOGI("do connect");
    AAFwk::Want want;
    want.SetAction("");
    want.SetElementName(BUNDLE_NAME, ABILITY_NAME);
    auto ret = AAFwk::ExtensionManagerClient::GetInstance().ConnectServiceExtensionAbility(
        want, g_connect->AsObject(), userId);
    if (ret != ERR_OK) {
        ZLOGE("connect ability fail, error code:%{public}d", ret);
        return;
    }
    g_connect->WaitConnect();
}

OHOS::sptr<OHOS::IRemoteObject> ConnectServiceInner(int userId)
{
    ZLOGI("Connect Service Inner access");
    std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
    if (g_connect == nullptr) {
        sptr<Connect> ipcConnect = new(std::nothrow) Connect();
        if (ipcConnect == nullptr) {
            ZLOGE("ipc connect is null");
            return nullptr;
        }
        g_connect = ipcConnect;
    }
    if (!g_connect->IsConnect()) {
        DoConnect(userId);
    }
    return g_connect->remoteObject_;
}
}