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
#define LOG_TAG "PermissionPolicyUtils"

#include "permission_policy_utils.h"
#include "uri_permission_util.h"

namespace OHOS {
namespace UDMF {
namespace {
constexpr unsigned int GRANT_READ_URI_PERMISSION = AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION;
constexpr unsigned int GRANT_WRITE_URI_PERMISSION = AAFwk::Want::FLAG_AUTH_WRITE_URI_PERMISSION;
constexpr unsigned int GRANT_PERSIST_URI_PERMISSION = AAFwk::Want::FLAG_AUTH_PERSISTABLE_URI_PERMISSION;
} // namespace

int32_t PermissionPolicyUtils::GetPermissionPolicyMode(const std::shared_ptr<Runtime> &runtime)
{
    if (runtime == nullptr) {
        return PERMISSION_POLICY_MODE_LEGACY;
    }
    return runtime->permissionPolicyMode;
}

unsigned int PermissionPolicyUtils::ConvertToGrantUriPermission(uint32_t permissionMask)
{
    permissionMask = UriPermissionUtil::NormalizeMask(permissionMask);
    unsigned int grantPermission = 0;
    if ((permissionMask & UriPermissionUtil::READ_FLAG) != 0) {
        grantPermission |= GRANT_READ_URI_PERMISSION;
    }
    if ((permissionMask & UriPermissionUtil::WRITE_FLAG) != 0) {
        grantPermission |= GRANT_WRITE_URI_PERMISSION;
    }
    if ((permissionMask & UriPermissionUtil::PERSIST_FLAG) != 0) {
        grantPermission |= GRANT_PERSIST_URI_PERMISSION;
    }
    return grantPermission;
}

uint32_t PermissionPolicyUtils::ToPermissionMaskByLegacyPolicy(int32_t permissionPolicy)
{
    if (permissionPolicy == static_cast<int32_t>(PermissionPolicy::ONLY_READ)) {
        return UriPermissionUtil::READ_FLAG;
    }
    if (permissionPolicy == static_cast<int32_t>(PermissionPolicy::READ_WRITE) ||
        permissionPolicy == static_cast<int32_t>(PermissionPolicy::UNKNOW)) {
        return UriPermissionUtil::READ_FLAG | UriPermissionUtil::WRITE_FLAG | UriPermissionUtil::PERSIST_FLAG;
    }
    return 0;
}

uint32_t PermissionPolicyUtils::ToPermissionMask(const std::shared_ptr<Object> &obj, int32_t permissionPolicyMode)
{
    if (obj == nullptr) {
        return 0;
    }
    if (permissionPolicyMode >= PERMISSION_POLICY_MODE_MASK) {
        int32_t permissionMask = 0;
        if (obj->GetValue(URI_PERMISSION_MASK, permissionMask) && permissionMask >= 0) {
            return UriPermissionUtil::NormalizeMask(static_cast<uint32_t>(permissionMask));
        }
    }
    int32_t permissionPolicy = static_cast<int32_t>(PermissionPolicy::UNKNOW);
    if (!obj->GetValue(PERMISSION_POLICY, permissionPolicy)) {
        return 0;
    }
    return ToPermissionMaskByLegacyPolicy(permissionPolicy);
}

uint32_t PermissionPolicyUtils::ToPermissionMask(const UriInfo &uriInfo, int32_t permissionPolicyMode)
{
    if (permissionPolicyMode >= PERMISSION_POLICY_MODE_MASK) {
        return UriPermissionUtil::NormalizeMask(uriInfo.permissionMask);
    }
    return ToPermissionMaskByLegacyPolicy(uriInfo.permission);
}

int32_t PermissionPolicyUtils::ToPermissionPolicyByLegacyMask(uint32_t permissionMask)
{
    permissionMask = UriPermissionUtil::NormalizeMask(permissionMask);
    if (permissionMask == 0) {
        return static_cast<int32_t>(PermissionPolicy::NO_PERMISSION);
    }
    if (permissionMask == UriPermissionUtil::READ_FLAG) {
        return static_cast<int32_t>(PermissionPolicy::ONLY_READ);
    }
    if ((permissionMask & UriPermissionUtil::WRITE_FLAG) != 0) {
        return static_cast<int32_t>(PermissionPolicy::READ_WRITE);
    }
    return static_cast<int32_t>(PermissionPolicy::NO_PERMISSION);
}

int32_t PermissionPolicyUtils::ToPermissionPolicy(uint32_t permissionMask)
{
    return ToPermissionPolicyByLegacyMask(permissionMask);
}

bool PermissionPolicyUtils::GetPropertiesUriAuthorizationMask(const std::shared_ptr<UnifiedDataProperties> &properties,
    uint32_t &permissionMask)
{
    if (properties == nullptr || properties->uriAuthorizationPolicies.empty()) {
        return false;
    }
    permissionMask = UriPermissionUtil::ToMask(properties->uriAuthorizationPolicies);
    return true;
}

bool PermissionPolicyUtils::GetAuthorizedUriPermissionMask(const std::shared_ptr<UnifiedDataProperties> &properties,
    const std::shared_ptr<Object> &recordObject, uint32_t availableMask, uint32_t &grantMask)
{
    availableMask = UriPermissionUtil::NormalizeMask(availableMask);
    if (availableMask == 0) {
        grantMask = 0;
        return false;
    }
    uint32_t propertiesPermissionMask = 0;
    bool hasPropertiesPermission = GetPropertiesUriAuthorizationMask(properties, propertiesPermissionMask);
    bool hasRecordPermission = false;
    uint32_t recordPermissionMask = 0;
    if (recordObject != nullptr) {
        int32_t uriAuthorizationPolicies = 0;
        if (recordObject->GetValue(URI_AUTHORIZATION_POLICIES, uriAuthorizationPolicies) &&
            uriAuthorizationPolicies >= 0) {
            hasRecordPermission = true;
            recordPermissionMask = UriPermissionUtil::NormalizeMask(static_cast<uint32_t>(uriAuthorizationPolicies));
        }
    }
    uint32_t configuredMask = hasRecordPermission ? recordPermissionMask :
        (hasPropertiesPermission ? propertiesPermissionMask : availableMask);
    grantMask = UriPermissionUtil::NormalizeMask(availableMask & configuredMask);
    return grantMask != 0;
}

void PermissionPolicyUtils::AppendGrantUriPermission(const std::map<std::string, uint32_t> &strUris,
    std::map<std::string, unsigned int> &uriPermissions)
{
    for (const auto &item : strUris) {
        const auto &uri = item.first;
        uint32_t availableMask = UriPermissionUtil::NormalizeMask(item.second);
        if (uri.empty()) {
            continue;
        }
        if (availableMask == 0) {
            continue;
        }
        unsigned int grantPermission = ConvertToGrantUriPermission(availableMask);
        if (grantPermission == 0) {
            continue;
        }
        auto iter = uriPermissions.find(uri);
        if (iter == uriPermissions.end()) {
            uriPermissions.emplace(uri, grantPermission);
            continue;
        }
        iter->second |= grantPermission;
    }
}
} // namespace UDMF
} // namespace OHOS
