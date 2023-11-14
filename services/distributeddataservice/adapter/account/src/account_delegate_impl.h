/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDATAMGR_ACCOUNT_DELEGATE_IMPL_H
#define DISTRIBUTEDDATAMGR_ACCOUNT_DELEGATE_IMPL_H

#include "account_delegate.h"
#include <mutex>
#include <memory.h>
#include "common_event_manager.h"
#include "common_event_subscriber.h"
#include "common_event_support.h"
#include "concurrent_map.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
using namespace OHOS::EventFwk;
using EventCallback = std::function<void(AccountEventInfo &account)>;

class EventSubscriber final : public CommonEventSubscriber {
public:
    ~EventSubscriber() {}
    explicit EventSubscriber(const CommonEventSubscribeInfo &info);
    void SetEventCallback(EventCallback callback);
    void OnReceiveEvent(const CommonEventData &event) override;
private:
    EventCallback eventCallback_ {};
};

class AccountDelegateImpl : public AccountDelegate {
public:
    ~AccountDelegateImpl();
    Status Subscribe(std::shared_ptr<Observer> observer) override;
    Status Unsubscribe(std::shared_ptr<Observer> observer) override;
    void NotifyAccountChanged(const AccountEventInfo &accountEventInfo);
    bool RegisterHashFunc(HashFunc hash) override;

protected:
    std::string DoHash(const void *data, size_t size, bool isUpper) const;
private:
    ConcurrentMap<std::string, std::shared_ptr<Observer>> observerMap_ {};
    HashFunc hash_ = nullptr;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_ACCOUNT_DELEGATE_IMPL_H
