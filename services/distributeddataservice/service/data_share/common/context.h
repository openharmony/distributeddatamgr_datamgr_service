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

#ifndef DATASHARESERVICE_CONTEXT_H
#define DATASHARESERVICE_CONTEXT_H

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "bundle_info.h"

namespace OHOS::DataShare {
enum AccessSystemMode : uint8_t {
    UNDEFINED,
    USER_SHARED_MODE,
    USER_SINGLE_MODE,
    MAX,
};
class Context {
public:
    explicit Context() {}
    explicit Context(const std::string &uri) : uri(uri) {}
    virtual ~Context() = default;
    std::string uri;
    int32_t currentUserId = -1;
    std::string permission;
    uint32_t callerTokenId = 0;
    std::string callerBundleName;
    std::string calledBundleName;
    std::string calledModuleName;
    std::string calledStoreName;
    std::string calledTableName;
    std::string calledSourceDir; // the dir of db
    std::string secretMetaKey;
    int version = -1;
    int errCode = -1;
    bool isRead = false;
    bool isAllowCrossPer = false; // can cross permission check, for special SA
    bool needAutoLoadCallerBundleName = false;
    bool isEncryptDb = false;
    AccessSystemMode accessSystemMode = AccessSystemMode::UNDEFINED;
    OHOS::AppExecFwk::BundleInfo bundleInfo;
    std::string type = "rdb";
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_CONTEXT_H
