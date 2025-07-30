/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTEDDATA_ACCOUNT_DELEGATE_MOCK_H
#define OHOS_DISTRIBUTEDDATA_ACCOUNT_DELEGATE_MOCK_H

#include <gmock/gmock.h>
#include <vector>
#include "account/account_delegate.h"
namespace OHOS {
namespace DistributedData {
class AccountDelegateMock : public AccountDelegate {
public:
    virtual ~AccountDelegateMock() = default;
    MOCK_METHOD(int32_t, Subscribe, (std::shared_ptr<AccountDelegate::Observer>));
    MOCK_METHOD(int32_t, Unsubscribe, (std::shared_ptr<AccountDelegate::Observer>));
    MOCK_METHOD(std::string, GetCurrentAccountId, (), (const));
    MOCK_METHOD(int32_t, GetUserByToken, (uint32_t), (const));
    MOCK_METHOD(void, SubscribeAccountEvent, ());
    MOCK_METHOD(void, UnsubscribeAccountEvent, ());
    MOCK_METHOD(bool, QueryUsers, (std::vector<int> &));
    MOCK_METHOD(bool, QueryForegroundUsers, (std::vector<int> &));
    MOCK_METHOD(bool, IsLoginAccount, ());
    MOCK_METHOD(bool, QueryForegroundUserId, (int &));
    MOCK_METHOD(bool, IsVerified, (int));
    MOCK_METHOD(bool, RegisterHashFunc, (HashFunc));
    MOCK_METHOD(bool, IsDeactivating, (int));
    MOCK_METHOD(void, BindExecutor, (std::shared_ptr<ExecutorPool>));
    MOCK_METHOD(std::string, GetUnencryptedAccountId, (int32_t), (const));
    MOCK_METHOD(bool, IsUserForeground, (int32_t));
};
}
}
#endif // OHOS_DISTRIBUTEDDATA_ACCOUNT_DELEGATE_MOCK_H