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
#define LOG_TAG "AccountDelegateDefaultImpl"

#include "account_delegate_default_impl.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
namespace {
    constexpr const char *DEFAULT_OHOS_ACCOUNT_UID = ""; // default UID
}

AccountDelegate::BaseInstance AccountDelegate::getInstance_ = AccountDelegateDefaultImpl::GetBaseInstance;
AccountDelegate *AccountDelegateDefaultImpl::GetBaseInstance()
{
    static AccountDelegateDefaultImpl accountDelegate;
    return &accountDelegate;
}

std::string AccountDelegateDefaultImpl::GetCurrentAccountId() const
{
    ZLOGD("no account part, return default.");
    return DEFAULT_OHOS_ACCOUNT_UID;
}

std::string AccountDelegateDefaultImpl::GetDeviceAccountIdByUID(int32_t uid) const
{
    ZLOGD("no account part, return default. uid:%d", uid);
    return std::to_string(0);
}

int32_t AccountDelegateDefaultImpl::GetUserByToken(uint32_t tokenId) const
{
    (void)tokenId;
    return 0;
}

bool AccountDelegateDefaultImpl::QueryUsers(std::vector<int> &users)
{
    ZLOGD("no account part.");
    users = {0}; // default user
    return true;
}

void AccountDelegateDefaultImpl::SubscribeAccountEvent()
{
    ZLOGD("no account part.");
}

void AccountDelegateDefaultImpl::UnsubscribeAccountEvent()
{
    ZLOGD("no account part.");
}

AccountDelegateDefaultImpl::~AccountDelegateDefaultImpl()
{
    ZLOGD("destruct");
}
}  // namespace DistributedKv
}  // namespace OHOS