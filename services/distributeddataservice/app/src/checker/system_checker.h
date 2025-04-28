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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_SYSTEM_CHECKER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_SYSTEM_CHECKER_H
#include <map>
#include "checker/checker_manager.h"
namespace OHOS {
namespace DistributedData {
class SystemChecker : public CheckerManager::Checker {
public:
    SystemChecker() noexcept;
    ~SystemChecker();
    void Initialize() override;
    bool SetTrustInfo(const CheckerManager::Trust &trust) override;
    bool SetDistrustInfo(const CheckerManager::Distrust &distrust) override;
    bool SetSwitchesInfo(const CheckerManager::Switches &switches) override;
    std::string GetAppId(const CheckerManager::StoreInfo &info) override;
    bool IsValid(const CheckerManager::StoreInfo &info) override;
    bool IsDistrust(const CheckerManager::StoreInfo &info) override;
    bool IsSwitches(const CheckerManager::StoreInfo &info) override;
    bool AddDynamicStore(const CheckerManager::StoreInfo &storeInfos) override;
    bool AddStaticStore(const CheckerManager::StoreInfo &storeInfos) override;
    std::vector<CheckerManager::StoreInfo> GetDynamicStores() override;
    std::vector<CheckerManager::StoreInfo> GetStaticStores() override;
    bool IsDynamic(const CheckerManager::StoreInfo &info) override;
    bool IsStatic(const CheckerManager::StoreInfo &info) override;

private:
    std::map<std::string, std::string> trusts_;
    std::map<std::string, std::string> distrusts_;
    std::map<std::string, std::string> switches_;
    static SystemChecker instance_;
    std::vector<CheckerManager::StoreInfo> dynamicStores_;
    std::vector<CheckerManager::StoreInfo> staticStores_;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_SYSTEM_CHECKER_H