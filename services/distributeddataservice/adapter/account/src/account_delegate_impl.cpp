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

#define LOG_TAG "EVENT_HANDLER"

#include "account_delegate_impl.h"
#include <unistd.h>
#include <thread>
#include "constant.h"
#include "crypto_utils.h"
#include "ohos_account_kits.h"
#include "permission_validator.h"

namespace OHOS {
namespace DistributedKv {
AccountDelegate::BaseInstance AccountDelegate::getInstance_ = AccountDelegateImpl::GetBaseInstance;

AccountDelegateImpl *AccountDelegateImpl::GetInstance()
{
    static AccountDelegateImpl accountDelegate;
    return &accountDelegate;
}

AccountDelegate *AccountDelegateImpl::GetBaseInstance()
{
    return AccountDelegateImpl::GetInstance();
}

AccountDelegateImpl::~AccountDelegateImpl()
{
    ZLOGE("destruct");
    observerMap_.Clear();
}

void AccountDelegateImpl::SubscribeAccountEvent()
{

}

std::string AccountDelegateImpl::GetCurrentHarmonyAccountId(const std::string &bundleName) const
{
    ZLOGD("start");
    if (!bundleName.empty() && PermissionValidator::IsAutoLaunchEnabled(bundleName)) {
        return Constant::DEFAULT_GROUP_ID;
    }
    auto ohosAccountInfo = AccountSA::OhosAccountKits::GetInstance().QueryOhosAccountInfo();
    if (!ohosAccountInfo.first) {
        ZLOGE("get ohosAccountInfo from OhosAccountKits is null, return default");
        return AccountSA::DEFAULT_OHOS_ACCOUNT_UID;
    }
    if (ohosAccountInfo.second.uid_.empty()) {
        ZLOGE("get ohosAccountInfo from OhosAccountKits is null, return default");
        return AccountSA::DEFAULT_OHOS_ACCOUNT_UID;
    }

    return CryptoUtils::Sha256UserId(ohosAccountInfo.second.uid_);
}

std::string AccountDelegateImpl::GetDeviceAccountIdByUID(int32_t uid) const
{
    return std::to_string(AccountSA::OhosAccountKits::GetInstance().GetDeviceAccountIdByUID(uid));
}

void AccountDelegateImpl::NotifyAccountChanged(const AccountEventInfo &accountEventInfo)
{
    observerMap_.ForEach([&](std::string key, std::shared_ptr<Observer> val) {
        val->OnAccountChanged(accountEventInfo);
    });
}

Status AccountDelegateImpl::Subscribe(std::shared_ptr<Observer> observer)
{
    ZLOGD("start");
    if (observer == nullptr || observer->Name().empty()) {
        return Status::INVALID_ARGUMENT;
    }
    if (observerMap_.ContainsKey(observer->Name())) {
        return Status::INVALID_ARGUMENT;
    }

    auto ret = observerMap_.Put(observer->Name(), observer);
    if (ret) {
        ZLOGD("end");
        return Status::SUCCESS;
    }
    ZLOGE("fail");
    return Status::ERROR;
}

Status AccountDelegateImpl::Unsubscribe(std::shared_ptr<Observer> observer)
{
    ZLOGD("start");
    if (observer == nullptr || observer->Name().empty()) {
        return Status::INVALID_ARGUMENT;
    }
    if (!observerMap_.ContainsKey(observer->Name())) {
        return Status::INVALID_ARGUMENT;
    }

    auto ret = observerMap_.Delete(observer->Name());
    if (ret) {
        ZLOGD("end");
        return Status::SUCCESS;
    }
    ZLOGD("fail");
    return Status::ERROR;
}
}  // namespace DistributedKv
}  // namespace OHOS
