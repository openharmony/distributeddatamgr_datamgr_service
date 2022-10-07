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

#ifndef DATASHARESERVICE_URI_UTILS_H
#define DATASHARESERVICE_URI_UTILS_H

#include <string>
#include <vector>
namespace OHOS::DataShare {
struct UriInfo {
    std::string bundleName;
    std::string moduleName;
    std::string storeName;
    std::string tableName;
};

class URIUtils {
public:
    static bool GetInfoFromURI(const std::string &uri,UriInfo& uriInfo);
    enum {
        URI_INDEX_BUNLDENAME = 0,
        URI_INDEX_MODULENAME,
        URI_INDEX_STORENAME,
        URI_INDEX_TABLENAME,
        URI_INDEX_MAX
    };
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_URI_UTILS_H
