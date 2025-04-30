/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef DISTRIBUTEDDATAMGR_DATAMGR_BUNDLE_MANAGER_ADAPTER_H
#define DISTRIBUTEDDATAMGR_DATAMGR_BUNDLE_MANAGER_ADAPTER_H

#include <string>
#include <vector>
#include "visibility.h"

namespace OHOS {
namespace DistributedData {
class API_EXPORT BundleMgrAdapter {
public:
    struct HapModuleInfo {
        std::string hapPath;
    };
    struct BundleInfo {
        std::vector<HapModuleInfo> hapModuleInfos;
        std::vector<std::string> moduleDirs;
    };
    enum BundleType {
        // get bundle info except abilityInfos
        BUNDLE_DEFAULT = 0x00000000,
        // get bundle info include abilityInfos
        BUNDLE_WITH_ABILITIES = 0x00000001,
        // get bundle info include request permissions
        BUNDLE_WITH_REQUESTED_PERMISSION = 0x00000010,
        // get bundle info include extension info
        BUNDLE_WITH_EXTENSION_INFO = 0x00000020,
        // get bundle info include hash value
        BUNDLE_WITH_HASH_VALUE = 0x00000030,
        // get bundle info include menu, only for dump usage
        BUNDLE_WITH_MENU = 0x00000040,
        // get bundle info include router map, only for dump usage
        BUNDLE_WITH_ROUTER_MAP = 0x00000080,
        // get bundle info include skill info
        BUNDLE_WITH_SKILL = 0x00000800,
    };
    static BundleMgrAdapter &GetInstance();
    std::pair<bool, BundleInfo> GetBundleInfo(const std::string &bundleName, BundleType type, int32_t user = 0);
    std::string GetBundleName(int32_t uid);
    std::string GetAppId(int32_t user, const std::string &bundleName);
};
} // namespace DistributedData
} // namespace OHOS
#endif //DISTRIBUTEDDATAMGR_DATAMGR_BUNDLE_MANAGER_ADAPTER_H
