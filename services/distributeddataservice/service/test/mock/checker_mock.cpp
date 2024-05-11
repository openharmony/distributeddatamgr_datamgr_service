/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "checker_mock.h"

namespace OHOS::DistributedData {
CheckerMock::CheckerMock() noexcept
{
    CheckerManager::GetInstance().RegisterPlugin(
        "SystemChecker", [this]() -> auto { return this; });
}
CheckerMock::~CheckerMock() {}
void CheckerMock::SetDynamic(const std::vector<CheckerManager::StoreInfo> &dynamicStores)
{
    dynamicInfos_ = dynamicStores;
}
void CheckerMock::SetStatic(const std::vector<CheckerManager::StoreInfo> &staticStores)
{
    staticInfos_ = staticStores;
}
void CheckerMock::Initialize() {}
bool CheckerMock::SetTrustInfo(const CheckerManager::Trust &trust)
{
    return true;
}
std::string CheckerMock::GetAppId(const CheckerManager::StoreInfo &info)
{
    return info.bundleName;
}
bool CheckerMock::IsValid(const CheckerManager::StoreInfo &info)
{
    return true;
}
bool CheckerMock::SetDistrustInfo(const CheckerManager::Distrust &distrust)
{
    return true;
};

bool CheckerMock::IsDistrust(const CheckerManager::StoreInfo &info)
{
    return true;
}
std::vector<CheckerManager::StoreInfo> CheckerMock::GetDynamicStores()
{
    return dynamicInfos_;
}
std::vector<CheckerManager::StoreInfo> CheckerMock::GetStaticStores()
{
    return staticInfos_;
}
bool CheckerMock::IsDynamic(const CheckerManager::StoreInfo &info)
{
    for (const auto &store : dynamicInfos_) {
        if (info.bundleName == store.bundleName && info.storeId == store.storeId) {
            return true;
        }
    }
    return false;
}
bool CheckerMock::IsStatic(const CheckerManager::StoreInfo &info)
{
    for (const auto &store : staticInfos_) {
        if (info.bundleName == store.bundleName && info.storeId == store.storeId) {
            return true;
        }
    }
    return false;
}
bool CheckerMock::AddDynamicStore(const CheckerManager::StoreInfo &storeInfo)
{
    return false;
}
bool CheckerMock::AddStaticStore(const CheckerManager::StoreInfo &storeInfo)
{
    return false;
}
bool CheckerMock::IsSwitches(const CheckerManager::StoreInfo &info)
{
    return false;
}
bool CheckerMock::SetSwitchesInfo(const CheckerManager::Switches &switches)
{
    return true;
}
} // namespace OHOS::DistributedData