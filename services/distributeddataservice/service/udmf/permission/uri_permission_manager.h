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

#ifndef UDMF_URI_PERMISSION_MANAGER_H
#define UDMF_URI_PERMISSION_MANAGER_H

#include "error_code.h"
#include "uri.h"

namespace OHOS {
namespace UDMF {
class UriPermissionManager {
public:
    static UriPermissionManager &GetInstance();
    Status GrantUriPermission(const std::vector<Uri> &readUris, const std::vector<Uri> &writeUris,
        uint32_t dstTokenId, uint32_t srcTokenId, bool isLocal);

private:
    UriPermissionManager() {}
    ~UriPermissionManager() {}
    UriPermissionManager(const UriPermissionManager &mgr) = delete;
    UriPermissionManager &operator=(const UriPermissionManager &mgr) = delete;

    struct GrantUriOptions {
        const std::vector<Uri> uris;
        const std::string bundleName;
        int32_t index {0};
        uint32_t tokenId {0};
        unsigned int permission {0};
    };
    Status ProcessUriPermission(const GrantUriOptions &options, bool isLocal);
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_URI_PERMISSION_MANAGER_H