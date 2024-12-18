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
#include <list>
#include <regex>
#include <thread>
#include <unistd.h>
#include "accesstoken_kit.h"
#include "log_print.h"
#include "ohos_account_kits.h"
#include "os_account_manager.h"

namespace OHOS {
namespace DistributedKv {
using namespace OHOS::EventFwk;
using namespace OHOS::AAFwk;
using namespace OHOS::DistributedData;
using namespace Security::AccessToken;
__attribute__((used)) static bool g_isInit = AccountDelegateNormalImpl::Init();

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
    users = {0}; // default user
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
    if (eventSubscriber_ == nullptr) {
        MatchingSkills matchingSkills;
        matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USER_REMOVED);
        matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USER_SWITCHED);
        matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USER_UNLOCKED);
        matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USER_STOPPING);
        matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USER_STOPPED);
        CommonEventSubscribeInfo info(matchingSkills);
        eventSubscriber_ = std::make_shared<EventSubscriber>(info);
        eventSubscriber_->SetEventCallback([this](AccountEventInfo& account) {
            UpdateUserStatus(account);
            account.harmonyAccountId = GetCurrentAccountId();
            NotifyAccountChanged(account);
        });
    }
    executors_->Execute(GetTask(0));
}

void AccountDelegateNormalImpl::UpdateUserStatus(const AccountEventInfo& account)
{
    uint32_t status = static_cast<uint32_t>(account.status);
    switch (status) {
        case static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_STOPPING):
        case static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_DELETE):
            userStatus_.Erase(atoi(account.userId.c_str()));
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
        auto result = CommonEventManager::SubscribeCommonEvent(eventSubscriber_);
        if (result) {
            ZLOGI("success to register subscriber.");
            return;
        }
        ZLOGD("fail to register subscriber, error:%{public}d, time:%{public}d", result, retry);

        if (retry + 1 > MAX_RETRY_TIME) {
            ZLOGE("fail to register subscriber!");
            return;
        }
        executors_->Schedule(std::chrono::seconds(RETRY_WAIT_TIME_S), GetTask(retry + 1));
    };
}

AccountDelegateNormalImpl::~AccountDelegateNormalImpl()
{
    ZLOGD("destruct");
    auto res = CommonEventManager::UnSubscribeCommonEvent(eventSubscriber_);
    if (!res) {
        ZLOGW("unregister account event fail res:%d", res);
    }
}

void AccountDelegateNormalImpl::UnsubscribeAccountEvent()
{
    auto res = CommonEventManager::UnSubscribeCommonEvent(eventSubscriber_);
    if (!res) {
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
    std::call_once(onceFlag, [&]() { AccountDelegate::RegisterAccountInstance(&normalAccountDelegate); });
    return true;
}
} // namespace DistributedKv
}  // namespace OHOS