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

#ifndef DISTRIBUTEDDATAMGR_ACCOUNT_DELEGATE_H
#define DISTRIBUTEDDATAMGR_ACCOUNT_DELEGATE_H

#include <mutex>
#include <memory>
#include "executor_pool.h"
#include "types.h"
#include "visibility.h"

namespace OHOS {
namespace DistributedKv {
enum class AccountStatus {
    HARMONY_ACCOUNT_LOGIN = 0, // the openHarmony account is logged in
    HARMONY_ACCOUNT_LOGOUT, // the openHarmony account is logged out
    HARMONY_ACCOUNT_DELETE, // the openHarmony account is deleted
    DEVICE_ACCOUNT_DELETE, // the device account is deleted
    DEVICE_ACCOUNT_SWITCHED, // the device account is switched
    DEVICE_ACCOUNT_UNLOCKED, // the device account is unlocked
};

struct AccountEventInfo {
    std::string harmonyAccountId;
    std::string userId;
    AccountStatus status;
};

class AccountDelegate {
public:
    class Observer {
    public:
        enum class LevelType {
            HIGH,
            LOW,
        };
        API_EXPORT virtual ~Observer() = default;
        API_EXPORT virtual void OnAccountChanged(const AccountEventInfo &eventInfo) = 0;

        // must specify unique name for observer
        API_EXPORT virtual std::string Name() = 0;
        API_EXPORT virtual LevelType GetLevel() = 0;
    };
    using HashFunc = std::string(*)(const void *data, size_t size, bool isUpper);
    API_EXPORT virtual ~AccountDelegate() = default;
    API_EXPORT virtual Status Subscribe(std::shared_ptr<Observer> observer) = 0;
    API_EXPORT virtual Status Unsubscribe(std::shared_ptr<Observer> observer) = 0;
    API_EXPORT virtual std::string GetCurrentAccountId() const = 0;
    API_EXPORT virtual int32_t GetUserByToken(uint32_t tokenId) const = 0;
    API_EXPORT virtual void SubscribeAccountEvent() = 0;
    API_EXPORT virtual void UnsubscribeAccountEvent() = 0;
    API_EXPORT virtual bool QueryUsers(std::vector<int> &users) = 0;
    API_EXPORT virtual bool QueryForegroundUsers(std::vector<int> &users) = 0;
    API_EXPORT virtual bool IsVerified(int userId) = 0;
    API_EXPORT virtual bool RegisterHashFunc(HashFunc hash) = 0;
    API_EXPORT virtual void BindExecutor(std::shared_ptr<ExecutorPool> executors) = 0;
    API_EXPORT static AccountDelegate *GetInstance();

private:
    using BaseInstance = AccountDelegate *(*)();
    static BaseInstance getInstance_;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_ACCOUNT_DELEGATE_H
