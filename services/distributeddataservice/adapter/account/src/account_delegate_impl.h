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

#include <memory.h>
#include <mutex>

#include "account/account_delegate.h"
#include "concurrent_map.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedData {
class AccountDelegateImpl : public AccountDelegate {
public:
    ~AccountDelegateImpl();
    int32_t Subscribe(std::shared_ptr<Observer> observer) override __attribute__((no_sanitize("cfi")));
    int32_t Unsubscribe(std::shared_ptr<Observer> observer) override __attribute__((no_sanitize("cfi")));
    void NotifyAccountChanged(const AccountEventInfo &accountEventInfo, int32_t timeout = 0);
    bool RegisterHashFunc(HashFunc hash) override;

protected:
    std::string DoHash(const void *data, size_t size, bool isUpper) const;

private:
    ConcurrentMap<std::string, std::shared_ptr<Observer>> observerMap_{};
    HashFunc hash_ = nullptr;
};
} // namespace DistributedData
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_ACCOUNT_DELEGATE_IMPL_H
