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
#define LOG_TAG "CustomUtdInstaller"

#include <regex>
#include <sstream>
#include <vector>

#include "error_code.h"
#include "log_print.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "utd_graph.h"
#include "utd_json_parse.h"
#include "utd_custom_persistence.h"
#include "preset_type_descriptors.h"
#include "custom_utd_installer.h"

namespace OHOS {
namespace UDMF {
constexpr const char *CUSTOM_UTD_PATH = "/data/service/el1/";
constexpr const char *CUSTOM_UTD_FILE = "/distributeddata/utd/utd-adt.json";
CustomUtdInstaller::CustomUtdInstaller()
{
}

CustomUtdInstaller::~CustomUtdInstaller()
{
}

CustomUtdInstaller &CustomUtdInstaller::GetInstance()
{
    static CustomUtdInstaller instance;
    return instance;
}

sptr<AppExecFwk::IBundleMgr> CustomUtdInstaller::GetBundleManager()
{
    auto samgrProxy = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgrProxy == nullptr) {
        ZLOGE("samgrProxy is null");
        return nullptr;
    }
    auto bmsProxy = samgrProxy->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bmsProxy == nullptr) {
        ZLOGE("failed to get bms from samgrProxy");
    }
    return iface_cast<AppExecFwk::IBundleMgr>(bmsProxy);
}

int32_t CustomUtdInstaller::InstallUtd(const std::string &bundleName, int32_t user) {
    std::string path = CUSTOM_UTD_PATH + std::to_string(user) + CUSTOM_UTD_FILE;
    ZLOGE("===ZYH=== PATH IS %{public}s", path.c_str());
    std::vector<TypeDescriptorCfg> customTyepCfgs = UtdCustomPersistence::GetInstance().GetCustomTypesFromCfg(path);
    std::vector <TypeDescriptorCfg> presetTypes = PresetTypeDescriptors::GetInstance().GetPresetTypes();
    std::vector <std::string> modules = GetModulesFromBundleName(bundleName, user);
    for (std::string module : modules) {
        ZLOGE("===ZYH=== module is %{public}s", module.c_str());
        auto utdTypes = GetModuleCustomUtdTypes(bundleName, module, user);
        ZLOGE("===ZYH=== declarationTypes size : %{public}d referenceType size : %{public}d", utdTypes.first.size(), utdTypes.second.size());
        if (!UtdCfgsChecker::GetInstance().CheckTypeDescriptors(utdTypes, presetTypes, customTyepCfgs, bundleName)) {
            ZLOGE("Parse json failed, moduleJson: %{public}s.", module.c_str());
            continue;
        }
        ZLOGE("===ZYH=== check end ");
        //Update customTyepCfgs used for subsequent persistence of type definitions.
        for (TypeDescriptorCfg &declarationType : utdTypes.first) {
            ZLOGE("===ZYH=== get GetValidDeclarationType failed, typeId: %{public}s, declarationTypes size : %{public}d", declarationType.typeId.c_str(), utdTypes.first.size());
            for (auto iter = customTyepCfgs.begin(); iter != customTyepCfgs.end();) {
                ZLOGE("===ZYH=== get 111111GetValidDeclarationType failed, typeId: %{public}s, customTyepCfgs size : %{public}d", declarationType.typeId.c_str(), customTyepCfgs.size());
                if (iter->typeId == declarationType.typeId) {
                    declarationType.installers = iter->installers;
                    iter = customTyepCfgs.erase(iter);
                    ZLOGE("===ZYH=== get GetValidDeclarationType failed, typeId1: %{public}s, typeId2: %{public}s,",
                          declarationType.typeId.c_str(), iter->typeId.c_str());
                } else {
                    ZLOGE("===ZYH=== not equal");
                    iter ++;
                }
            }
            declarationType.installers.emplace(bundleName);
            declarationType.owner = bundleName;
            ZLOGE("===ZYH=== get 222222GetValidDeclarationType failed, typeId: %{public}s, customTyepCfgs size : %{public}d", declarationType.typeId.c_str(), customTyepCfgs.size());
            customTyepCfgs.push_back(declarationType);
        }
        for (TypeDescriptorCfg &referenceType : utdTypes.second) {
            for (auto iter = customTyepCfgs.begin(); iter != customTyepCfgs.end();) {
                if (iter->typeId == referenceType.typeId) {
                    referenceType.installers = iter->installers;
                    iter = customTyepCfgs.erase(iter);
                    ZLOGE("===ZYH=== get GetValidDeclarationType failed, typeId1: %{public}s, typeId2: %{public}s,",
                        referenceType.typeId.c_str(), iter->typeId.c_str());
                } else {
                    iter ++;
                }
            }
            referenceType.installers.emplace(bundleName);
            ZLOGE("===ZYH=== get GetValidDeclarationType failed, typeId: %{public}s. customTyepCfgs size : %{public}d", referenceType.typeId.c_str(), customTyepCfgs.size());
            customTyepCfgs.push_back(referenceType);
        }
        UtdCustomPersistence::GetInstance().PersistingCustomUtdData(customTyepCfgs, path);
    }
    return E_OK;
}

