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
#define LOG_TAG "ScreenOff"

#include "screen_off.h"
#include "log_print.h"
#include "power_mgr_client.h"

namespace OHOS {
namespace DistributedData {
__attribute__((used)) ScreenOff ScreenOff::instance_;
ScreenOff::ScreenOff() noexcept
{
    BackupRuleManager::GetInstance().RegisterPlugin(
        "ScreenOff", [this]() -> auto { return this; });
}

bool ScreenOff::CanBackup()
{
    auto &powerMgrClient = PowerMgr::PowerMgrClient::GetInstance();
    if (powerMgrClient.IsScreenOn()) {
        ZLOGE("the device screen is on.");
        return false;
    }
    return true;
}
} // namespace DistributedData
} // namespace OHOS