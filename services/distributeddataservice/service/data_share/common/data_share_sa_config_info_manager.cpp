/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#define LOG_TAG "DSConfigInfoMgr"

#include <fstream>

#include "data_share_sa_config_info_manager.h"

#include "common_utils.h"
#include "datashare_errno.h"
#include "ipc_skeleton.h"
#include "log_print.h"
#include "utils.h"

namespace OHOS::DataShare {
static constexpr const char *CONFIG_PATH = "/system/etc/distributeddata/arkdata/datashare/";
static constexpr const char *FILE_NAME = "/datashare.json";
// keep MAX_FILE_SIZE consistent with the file size defined in data_share_profile_config.cpp
static constexpr size_t MAX_FILE_SIZE = 10 * 1024 * 1024;
static constexpr size_t MAX_STRING_SIZE = 256;

bool SAConfigProxyData::Marshal(json &node) const
{
    SetValue(node[GET_NAME(uri)], uri);
    SetValue(node[GET_NAME(requiredReadPermission)], requiredReadPermission);
    SetValue(node[GET_NAME(requiredWritePermission)], requiredWritePermission);
    SetValue(node[GET_NAME(profile)], profile);
    return true;
}

bool SAConfigProxyData::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(uri), uri);
    if (uri.size() >= MAX_STRING_SIZE || !URIUtils::IsDataProxyURI(uri)) {
        ZLOGE("uri:%{public}s is invalid", URIUtils::Anonymous(uri).c_str());
        uri = "";
        return false;
    }
    GetValue(node, GET_NAME(requiredReadPermission), requiredReadPermission);
    if (requiredReadPermission.size() >= MAX_STRING_SIZE) {
        ZLOGE("readPermission size:%{public}zu is invalid", requiredReadPermission.size());
        requiredReadPermission = "";
        return false;
    }
    GetValue(node, GET_NAME(requiredWritePermission), requiredWritePermission);
    if (requiredWritePermission.size() >= MAX_STRING_SIZE) {
        ZLOGE("writePermisison size:%{public}zu is invalid", requiredWritePermission.size());
        requiredWritePermission = "";
        return false;
    }
    GetValue(node, GET_NAME(profile), profile);
    return true;
}

bool DataShareSAConfigInfo::Marshal(json &node) const
{
    SetValue(node[GET_NAME(proxyData)], proxyData);
    return true;
}

bool DataShareSAConfigInfo::Unmarshal(const json &node)
{
    GetValue(node, GET_NAME(proxyData), proxyData);
    return true;
}

std::shared_ptr<DataShareSAConfigInfoManager> DataShareSAConfigInfoManager::GetInstance()
{
    static std::shared_ptr<DataShareSAConfigInfoManager> proxy = std::make_shared<DataShareSAConfigInfoManager>();
    return proxy;
}

int32_t DataShareSAConfigInfoManager::LoadConfigInfo(const std::string &pathName, DataShareSAConfigInfo &configInfo)
{
    // pathName is a valid SAID string consisting only of digits, like "12321"
    // the real path is /system/etc/distributeddata/arkdata/datashare/12321(SAID)/datashare.json
    std::string filePath = std::string(CONFIG_PATH) + pathName + std::string(FILE_NAME);
    std::string jsonStr;
    std::ifstream fin(filePath);
    if (!fin.is_open()) {
        ZLOGE("Open config file failed");
        return E_ERROR;
    }
    while (fin.good()) {
        std::string line;
        std::getline(fin, line);
        if (line.size() > MAX_FILE_SIZE - jsonStr.size()) {
            ZLOGE("Config file exceeds max size, systemAbilityId: %{public}s,", pathName.c_str());
            return E_ERROR;
        }
        jsonStr += line;
    }
    if (!configInfo.Unmarshall(jsonStr)) {
        ZLOGE("Unmarshall DataShareSAConfigInfo failed");
        return E_ERROR;
    }
    return E_OK;
}

int32_t DataShareSAConfigInfoManager::GetDataShareSAConfigInfo(const std::string &bundleName, int32_t systemAbilityId,
    DataShareSAConfigInfo &info)
{
    if (!DataShareThreadLocal::IsFromSystemApp()) {
        ZLOGE("Not allow normal app visit SA, bundle:%{public}s, callingPid:%{public}d, systemAbilityId:%{public}d",
            bundleName.c_str(), IPCSkeleton::GetCallingPid(), systemAbilityId);
        return E_NOT_SYSTEM_APP;
    }
    std::string configKey = bundleName + std::to_string(systemAbilityId);
    auto it = configCache_.Find(configKey);
    if (it.first) {
        info = it.second;
        return E_OK;
    }
    DataShareSAConfigInfo configInfo;
    auto ret = LoadConfigInfo(std::to_string(systemAbilityId), configInfo);
    if (ret != E_OK) {
        ZLOGE("load configInfo failed:%{public}d, bundle:%{public}s, callingPid:%{public}d, systemAbilityId:%{public}d",
            ret, bundleName.c_str(), IPCSkeleton::GetCallingPid(), systemAbilityId);
        return ret;
    }
    std::vector<SAConfigProxyData> &proxyDatas = configInfo.proxyData;
    std::sort(proxyDatas.begin(), proxyDatas.end(), [](const SAConfigProxyData &curr,
        const SAConfigProxyData &prev) {
        return curr.uri.length() > prev.uri.length();
    });
    configCache_.Insert(configKey, configInfo);
    info = configInfo;
    return E_OK;
}
} // namespace OHOS::DataShare