int32_t CustomUtdInstaller::UninstallUtd(const std::string &bundleName, int32_t user)
{
    ZLOGE("===ZYH=== Uninstall");
    std::string path = CUSTOM_UTD_PATH + std::to_string(user) + CUSTOM_UTD_FILE;
    std::vector<TypeDescriptorCfg> customTyepCfgs = UtdCustomPersistence::GetInstance().GetCustomTypesFromCfg(path);
    std::vector<TypeDescriptorCfg> deleteTypes;
    for (auto iter = customTyepCfgs.begin(); iter != customTyepCfgs.end();) {
        auto it = find (iter->installers.begin(), iter->installers.end(), bundleName);
        if (it != iter->installers.end()) {
            iter->installers.erase(it);
        }
        if (iter->installers.empty()) {
            deleteTypes.push_back(*iter);
        }
        iter++;
    }
    for (auto dtype: deleteTypes) {
        for (auto customIt = customTyepCfgs.begin(); customIt != customTyepCfgs.end();) {
            auto delIt = find(customIt->belongingToTypes.begin(), customIt->belongingToTypes.end(), dtype.typeId);
            if (delIt != customIt->belongingToTypes.end() && customIt->typeId == dtype.typeId) {
                customIt = customTyepCfgs.erase(customIt);
            } else {
                customIt++;
            }
        }
    }
    ZLOGE("===ZYH=== READY TO WRITE!!!!!! ");
    UtdCustomPersistence::GetInstance().PersistingCustomUtdData(customTyepCfgs, path);
    ZLOGE("===ZYH===  WRITE DONE!!!!!! ");
    return E_OK;
}

std::vector<std::string> CustomUtdInstaller::GetModulesFromBundleName(const std::string &bundleName, int32_t user)
{
    std::vector<std::string> modules;
    auto bundlemgr = GetBundleManager();
    AppExecFwk::BundleInfo bundleInfo;
    if (!bundlemgr->GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT, bundleInfo, user)) {
        ZLOGE("===ZYH=== get local bundle info failed");
    }
    return bundleInfo.hapModuleNames;
}

UtdCfgsChecker::CustomUtdCfgs CustomUtdInstaller::GetModuleCustomUtdTypes(const std::string &bundleName, const std::string &moduleName,
    int32_t user)
{
    auto bundlemgr = GetBundleManager();
    std::string jsonStr;
    auto status = bundlemgr->GetJsonProfile(AppExecFwk::ProfileType::UTD_SDT_PROFILE, bundleName, moduleName, jsonStr,
        user);
    if (status != NO_ERROR) {
        ZLOGE("===ZYH=== get local bundle info failed");
    }
    ZLOGE("===ZYH=== jsonStr is %{public}s", jsonStr.c_str());

    std::vector<TypeDescriptorCfg> declarationType;
    std::vector<TypeDescriptorCfg> referenceType;
    std::pair<std::vector<TypeDescriptorCfg>, std::vector<TypeDescriptorCfg>> typeCfgs;
    UtdJsonParse utdJsonParse_;
    int32_t res = utdJsonParse_.ParseJsonData(jsonStr, declarationType, referenceType);
    ZLOGE("===ZYH=== res: %{public}d", res);
    if (!jsonStr.empty() && res == E_OK) {
        ZLOGE("===ZYH=== declarationTypes size : %{public}d referenceType size : %{public}d", declarationType.size(), referenceType.size());
        typeCfgs.first = declarationType;
        typeCfgs.second = referenceType;
    }
    ZLOGE("===ZYH=== declarationTypes size : %{public}d referenceType size : %{public}d", typeCfgs.first.size(), typeCfgs.second.size());

    return typeCfgs;
}
}
}