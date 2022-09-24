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

namespace OHOS::DataShare {
bool URIUtils::GetInfoFromURI(const std::string &uri, std::string &bundleName, std::string &moduleName,
    std::string &storeName, std::string &tableName)
{
    constexpr int offset_0 = 0;
    constexpr int offset_1 = 1;
    constexpr int offset_2 = 2;
    std::string uriStr = uri;
    std::string bundle = uriStr.substr(uriStr.find_first_of("/") + offset_2, uriStr.size() - uriStr.find_first_of("/") + offset_1);
    bundleName = bundle.substr(offset_0, bundle.find_first_of("/"));
    if (bundleName == "") {
        ZLOGE("Invalid bundleName");
        return false;
    }
    moduleName =
        bundle.substr(bundle.find_first_of("/") + offset_1, bundle.find_last_of("/") - bundle.find_first_of("/") - offset_1);
    if (moduleName == "") {
        ZLOGE("Invalid moduleName");
        return false;
    }
    storeName = uriStr.substr(uriStr.find_last_of("/") + offset_1, uriStr.find_last_of(":") - uriStr.find_last_of("/") - offset_1);
    if (storeName == "") {
        ZLOGE("Invalid storeName");
        return false;
    }
    tableName = uriStr.substr(uriStr.find_last_of(":") + offset_1, uriStr.find_first_of("?") - uriStr.find_last_of(":") - offset_1);
    if (tableName == "") {
        ZLOGE("Invalid tableName");
        return false;
    }
    return true;
}
} // namespace OHOS::DataShare