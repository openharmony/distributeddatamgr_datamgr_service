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
#ifndef DISTRIBUTEDDATAMGR_ACCOUNT_DELEGATE_DEFAULT_IMPL_H
#define DISTRIBUTEDDATAMGR_ACCOUNT_DELEGATE_DEFAULT_IMPL_H

#include "account_delegate_impl.h"

namespace OHOS {
namespace DistributedData {
class AccountDelegateDefaultImpl final : public AccountDelegateImpl {
public:
    std::string GetCurrentAccountId() const override;
    std::string GetUnencryptedAccountId(int32_t userId = 0) const override;
    int32_t GetUserByToken(uint32_t tokenId) const override;
    bool QueryUsers(std::vector<int> &users) override;
    bool QueryForegroundUsers(std::vector<int> &users) override;
    bool IsLoginAccount() override;
    bool IsVerified(int userId) override;
    bool IsDeactivating(int userId) override;
    void SubscribeAccountEvent() override;
    void UnsubscribeAccountEvent() override;
    void BindExecutor(std::shared_ptr<ExecutorPool> executors) override;
    bool QueryForegroundUserId(int &foregroundUserId) override;
    bool IsUserForeground(int32_t userId) override;
    static bool Init();
    bool IsOsAccountConstraintEnabled() override;

private:
    ~AccountDelegateDefaultImpl();
};
} // namespace DistributedData
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_ACCOUNT_DELEGATE_DEFAULT_IMPL_H