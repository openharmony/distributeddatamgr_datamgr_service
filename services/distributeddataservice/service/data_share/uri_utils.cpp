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
    if (!CheckUriFormat(splitUri, tableNameEmpty)) {
        ZLOGE("Invalid uri: %{public}s", uri.c_str());
        return false;
    }

    uriInfo.bundleName = splitUri[URI_INDEX_BUNLDENAME];
    uriInfo.moduleName = splitUri[URI_INDEX_MODULENAME];
    uriInfo.storeName = splitUri[URI_INDEX_STORENAME];
    if (splitUri.size() > URI_INDEX_MIN) {
        uriInfo.tableName = splitUri[URI_INDEX_TABLENAME];
    }
    return true;
}

bool URIUtils::CheckUriFormat(const std::vector<std::string> &splitUri, bool tableNameEmpty)
{
    if (splitUri.size() < URI_INDEX_MIN) {
        return false;
    }
    if (splitUri[URI_INDEX_BUNLDENAME].empty() || splitUri[URI_INDEX_MODULENAME].empty() ||
        splitUri[URI_INDEX_STORENAME].empty()) {
        ZLOGE("Uri has empty field!");
        return false;
    }

    if (!tableNameEmpty) {
        if (splitUri.size() < URI_INDEX_MAX) {
            ZLOGE("Uri need contains tableName");
            return false;
        }
        if (splitUri[URI_INDEX_TABLENAME].empty()) {
            ZLOGE("Uri tableName can't be empty!");
            return false;
        }
    }
    return true;
}
} // namespace OHOS::DataShare