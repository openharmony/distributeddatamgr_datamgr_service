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

#ifndef LDBPROJ_PROFILE_INFO_UTILS_H
#define LDBPROJ_PROFILE_INFO_UTILS_H

#include "uri_utils.h"
#include "bundle_mgr_proxy.h"
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

class ProfileInfoUtils {
public:
    ProfileInfoUtils() = default;
    bool LoadProfileInfoFromExtension(UriInfo &uriInfo, uint32_t tokenId, ProfileInfo &profileInfo, bool &isSingleApp);
    bool CheckCrossUserMode(ProfileInfo &profileInfo, UriInfo &uriInfo, int32_t userId, const bool isSingleApp);

private:
    static BundleMgrProxy bmsProxy_;
    static constexpr int32_t USERMODE_SHARED = 1;
    static constexpr int32_t USERMODE_UNIQUE = 2;
};
} // namespace OHOS::DataShare
#endif // LDBPROJ_PROFILE_INFO_UTILS_H
