/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef DATA_SHARE_PROFILE_INFO_H
#define DATA_SHARE_PROFILE_INFO_H

#include "bundle_info.h"
#include "uri_utils.h"
#include "serializable/serializable.h"
#include "resource_manager.h"

namespace OHOS::DataShare {
using namespace OHOS::Global::Resource;
struct Config final : public DistributedData::Serializable {
    std::string uri = "*";
    int crossUserMode = 0;
    std::string  writePermission = "";
    std::string  readPermission = "";
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
};

struct ProfileInfo : public DistributedData::Serializable {
    std::vector<Config> tableConfig;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
};

class DataShareProfileInfo {
public:
    DataShareProfileInfo() = default;
    bool LoadProfileInfoFromExtension(const AppExecFwk::BundleInfo &bundleInfo,
        ProfileInfo &profileInfo, bool &isSingleApp);

private:
    bool GetResProfileByMetadata(const std::vector<AppExecFwk::Metadata> &metadata, const std::string &resourcePath,
        bool isCompressed, std::vector<std::string> &profileInfos) const;
    bool GetResConfigFile(const AppExecFwk::ExtensionAbilityInfo &extensionInfo,
        std::vector<std::string> &profileInfos);
    std::shared_ptr<ResourceManager> InitResMgr(const std::string &basicString) const;
    bool GetResFromResMgr(const std::string &resName, ResourceManager &resMgr, bool isCompressed,
        std::vector<std::string> &profileInfos) const;
    std::string TransformFileToJsonString(const std::string &resPath) const;
    bool IsFileExisted(const std::string &filePath) const;
};
} // namespace OHOS::DataShare
#endif // DATA_SHARE_PROFILE_INFO_H
