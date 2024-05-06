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

#ifndef DISTRIBUTEDDATAMGR_PROFILE_CONFIG_H
#define DISTRIBUTEDDATAMGR_PROFILE_CONFIG_H

#include <string>
#include <map>
#include <vector>

#include "bundle_info.h"
#include "resource_manager.h"
#include "serializable/serializable.h"

namespace OHOS::DataShare {
using namespace OHOS::Global::Resource;
struct Config final : public DistributedData::Serializable {
    std::string uri = "*";
    int crossUserMode = 0;
    std::string writePermission = "";
    std::string readPermission = "";
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
};

struct ProfileInfo : public DistributedData::Serializable {
    std::vector<Config> tableConfig;
    bool isSilentProxyEnable = true;
    std::string storeName;
    std::string tableName;
    std::string scope = "module";
    std::string type = "rdb";
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
};

enum AccessCrossMode : uint8_t {
    USER_UNDEFINED,
    USER_SHARED,
    USER_SINGLE,
    USER_MAX,
};

class API_EXPORT DataShareProfileConfig {
public:
    constexpr static int8_t TABLE_MATCH_PRIORITY = 3;
    constexpr static int8_t STORE_MATCH_PRIORITY = 2;
    constexpr static int8_t COMMON_MATCH_PRIORITY = 1;
    constexpr static int8_t UNDEFINED_PRIORITY = -1;

    static bool GetProfileInfo(const std::string &calledBundleName, int32_t currentUserId,
        std::map<std::string, ProfileInfo> &profileInfos);
    static std::pair<int, ProfileInfo> GetDataProperties(const std::vector<AppExecFwk::Metadata> &metadata,
        const std::string &resPath, const std::string &hapPath, const std::string &name);
    static AccessCrossMode GetAccessCrossMode(const ProfileInfo &profileInfo,
        const std::string &tableUri, const std::string &storeUri);
private:
    static std::shared_ptr<ResourceManager> InitResMgr(const std::string &resourcePath);
    static std::string GetProfileInfoByMetadata(const std::vector<AppExecFwk::Metadata> &metadata,
        const std::string &resourcePath, const std::string &hapPath, const std::string &name);
    static std::string GetResFromResMgr(const std::string &resName, ResourceManager &resMgr,
        const std::string &hapPath);
    static std::string ReadProfile(const std::string &resPath);
    static bool IsFileExisted(const std::string &filePath);
    static std::mutex infosMutex_;
    static void SetCrossUserMode(uint8_t priority, uint8_t crossMode,
        std::pair<AccessCrossMode, int8_t> &mode);
    static constexpr const char *DATA_SHARE_EXTENSION_META = "ohos.extension.dataShare";
};
} // namespace OHOS::DataShare
#endif // DISTRIBUTEDDATAMGR_PROFILE_CONFIG_H
