/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTEDDATA_ACCOUNT_DELEGATE_MOCK_PROXY_H
#define OHOS_DISTRIBUTEDDATA_ACCOUNT_DELEGATE_MOCK_PROXY_H

#include "account_delegate_mock.h"
#include "account/account_delegate.h"
namespace OHOS {
namespace DistributedData {
class AccountDelegateMockProxy : public AccountDelegate {
public:
    AccountDelegateMockProxy();
    ~AccountDelegateMockProxy() = default;
    int32_t Subscribe(std::shared_ptr<Observer> observer);
    int32_t Unsubscribe(std::shared_ptr<Observer> observer);
    std::string GetCurrentAccountId() const;
    int32_t GetUserByToken(uint32_t tokenId) const;
    void SubscribeAccountEvent();
    void UnsubscribeAccountEvent();
    bool QueryUsers(std::vector<int> &users);
    bool QueryForegroundUsers(std::vector<int> &users);
    bool QueryForegroundUserId(int &foregroundUserId);
    bool IsLoginAccount();
    bool IsVerified(int userId);
    bool RegisterHashFunc(HashFunc hash);
    bool IsDeactivating(int userId);
    void BindExecutor(std::shared_ptr<ExecutorPool> executors);
    std::string GetUnencryptedAccountId(int32_t userId = 0) const;
    bool IsUserForeground(int32_t userId);
    bool IsOsAccountConstraintEnabled();
    void SetAccountDelegateMock(std::shared_ptr<AccountDelegateMock> accountDelegate);

private:
    std::shared_ptr<AccountDelegateMock> accountDelegate_;
};
}
}
#endif // OHOS_DISTRIBUTEDDATA_ACCOUNT_DELEGATE_MOCK_PROXY_H