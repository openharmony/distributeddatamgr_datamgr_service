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

#define LOG_TAG "DataShareProfileConfig"

#include "data_share_profile_config.h"

#include <algorithm>
#include <cerrno>
#include <fstream>
#include <sstream>
#include <unistd.h>

#include "bundle_mgr_proxy.h"
#include "log_print.h"
#include "uri_utils.h"

namespace OHOS {
namespace DataShare {
// using namespace OHOS::Rdb;
std::mutex DataShareProfileConfig::infosMutex_;

constexpr const char *DATA_SHARE_PROFILE_META = "ohos.extension.dataShare";
constexpr const char *PROFILE_FILE_PREFIX = "$profile:";
constexpr const char *SEPARATOR = "/";
static constexpr int PATH_SIZE = 2;
const size_t PROFILE_PREFIX_LEN = strlen(PROFILE_FILE_PREFIX);
const std::string ProfileInfo::MODULE_SCOPE = "module";
const std::string ProfileInfo::APPLICATION_SCOPE = "application";
const std::string ProfileInfo::RDB_TYPE = "rdb";
const std::string ProfileInfo::PUBLISHED_DATA_TYPE = "publishedData";
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
    bool ret = GetValue(node, GET_NAME(uri), uri);
    GetValue(node, GET_NAME(crossUserMode), crossUserMode);
    GetValue(node, GET_NAME(readPermission), readPermission);
    GetValue(node, GET_NAME(writePermission), writePermission);
    return ret;
}

bool ProfileInfo::Marshal(json &node) const
{
    SetValue(node[GET_NAME(tableConfig)], tableConfig);
    SetValue(node[GET_NAME(isSilentProxyEnable)], isSilentProxyEnable);
    SetValue(node[GET_NAME(path)], storeName + SEPARATOR + tableName);
    SetValue(node[GET_NAME(scope)], scope);
    SetValue(node[GET_NAME(type)], type);
    return true;
}

bool ProfileInfo::Unmarshal(const json &node)
{
    bool ret = GetValue(node, GET_NAME(tableConfig), tableConfig);
    GetValue(node, GET_NAME(isSilentProxyEnable), isSilentProxyEnable);
    std::string path;
    ret = GetValue(node, GET_NAME(path), path);
    if (!ret) {
        return false;
    }
    std::vector<std::string> splitPath;
    SplitStr(path, SEPARATOR, splitPath);
    if (splitPath.size() < PATH_SIZE) {
        return false;
    }

    if (splitPath[0].empty() || splitPath[1].empty()) {
        return false;
    }
    storeName = splitPath[0];
    tableName = splitPath[1];
    GetValue(node, GET_NAME(scope), scope);
    GetValue(node, GET_NAME(type), type);
    return ret;
}

bool DataShareProfileConfig::GetResConfigFile(
    const AppExecFwk::ExtensionAbilityInfo &extensionInfo, std::string &profileInfo)
{
    bool isCompressed = !extensionInfo.hapPath.empty();
    std::string resourcePath = isCompressed ? extensionInfo.hapPath : extensionInfo.resourcePath;
    std::string resProfile = GetResProfileByMetadata(extensionInfo.metadata, resourcePath, isCompressed);
    if (resProfile.empty()) {
        return false;
    }
    profileInfo = resProfile;
    return true;
}

std::pair<bool, std::string> DataShareProfileConfig::GetDataPropertiesFromProxyDatas(const OHOS::AppExecFwk::ProxyData &proxyData,
    const std::string &resourcePath, bool isCompressed)
{
    std::string info = GetResProfileByMetadata(proxyData.metadata, resourcePath, isCompressed);
    if (info.empty()) {
        return std::make_pair(false, info);
    }
    return std::make_pair(true, info);
}

std::string DataShareProfileConfig::GetResProfileByMetadata(
    const AppExecFwk::Metadata &metadata, const std::string &resourcePath, bool isCompressed)
{
    std::string info;
    if (metadata.name.empty() || resourcePath.empty()) {
        return info;
    }
    std::shared_ptr<ResourceManager> resMgr = InitResMgr(resourcePath);
    if (resMgr == nullptr) {
        return info;
    }
    
    if (metadata.name == "dataProperties") {
        info = GetResFromResMgr(metadata.resource, *resMgr, isCompressed);
    }
    return info;
}

std::string DataShareProfileConfig::GetResProfileByMetadata(
    const std::vector<AppExecFwk::Metadata> &metadata, const std::string &resourcePath, bool isCompressed)
{
    std::string profileInfo;
    if (metadata.empty() || resourcePath.empty()) {
        return profileInfo;
    }
    std::shared_ptr<ResourceManager> resMgr = InitResMgr(resourcePath);
    if (resMgr == nullptr) {
        return profileInfo;
    }

    auto it = std::find_if(metadata.begin(), metadata.end(), [](AppExecFwk::Metadata meta) {
        return meta.name == DATA_SHARE_PROFILE_META;
    });
    if (it != metadata.end()) {
        return GetResFromResMgr((*it).resource, *resMgr, isCompressed);
    }

    return profileInfo;
}

std::shared_ptr<ResourceManager> DataShareProfileConfig::InitResMgr(const std::string &resourcePath)
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
    resMgr->AddResource(resourcePath.c_str());
    return resMgr;
}

