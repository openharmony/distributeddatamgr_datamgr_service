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

#ifndef DOCUMENT_CHECK_H
#define DOCUMENT_CHECK_H

#include <cstdint>
#include <vector>
#include "json_common.h"

namespace DocumentDB {
class JsonCommon;
class CheckCommon {
public:
    CheckCommon() = default;
    ~CheckCommon() = default;

    static bool CheckCollectionName(const std::string &collectionName, std::string &lowerCaseName, int &errCode);
    static int CheckFilter(JsonObject &document);
    static int CheckFilter(JsonObject &document, bool &isOnlyId, std::vector<std::vector<std::string>> &filterPath);
    static bool CheckFilter(const std::string &filter, std::string &idStr, int &errCode);
    static int CheckIdFormat(JsonObject &data);
    static int CheckIdFormat(JsonObject &data, bool &isIdExisit);
    static int CheckDocument(JsonObject &document);
    static bool CheckUpdata(JsonObject &updata, std::vector<std::vector<std::string>> &path);
    static bool CheckDocument(const std::string &updateStr, int &errCode);
    static bool CheckProjection(JsonObject &projectionObj, std::vector<std::vector<std::string>> &path);
};
using Key = std::vector<uint8_t>;
using Value = std::vector<uint8_t>;

constexpr const char *COLL_PREFIX = "GRD_COLL_";
} // DocumentDB
#endif // DOCUMENT_CHECK_H