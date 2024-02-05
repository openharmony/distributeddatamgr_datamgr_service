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
#include "custom_utd_json_parser.h"
#include "custom_utd_store.h"
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
        ZLOGE("samgrProxy is null.");
        return nullptr;
    }
    auto bmsProxy = samgrProxy->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY_ID);
    if (bmsProxy == nullptr) {
        ZLOGE("failed to get bms from samgrProxy.");
    }
    return iface_cast<AppExecFwk::IBundleMgr>(bmsProxy);
}

int32_t CustomUtdInstaller::InstallUtd(const std::string &bundleName, int32_t user)
{
    std::string path = CUSTOM_UTD_PATH + std::to_string(user) + CUSTOM_UTD_FILE;
    std::vector<TypeDescriptorCfg> customTyepCfgs = CustomUtdStore::GetInstance().GetTypeCfgs(path);
    std::vector <TypeDescriptorCfg> presetTypes = PresetTypeDescriptors::GetInstance().GetPresetTypes();
    std::vector <std::string> modules = GetHapModules(bundleName, user);
    bool isSucc = true;
    for (std::string module : modules) {
        auto utdTypes = GetModuleCustomUtdTypes(bundleName, module, user);
        if (utdTypes.first.empty() && utdTypes.second.empty()) {
            ZLOGD("Module custom utd types is empty.");
            continue;
        }
        if (!UtdCfgsChecker::GetInstance().CheckTypeDescriptors(utdTypes, presetTypes, customTyepCfgs, bundleName)) {
            ZLOGE("Parse json failed, moduleName: %{public}s, bundleName: %{public}s.", module.c_str(),
                bundleName.c_str());
            isSucc = false;
            continue;
        }
        if (SaveCustomUtds(utdTypes, customTyepCfgs, bundleName, path) != E_OK) {
            ZLOGE("Install save custom utds failed, moduleName: %{public}s, bundleName: %{public}s.", module.c_str(),
                bundleName.c_str());
            isSucc = false;
            continue;
        }
    }
    if (!isSucc) {
        return E_ERROR;
    }
    return E_OK;
}

int32_t CustomUtdInstaller::UninstallUtd(const std::string &bundleName, int32_t user)
{
    std::string path = CUSTOM_UTD_PATH + std::to_string(user) + CUSTOM_UTD_FILE;
    std::vector<TypeDescriptorCfg> customTyepCfgs = CustomUtdStore::GetInstance().GetTypeCfgs(path);
    std::vector<TypeDescriptorCfg> deletionMock;
    if (!customTyepCfgs.empty()) {
        deletionMock.insert(deletionMock.end(), customTyepCfgs.begin(), customTyepCfgs.end());
    }
    for (auto iter = deletionMock.begin(); iter != deletionMock.end();) {
        auto it = find (iter->installerBundles.begin(), iter->installerBundles.end(), bundleName);
        if (it != iter->installerBundles.end()) {
            iter->installerBundles.erase(it);
        }
        if (iter->installerBundles.empty()) {
            iter = deletionMock.erase(iter);
        } else {
            iter++;
        }
    }
    std::vector <TypeDescriptorCfg> presetTypes = PresetTypeDescriptors::GetInstance().GetPresetTypes();
    if (!UtdCfgsChecker::GetInstance().CheckBelongingToTypes(deletionMock, presetTypes)) {
        ZLOGW("Uninstall error, because of belongingToTypes check failed.");
        return E_ERROR;
    }
    for (auto customIter = customTyepCfgs.begin(); customIter != customTyepCfgs.end();) {
        auto InstallerIter = find (customIter->installerBundles.begin(), customIter->installerBundles.end(),
            bundleName);
        if (InstallerIter != customIter->installerBundles.end()) {
            customIter->installerBundles.erase(InstallerIter);
        }
        if (customIter->installerBundles.empty()) {
            customIter = customTyepCfgs.erase(customIter);
        } else {
            customIter++;
        }
    }
    if (CustomUtdStore::GetInstance().SaveTypeCfgs(customTyepCfgs, path) != E_OK) {
        ZLOGE("Save type cfgs failed, bundleName: %{public}s.", bundleName.c_str());
        return E_ERROR;
    }
    return E_OK;
}

std::vector<std::string> CustomUtdInstaller::GetHapModules(const std::string &bundleName, int32_t user)
{
    auto bundlemgr = GetBundleManager();
    AppExecFwk::BundleInfo bundleInfo;
    if (!bundlemgr->GetBundleInfo(bundleName, AppExecFwk::BundleFlag::GET_BUNDLE_DEFAULT, bundleInfo, user)) {
        ZLOGE("Get local bundle info failed, bundleName: %{public}s.", bundleName.c_str());
    }
    return bundleInfo.hapModuleNames;
}

CustomUtdCfgs CustomUtdInstaller::GetModuleCustomUtdTypes(const std::string &bundleName,
    const std::string &moduleName, int32_t user)
{
    auto bundlemgr = GetBundleManager();
    std::string jsonStr;
    CustomUtdCfgs typeCfgs;
    auto status = bundlemgr->GetJsonProfile(AppExecFwk::ProfileType::UTD_SDT_PROFILE, bundleName, moduleName, jsonStr,
        user);
    if (status != NO_ERROR) {
        ZLOGD("get json profile failed, bundleName: %{public}s.", bundleName.c_str());
        return typeCfgs;
    }
    if (jsonStr.empty()) {
        ZLOGE("JsonStr is empty, bundleName: %{public}s.", bundleName.c_str());
        return typeCfgs;
    }
    std::vector<TypeDescriptorCfg> declarationType;
    std::vector<TypeDescriptorCfg> referenceType;

    CustomUtdJsonParser customUtdJsonParser_;
    bool res = customUtdJsonParser_.ParseUserCustomUtdJson(jsonStr, declarationType, referenceType);
    if (!jsonStr.empty() && res) {
        typeCfgs.first = declarationType;
        typeCfgs.second = referenceType;
    }
    return typeCfgs;
}

int32_t CustomUtdInstaller::SaveCustomUtds(const CustomUtdCfgs &utdTypes, std::vector<TypeDescriptorCfg> customTyepCfgs,
    const std::string &bundleName, const std::string &path)
{
    for (TypeDescriptorCfg declarationType : utdTypes.first) {
        for (auto iter = customTyepCfgs.begin(); iter != customTyepCfgs.end();) {
            if (iter->typeId == declarationType.typeId) {
                declarationType.installerBundles = iter->installerBundles;
                iter = customTyepCfgs.erase(iter);
            } else {
                iter ++;
            }
        }
        declarationType.installerBundles.emplace(bundleName);
        declarationType.ownerBundle = bundleName;
        customTyepCfgs.push_back(declarationType);
    }
    for (TypeDescriptorCfg referenceType : utdTypes.second) {
        bool found = false;
        for (auto &typeCfg : customTyepCfgs) {
            if (typeCfg.typeId == referenceType.typeId) {
                typeCfg.installerBundles.emplace(bundleName);
                found = true;
                break;
            }
        }
        if (!found) {
            referenceType.installerBundles.emplace(bundleName);
            customTyepCfgs.push_back(referenceType);
        }
    }
    if (CustomUtdStore::GetInstance().SaveTypeCfgs(customTyepCfgs, path) != E_OK) {
        ZLOGE("Save type cfgs failed, bundleName: %{public}s.", bundleName.c_str());
        return E_ERROR;
    }
    return E_OK;
}
}
}