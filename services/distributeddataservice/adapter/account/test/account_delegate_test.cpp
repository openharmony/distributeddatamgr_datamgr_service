/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#define LOG_TAG "AccountDelegateTest"
#include <gtest/gtest.h>
#include <memory.h>
#include "account_delegate.h"
#include "account_delegate_impl.h"
#include "account_delegate_normal_impl.h"
#include "ipc_skeleton.h"
#include "ohos_account_kits.h"
#include "os_account_manager.h"
#include "log_print.h"
namespace {
using namespace OHOS::DistributedKv;
using namespace testing::ext;
class AccountObserver : public AccountDelegate::Observer {
public:
    void OnAccountChanged(const AccountEventInfo &info) override
    {
    }
    // must specify unique name for observer
    std::string Name() override
    {
        return name_;
    }

    LevelType GetLevel() override
    {
        return LevelType::LOW;
    }

    void SetName(const std::string &name)
    {
        name_ = name;
    }
private:
    std::string name_ = "accountTestObserver";
};

class AccountDelegateTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}

protected:
    static const std::string DEFAULT_OHOS_ACCOUNT_UID;
    static const uint32_t INVALID_TOKEN_ID;
    static const int32_t INVALID_USER;
    static const int userId;
};
const std::string AccountDelegateTest::DEFAULT_OHOS_ACCOUNT_UID = "ohosAnonymousUid";
const uint32_t AccountDelegateTest::INVALID_TOKEN_ID = -1;
const int32_t AccountDelegateTest::INVALID_USER = -1;
const int AccountDelegateTest::userId = 100;

/**
* @tc.name: Subscribe001
* @tc.desc: subscribe user change, the observer is invalid
* @tc.type: FUNC
* @tc.require: AR000D08K2 AR000CQDUM
* @tc.author: hongbo
*/
HWTEST_F(AccountDelegateTest, Subscribe001, TestSize.Level0)
{
    auto account = AccountDelegate::GetInstance();
    EXPECT_NE(account, nullptr);
    auto status = account->Subscribe(nullptr);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    auto observer = std::make_shared<AccountObserver>();
    observer->SetName("");
    status = account->Subscribe(observer);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: Subscribe002
* @tc.desc: subscribe user change
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(AccountDelegateTest, Subscribe002, TestSize.Level0)
{
    auto account = AccountDelegate::GetInstance();
    EXPECT_NE(account, nullptr);
    auto observer = std::make_shared<AccountObserver>();
    auto status = account->Subscribe(observer);
    EXPECT_EQ(status, Status::SUCCESS);
    observer->SetName("Subscribe002Observer");
    status = account->Subscribe(observer);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: Subscribe003
* @tc.desc: subscribe user change, the observer is invalid
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(AccountDelegateTest, Subscribe003, TestSize.Level0)
{
    auto account = AccountDelegate::GetInstance();
    EXPECT_NE(account, nullptr);
    auto observer = std::make_shared<AccountObserver>();
    auto status = account->Subscribe(observer);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    observer->SetName("Subscribe003Observer");
    status = account->Subscribe(observer);
    EXPECT_EQ(status, Status::SUCCESS);
    observer->SetName("Subscribe002ObserverAfter");
    status = account->Subscribe(observer);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: Unsubscribe001
* @tc.desc: unsubscribe user change, the observer is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(AccountDelegateTest, Unsubscribe001, TestSize.Level0)
{
    auto account = AccountDelegate::GetInstance();
    EXPECT_NE(account, nullptr);
    auto status = account->Unsubscribe(nullptr);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    auto observer = std::make_shared<AccountObserver>();
    observer->SetName("");
    status = account->Unsubscribe(observer);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    observer->SetName("Unsubscribe001Observer");
    status = account->Unsubscribe(observer);
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: Unsubscribe002
* @tc.desc: unsubscribe user change
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(AccountDelegateTest, Unsubscribe002, TestSize.Level0)
{
    auto account = AccountDelegate::GetInstance();
    EXPECT_NE(account, nullptr);
    auto observer = std::make_shared<AccountObserver>();
    account->Subscribe(observer);
    auto status = account->Unsubscribe(observer);
    EXPECT_EQ(status, Status::SUCCESS);
    observer->SetName("Unsubscribe002ObserverPre");
    status = account->Subscribe(observer);
    EXPECT_EQ(status, Status::SUCCESS);
    observer->SetName("Unsubscribe002ObserverAfter");
    status = account->Subscribe(observer);
    EXPECT_EQ(status, Status::SUCCESS);
    status = account->Unsubscribe(observer);
    EXPECT_EQ(status, Status::SUCCESS);
    observer->SetName("Unsubscribe002ObserverPre");
    status = account->Unsubscribe(observer);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: GetCurrentAccountId
* @tc.desc: get current account Id
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(AccountDelegateTest, GetCurrentAccountId, TestSize.Level0)
{
    auto accountId = AccountDelegate::GetInstance()->GetCurrentAccountId();
    EXPECT_EQ(accountId, DEFAULT_OHOS_ACCOUNT_UID);
}

/**
* @tc.name: GetUserByToken
* @tc.desc: get user by token
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(AccountDelegateTest, GetUserByToken, TestSize.Level0)
{
    auto user = AccountDelegate::GetInstance()->GetUserByToken(INVALID_TOKEN_ID);
    EXPECT_EQ(user, INVALID_USER);
    user = AccountDelegate::GetInstance()->GetUserByToken(OHOS::IPCSkeleton::GetCallingTokenID());
    EXPECT_EQ(user, 0);
}

/**
* @tc.name: QueryUsers
* @tc.desc: query users
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(AccountDelegateTest, QueryUsers, TestSize.Level0)
{
    std::vector<int32_t> users;
    AccountDelegate::GetInstance()->QueryUsers(users);
    EXPECT_GT(users.size(), 0);
}

/**
* @tc.name: IsVerified
* @tc.desc: query users
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(AccountDelegateTest, IsVerified, TestSize.Level0)
{
    auto user = AccountDelegate::GetInstance()->IsVerified(userId);
    EXPECT_EQ(user, true);
}

/**
* @tc.name: UnsubscribeAccountEvent
* @tc.desc: get current account Id
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(AccountDelegateTest, UnsubscribeAccountEvent, TestSize.Level0)
{
    auto account = AccountDelegate::GetInstance();
    auto observer = std::make_shared<AccountObserver>();
    account->Subscribe(observer);
    auto executor = std::make_shared<OHOS::ExecutorPool>(12, 5);
    AccountDelegate::GetInstance()->UnsubscribeAccountEvent();
    AccountDelegate::GetInstance()->BindExecutor(executor);
    auto status = account->Unsubscribe(observer);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: QueryForegroundUsers
* @tc.desc: query foreground users
* @tc.type: FUNC
* @tc.require:
* @tc.author: ht
*/
HWTEST_F(AccountDelegateTest, QueryForegroundUsers, TestSize.Level0)
{
    std::vector<int32_t> users;
    EXPECT_TRUE(AccountDelegate::GetInstance()->QueryForegroundUsers(users));
    std::vector<OHOS::AccountSA::ForegroundOsAccount> accounts;
    OHOS::AccountSA::OsAccountManager::GetForegroundOsAccounts(accounts);
    EXPECT_EQ(accounts.size(), users.size());
    for (int i = 0; i < users.size(); i++) {
        EXPECT_EQ(users[i], accounts[i].localId);
    }
}
} // namespace
