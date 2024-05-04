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
#define LOG_TAG "URIUtils"

#include "uri_utils.h"

#include <string>

#include "log_print.h"
#include "string_ex.h"
#include "uri.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
bool URIUtils::GetInfoFromURI(const std::string &uri, UriInfo &uriInfo)
{
    Uri uriTemp(uri);
    std::vector<std::string> splitUri;
    SplitStr(uriTemp.GetPath(), "/", splitUri);
    if (splitUri.size() < PARAM_SIZE) {
        ZLOGE("Invalid uri: %{public}s", DistributedData::Anonymous::Change(uri).c_str());
        return false;
    }

    if (splitUri[BUNDLE_NAME].empty() || splitUri[MODULE_NAME].empty() ||
        splitUri[STORE_NAME].empty() || splitUri[TABLE_NAME].empty()) {
        ZLOGE("Uri has empty field! bundleName: %{public}s  uri: %{public}s", splitUri[BUNDLE_NAME].c_str(),
            DistributedData::Anonymous::Change(uri).c_str());
        return false;
    }

    uriInfo.bundleName = splitUri[BUNDLE_NAME];
    uriInfo.moduleName = splitUri[MODULE_NAME];
    uriInfo.storeName = splitUri[STORE_NAME];
    uriInfo.tableName = splitUri[TABLE_NAME];
    return true;
}

bool URIUtils::IsDataProxyURI(const std::string &uri)
{
    return uri.compare(0, DATA_PROXY_SCHEMA_LEN, URIUtils::DATA_PROXY_SCHEMA) == 0;
}

bool URIUtils::GetBundleNameFromProxyURI(const std::string &uri, std::string &bundleName)
{
    Uri uriTemp(uri);
    if (!uriTemp.GetAuthority().empty()) {
        bundleName = uriTemp.GetAuthority();
    }
    return true;
}

void URIUtils::FormatUri(std::string &uri)
{
    auto pos = uri.find_last_of('?');
    if (pos == std::string::npos) {
        return;
    }

    uri.resize(pos);
}

UriConfig URIUtils::GetUriConfig(const std::string &uri)
{
    UriConfig uriConfig;
    Uri uriTemp(uri);
    uriConfig.authority = uriTemp.GetAuthority();
    uriConfig.path = uriTemp.GetPath();
    uriTemp.GetPathSegments(uriConfig.pathSegments);
    uriConfig.scheme = uriTemp.GetScheme();
    std::string convertUri = DATA_PROXY_SCHEMA + uriConfig.authority + uriConfig.path;
    size_t schemePos = convertUri.find(PARAM_URI_SEPARATOR);
    if (schemePos != std::string::npos) {
        convertUri.replace(schemePos, PARAM_URI_SEPARATOR_LEN, SCHEME_SEPARATOR);
    }
    uriConfig.formatUri = convertUri;
    return uriConfig;
}

std::string URIUtils::Anonymous(const std::string &uri)
{
    if (uri.length() <= END_LENGTH) {
        return DEFAULT_ANONYMOUS;
    }

    return (DEFAULT_ANONYMOUS + uri.substr(uri.length() - END_LENGTH, END_LENGTH));
}

std::pair<bool, uint32_t> URIUtils::Strtoul(const std::string &str)
{
    unsigned long data = 0;
    if (str.empty()) {
        return std::make_pair(false, data);
    }
    char* end = nullptr;
    errno = 0;
    data = strtoul(str.c_str(), &end, 10);
    if (errno == ERANGE || end == str || *end != '\0') {
        return std::make_pair(false, data);
    }
    return std::make_pair(true, data);
}

std::map<std::string, std::string> URIUtils::GetQueryParams(const std::string& uri)
{
    size_t queryStartPos = uri.find('?');
    if (queryStartPos == std::string::npos) {
        return {};
    }
    std::map<std::string, std::string> params;
    std::string queryParams = uri.substr(queryStartPos + 1);
    size_t startPos = 0;
    while (startPos < queryParams.size()) {
        size_t delimiterIndex = queryParams.find('&', startPos);
        if (delimiterIndex == std::string::npos) {
            delimiterIndex = queryParams.size();
        }
        size_t equalIndex = queryParams.find('=', startPos);
        if (equalIndex == std::string::npos || equalIndex > delimiterIndex) {
            startPos = delimiterIndex + 1;
            continue;
        }
        std::string key = queryParams.substr(startPos, equalIndex - startPos);
        std::string value = queryParams.substr(equalIndex + 1, delimiterIndex - equalIndex - 1);
        params[key] = value;
        startPos = delimiterIndex + 1;
    }
    return params;
}
} // namespace OHOS::DataShare