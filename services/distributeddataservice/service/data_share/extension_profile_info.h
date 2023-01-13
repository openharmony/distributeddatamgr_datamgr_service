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

#ifndef EXTENSION_PROFILE_INFO_H
#define EXTENSION_PROFILE_INFO_H

#include "bundle_info.h"
#include "uri_utils.h"
#include "serializable/serializable.h"

namespace OHOS::DataShare {
struct Config final : public DistributedData::Serializable {
    std::string scope = "*";
    int crossUserMode = 0;
    std::string  writePermission = "";
    std::string  readPermission = "";
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
};

struct ProfileInfo : public DistributedData::Serializable {
    std::vector<Config> tablesConfig;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
};

class ExtensionProfileInfo {
public:
    ExtensionProfileInfo() = default;
    bool LoadProfileInfoFromExtension(UriInfo &uriInfo, ProfileInfo &profileInfo,
        bool &isSingleApp, AppExecFwk::BundleInfo &bundleInfo);
};
} // namespace OHOS::DataShare
#endif // EXTENSION_PROFILE_INFO_H
