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

#ifndef DATASHARESERVICE_URI_UTILS_H
#define DATASHARESERVICE_URI_UTILS_H

#include <string>
namespace OHOS::DataShare {
struct UriInfo {
    std::string bundleName;
    std::string moduleName;
    std::string storeName;
    std::string tableName;
};

class URIUtils {
public:
    static bool GetInfoFromURI(const std::string &uri, UriInfo &uriInfo);
    static bool GetBundleNameFromProxyURI(const std::string &uri, std::string &bundleName);
    static bool GetInfoFromProxyURI(
        const std::string &uri, int32_t &user, uint32_t &callerTokenId, std::string &calledBundleName);
    static bool IsDataProxyURI(const std::string &uri);
    static void FormatUri(std::string &uri);
    static constexpr const char *DATA_SHARE_SCHEMA = "datashare:///";;
    static constexpr const char DATA_PROXY_SCHEMA[] = "datashareproxy://";
    static constexpr int DATA_PROXY_SCHEMA_LEN = sizeof(DATA_PROXY_SCHEMA) - 1;

private:
    enum PATH_PARAM : int32_t {
        BUNDLE_NAME = 0,
        MODULE_NAME,
        STORE_NAME,
        TABLE_NAME,
        PARAM_SIZE
    };
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_URI_UTILS_H