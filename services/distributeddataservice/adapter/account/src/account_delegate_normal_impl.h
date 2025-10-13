/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef DISTRIBUTEDDATAMGR_ACCOUNT_DELEGATE_NORMAL_IMPL_H
#define DISTRIBUTEDDATAMGR_ACCOUNT_DELEGATE_NORMAL_IMPL_H

#include "account_delegate_impl.h"
#include "executor_pool.h"
#include "log_print.h"
#include "os_account_subscriber.h"

namespace OHOS {
namespace DistributedData {
using namespace AccountSA;
using EventCallback = std::function<void(AccountEventInfo &account, int32_t timeout)>;
class AccountSubscriber final : public OsAccountSubscriber {
public:
    ~AccountSubscriber() {}
    explicit AccountSubscriber(const OsAccountSubscribeInfo &subscribeInfo, std::shared_ptr<ExecutorPool> executors);
    void SetEventCallback(EventCallback callback);
    void OnStateChanged(const OsAccountStateData &data) override;
private:
    EventCallback eventCallback_{};
    std::shared_ptr<ExecutorPool> executors_ = nullptr;
};
class AccountDelegateNormalImpl final : public AccountDelegateImpl {
public:
    AccountDelegateNormalImpl();
    std::string GetCurrentAccountId() const override;
    int32_t GetUserByToken(uint32_t tokenId) const override;
    bool QueryUsers(std::vector<int> &users) override;
    bool QueryForegroundUsers(std::vector<int> &users) override;
    bool IsLoginAccount() override;
    bool IsVerified(int userId) override;
    bool IsDeactivating(int userId) override;
    void SubscribeAccountEvent() override;
    void UnsubscribeAccountEvent() override;
    void BindExecutor(std::shared_ptr<ExecutorPool> executors) override;
    std::string GetUnencryptedAccountId(int32_t userId = 0) const override;
    bool QueryForegroundUserId(int &foregroundUserId) override;
    bool IsUserForeground(int32_t userId) override;
    static bool Init();

private:
    ~AccountDelegateNormalImpl();
    std::string Sha256AccountId(const std::string &plainText) const;
    ExecutorPool::Task GetTask(uint32_t retry);
    void UpdateUserStatus(const AccountEventInfo &account);
    static constexpr int MAX_RETRY_TIME_S = 300;
    static constexpr int RETRY_WAIT_TIME_S = 1;
    std::shared_ptr<AccountSubscriber> accountSubscriber_{};
    std::shared_ptr<ExecutorPool> executors_;
    ConcurrentMap<int32_t, bool> userStatus_{};
    ConcurrentMap<int32_t, bool> userDeactivating_{};
};
} // namespace DistributedData
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_ACCOUNT_DELEGATE_NORMAL_IMPL_H