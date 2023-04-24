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
#include <vector>
#include <set>
#include <functional>
#include "json_object.h"

namespace DocumentDB {
class JsonCommon
{
public:
    JsonCommon() = default;
    ~JsonCommon();

    static ValueObject GetValueByFiled(JsonObject &node, const std::string& filed);
    static ValueObject GetValueByFiled(JsonObject &node, const std::string& filed, bool &isFiledExist);
    static bool CheckJsonField(JsonObject &node);
    static bool CheckProjectionField(JsonObject &node);
    static int ParseNode(JsonObject &Node, std::vector<std::string> singlePath, 
                        std::vector<std::vector<std::string>> &resultPath, bool isFirstFloor);
    static std::vector<std::vector<std::string>> ParsePath(const JsonObject &node);
    static std::vector<ValueObject>  GetLeafValue(const JsonObject &node);
    static bool isValueEqual(const ValueObject &srcValue, const ValueObject &targetValue);
    static int Append(const JsonObject &src, const JsonObject &add);
    static bool isJsonNodeMatch(const JsonObject &src, const JsonObject &target, int &externErrCode);
private:
    static bool JsonEqualJudge(JsonFieldPath &itemPath, const JsonObject &src, const JsonObject &item, int &isAlreadyMatched, 
                                bool &isCollapse, int &isMatchFlag);
    static bool CheckNode(JsonObject &Node, std::set<std::string> filedSet, bool &errFlag);
    static bool CheckProjectionNode(JsonObject &Node, std::set<std::string> filedSet, bool &errFlag, bool isFirstFloor);
    static int CheckLeafNode(const JsonObject &Node, std::vector<ValueObject> &leafValue);
    static bool isArrayMathch(const JsonObject &src, const JsonObject &target, int &flag);
};
} // DocumentDB
#endif // JSON_COMMON_H