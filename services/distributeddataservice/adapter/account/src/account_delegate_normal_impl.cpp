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
#define LOG_TAG "AccountDelegateNormalImpl"

#include "account_delegate_normal_impl.h"

#include <algorithm>
#include <endian.h>
#include <regex>
#include <thread>
#include <unistd.h>

#include "accesstoken_kit.h"
#include "log_print.h"
#include "ohos_account_kits.h"
#include "os_account_manager.h"
#include "os_account_subscribe_info.h"

namespace OHOS {
namespace DistributedData {
using namespace OHOS::AAFwk;
using namespace OHOS::DistributedData;
using namespace Security::AccessToken;
__attribute__((used)) static bool g_isInit = AccountDelegateNormalImpl::Init();
static constexpr int32_t STOPPING_TIMEOUT = 5;
static constexpr const char *TRANS_CONSTRAINT = "constraint.distributed.transmission.outgoing";
static inline const std::map<int32_t, AccountStatus> STATUS = {
    { AccountSA::REMOVED, AccountStatus::DEVICE_ACCOUNT_DELETE },
    { AccountSA::SWITCHED, AccountStatus::DEVICE_ACCOUNT_SWITCHED },
    { AccountSA::UNLOCKED, AccountStatus::DEVICE_ACCOUNT_UNLOCKED },
    { AccountSA::STOPPING, AccountStatus::DEVICE_ACCOUNT_STOPPING },
    { AccountSA::STOPPED, AccountStatus::DEVICE_ACCOUNT_STOPPED }
};

AccountSubscriber::AccountSubscriber(const OsAccountSubscribeInfo &info, std::shared_ptr<ExecutorPool> executors)
    : OsAccountSubscriber(info), executors_(executors)
{
}

void AccountSubscriber::OnStateChanged(const OsAccountStateData &data)
{
    ZLOGI("state change. state:%{public}d, from %{public}d to %{public}d", data.state, data.fromId, data.toId);

    auto it = STATUS.find(data.state);
    if (it == STATUS.end()) {
        return;
    }
    AccountEventInfo accountEventInfo;
    accountEventInfo.userId = std::to_string(data.toId);
    accountEventInfo.status = it->second;
    int32_t timeout = accountEventInfo.status == AccountStatus::DEVICE_ACCOUNT_STOPPING ? STOPPING_TIMEOUT : 0;
    if (eventCallback_) {
        eventCallback_(accountEventInfo, timeout);
    }
    if (data.callback == nullptr) {
        return;
    }
    if (executors_ != nullptr) {
        executors_->Execute([callback = data.callback]() {
            callback->OnComplete();
        });
    } else {
        data.callback->OnComplete();
    }
}

void AccountSubscriber::SetEventCallback(EventCallback callback)
{
    eventCallback_ = callback;
}

AccountDelegateNormalImpl::AccountDelegateNormalImpl()
{
    userDeactivating_.InsertOrAssign(0, false);
}

std::string AccountDelegateNormalImpl::GetCurrentAccountId() const
{
    ZLOGD("start");
    auto ohosAccountInfo = AccountSA::OhosAccountKits::GetInstance().QueryOhosAccountInfo();
    if (!ohosAccountInfo.first) {
        ZLOGE("get ohosAccountInfo from OhosAccountKits is null, return default");
        return AccountSA::DEFAULT_OHOS_ACCOUNT_UID;
    }
    if (ohosAccountInfo.second.uid_.empty()) {
        ZLOGE("get ohosAccountInfo from OhosAccountKits is null, return default");
        return AccountSA::DEFAULT_OHOS_ACCOUNT_UID;
    }

    return Sha256AccountId(ohosAccountInfo.second.uid_);
}

int32_t AccountDelegateNormalImpl::GetUserByToken(uint32_t tokenId) const
{
    auto type = AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (type == TOKEN_NATIVE || type == TOKEN_SHELL) {
        return 0;
    }

    HapTokenInfo tokenInfo;
    auto result = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (result != RET_SUCCESS) {
        ZLOGE("token:0x%{public}x, result:%{public}d", tokenId, result);
        return -1;
    }
    return tokenInfo.userID;
}

bool AccountDelegateNormalImpl::QueryUsers(std::vector<int> &users)
{
    users = { 0 }; // default user
    return AccountSA::OsAccountManager::QueryActiveOsAccountIds(users) == 0;
}

bool AccountDelegateNormalImpl::QueryForegroundUsers(std::vector<int> &users)
{
    std::vector<AccountSA::ForegroundOsAccount> accounts;
    if (AccountSA::OsAccountManager::GetForegroundOsAccounts(accounts) != 0) {
        return false;
    }
    for (auto &account : accounts) {
        users.push_back(account.localId);
    }
    return true;
}

bool AccountDelegateNormalImpl::IsLoginAccount()
{
    return GetCurrentAccountId() != AccountSA::DEFAULT_OHOS_ACCOUNT_UID;
}

bool AccountDelegateNormalImpl::IsVerified(int userId)
{
    auto [success, res] = userStatus_.Find(userId);
    if (success && res) {
        return true;
    }
    auto status = AccountSA::OsAccountManager::IsOsAccountVerified(userId, res);
    if (status == 0) {
        userStatus_.InsertOrAssign(userId, res);
    }
    return status == 0 && res;
}

void AccountDelegateNormalImpl::SubscribeAccountEvent()
{
    ZLOGI("Subscribe account event listener start.");
    if (accountSubscriber_ == nullptr) {
        std::set<OsAccountState> states;
        states.insert(AccountSA::REMOVED);
        states.insert(AccountSA::SWITCHED);
        states.insert(AccountSA::UNLOCKED);
        states.insert(AccountSA::STOPPING);
        states.insert(AccountSA::STOPPED);
        OsAccountSubscribeInfo info(states, true);
        accountSubscriber_ = std::make_shared<AccountSubscriber>(info, executors_);
        accountSubscriber_->SetEventCallback([this](AccountEventInfo &account, int32_t timeout) {
            UpdateUserStatus(account);
            account.harmonyAccountId = GetCurrentAccountId();
            NotifyAccountChanged(account, timeout);
        });
    }
    executors_->Execute(GetTask(0));
}

void AccountDelegateNormalImpl::UpdateUserStatus(const AccountEventInfo &account)
{
    uint32_t status = static_cast<uint32_t>(account.status);
    switch (status) {
        case static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_STOPPING):
        case static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_DELETE):
            userStatus_.Erase(atoi(account.userId.c_str()));
            userDeactivating_.Erase(atoi(account.userId.c_str()));
            break;
        case static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_UNLOCKED):
            userStatus_.InsertOrAssign(atoi(account.userId.c_str()), true);
            break;
        case static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_STOPPED):
            userStatus_.InsertOrAssign(atoi(account.userId.c_str()), false);
            break;
        default:
            break;
    }
}

