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

#ifndef DATASHARESERVICE_DATA_SHARE_CONFIG_INFO_MANAGER_H
#define DATASHARESERVICE_DATA_SHARE_CONFIG_INFO_MANAGER_H

#include "concurrent_map.h"
#include "data_share_profile_config.h"
#include "serializable/serializable.h"
#include "visibility.h"

namespace OHOS::DataShare {

struct SAConfigProxyData final : public DistributedData::Serializable {
    std::string uri;
    std::string requiredReadPermission;
    std::string requiredWritePermission;
    ProfileInfo  profile;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
};

struct DataShareSAConfigInfo final : public DistributedData::Serializable {
    bool singleton = true;
    std::vector<SAConfigProxyData> proxyData;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
};

class DataShareSAConfigInfoManager {
public:
    DataShareSAConfigInfoManager() = default;
    ~DataShareSAConfigInfoManager() = default;
    static std::shared_ptr<DataShareSAConfigInfoManager> GetInstance();
    int32_t GetDataShareSAConfigInfo(const std::string &bundleName, int32_t systemAbilityId,
        DataShareSAConfigInfo &info);
private:
    int32_t LoadConfigInfo(const std::string &pathName, DataShareSAConfigInfo &configInfo);
    ConcurrentMap<std::string, DataShareSAConfigInfo> configCache_;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_DATA_SHARE_CONFIG_INFO_MANAGER_H