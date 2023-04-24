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
#include <climits>
#include <functional>
#include "json_common.h"
#include "doc_errno.h"
#include "log_print.h"
#include "securec.h"

namespace DocumentDB {
ValueObject JsonCommon::GetValueByFiled(JsonObject &node, const std::string& filed)
{
    while (!node.IsNull()) {
        if (node.GetItemFiled() == filed) {
            auto itemValue = node.GetItemValue();
            return itemValue;
        }
        if (node.GetNext().IsNull()) {
            return ValueObject();
        }
        auto nodeNew = node.GetNext();
        node = nodeNew;
    }
    return ValueObject();
}

ValueObject JsonCommon::GetValueByFiled(JsonObject &node, const std::string& filed, bool &isFiledExist)
{
    while (!node.IsNull()) {
        if (node.GetItemFiled() == filed) {
            auto itemValue = node.GetItemValue();
            isFiledExist = true;
            return itemValue;
        }
        if (node.GetNext().IsNull()) {
            isFiledExist = false;
            return ValueObject();
        }
        auto nodeNew = node.GetNext();
        node = nodeNew;
    }
    isFiledExist = false;
    return ValueObject();
}

void JsonCommon::CheckLeafNode(const JsonObject &node, std::vector<ValueObject> &leafValue)
{
    if (node.GetChild().IsNull()) {
        auto itemValue = node.GetItemValue();
        leafValue.emplace_back(itemValue);
    }
    if (!node.GetChild().IsNull()) {
        auto nodeNew = node.GetChild();
        CheckLeafNode(nodeNew, leafValue);
    }
    if (!node.GetNext().IsNull()) {
        auto nodeNew = node.GetNext();
        CheckLeafNode(nodeNew, leafValue);
    }
}

std::vector<ValueObject>  JsonCommon::GetLeafValue(const JsonObject &node)
{   
    std::vector<ValueObject> leafValue;
    if (node.IsNull()) {
        GLOGE("Get leafValue faied, node is empty");
        return leafValue;
    }
    CheckLeafNode(node, leafValue);
    return leafValue;
}

bool JsonCommon::CheckNode(JsonObject &node, std::set<std::string> filedSet, bool &errFlag) 
{
    if (!errFlag) {
        return false;
    }
    std::string fieldName; 
    if (!node.IsNull()) {
        int ret = 0;
        fieldName = node.GetItemFiled(ret);
        if (filedSet.find(fieldName) == filedSet.end()) {
            if (ret == E_OK) {
                filedSet.insert(fieldName);
            }
            if (ret == E_OK && fieldName.empty()) {
                errFlag = false;
                return false;
            }
        } else {
            errFlag = false;
            return false;
        }
        for (int i = 0; i < fieldName.size(); i++) {
            if (!((isalpha(fieldName[i])) || (isdigit(fieldName[i])) || '_' == fieldName[i])) {
                errFlag = false;
                return false;
            }
            if (i == 0 && (isdigit(fieldName[i]))) {
                errFlag = false;
                return false;
            }
        } 
    }
    if (!node.GetChild().IsNull()) {
        auto nodeNew = node.GetChild();
        std::set<std::string> newFiledSet;
        CheckNode(nodeNew, newFiledSet, errFlag);
    }
    if (!node.GetNext().IsNull()) {
        auto nodeNew = node.GetNext();
        CheckNode(nodeNew, filedSet, errFlag);
    }
    return errFlag;
}

bool JsonCommon::CheckJsonField(JsonObject &jsonObj) 
{
    std::set<std::string> filedSet;
    bool errFlag = true;
    return CheckNode(jsonObj, filedSet, errFlag);
}

bool JsonCommon::CheckProjectionNode(JsonObject &node, std::set<std::string> filedSet, bool &errFlag, bool isFirstFloor) 
{
    if (!errFlag) {
        return false;
    }
    std::string fieldName; 
    if (!node.IsNull()) {
        int ret = 0;
        fieldName = node.GetItemFiled(ret);
        if (filedSet.find(fieldName) == filedSet.end()) {
            if (ret == E_OK) {
                filedSet.insert(fieldName);
                if (fieldName.empty()) {
                    errFlag = false;
                    return false;
                }
            }
        } else {
            errFlag = false;
            return false;
        }
        for (int i = 0; i < fieldName.size(); i++) {
            if (!((isalpha(fieldName[i])) || (isdigit(fieldName[i])) || ('_' == fieldName[i]) || (isFirstFloor && '.' == fieldName[i]))) {
                errFlag = false;
                return false;
            }
            if (i == 0 && (isdigit(fieldName[i]))) {
                errFlag = false;
                return false;
            }
        } 
    }
    if (!node.GetChild().IsNull()) {
        auto nodeNew = node.GetChild();
        std::set<std::string> newFiledSet;
        CheckProjectionNode(nodeNew, newFiledSet, errFlag, false);
    }
    if (!node.GetNext().IsNull()) {
        auto nodeNew = node.GetNext();
        CheckProjectionNode(nodeNew, filedSet, errFlag, isFirstFloor);
    }
    return errFlag;
}

bool JsonCommon::CheckProjectionField(JsonObject &jsonObj) 
{
    std::set<std::string> filedSet;
    bool errFlag = true;
    bool isFirstFloor = true;
    return CheckProjectionNode(jsonObj, filedSet, errFlag, isFirstFloor);
}

int JsonCommon::ParseNode(JsonObject &node, std::vector<std::string> singlePath, 
                        std::vector<std::vector<std::string>> &resultPath, bool isFirstFloor)
{
    std::vector<std::string> fatherPath;
    if (isFirstFloor) {
        std::string tempParseName;
        std::vector<std::string> allFiledsName;
        std::string priFieldName = node.GetItemFiled();
        for (int j = 0; j < priFieldName.size(); j++) {
            if (priFieldName[j] != '.') {
                tempParseName = tempParseName + priFieldName[j];
            }
            if (priFieldName[j] == '.' || j == priFieldName.size() - 1) {
                allFiledsName.emplace_back(tempParseName);
                tempParseName.clear();
            }
        }
        fatherPath = singlePath;
        singlePath.insert(singlePath.end(), allFiledsName.begin(), allFiledsName.end());
    } else {
        std::vector<std::string> allFiledsName;
        allFiledsName.emplace_back(node.GetItemFiled());
        fatherPath = singlePath;
        singlePath.insert(singlePath.end(), allFiledsName.begin(), allFiledsName.end());
    }
    if (!node.GetChild().IsNull() && node.GetChild().GetItemFiled() != "") {
        auto nodeNew = node.GetChild();
        ParseNode(nodeNew, singlePath, resultPath, false);
    } else {
        resultPath.emplace_back(singlePath);
    }
    if (!node.GetNext().IsNull()) {
        auto nodeNew = node.GetNext();
        ParseNode(nodeNew, fatherPath, resultPath, isFirstFloor);
    }
    return 0;
}

std::vector<std::vector<std::string>> JsonCommon::ParsePath(const JsonObject &root)
{
    std::vector<std::vector<std::string>> resultPath;
    auto projectionJson = root.GetChild();
    if (projectionJson.IsNull()) {
        GLOGE("projectionJson is null");
    }
    std::vector<std::string> singlePath;
    ParseNode(projectionJson, singlePath, resultPath, true);
    return resultPath;
}

namespace {
JsonFieldPath ExpendPath(const JsonFieldPath &path, bool &isCollapse)
{
    if (path.size() > 1) { // only first lever has collapse field
        return path;
    }
    JsonFieldPath splitPath;
    const std::string &str = path[0];
    size_t start = 0;
    size_t end = 0;
    while ((end = str.find('.', start)) != std::string::npos) {
        splitPath.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    if (start < str.length()) {
        splitPath.push_back(str.substr(start));
    }
    isCollapse = (splitPath.size() > 1);
    return splitPath;
}

JsonFieldPath ExpendPathForField(const JsonFieldPath &path, bool &isCollapse)
{
   
    JsonFieldPath splitPath;
    const std::string &str = path[0];
    size_t start = 0;
    size_t end = 0;
    while ((end = str.find('.', start)) != std::string::npos) {
        splitPath.push_back(str.substr(start, end - start));
        start = end + 1;
    }
    if (start < str.length()) {
        splitPath.push_back(str.substr(start));
    }
    isCollapse = (splitPath.size() > 1);
    for (int i = 1; i < path.size(); i++) {
        splitPath.emplace_back(path[i]);
    }
    return splitPath;
}

void JsonObjectIterator(const JsonObject &obj, JsonFieldPath path,
    std::function<bool (const JsonFieldPath &path, const JsonObject &father, const JsonObject &item)> foo)
{
    JsonObject child = obj.GetChild();
    while(!child.IsNull()) {
        JsonFieldPath childPath = path;
        childPath.push_back(child.GetItemFiled());
        if (foo != nullptr && foo(childPath, obj, child)) {
            JsonObjectIterator(child, childPath, foo);
        }
        child = child.GetNext();
    }
    return;
}

void JsonObjectIterator(const JsonObject &obj, JsonFieldPath path,
    std::function<bool (JsonFieldPath &path, const JsonObject &item)> foo)
{
    JsonObject child = obj.GetChild();
    while(!child.IsNull()) {
        bool isCollapse = false;
        JsonFieldPath childPath = path;
        childPath.push_back(child.GetItemFiled());
        if (foo != nullptr && foo(childPath, child)) {
            JsonObjectIterator(child, childPath, foo);
        }
        child = child.GetNext();
    }
    return;
}
}

int JsonCommon::Append(const JsonObject &src, const JsonObject &add)
{
    int externErrCode = E_OK;
    JsonObjectIterator(add, {},
        [&src, &externErrCode](const JsonFieldPath &path, const JsonObject &father, const JsonObject &item) {
        bool isCollapse = false;
        JsonFieldPath itemPath = ExpendPath(path, isCollapse);
        JsonFieldPath fatherPath = itemPath;
        fatherPath.pop_back();
        int errCode = E_OK;
        if (src.IsFieldExists(itemPath)) {
            JsonObject srcItem = src.FindItem(itemPath, errCode);
            if (errCode != E_OK) {
                externErrCode = (externErrCode == E_OK ? errCode : externErrCode);
                GLOGE("Find item in source json object failed. %d", errCode);
                return false;
            }
            if (srcItem.GetType() == JsonObject::Type::JSON_LEAF && item.GetType() == JsonObject::Type::JSON_LEAF) {
                srcItem.SetItemValue(item.GetItemValue());
                return false; // Both leaf node, no need iterate
            } else if (srcItem.GetType() != item.GetType()) {
                JsonObject srcFatherItem = src.FindItem(fatherPath, errCode);
                if (errCode != E_OK) {
                    externErrCode = (externErrCode == E_OK ? errCode : externErrCode);
                    GLOGE("Find father item in source json object failed. %d", errCode);
                    return false;
                }
                srcFatherItem.DeleteItemFromObject(itemPath.back());
                srcFatherItem.AddItemToObject(itemPath.back(), item);
                return false; // Different node types, overwrite directly, skip child node
            }
            return true; // Both array or object
        } else {
            if (isCollapse) {
                GLOGE("Add collapse item to object failed, path not exist.");
                externErrCode = -E_DATA_CONFLICT;
                return false;
            }
            JsonObject srcFatherItem = src.FindItem(fatherPath, errCode);
            if (errCode == E_OK) {
                errCode = srcFatherItem.AddItemToObject(itemPath.back(), item);
                if (errCode != E_OK) {
                    externErrCode = (externErrCode == E_OK ? errCode : externErrCode);
                    GLOGE("Add item to object failed. %d", errCode);
                    return false;
                }
            } else {
                externErrCode = -E_DATA_CONFLICT;
                GLOGE("Find father item in source json object failed. %d", errCode);
            }
            return false; // Source path not exist, overwrite directly, skip child node
        }
    });
    return externErrCode;
}
bool JsonCommon::isValueEqual(const ValueObject &srcValue, const ValueObject &targetValue)
{
    if (srcValue.GetValueType() == targetValue.GetValueType()) {
        switch (srcValue.GetValueType()) {
            case ValueObject::ValueType::VALUE_NULL:
                return true;
            case ValueObject::ValueType::VALUE_BOOL:
                return srcValue.GetBoolValue() == targetValue.GetBoolValue() ? true : false;
            case ValueObject::ValueType::VALUE_NUMBER:
                return srcValue.GetDoubleValue() == targetValue.GetDoubleValue() ? true : false;
            case ValueObject::ValueType::VALUE_STRING:
                return srcValue.GetStringValue() == targetValue.GetStringValue() ? true : false;
        }
    }
    return false;
}

bool JsonCommon::isArrayMathch(const JsonObject &src, const JsonObject &target, int &isAlreadyMatched) 
{
    JsonObject srcChild = src.GetChild();
    JsonObject targetObj = target;
    bool isMatch = false;
    int errCode = 0;
    while (!srcChild.IsNull()) {
        if (srcChild.GetType() == JsonObject::Type::JSON_OBJECT && target.GetType() == JsonObject::Type::JSON_OBJECT
            && (isJsonNodeMatch(srcChild, target, errCode))) {
            isMatch = true;
            isAlreadyMatched = 1;
            break;
        }
        srcChild = srcChild.GetNext();
    }
    return isMatch;
}

bool JsonCommon::JsonEqualJudge(JsonFieldPath &itemPath, const JsonObject &src, const JsonObject &item, 
                                int &isAlreadyMatched, bool &isCollapse, int &isMatchFlag) 
{
    int errCode;
    JsonObject srcItem = src.FindItemIncludeArray(itemPath, errCode);
    auto GranpaPath = itemPath;
    auto lastFiledName = GranpaPath.back();
    GranpaPath.pop_back();
    JsonObject GranpaItem = src.FindItemIncludeArray(GranpaPath, errCode);
    if (GranpaItem.GetType() == JsonObject::Type::JSON_ARRAY && isCollapse) {
        JsonObject FatherItem = GranpaItem.GetChild();
        while (!FatherItem.IsNull()) {
            bool isEqual = (FatherItem.GetObjectItem(lastFiledName, errCode).Print() == item.Print());
            if (isEqual) {                       
                GLOGI("Filter value is equal with src");
                isMatchFlag = isEqual;
                isAlreadyMatched = 1;
            } 
            FatherItem = FatherItem.GetNext();
        }
    }
    if (srcItem.GetType() == JsonObject::Type::JSON_ARRAY && item.GetType() == JsonObject::Type::JSON_ARRAY && !isAlreadyMatched) {
        bool isEqual = (srcItem.Print() == item.Print());
        if (!isEqual) {
            GLOGI("Filter value is No equal with src");
            isMatchFlag = isEqual;
        }
        isAlreadyMatched = isMatchFlag;
        return false; // Both leaf node, no need iterate
    }
    if (srcItem.GetType() == JsonObject::Type::JSON_LEAF && item.GetType() == JsonObject::Type::JSON_LEAF && !isAlreadyMatched) {
        bool isEqual = isValueEqual(srcItem.GetItemValue(), item.GetItemValue());
        if (!isEqual) {
            GLOGI("Filter value is No equal with src");
            isMatchFlag = isEqual;
        }
        isAlreadyMatched = isMatchFlag;
        return false; // Both leaf node, no need iterate
    } else if (srcItem.GetType() != item.GetType()) {
        if (srcItem.GetType() == JsonObject::Type::JSON_ARRAY) {
            GLOGI("srcItem Type is ARRAY, item Type is not ARRAY");
            bool isEqual = isArrayMathch(srcItem, item, isAlreadyMatched);
            if (!isEqual) {
                isMatchFlag = isEqual;
            }
            return true;
        }
        GLOGI("valueType is different");
        isMatchFlag = false;
        return false; // Different node types, overwrite directly, skip child node
    }
    return true; // Both array or object
}

bool JsonCommon::isJsonNodeMatch(const JsonObject &src, const JsonObject &target, int &externErrCode)
{
    externErrCode = E_OK;
    int isMatchFlag = true;
    JsonObjectIterator(target, {},
        [&src, &isMatchFlag, &externErrCode](JsonFieldPath &path, const JsonObject &item) {
        int isAlreadyMatched = 0;
        bool isCollapse = false;
        if (isMatchFlag == false) {
            return false;
        }
        JsonFieldPath itemPath = ExpendPath(path, isCollapse);
        int errCode = 0;
        if (src.IsFieldExistsIncludeArray(itemPath)) {
            return JsonEqualJudge(itemPath, src, item, isAlreadyMatched, isCollapse, isMatchFlag);
        } else {
            if (isCollapse) {
                GLOGE("Match failed, path not exist.");
                isMatchFlag = false;
                return false;
            }
            GLOGI("Not match anything");
            if (isAlreadyMatched == 0) {
                isMatchFlag = false;
            }
            std::vector<ValueObject> ItemLeafValue = GetLeafValue(item);
            int isNULLFlag = true;
            for (auto ValueItem : ItemLeafValue) {
                if (ValueItem.GetValueType() != ValueObject::ValueType::VALUE_NULL) {
                    GLOGI("leaf value is not null");
                    isNULLFlag = false;
                } else {
                    GLOGI("filter leaf is null, Src leaf is dont exist");
                    isMatchFlag = true;
                }
            }
            return false; // Source path not exist, if leaf value is null, isMatchFlag become true, else it will become false.
        }
    });
    return isMatchFlag;
}
} // namespace DocumentDB