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
AccountDelegate::BaseInstance AccountDelegate::getInstance_ = AccountDelegateNormalImpl::GetBaseInstance;
AccountDelegate *AccountDelegateNormalImpl::GetBaseInstance()
{
    static AccountDelegateNormalImpl accountDelegate;
    return &accountDelegate;
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

std::string AccountDelegateNormalImpl::GetDeviceAccountIdByUID(int32_t uid) const
{
    int userId = 0;
    auto ret = AccountSA::OsAccountManager::GetOsAccountLocalIdFromUid(uid, userId);
    if (ret != 0) {
        ZLOGE("failed get os account local id from uid, ret:%{public}d", ret);
        return {};
    }
    return std::to_string(userId);
}

int32_t AccountDelegateNormalImpl::GetUserByToken(uint32_t tokenId) const
{
    auto type = AccessTokenKit::GetTokenTypeFlag(tokenId);
    if (type == TOKEN_NATIVE || type == TOKEN_SHELL) {
        return 0;
    }

    HapTokenInfo tokenInfo;
    if (AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo) != RET_SUCCESS) {
        return -1;
    }

    return tokenInfo.userID;
}
    
bool AccountDelegateNormalImpl::QueryUsers(std::vector<int> &users)
{
    users = {0}; // default user
    return AccountSA::OsAccountManager::QueryActiveOsAccountIds(users) == 0;
}

void AccountDelegateNormalImpl::SubscribeAccountEvent()
{
    ZLOGI("Subscribe account event listener start.");
    MatchingSkills matchingSkills;
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USER_REMOVED);
    matchingSkills.AddEvent(CommonEventSupport::COMMON_EVENT_USER_SWITCHED);
    CommonEventSubscribeInfo info(matchingSkills);
    eventSubscriber_ = std::make_shared<EventSubscriber>(info);
    eventSubscriber_->SetEventCallback([&](AccountEventInfo &account) {
        account.harmonyAccountId = GetCurrentAccountId();
        NotifyAccountChanged(account);
    });

    std::thread th = std::thread([eventSubscriber = eventSubscriber_]() {
        int tryTimes = 0;
        constexpr int MAX_RETRY_TIME = 300;
        constexpr int RETRY_WAIT_TIME_S = 1;

        // we use this method to make sure register success
        while (tryTimes < MAX_RETRY_TIME) {
            auto result = CommonEventManager::SubscribeCommonEvent(eventSubscriber);
            if (result) {
                break;
            }

            ZLOGE("EventManager: Fail to register subscriber, error:%d", result);
            sleep(RETRY_WAIT_TIME_S);
            tryTimes++;
        }
        if (tryTimes == MAX_RETRY_TIME) {
            ZLOGE("EventManager: Fail to register subscriber!");
        }
        ZLOGI("EventManager: Success to register subscriber.");
    });
    th.detach();
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
}  // namespace DistributedKv
}  // namespace OHOS