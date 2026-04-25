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

#ifndef UDMF_PERMISSION_POLICY_UTILS_H
#define UDMF_PERMISSION_POLICY_UTILS_H

#include <map>

#include "unified_data.h"

namespace OHOS {
namespace UDMF {
class PermissionPolicyUtils {
public:
    static int32_t GetPermissionPolicyMode(const std::shared_ptr<Runtime> &runtime);
    static uint32_t ToPermissionMask(const std::shared_ptr<Object> &obj, int32_t permissionPolicyMode);
    static uint32_t ToPermissionMask(const UriInfo &uriInfo, int32_t permissionPolicyMode);
    static int32_t ToPermissionPolicy(uint32_t permissionMask);
    static bool GetAuthorizedUriPermissionMask(const std::shared_ptr<UnifiedDataProperties> &properties,
        const std::shared_ptr<Object> &recordObject, uint32_t availableMask, uint32_t &grantMask);
    static void AppendGrantUriPermission(const std::map<std::string, uint32_t> &strUris,
        std::map<std::string, unsigned int> &uriPermissions);

private:
    PermissionPolicyUtils() = delete;
    ~PermissionPolicyUtils() = delete;

    static unsigned int ConvertToGrantUriPermission(uint32_t permissionMask);
    static uint32_t ToPermissionMaskByLegacyPolicy(int32_t permissionPolicy);
    static int32_t ToPermissionPolicyByLegacyMask(uint32_t permissionMask);
    static bool GetPropertiesUriAuthorizationMask(const std::shared_ptr<UnifiedDataProperties> &properties,
        uint32_t &permissionMask);
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_PERMISSION_POLICY_UTILS_H