ExecutorPool::Task AccountDelegateNormalImpl::GetTask(uint32_t retry)
{
    return [this, retry] {
        auto result = OsAccountManager::SubscribeOsAccount(accountSubscriber_);
        if (result == ERR_OK) {
            ZLOGI("success to register subscriber.");
            return;
        }
        ZLOGD("fail to register subscriber, error:%{public}d, time:%{public}d", result, retry);

        if (retry + 1 > MAX_RETRY_TIMES) {
            ZLOGE("fail to register subscriber!");
            return;
        }
        executors_->Schedule(std::chrono::seconds(RETRY_WAIT_TIME_S), GetTask(retry + 1));
    };
}

AccountDelegateNormalImpl::~AccountDelegateNormalImpl()
{
    ZLOGD("destruct");
    auto res = OsAccountManager::UnsubscribeOsAccount(accountSubscriber_);
    if (res != ERR_OK) {
        ZLOGW("unregister account event fail res:%d", res);
    }
}

void AccountDelegateNormalImpl::UnsubscribeAccountEvent()
{
    auto res = OsAccountManager::UnsubscribeOsAccount(accountSubscriber_);
    if (res != ERR_OK) {
        ZLOGW("unregister account event fail res:%d", res);
    }
}

std::string AccountDelegateNormalImpl::Sha256AccountId(const std::string &plainText) const
{
    std::regex pattern("^[0-9]+$");
    if (!std::regex_match(plainText, pattern)) {
        return plainText;
    }

    int64_t plain;
    std::string::size_type int64MaxLen(std::to_string(INT64_MAX).size());
    // plain text length must be less than INT64_MAX string.
    plain = atoll(plainText.c_str());
    if (plain == 0) {
        return plainText;
    }
    if (plain == INT64_MAX) {
        plain = atoll(plainText.substr(plainText.size() - int64MaxLen + 1, int64MaxLen - 1).c_str());
    }

    auto plainVal = htobe64(plain);
    return DoHash(static_cast<void *>(&plainVal), sizeof(plainVal), true);
}

