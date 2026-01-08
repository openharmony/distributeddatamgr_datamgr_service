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

#include "account_delegate_mock_proxy.h"
namespace OHOS {
namespace DistributedData {
    AccountDelegateMockProxy::AccountDelegateMockProxy()
    {
        accountDelegate_ = std::make_shared<AccountDelegateMock>();
    }
    int32_t AccountDelegateMockProxy::Subscribe(std::shared_ptr<Observer> observer)
    {
        return accountDelegate_->Subscribe(observer);
    }
    int32_t AccountDelegateMockProxy::Unsubscribe(std::shared_ptr<Observer> observer)
    {
        return accountDelegate_->Unsubscribe(observer);
    }
    std::string AccountDelegateMockProxy::GetCurrentAccountId() const
    {
        return accountDelegate_->GetCurrentAccountId();
    }
    int32_t AccountDelegateMockProxy::GetUserByToken(uint32_t tokenId) const
    {
        return accountDelegate_->GetUserByToken(tokenId);
    }
    void AccountDelegateMockProxy::SubscribeAccountEvent()
    {
        accountDelegate_->SubscribeAccountEvent();
    }
    void AccountDelegateMockProxy::UnsubscribeAccountEvent()
    {
        accountDelegate_->UnsubscribeAccountEvent();
    }
    bool AccountDelegateMockProxy::QueryUsers(std::vector<int> &users)
    {
        return accountDelegate_->QueryUsers(users);
    }
    bool AccountDelegateMockProxy::QueryForegroundUsers(std::vector<int> &users)
    {
        return accountDelegate_->QueryForegroundUsers(users);
    }
    bool AccountDelegateMockProxy::QueryForegroundUserId(int &foregroundUserId)
    {
        return accountDelegate_->QueryForegroundUserId(foregroundUserId);
    }
    bool AccountDelegateMockProxy::IsLoginAccount()
    {
        return accountDelegate_->IsLoginAccount();
    }
    bool AccountDelegateMockProxy::IsVerified(int userId)
    {
        return accountDelegate_->IsVerified(userId);
    }
    bool AccountDelegateMockProxy::RegisterHashFunc(HashFunc hash)
    {
        return accountDelegate_->RegisterHashFunc(hash);
    }
    bool AccountDelegateMockProxy::IsDeactivating(int userId)
    {
        return accountDelegate_->IsDeactivating(userId);
    }
    void AccountDelegateMockProxy::BindExecutor(std::shared_ptr<ExecutorPool> executors)
    {
        accountDelegate_->BindExecutor(executors);
    }
    std::string AccountDelegateMockProxy::GetUnencryptedAccountId(int32_t userId) const
    {
        return accountDelegate_->GetUnencryptedAccountId(userId);
    }
    bool AccountDelegateMockProxy::IsUserForeground(int32_t userId)
    {
        return accountDelegate_->IsUserForeground(userId);
    }
    bool AccountDelegateMockProxy::IsOsAccountConstraintEnabled()
    {
        return accountDelegate_->IsOsAccountConstraintEnabled();
    }
    void AccountDelegateMockProxy::SetAccountDelegateMock(std::shared_ptr<AccountDelegateMock> accountDelegate)
    {
        accountDelegate_ = accountDelegate;
    }
} // namespace DistributedData
} // namespace OHOS