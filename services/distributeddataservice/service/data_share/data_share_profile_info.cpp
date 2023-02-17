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

#include <cerrno>
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

bool DataShareProfileInfo::GetProfileInfoFromExtension(const AppExecFwk::BundleInfo &bundleInfo,
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
            bool isCompressed = !item.hapPath.empty();
            std::string resourcePath = isCompressed ? item.hapPath : item.resourcePath;
            if (!GetResProfileByMetadata(item.metadata, resourcePath, isCompressed, infos) || infos.empty()) {
                ZLOGE("failed, bundleName is %{public}s, resourcePath is %{public}s, metadata.size is %{public}zu,"
                    "infos.size is %{public}zu", bundleInfo.name.c_str(), resourcePath.c_str(), item.metadata.size(),
                    infos.size());
                return false;
            }
            return profileInfo.Unmarshall(infos[0]);
        }
    }
    ZLOGE("not find datashare extension, bundleName is %{public}s", bundleInfo.name.c_str());
    return false;
}

bool DataShareProfileInfo::GetResProfileByMetadata(const std::vector<AppExecFwk::Metadata> &metadata,
    const std::string &resourcePath, bool isCompressed, std::vector<std::string> &profileInfos) const
{
    if (metadata.empty() || resourcePath.empty()) {
        return false;
    }
    std::shared_ptr<ResourceManager> resMgr = InitResMgr(resourcePath);
    if (resMgr == nullptr) {
        return false;
    }

    for (auto &meta : metadata) {
        if (meta.name.compare(DATA_SHARE_PROFILE_META) == 0) {
            return GetResFromResMgr(meta.resource, *resMgr, isCompressed, profileInfos);
        }
    }
    return false;
}

std::shared_ptr<ResourceManager> DataShareProfileInfo::InitResMgr(const std::string &resourcePath) const
{
    std::shared_ptr<ResourceManager> resMgr(CreateResourceManager());
    if (resMgr == nullptr) {
        return nullptr;
    }

    std::unique_ptr<ResConfig> resConfig(CreateResConfig());
    if (resConfig == nullptr) {
        return nullptr;
    }
    resMgr->UpdateResConfig(*resConfig);

    if (!resMgr->AddResource(resourcePath.c_str())) {
        ZLOGE("add resource failed, resourcePath is %{public}s", resourcePath.c_str());
        return nullptr;
    }
    return resMgr;
}

bool DataShareProfileInfo::GetResFromResMgr(const std::string &resName, ResourceManager &resMgr,
    bool isCompressed, std::vector<std::string> &profileInfos) const
{
    if (resName.empty()) {
        return false;
    }

    size_t pos = resName.rfind(PROFILE_FILE_PREFIX);
    if ((pos == std::string::npos) || (pos == resName.length() - PROFILE_PREFIX_LEN)) {
        ZLOGE("res name invalid, resName is %{public}s", resName.c_str());
        return false;
    }
    std::string profileName = resName.substr(pos + PROFILE_PREFIX_LEN);
    // hap is compressed status, get file content.
    if (isCompressed) {
        ZLOGD("compressed status.");
        std::unique_ptr<uint8_t[]> fileContent = nullptr;
        size_t len = 0;
        RState ret = resMgr.GetProfileDataByName(profileName.c_str(), len, fileContent);
        if (ret != SUCCESS || fileContent == nullptr) {
            ZLOGE("failed, ret is %{public}d, profileName is %{public}s",
                ret, profileName.c_str());
            return false;
        }
        if (len == 0) {
            ZLOGE("fileContent is empty, profileName is %{public}s", profileName.c_str());
            return false;
        }
        std::string rawData(fileContent.get(), fileContent.get() + len);
        if (!Config::IsJson(rawData)) {
            ZLOGE("rawData is not json, profileName is %{public}s", profileName.c_str());
            return false;
        }
        profileInfos.push_back(std::move(rawData));
        return true;
    }
    // hap is decompressed status, get file path then read file.
    std::string resPath;
    RState ret = resMgr.GetProfileByName(profileName.c_str(), resPath);
    if (ret != SUCCESS) {
        ZLOGE("profileName not found, ret is %{public}d, profileName is %{public}s", ret, profileName.c_str());
        return false;
    }
    std::string profile = ReadProfile(resPath);
    if (profile.empty()) {
        ZLOGE("Read profile failed, resPath is %{public}s", resPath.c_str());
        return false;
    }
    profileInfos.push_back(std::move(profile));
    return true;
}

bool DataShareProfileInfo::IsFileExisted(const std::string &filePath) const
{
    if (filePath.empty()) {
        return false;
    }
    if (access(filePath.c_str(), F_OK) != 0) {
        ZLOGE("can not access file, errno is %{public}d, filePath is %{public}s", errno, filePath.c_str());
        return false;
    }
    return true;
}

std::string DataShareProfileInfo::ReadProfile(const std::string &resPath) const
{
    if (!IsFileExisted(resPath)) {
        return "";
    }
    std::fstream in;
    in.open(resPath, std::ios_base::in | std::ios_base::binary);
    if (!in.is_open()) {
        ZLOGE("the file can not open, errno is %{public}d", errno);
        return "";
    }
    in.seekg(0, std::ios::end);
    int64_t size = in.tellg();
    if (size <= 0) {
        ZLOGE("the file is empty, resPath is %{public}s", resPath.c_str());
        return "";
    }
    in.seekg(0, std::ios::beg);
    std::ostringstream tmp;
    tmp << in.rdbuf();
    return tmp.str();
}
} // namespace OHOS::DataShare