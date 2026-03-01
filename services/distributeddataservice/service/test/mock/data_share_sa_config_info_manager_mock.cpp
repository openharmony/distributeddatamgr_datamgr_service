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

#include "data_share_sa_config_info_manager.h"

#include "common_utils.h"
#include "datashare_errno.h"
#include "ipc_skeleton.h"

namespace OHOS::DataShare {
// static constexpr const char *CONFIG_PATH = "/system/etc/distributeddata/arkdata/datashare/";
// static constexpr const char *FILE_NAME = "/datashare.json";
// static constexpr size_t MAX_FILE_SIZE = 10 * 1024 * 1024;
// static constexpr size_t MAX_STRING_SIZE = 256;

bool SAConfigProxyData::Marshal(json &node) const
{
    return true;
}

bool SAConfigProxyData::Unmarshal(const json &node)
{
    return true;
}

bool DataShareSAConfigInfo::Marshal(json &node) const
{
    return true;
}

bool DataShareSAConfigInfo::Unmarshal(const json &node)
{
    return true;
}

std::shared_ptr<DataShareSAConfigInfoManager> DataShareSAConfigInfoManager::GetInstance()
{
    static std::shared_ptr<DataShareSAConfigInfoManager> proxy = std::make_shared<DataShareSAConfigInfoManager>();
    return proxy;
}

int32_t DataShareSAConfigInfoManager::LoadConfigInfo(const std::string &pathName, DataShareSAConfigInfo &configInfo)
{
    SAConfigProxyData testConfig1;
    testConfig1.uri = "datashareproxy://com.acts.datasharetest/SAID=12321/longertest";
    testConfig1.requiredReadPermission = "read1";
    testConfig1.requiredWritePermission = "write1";
    SAConfigProxyData testConfig2;
    testConfig2.uri = "datashareproxy://com.acts.datasharetest/SAID=12321/uri";
    testConfig2.requiredReadPermission = "read2";
    testConfig2.requiredWritePermission = "write2";
    SAConfigProxyData testConfig3;
    testConfig3.uri = "datashareproxy://com.acts.datasharetest/SAID=12321/t";
    testConfig3.requiredReadPermission = "read3";
    testConfig3.requiredWritePermission = "write3";
    configInfo.proxyData.push_back(testConfig1);
    configInfo.proxyData.push_back(testConfig2);
    configInfo.proxyData.push_back(testConfig3);
    return E_OK;
}

int32_t DataShareSAConfigInfoManager::GetDataShareSAConfigInfo(const std::string &bundleName, int32_t systemAbilityId,
    DataShareSAConfigInfo &info)
{
    if (!DataShareThreadLocal::IsFromSystemApp()) {
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
        return ret;
    }
    configCache_.Insert(configKey, configInfo);
    info = configInfo;
    return E_OK;
}
} // namespace OHOS::DataShare