std::string DataShareProfileConfig::GetResFromResMgr(
    const std::string &resName, ResourceManager &resMgr, bool isCompressed)
{
    std::string profileInfo;
    if (resName.empty()) {
        return profileInfo;
    }

    size_t pos = resName.rfind(PROFILE_FILE_PREFIX);
    if ((pos == std::string::npos) || (pos == resName.length() - PROFILE_PREFIX_LEN)) {
        ZLOGE("res name invalid, resName is %{public}s", resName.c_str());
        return profileInfo;
    }
    std::string profileName = resName.substr(pos + PROFILE_PREFIX_LEN);
    // hap is compressed status, get file content.
    if (isCompressed) {
        ZLOGD("compressed status.");
        std::unique_ptr<uint8_t[]> fileContent = nullptr;
        size_t len = 0;
        RState ret = resMgr.GetProfileDataByName(profileName.c_str(), len, fileContent);
        if (ret != SUCCESS || fileContent == nullptr) {
            ZLOGE("failed, ret is %{public}d, profileName is %{public}s", ret, profileName.c_str());
            return profileInfo;
        }
        if (len == 0) {
            ZLOGE("fileContent is empty, profileName is %{public}s", profileName.c_str());
            return profileInfo;
        }
        std::string rawData(fileContent.get(), fileContent.get() + len);
        if (!Config::IsJson(rawData)) {
            ZLOGE("rawData is not json, profileName is %{public}s", profileName.c_str());
            return profileInfo;
        }
        return rawData;
    }
    // hap is decompressed status, get file path then read file.
    std::string resPath;
    RState ret = resMgr.GetProfileByName(profileName.c_str(), resPath);
    if (ret != SUCCESS) {
        ZLOGE("profileName not found, ret is %{public}d, profileName is %{public}s", ret, profileName.c_str());
        return profileInfo;
    }
    std::string profile = ReadProfile(resPath);
    if (profile.empty()) {
        ZLOGE("Read profile failed, resPath is %{public}s", resPath.c_str());
        return profileInfo;
    }
    return profile;
}

bool DataShareProfileConfig::IsFileExisted(const std::string &filePath)
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

std::string DataShareProfileConfig::ReadProfile(const std::string &resPath)
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

bool DataShareProfileConfig::GetProfileInfo(const std::string &calledBundleName, int32_t currentUserId,
    std::map<std::string, ProfileInfo> &profileInfos)
{
    AppExecFwk::BundleInfo bundleInfo;
    if (!BundleMgrProxy::GetInstance()->GetBundleInfoFromBMS(calledBundleName, currentUserId, bundleInfo)) {
        ZLOGE("data share GetBundleInfoFromBMS failed! bundleName: %{public}s, currentUserId = %{public}d",
              calledBundleName.c_str(), currentUserId);
        return false;
    }
    for (auto &item : bundleInfo.extensionInfos) {
        if (item.type != AppExecFwk::ExtensionAbilityType::DATASHARE) {
            continue;
        }
        std::string info;
        auto ret = GetResConfigFile(item, info);
        if (!ret) {
            continue;
        }
        ProfileInfo profileInfo;
        if (!profileInfo.Unmarshall(info)) {
            ZLOGE("profileInfo Unmarshall error. infos: %{public}s", info.c_str());
            continue;
        }
        profileInfos[item.uri] = profileInfo;
    }
    return true;
}

AccessCrossMode DataShareProfileConfig::GetFromTableConfigs(const ProfileInfo &profileInfo,
        const std::string &tableUri, const std::string &storeUri)
{
    for (auto const &item : profileInfo.tableConfig) {
        if (item.uri == tableUri) {
            SetCrossUserMode(TABLE_MATCH_PRIORITY, item.crossUserMode);
            continue;
        }
        if (item.uri == storeUri) {
            SetCrossUserMode(STORE_MATCH_PRIORITY, item.crossUserMode);
            continue;
        }
    }
    return GetCrossUserMode();
}

void DataShareProfileConfig::SetCrossUserMode(uint8_t priority, uint8_t crossMode)
{
    if (crossMode_.second < priority && crossMode > AccessCrossMode::USER_UNDEFINED &&
        crossMode < AccessCrossMode::USER_MAX) {
        crossMode_.first = static_cast<AccessCrossMode>(crossMode);
        crossMode_.second = priority;
    }
}

AccessCrossMode DataShareProfileConfig::GetCrossUserMode()
{
    if (crossMode_.second != UNDEFINED_PRIORITY) {
        return crossMode_.first;
    }
    return AccessCrossMode::USER_UNDEFINED;
}
} // namespace DataShare
} // namespace OHOS

