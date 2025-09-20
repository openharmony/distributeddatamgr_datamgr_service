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
#define LOG_TAG "UriPermissionManager"

#include "uri_permission_manager.h"

#include "log_print.h"
#include "preprocess_utils.h"
#include "uri_permission_manager_client.h"

namespace OHOS {
namespace UDMF {
constexpr const std::uint32_t GRANT_URI_PERMISSION_MAX_SIZE = 10000;
constexpr unsigned int READ_PERMISSION = AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION;
constexpr unsigned int READ_WRITE_PERMISSION = AAFwk::Want::FLAG_AUTH_READ_URI_PERMISSION |
    AAFwk::Want::FLAG_AUTH_WRITE_URI_PERMISSION | AAFwk::Want::FLAG_AUTH_PERSISTABLE_URI_PERMISSION;

UriPermissionManager &UriPermissionManager::GetInstance()
{
    static UriPermissionManager instance;
    return instance;
}

Status UriPermissionManager::GrantUriPermission(const std::vector<Uri> &readUris, const std::vector<Uri> &writeUris,
    uint32_t dstTokenId, uint32_t srcTokenId, bool isLocal)
{
    std::string bundleName;
    if (!PreProcessUtils::GetHapBundleNameByToken(dstTokenId, bundleName)) {
        ZLOGE("BundleName get failed tokenId:%{public}u", dstTokenId);
        return E_ERROR;
    }
    int32_t instIndex = -1;
    if (!PreProcessUtils::GetInstIndex(dstTokenId, instIndex)) {
        ZLOGE("InstIndex get failed tokenId:%{public}u", dstTokenId);
        return E_ERROR;
    }

    //  GrantUriPermission is time-consuming, need recording the begin,end time in log.
    ZLOGI("GrantUriPermission begin, url size:%{public}zu, instIndex:%{public}d.",
        readUris.size() + writeUris.size(), instIndex);
    GrantUriOptions readOptions {
        .uris = std::move(readUris),
        .bundleName = bundleName,
        .index = instIndex,
        .tokenId = srcTokenId,
        .permission = READ_PERMISSION
    };
    auto status = ProcessUriPermission(readOptions, isLocal);
    if (status != E_OK) {
        ZLOGE("grant uri read permission failed, status:%{public}d, instIndex:%{public}d.", status, instIndex);
        return E_NO_PERMISSION;
    }
    ZLOGI("GrantUriPermission with read-write.");
    GrantUriOptions writeOptions {
        .uris = std::move(writeUris),
        .bundleName = bundleName,
        .index = instIndex,
        .tokenId = srcTokenId,
        .permission = READ_WRITE_PERMISSION
    };
    status = ProcessUriPermission(writeOptions, isLocal);
    if (status != E_OK) {
        ZLOGE("grant uri write permission failed, status:%{public}d, instIndex:%{public}d.", status, instIndex);
        return E_NO_PERMISSION;
    }
    ZLOGI("GrantUriPermission end.");
    return E_OK;
}

Status UriPermissionManager::ProcessUriPermission(const GrantUriOptions &options, bool isLocal)
{
    int32_t status = E_NO_PERMISSION;
    for (size_t index = 0; index < options.uris.size(); index += GRANT_URI_PERMISSION_MAX_SIZE) {
        std::vector<Uri> uriLst(options.uris.begin() + index,
            options.uris.begin() + std::min(index + GRANT_URI_PERMISSION_MAX_SIZE, options.uris.size()));
        if (isLocal) {
            //  Because the GrantHeriPermission function does not support cross device.
            status = AAFwk::UriPermissionManagerClient::GetInstance().GrantUriPermission(uriLst,
                options.permission, options.bundleName, options.index, options.tokenId);
            if (status != ERR_OK) {
                return E_NO_PERMISSION;
            }
        } else {
            status = AAFwk::UriPermissionManagerClient::GetInstance().GrantUriPermissionPrivileged(uriLst,
                options.permission, options.bundleName, options.index);
            if (status != ERR_OK) {
                return E_NO_PERMISSION;
            }
        }
    }
    return E_OK;
}
} // namespace UDMF
} // namespace OHOS
