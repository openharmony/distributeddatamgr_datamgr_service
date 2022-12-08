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

#ifndef OHOS_SECURITY_H
#define OHOS_SECURITY_H
#include <concurrent_map.h>
#include <string>
#include <task_scheduler.h>
#include "app_device_change_listener.h"
#include "iprocess_system_api_adapter.h"
#include "kv_store_delegate_manager.h"
#include "sensitive.h"
#include "visibility.h"

namespace OHOS::DistributedKv {
class Security
    : public DistributedDB::IProcessSystemApiAdapter,
      public AppDistributedKv::AppDeviceChangeListener {
public:
    using DBStatus = DistributedDB::DBStatus;
    using OnAccessControlledEvent = DistributedDB::OnAccessControlledEvent;
    using SecurityOption = DistributedDB::SecurityOption;
    Security();
    ~Security() override;
    static bool IsSupportSecurity();

    DBStatus RegOnAccessControlledEvent(const OnAccessControlledEvent &callback) override;

    // Check is the access of this device in locked state
    bool IsAccessControlled() const override;

    // Set the SecurityOption to the targe filepath.
    // If the filePath is a directory, the function would not effective.
    DBStatus SetSecurityOption(const std::string &filePath, const SecurityOption &option) override;

    // Get the SecurityOption of the targe filepath.
    DBStatus GetSecurityOption(const std::string &filePath, SecurityOption &option) const override;

    // Check if the target device can save the data at the give sensitive class.
    bool CheckDeviceSecurityAbility(const std::string &deviceId, const SecurityOption &option) const override;

    void OnDeviceChanged(const AppDistributedKv::DeviceInfo &info,
                         const AppDistributedKv::DeviceChangeType &type) const override;

    AppDistributedKv::ChangeLevelType GetChangeLevelType() const override;

private:
    enum {
        NO_PWD = -1,
        UNLOCK,
        LOCKED,
        UNINITIALIZED,
    };
    static const std::string LABEL_VALUES[DistributedDB::S4 + 1];
    static const std::string Convert2Name(const SecurityOption &option);
    static int Convert2Security(const std::string &name);
    bool IsExits(const std::string &file) const;
    Sensitive GetSensitiveByUuid(const std::string &uuid) const;
    bool EraseSensitiveByUuid(const std::string &uuid) const;
    bool IsXattrValueValid(const std::string& value) const;
    int32_t GetCurrentUserStatus() const;
    DBStatus SetFileSecurityOption(const std::string &filePath, const SecurityOption &option);
    DBStatus SetDirSecurityOption(const std::string &filePath, const SecurityOption &option);
    DBStatus GetFileSecurityOption(const std::string &filePath, SecurityOption &option) const;
    DBStatus GetDirSecurityOption(const std::string &filePath, SecurityOption &option) const;

    mutable TaskScheduler taskScheduler_;
    mutable ConcurrentMap<std::string, Sensitive> devicesUdid_;
};
} // namespace OHOS::DistributedKv

#endif // OHOS_SECURITY_H
