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

#ifndef JSON_COMMON_H
#define JSON_COMMON_H

#include <cstdint>
#include <functional>
#include <set>
#include <vector>

#include "json_object.h"

namespace DocumentDB {
class JsonCommon {
public:
    JsonCommon() = default;
    ~JsonCommon();

    static ValueObject GetValueInSameLevel(JsonObject &node, const std::string &field);
    static ValueObject GetValueInSameLevel(JsonObject &node, const std::string &field, bool &isFieldExist);
    static bool CheckJsonField(JsonObject &node);
    static bool CheckProjectionField(JsonObject &node, int &errCode);
    static int ParseNode(JsonObject &Node, std::vector<std::string> singlePath,
        std::vector<std::vector<std::string>> &resultPath, bool isFirstLevel);
    static std::vector<std::vector<std::string>> ParsePath(const JsonObject &node, int &errCode);
    static std::vector<ValueObject> GetLeafValue(const JsonObject &node);
    static bool isValueEqual(const ValueObject &srcValue, const ValueObject &targetValue);
    static int Append(const JsonObject &src, const JsonObject &add, bool isReplace);
    static bool IsJsonNodeMatch(const JsonObject &src, const JsonObject &target, int &errCode);

private:
    static bool JsonEqualJudge(JsonFieldPath &itemPath, const JsonObject &src, const JsonObject &item,
        int &isAlreadyMatched, bool &isCollapse, int &isMatchFlag);
    static bool CheckNode(JsonObject &Node);
    static bool CheckProjectionNode(JsonObject &Node, bool isFirstLevel, int &errCode);
    static void CheckLeafNode(const JsonObject &Node, std::vector<ValueObject> &leafValue);
    static bool IsArrayMatch(const JsonObject &src, const JsonObject &target, int &isAlreadyMatched);
};
} // namespace DocumentDB
#endif // JSON_COMMON_H