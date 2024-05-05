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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CHECKER_CHECKER_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CHECKER_CHECKER_MANAGER_H
#include <vector>
#include "visibility.h"
#include "concurrent_map.h"
namespace OHOS {
namespace DistributedData {
class CheckerManager {
public:
    struct Trust {
        std::string bundleName;
        std::string appId;
        std::string packageName;
        std::string base64Key;
        std::string checker;
    };
    using Distrust = Trust;
    using Switches = Trust;
    struct StoreInfo {
        pid_t uid;
        uint32_t tokenId;
        std::string bundleName;
        std::string storeId;
    };
    class Checker {
    public:
        virtual void Initialize() = 0;
        virtual bool SetTrustInfo(const Trust &trust) = 0;
        virtual bool SetDistrustInfo(const Distrust &distrust) = 0;
        virtual bool SetSwitchesInfo(const Switches &switches) = 0;
        virtual std::string GetAppId(const StoreInfo &info) = 0;
        virtual bool IsValid(const StoreInfo &info) = 0;
        virtual bool IsDistrust(const StoreInfo &info) = 0;
        virtual bool IsSwitches(const StoreInfo &info) = 0;
        virtual bool AddDynamicStore(const StoreInfo& storeInfo) = 0;
        virtual bool AddStaticStore(const StoreInfo& storeInfo) = 0;
        virtual std::vector<StoreInfo> GetDynamicStores() = 0;
        virtual std::vector<StoreInfo> GetStaticStores() = 0;
        virtual bool IsDynamic(const StoreInfo &info) = 0;
        virtual bool IsStatic(const StoreInfo &info) = 0;
    protected:
        API_EXPORT ~Checker() = default;
    };
    API_EXPORT static CheckerManager &GetInstance();
    API_EXPORT void RegisterPlugin(const std::string &checker, std::function<Checker *()> getter);
    API_EXPORT std::string GetAppId(const StoreInfo &info);
    API_EXPORT std::vector<StoreInfo> GetDynamicStores();
    API_EXPORT std::vector<StoreInfo> GetStaticStores();
    API_EXPORT bool IsDynamic(const StoreInfo &info);
    API_EXPORT bool IsStatic(const StoreInfo &info);
    API_EXPORT bool IsValid(const StoreInfo &info);
    API_EXPORT bool IsDistrust(const StoreInfo &info);
    API_EXPORT bool IsSwitches(const StoreInfo &info);
    API_EXPORT void LoadCheckers(std::vector<std::string> &checkers);
    API_EXPORT Checker *GetChecker(const std::string &checker);
private:
    std::map<std::string, Checker *> checkers_;
    ConcurrentMap<std::string, std::function<Checker *()>> getters_;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_CHECKER_CHECKER_MANAGER_H