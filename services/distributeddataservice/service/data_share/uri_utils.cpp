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

#include <vector>

#include "log_print.h"
#include "string_ex.h"
#include "uri.h"

namespace OHOS::DataShare {
bool URIUtils::GetInfoFromURI(const std::string &uri, UriInfo &uriInfo)
{
    Uri uriTemp(uri);
    std::vector<std::string> splitUri;
    SplitStr(uriTemp.GetPath(), "/", splitUri);
    if (splitUri.size() < URI_INDEX_MAX) {
        ZLOGE("Invalid uri: %{public}s", uri.c_str());
        return false;
    }

    if (splitUri[URI_INDEX_BUNLDENAME].empty() || splitUri[URI_INDEX_MODULENAME].empty() ||
        splitUri[URI_INDEX_STORENAME].empty() || splitUri[URI_INDEX_TABLENAME].empty()) {
        ZLOGE("Uri has empty field!");
        return false;
    }

    uriInfo.bundleName = splitUri[URI_INDEX_BUNLDENAME];
    uriInfo.moduleName = splitUri[URI_INDEX_MODULENAME];
    uriInfo.storeName = splitUri[URI_INDEX_STORENAME];
    uriInfo.tableName = splitUri[URI_INDEX_TABLENAME];
    return true;
}
bool UriInfo::operator==(const UriInfo &rhs) const
{
    return bundleName == rhs.bundleName && moduleName == rhs.moduleName && storeName == rhs.storeName &&
           tableName == rhs.tableName;
}
bool UriInfo::operator!=(const UriInfo &rhs) const
{
    return !(rhs == *this);
}
bool UriInfo::operator<(const UriInfo &rhs) const
{
    if (bundleName < rhs.bundleName) {
        return true;
    }
    if (rhs.bundleName < bundleName) {
        return false;
    }
    if (moduleName < rhs.moduleName) {
        return true;
    }
    if (rhs.moduleName < moduleName) {
        return false;
    }
    if (storeName < rhs.storeName) {
        return true;
    }
    if (rhs.storeName < storeName) {
        return false;
    }
    return tableName < rhs.tableName;
}
bool UriInfo::operator>(const UriInfo &rhs) const
{
    return rhs < *this;
}
bool UriInfo::operator<=(const UriInfo &rhs) const
{
    return !(rhs < *this);
}
bool UriInfo::operator>=(const UriInfo &rhs) const
{
    return !(*this < rhs);
}
} // namespace OHOS::DataShare