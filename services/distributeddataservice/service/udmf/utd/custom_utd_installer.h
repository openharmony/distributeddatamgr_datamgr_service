/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef DATAMGR_SERVICE_UTD_MANAGER_H
#define DATAMGR_SERVICE_UTD_MANAGER_H

#include <vector>
#include <string>

#include "bundlemgr/bundle_mgr_proxy.h"
#include "utd_common.h"
#include "utd_cfgs_checker.h"
#include "system_ability.h"

namespace OHOS {
namespace UDMF {
class CustomUtdInstaller {
public:
    static CustomUtdInstaller &GetInstance();
    int32_t InstallUtd(const std::string &bundleName, int32_t user);
    int32_t UninstallUtd(const std::string &bundleName, int32_t user);

private:
    CustomUtdInstaller();
    ~CustomUtdInstaller();
    CustomUtdInstaller(const CustomUtdInstaller &obj) = delete;
    CustomUtdInstaller &operator=(const CustomUtdInstaller &obj) = delete;
    sptr<AppExecFwk::IBundleMgr> GetBundleManager();
    std::vector<std::string> GetHapModules(const std::string &bundleName, int32_t user);
    CustomUtdCfgs GetModuleCustomUtdTypes(const std::string &bundleName, const std::string &moduleName, int32_t user);
    int32_t SaveCustomUtds(const CustomUtdCfgs &utdTypes, std::vector<TypeDescriptorCfg> customTyepCfgs,
        const std::string &bundleName, const std::string &path);
};
} // namespace UDMF
} // namespace OHOS
#endif // DATAMGR_SERVICE_UTD_MANAGER_H
