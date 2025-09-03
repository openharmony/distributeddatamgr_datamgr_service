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
#define LOG_TAG "Utils"

#include "utils.h"

#include <string>

#include "log_print.h"
#include "string_ex.h"
#include "uri.h"
#include "utils/anonymous.h"

namespace OHOS::DataShare {
// Size of the characters that need to be left for both head and tail.
constexpr int32_t REVEAL_SIZE = 4;
constexpr const char *REPLACE_CHAIN = "***";
constexpr const char *DEFAULT_ANONYMOUS = "******";
bool URIUtils::GetInfoFromURI(const std::string &uri, UriInfo &uriInfo)
{
    Uri uriTemp(uri);
    std::vector<std::string> splitUri;
    SplitStr(uriTemp.GetPath(), "/", splitUri);
    if (splitUri.size() < PARAM_SIZE) {
        return false;
    }

    if (splitUri[BUNDLE_NAME].empty() || splitUri[MODULE_NAME].empty() ||
        splitUri[STORE_NAME].empty() || splitUri[TABLE_NAME].empty()) {
        ZLOGE("Uri has empty field! bundleName: %{public}s  uri: %{public}s", splitUri[BUNDLE_NAME].c_str(),
            URIUtils::Anonymous(uri).c_str());
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

bool URIUtils::GetAppIndexFromProxyURI(const std::string &uri, int32_t &appIndex)
{
    auto queryParams = URIUtils::GetQueryParams(uri);
    if (!queryParams[APP_INDEX].empty()) {
        auto [success, data] = URIUtils::Strtoul(queryParams[APP_INDEX]);
        if (!success) {
            appIndex = -1;
            ZLOGE("appIndex is invalid! appIndex: %{public}s", queryParams[APP_INDEX].c_str());
            return false;
        }
        appIndex = static_cast<int32_t>(data);
    }
    return true;
}

std::pair<bool, int32_t> URIUtils::GetUserFromProxyURI(const std::string &uri)
{
    auto queryParams = URIUtils::GetQueryParams(uri);
    if (queryParams[USER_PARAM].empty()) {
        // -1 is placeholder for visit provider's user
        return std::make_pair(true, -1);
    }
    auto [success, data] = URIUtils::Strtoul(queryParams[USER_PARAM]);
    if (!success) {
        return std::make_pair(false, -1);
    }
    if (data < 0 || data > INT32_MAX) {
        return std::make_pair(false, -1);
    }
    return std::make_pair(true, data);
}

void URIUtils::FormatUri(std::string &uri)
{
    auto pos = uri.find_last_of('?');
    if (pos == std::string::npos) {
        return;
    }

    uri.resize(pos);
}

std::string URIUtils::FormatConstUri(const std::string &uri)
{
    auto pos = uri.find_last_of('?');
    if (pos == std::string::npos) {
        return uri;
    }

    return uri.substr(0, pos);
}

__attribute__((no_sanitize("cfi"))) UriConfig URIUtils::GetUriConfig(const std::string &uri)
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
    data = strtoul(str.c_str(), &end, END_LENGTH);
    if (errno == ERANGE || end == nullptr || end == str || *end != '\0') {
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

std::string StringUtils::GeneralAnonymous(const std::string &name)
{
    // To short to be partial anonymized
    if (name.length() <= REVEAL_SIZE) {
        return DEFAULT_ANONYMOUS;
    }

    // only leave HEAD
    if (name.length() <= (REVEAL_SIZE + REVEAL_SIZE)) {
        return (name.substr(0, REVEAL_SIZE) + REPLACE_CHAIN);
    }
    // leave 4 char at head and tail respectively
    return (name.substr(0, REVEAL_SIZE) + REPLACE_CHAIN + name.substr(name.length() - REVEAL_SIZE, REVEAL_SIZE));
}

} // namespace OHOS::DataShare