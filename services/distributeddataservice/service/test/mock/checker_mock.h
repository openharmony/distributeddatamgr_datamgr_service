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
#ifndef OHOS_DISTRIBUTEDDATA_SERVICE_TEST_CHECKER_MOCK_H
#define OHOS_DISTRIBUTEDDATA_SERVICE_TEST_CHECKER_MOCK_H

#include "checker/checker_manager.h"
namespace OHOS {
namespace DistributedData {
class CheckerMock : public CheckerManager::Checker {
public:
    CheckerMock() noexcept;
    ~CheckerMock();
    void SetDynamic(const std::vector<CheckerManager::StoreInfo> &dynamicStores);
    void SetStatic(const std::vector<CheckerManager::StoreInfo> &staticStores);
    void Initialize() override;
    bool SetTrustInfo(const CheckerManager::Trust &trust) override;
    std::string GetAppId(const CheckerManager::StoreInfo &info) override;
    bool IsValid(const CheckerManager::StoreInfo &info) override;
    bool SetDistrustInfo(const CheckerManager::Distrust &distrust) override;
    bool IsDistrust(const CheckerManager::StoreInfo &info) override;
    std::vector<CheckerManager::StoreInfo> GetDynamicStores() override;
    std::vector<CheckerManager::StoreInfo> GetStaticStores() override;
    bool IsDynamic(const CheckerManager::StoreInfo &info) override;
    bool IsStatic(const CheckerManager::StoreInfo &info) override;
    bool AddDynamicStore(const CheckerManager::StoreInfo &storeInfo) override;
    bool AddStaticStore(const CheckerManager::StoreInfo &storeInfo) override;
    bool IsSwitches(const CheckerManager::StoreInfo &info) override;
    bool SetSwitchesInfo(const CheckerManager::Switches &switches) override;

private:
    std::vector<CheckerManager::StoreInfo> dynamicInfos_;
    std::vector<CheckerManager::StoreInfo> staticInfos_;
};

} // namespace DistributedData
} // namespace OHOS
#endif //OHOS_DISTRIBUTEDDATA_SERVICE_TEST_CHECKER_MOCK_H
