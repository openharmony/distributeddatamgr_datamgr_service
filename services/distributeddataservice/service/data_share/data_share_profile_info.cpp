/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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
 
#define LOG_TAG "DataShareProfileInfo"
#include "data_share_profile_info.h"

#include <fstream>
#include <sstream>
#include <unistd.h>

#include "bundle_info.h"
#include "log_print.h"

namespace OHOS::DataShare {
namespace {
    constexpr const char* DATA_SHARE_PROFILE_META = "ohos.extension.dataShare";
    constexpr const char* PROFILE_FILE_PREFIX = "$profile:";
    const size_t PROFILE_PREFIX_LEN = strlen(PROFILE_FILE_PREFIX);
}
bool Config::Marshal(json &node) const
{
    SetValue(node[GET_NAME(uri)], uri);
    SetValue(node[GET_NAME(crossUserMode)], crossUserMode);
    SetValue(node[GET_NAME(readPermission)], readPermission);
    SetValue(node[GET_NAME(writePermission)], writePermission);
    return true;
}

bool Config::Unmarshal(const json &node)
{
    bool ret = true;
    ret = GetValue(node, GET_NAME(uri), uri) && ret;
    GetValue(node, GET_NAME(crossUserMode), crossUserMode);
    GetValue(node, GET_NAME(readPermission), readPermission);
    GetValue(node, GET_NAME(writePermission), writePermission);
    return ret;
}

bool ProfileInfo::Marshal(json &node) const
{
    SetValue(node[GET_NAME(tableConfig)], tableConfig);
    return true;
}

bool ProfileInfo::Unmarshal(const json &node)
{
    bool ret = true;
    ret = GetValue(node, GET_NAME(tableConfig), tableConfig) && ret;
    return ret;
}

bool DataShareProfileInfo::LoadProfileInfoFromExtension(const AppExecFwk::BundleInfo &bundleInfo,
    ProfileInfo &profileInfo, bool &isSingleApp)
{
    isSingleApp = bundleInfo.singleton;
    // non singleApp don't need get profileInfo
    if (!isSingleApp) {
        return true;
    }

    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type == AppExecFwk::ExtensionAbilityType::DATASHARE) {
            std::vector<std::string> infos;
            auto ret = GetResConfigFile(item, infos);
            if (!ret) {
                ZLOGE("GetProfileFromExtension failed!");
                return false;
            }
            return profileInfo.Unmarshall(infos[0]);
        }
    }
    ZLOGE("not find datashare extension!");
    return false;
}

bool DataShareProfileInfo::GetResConfigFile(const AppExecFwk::ExtensionAbilityInfo &extensionInfo,
    std::vector<std::string> &profileInfos)
{
    bool isCompressed = !extensionInfo.hapPath.empty();
    std::string resourcePath = isCompressed ? extensionInfo.hapPath : extensionInfo.resourcePath;
    if (!GetResProfileByMetadata(extensionInfo.metadata, resourcePath, isCompressed, profileInfos)) {
        ZLOGE("GetResProfileByMetadata failed");
        return false;
    }
    if (profileInfos.empty()) {
        ZLOGE("no valid file can be obtained");
        return false;
    }
    int32_t infoSize = profileInfos.size();
    ZLOGD("The size of the profile info is : %{public}d", infoSize);
    return true;
}

bool DataShareProfileInfo::GetResProfileByMetadata(const std::vector<AppExecFwk::Metadata> &metadata,
    const std ::string &resourcePath, bool isCompressed, std::vector<std::string> &profileInfos) const
{
    if (metadata.empty()) {
        ZLOGE("GetResProfileByMetadata failed due to empty metadata");
        return false;
    }
    if (resourcePath.empty()) {
        ZLOGE("GetResProfileByMetadata failed due to empty resourcePath");
        return false;
    }
    std::shared_ptr<ResourceManager> resMgr = InitResMgr(resourcePath);
    if (resMgr == nullptr) {
        ZLOGE("GetResProfileByMetadata init resMgr failed");
        return false;
    }

    std::string dataShareProfileMeta = DATA_SHARE_PROFILE_META;
    for_each(metadata.begin(), metadata.end(),
        [this, &resMgr, &dataShareProfileMeta, isCompressed, &profileInfos](const AppExecFwk::Metadata& data)->void {
            if ((dataShareProfileMeta.compare(data.name) == 0)
                && (!GetResFromResMgr(data.resource, *resMgr, isCompressed, profileInfos))) {
                ZLOGW("GetResFromResMgr failed");
            }
        });
    return true;
}