void AccountDelegateNormalImpl::BindExecutor(std::shared_ptr<ExecutorPool> executors)
{
    executors_ = executors;
}

std::string AccountDelegateNormalImpl::GetUnencryptedAccountId(int32_t userId) const
{
    AccountSA::OhosAccountInfo info;
    auto ret = AccountSA::OhosAccountKits::GetInstance().GetOsAccountDistributedInfo(userId, info);
    if (ret != ERR_OK) {
        ZLOGE("GetUnencryptedAccountId failed: %{public}d", ret);
        return "";
    }
    return Sha256AccountId(info.GetRawUid());
}

bool AccountDelegateNormalImpl::QueryForegroundUserId(int &foregroundUserId)
{
    int32_t status = AccountSA::OsAccountManager::GetForegroundOsAccountLocalId(foregroundUserId);
    if (status != ERR_OK) {
        ZLOGE("GetForegroundOsAccountLocalId failed, status: %{public}d", status);
        return false;
    }
    return true;
}

bool AccountDelegateNormalImpl::Init()
{
    static AccountDelegateNormalImpl normalAccountDelegate;
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&]() {
        AccountDelegate::RegisterAccountInstance(&normalAccountDelegate);
    });
    return true;
}

bool AccountDelegateNormalImpl::IsUserForeground(int32_t userId)
{
    bool isForeground = false;
    if (AccountSA::OsAccountManager::IsOsAccountForeground(userId, isForeground) != 0) {
        ZLOGE("check foreground user error, userId:%{public}d", userId);
        return false;
    }
    return isForeground;
}

bool AccountDelegateNormalImpl::IsDeactivating(int userId)
{
    auto [success, res] = userDeactivating_.Find(userId);
    if (success && !res) {
        return res;
    }
    auto status = AccountSA::OsAccountManager::IsOsAccountDeactivating(userId, res);
    if (status != 0) {
        ZLOGE("IsOsAccountDeactivating failed: %{public}d, user:%{public}d", status, userId);
        return true;
    }
    userDeactivating_.InsertOrAssign(userId, res);
    return res;
}

bool AccountDelegateNormalImpl::IsOsAccountConstraintEnabled()
{
    int userId = 0;
    if (!QueryForegroundUserId(userId)) {
        return true;
    }
    bool isEnabled = true;
    int32_t status = AccountSA::OsAccountManager::CheckOsAccountConstraintEnabled(userId, TRANS_CONSTRAINT, isEnabled);
    ZLOGI("status: %{public}d, userId is %{public}d, isEnabled is %{public}d", status, userId, isEnabled);
    return isEnabled;
}
} // namespace DistributedData
} // namespace OHOS