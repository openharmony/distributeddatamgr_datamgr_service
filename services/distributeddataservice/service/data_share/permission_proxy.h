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

#ifndef DATASHARESERVICE_PERMISSION_PROXY_H
#define DATASHARESERVICE_PERMISSION_PROXY_H

#include <string>

#include "bundle_info.h"
#include "bundle_mgr_proxy.h"
#include "data_share_profile_info.h"
#include "metadata/store_meta_data.h"

namespace OHOS::DataShare {
class PermissionProxy {
public:
    enum class CrossUserMode {
        UNDEFINED = 0,
        SHARED,
        UNIQUE,
    };

    static bool GetBundleInfo(const std::string &bundleName, uint32_t tokenId, AppExecFwk::BundleInfo &bundleInfo);
    static bool QueryWritePermission(const std::string &bundleName, uint32_t tokenId,
        std::string &permission, const AppExecFwk::BundleInfo &bundleInfo);
    static bool QueryReadPermission(const std::string &bundleName, uint32_t tokenId,
        std::string &permission, const AppExecFwk::BundleInfo &bundleInfo);
    static bool QueryMetaData(const std::string &bundleName, const std::string &storeName,
        DistributedData::StoreMetaData &metaData, int32_t userId);
    static bool ConvertTableNameByCrossUserMode(const ProfileInfo &profileInfo,
        int32_t userId, bool isSingleApp, UriInfo &uriInfo);

private:
    static void FillData(DistributedData::StoreMetaData &data, int32_t userId);
    static CrossUserMode GetCrossUserMode(const ProfileInfo &profileInfo, const UriInfo &uriInfo);
    static BundleMgrProxy bmsProxy_;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_PERMISSION_PROXY_H