std::shared_ptr<ResourceManager> DataShareProfileInfo::InitResMgr(const std::string &resourcePath) const
{
    if (resourcePath.empty()) {
        ZLOGE("InitResMgr failed due to invalid param");
        return nullptr;
    }
    std::shared_ptr<ResourceManager> resMgr(CreateResourceManager());
    if (resMgr == nullptr) {
        ZLOGE("InitResMgr resMgr is nullptr");
        return nullptr;
    }

    std::unique_ptr<ResConfig> resConfig(CreateResConfig());
    if (resConfig == nullptr) {
        ZLOGE("InitResMgr resConfig is nullptr");
        return nullptr;
    }
    resMgr->UpdateResConfig(*resConfig);

    if (!resourcePath.empty() && !resMgr->AddResource(resourcePath.c_str())) {
        ZLOGE("InitResMgr AddResource failed, resourcePath is %{private}s", resourcePath.c_str());
        return nullptr;
    }
    return resMgr;
}

bool DataShareProfileInfo::GetResFromResMgr(const std::string &resName, ResourceManager &resMgr,
    bool isCompressed, std::vector<std::string> &profileInfos) const
{
    if (resName.empty()) {
        ZLOGE("GetResFromResMgr res name is empty");
        return false;
    }

    size_t pos = resName.rfind(PROFILE_FILE_PREFIX);
    if ((pos == std::string::npos) || (pos == resName.length() - PROFILE_PREFIX_LEN)) {
        ZLOGE("GetResFromResMgr res name is invalid");
        return false;
    }
    std::string profileName = resName.substr(pos + PROFILE_PREFIX_LEN);
    // hap is compressed status, get file content.
    if (isCompressed) {
        ZLOGD("compressed status.");
        std::unique_ptr<uint8_t[]> fileContent = nullptr;
        size_t len = 0;
        RState ret = resMgr.GetProfileDataByName(profileName.c_str(), len, fileContent);
        if (ret != SUCCESS) {
            ZLOGE("GetProfileDataByName failed, ret is %{public}d", ret);
            return false;
        }
        if (fileContent == nullptr || len == 0) {
            ZLOGE("invalid data, fileContent is empty");
            return false;
        }
        std::string rawData(fileContent.get(), fileContent.get() + len);
        if (!Config::IsJson(rawData)) {
            ZLOGE("invalid rawData, not satisfied the json format");
            return false;
        }
        profileInfos.push_back(std::move(rawData));
        return true;
    }
    // hap is decompressed status, get file path then read file.
    std::string resPath;
    RState ret = resMgr.GetProfileByName(profileName.c_str(), resPath);
    if (ret != SUCCESS) {
        ZLOGE("GetResFromResMgr profileName cannot be found, ret is %{public}d", ret);
        return false;
    }
    ZLOGD("GetResFromResMgr resPath is %{private}s", resPath.c_str());
    std::string profile = ReadProfile(resPath);
    if (profile.empty()) {
        ZLOGE("Read profile failed");
        return false;
    }
    profileInfos.push_back(std::move(profile));
    return true;
}

bool DataShareProfileInfo::IsFileExisted(const std::string &filePath) const
{
    if (filePath.empty()) {
        ZLOGE("the file is not existed due to empty file path");
        return false;
    }

    if (access(filePath.c_str(), F_OK) != 0) {
        ZLOGE("can not access the file: %{private}s", filePath.c_str());
        return false;
    }
    return true;
}

std::string DataShareProfileInfo::ReadProfile(const std::string &resPath) const
{
    if (!IsFileExisted(resPath)) {
        ZLOGE("the file is not existed");
        return "";
    }
    std::fstream in;
    char errBuffer[256];
    errBuffer[0] = '\0';
    in.open(resPath, std::ios_base::in | std::ios_base::binary);
    if (!in.is_open()) {
        strerror_r(errno, errBuffer, sizeof(errBuffer));
        ZLOGE("the file cannot be open due to  %{public}s", errBuffer);
        return "";
    }
    in.seekg(0, std::ios::end);
    int64_t size = in.tellg();
    if (size <= 0) {
        ZLOGE("the file is an empty file");
        return "";
    }
    in.seekg(0, std::ios::beg);
    std::ostringstream tmp;
    tmp << in.rdbuf();
    return tmp.str();
}
} // namespace OHOS::DataShare