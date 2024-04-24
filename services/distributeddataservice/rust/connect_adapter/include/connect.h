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

#ifndef RUST_PROJ_MOCK_CONNECT_H
#define RUST_PROJ_MOCK_CONNECT_H

#include <mutex>
#include <condition_variable>
#include "ability_connect_callback_stub.h"
#include "extension_manager_client.h"
#include "extension_manager_proxy.h"
#include "hilog/log_cpp.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "iremote_broker.h"

static const OHOS::HiviewDFX::HiLogLabel lable = { LOG_CORE, 0xD001610, "Connect"};

#define ZLOGD(fmt, ...)                                                       \
    do {                                                                      \
        if (HiLogIsLoggable(lable.domain, lable.tag, LOG_DEBUG)) {            \
            ((void)HILOG_IMPL(lable.type, LOG_DEBUG, lable.domain, lable.tag, \
                LOG_TAG "::%{public}s: " fmt, __FUNCTION__, ##__VA_ARGS__));  \
        }                                                                     \
    } while (0)

#define ZLOGI(fmt, ...)                                                       \
    do {                                                                      \
        if (HiLogIsLoggable(lable.domain, lable.tag, LOG_INFO)) {             \
            ((void)HILOG_IMPL(lable.type, LOG_INFO, lable.domain, lable.tag,  \
                LOG_TAG "::%{public}s: " fmt, __FUNCTION__, ##__VA_ARGS__));  \
        }                                                                     \
    } while (0)

#define ZLOGW(fmt, ...)                                                       \
    do {                                                                      \
        if (HiLogIsLoggable(lable.domain, lable.tag, LOG_WARN)) {             \
            ((void)HILOG_IMPL(lable.type, LOG_WARN, lable.domain, lable.tag,  \
                LOG_TAG "::%{public}s: " fmt, __FUNCTION__, ##__VA_ARGS__));  \
        }                                                                     \
    } while (0)

#define ZLOGE(fmt, ...)                                                       \
    do {                                                                      \
        if (HiLogIsLoggable(lable.domain, lable.tag, LOG_ERROR)) {            \
            ((void)HILOG_IMPL(lable.type, LOG_ERROR, lable.domain, lable.tag, \
                LOG_TAG "::%{public}s: " fmt, __FUNCTION__, ##__VA_ARGS__));  \
        }                                                                     \
    } while (0)

class Connect : public OHOS::AAFwk::AbilityConnectionStub {
public:
    void OnAbilityConnectDone(const OHOS::AppExecFwk::ElementName &element,
                              const OHOS::sptr<OHOS::IRemoteObject> &remoteObject,
                              int resultCode) override;

    void OnAbilityDisconnectDone(const OHOS::AppExecFwk::ElementName &element,
                                 int resultCode) override;
    bool IsConnect();
    void WaitConnect();

public:
    OHOS::sptr<OHOS::IRemoteObject> remoteObject_ = nullptr;
    std::mutex mtx_;
    std::condition_variable cv_;
    // Used to avoid wake-up loss
    bool flag_ = false;
    static constexpr int32_t CONNECT_TIMEOUT = 5;
};

namespace ConnectInner {
    void DoConnect(int userId);
    OHOS::sptr<OHOS::IRemoteObject> ConnectServiceInner(int userId);
}
#endif // RUST_PROJ_MOCK_CONNECT_H
