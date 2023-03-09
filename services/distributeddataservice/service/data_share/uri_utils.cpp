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
#define LOG_TAG "URIUtils"

#include "uri_utils.h"

#include "log_print.h"
#include "string_ex.h"
#include "uri.h"

namespace OHOS::DataShare {
bool URIUtils::GetInfoFromURI(const std::string &uri, UriInfo &uriInfo, bool tableNameEmpty)
{
    Uri uriTemp(uri);
    std::vector<std::string> splitUri;
    SplitStr(uriTemp.GetPath(), "/", splitUri);
    if (!IsValidPath(splitUri, tableNameEmpty)) {
        ZLOGE("Invalid uri: %{public}s", uri.c_str());
        return false;
    }

    uriInfo.bundleName = splitUri[BUNDLE_NAME];
    uriInfo.moduleName = splitUri[MODULE_NAME];
    uriInfo.storeName = splitUri[STORE_NAME];
    if (splitUri.size() > OPTIONAL_BEGIN) {
        uriInfo.tableName = splitUri[TABLE_NAME];
    }
    return true;
}

bool URIUtils::IsValidPath(const std::vector<std::string> &splitUri, bool tableNameEmpty)
{
    if (splitUri.size() < OPTIONAL_BEGIN) {
        return false;
    }
    if (splitUri[BUNDLE_NAME].empty() || splitUri[MODULE_NAME].empty() ||
        splitUri[STORE_NAME].empty()) {
        ZLOGE("Uri has empty field!");
        return false;
    }

    if (!tableNameEmpty) {
        if (splitUri.size() < PARAM_BUTT) {
            ZLOGE("Uri need contains tableName");
            return false;
        }
        if (splitUri[TABLE_NAME].empty()) {
            ZLOGE("Uri tableName can't be empty!");
            return false;
        }
    }
    return true;
}
} // namespace OHOS::DataShare