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
#include "json_common.h"

#include <climits>
#include <functional>

#include "doc_errno.h"
#include "log_print.h"
#include "securec.h"

namespace DocumentDB {
ValueObject JsonCommon::GetValueByFiled(JsonObject &node, const std::string &filed)
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

ValueObject JsonCommon::GetValueByFiled(JsonObject &node, const std::string &filed, bool &isFiledExist)
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

std::vector<ValueObject> JsonCommon::GetLeafValue(const JsonObject &node)
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
        for (size_t i = 0; i < fieldName.size(); i++) {
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

bool JsonCommon::CheckProjectionNode(JsonObject &node, std::set<std::string> filedSet, bool &errFlag,
    bool isFirstFloor, int &errCode)
{
    if (!errFlag) {
        return false;
    }
    std::string fieldName;
    if (!node.IsNull()) {
        int ret = 0;
        fieldName = node.GetItemFiled(ret);
        if (fieldName.empty()) {
            errCode = -E_INVALID_ARGS;
            errFlag = false;
            return false;
        }
        if (filedSet.find(fieldName) == filedSet.end() && ret == E_OK) {
            filedSet.insert(fieldName);
        } else {
            errCode = -E_INVALID_JSON_FORMAT;
            errFlag = false;
            return false;
        }
        for (size_t i = 0; i < fieldName.size(); i++) {
            if (!((isalpha(fieldName[i])) || (isdigit(fieldName[i])) || ('_' == fieldName[i]) ||
                    (isFirstFloor && '.' == fieldName[i]))) {
                errCode = -E_INVALID_ARGS;
                errFlag = false;
                return false;
            }
            if (i == 0 && (isdigit(fieldName[i]))) {
                errCode = -E_INVALID_ARGS;
                errFlag = false;
                return false;
            }
        }
    }
    if (!node.GetChild().IsNull()) {
        auto nodeNew = node.GetChild();
        std::set<std::string> newFiledSet;
        CheckProjectionNode(nodeNew, newFiledSet, errFlag, false, errCode);
    }
    if (!node.GetNext().IsNull()) {
        auto nodeNew = node.GetNext();
        CheckProjectionNode(nodeNew, filedSet, errFlag, isFirstFloor, errCode);
    }
    return errFlag;
}

bool JsonCommon::CheckProjectionField(JsonObject &jsonObj, int &errCode)
{
    std::set<std::string> filedSet;
    bool errFlag = true;
    bool isFirstFloor = true;
    return CheckProjectionNode(jsonObj, filedSet, errFlag, isFirstFloor, errCode);
}

int JsonCommon::ParseNode(JsonObject &node, std::vector<std::string> singlePath,
    std::vector<std::vector<std::string>> &resultPath, bool isFirstFloor)
{
    while (!node.IsNull()) {
        int insertCount = 0;
        if (isFirstFloor) {
            std::string tempParseName;
            std::vector<std::string> allFiledsName;
            std::string priFieldName = node.GetItemFiled();
            for (size_t j = 0; j < priFieldName.size(); j++) {
                if (priFieldName[j] != '.') {
                    tempParseName = tempParseName + priFieldName[j];
                }
                if (priFieldName[j] == '.' || j == priFieldName.size() - 1) {
                    if (j > 0 && priFieldName[j] == '.' && priFieldName[j - 1] == '.') {
                        return -E_INVALID_ARGS;
                    }
                    allFiledsName.emplace_back(tempParseName);
                    insertCount++;
                    tempParseName.clear();
                }
            }
            singlePath.insert(singlePath.end(), allFiledsName.begin(), allFiledsName.end());
        } else {
            std::vector<std::string> allFiledsName;
            allFiledsName.emplace_back(node.GetItemFiled());
            insertCount++;
            singlePath.insert(singlePath.end(), allFiledsName.begin(), allFiledsName.end());
        }
        if (!node.GetChild().IsNull() && node.GetChild().GetItemFiled() != "") {
            auto nodeNew = node.GetChild();
            ParseNode(nodeNew, singlePath, resultPath, false);
        } else {
            resultPath.emplace_back(singlePath);
        }
        for (int i = 0; i < insertCount; i++) {
            singlePath.pop_back();
        }
        node = node.GetNext();
    }
    return E_OK;
}

std::vector<std::vector<std::string>> JsonCommon::ParsePath(const JsonObject &root, int &errCode)
{
    std::vector<std::vector<std::string>> resultPath;
    auto projectionJson = root.GetChild();
    if (projectionJson.IsNull()) {
        GLOGE("projectionJson is null");
    }
    std::vector<std::string> singlePath;
    errCode = ParseNode(projectionJson, singlePath, resultPath, true);
    return resultPath;
}

namespace {
JsonFieldPath SplitePath(const JsonFieldPath &path, bool &isCollapse)
{
    if (path.size() > 1 || path.empty()) { // only first lever has collapse field
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
    const std::string &str = path.back();
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
    std::function<bool(const JsonFieldPath &path, const JsonObject &father, const JsonObject &item)> AppendFoo)
{
    JsonObject child = obj.GetChild();
    while (!child.IsNull()) {
        JsonFieldPath childPath = path;
        childPath.push_back(child.GetItemFiled());
        if (AppendFoo != nullptr && AppendFoo(childPath, obj, child)) {
            JsonObjectIterator(child, childPath, AppendFoo);
        }
        child = child.GetNext();
    }
    return;
}

void JsonObjectIterator(const JsonObject &obj, JsonFieldPath path,
    std::function<bool(JsonFieldPath &path, const JsonObject &item)> MatchFoo)
{
    JsonObject child = obj.GetChild();
    while (!child.IsNull()) {
        JsonFieldPath childPath = path;
        childPath.push_back(child.GetItemFiled());
        if (MatchFoo != nullptr && MatchFoo(childPath, child)) {
            JsonObjectIterator(child, childPath, MatchFoo);
        }
        child = child.GetNext();
    }
    return;
}

bool IsNumber(const std::string &str)
{
    return std::all_of(str.begin(), str.end(), [](char c) {
        return std::isdigit(c);
    });
}

bool AddSpliteFiled(const JsonObject &src, const JsonObject &item, const JsonFieldPath &itemPath, int &externErrCode)
{
    int errCode = 0;
    JsonFieldPath abandonPath;
    JsonFieldPath hitPath = itemPath;
    while (!hitPath.empty()) {
        JsonObject srcFatherItem = src.FindItem(hitPath, errCode);
        abandonPath.emplace_back(hitPath.back());
        if (!srcFatherItem.IsNull()) {
            break;
        }
        hitPath.pop_back();
    }
    if (!hitPath.empty()) {
        JsonFieldPath preHitPath = hitPath;
        preHitPath.pop_back();
        JsonObject preHitItem = src.FindItem(preHitPath, errCode);
        JsonObject hitItem = preHitItem.GetObjectItem(hitPath.back(), errCode);
        if (!abandonPath.empty()) {
            abandonPath.pop_back();
        }
        if (!hitItem.IsNull()) {
            JsonFieldPath newHitPath;
            for (int i = abandonPath.size() - 1; i > -1; i--) {
                if (hitItem.GetType() != JsonObject::Type::JSON_OBJECT) {
                    GLOGE("Add collapse item to object failed, path not exist.");
                    externErrCode = -E_DATA_CONFLICT;
                    return false;
                }
                if (IsNumber(abandonPath[i])) {
                    externErrCode = -E_DATA_CONFLICT;
                    return false;
                }
                (i == 0) ? errCode = hitItem.AddItemToObject(abandonPath[i], item)
                         : errCode = hitItem.AddItemToObject(abandonPath[i]);
                externErrCode = (externErrCode == E_OK ? errCode : externErrCode);
                newHitPath.emplace_back(abandonPath[i]);
                hitItem = hitItem.FindItem(newHitPath, errCode);
                newHitPath.pop_back();
            }
            return false;
        }
    }
    JsonObject hitItem = src.FindItem(hitPath, errCode);
    JsonFieldPath newHitPath;
    for (int i = abandonPath.size() - 1; i > -1; i--) {
        if (hitItem.GetType() != JsonObject::Type::JSON_OBJECT) {
            GLOGE("Add collapse item to object failed, path not exist.");
            externErrCode = -E_DATA_CONFLICT;
            return false;
        }
        if (IsNumber(abandonPath[i])) {
            externErrCode = -E_DATA_CONFLICT;
            return false;
        }
        (i == 0) ? errCode = hitItem.AddItemToObject(abandonPath[i], item)
                 : errCode = hitItem.AddItemToObject(abandonPath[i]);
        externErrCode = (externErrCode == E_OK ? errCode : externErrCode);
        newHitPath.emplace_back(abandonPath[i]);
        hitItem = hitItem.FindItem(newHitPath, errCode);
        newHitPath.pop_back();
    }
    return false;
}

bool JsonValueReplace(const JsonObject &src, const JsonFieldPath &fatherPath, const JsonObject &father,
    const JsonObject &item, int &externErrCode)
{
    int errCode = 0;
    JsonFieldPath granPaPath = fatherPath;
    if (!granPaPath.empty()) {
        granPaPath.pop_back();
        JsonObject fatherItem = src.FindItem(granPaPath, errCode);
        if (errCode != E_OK) {
            externErrCode = (externErrCode == E_OK ? errCode : externErrCode);
            GLOGE("Find father item in source json object failed. %d", errCode);
            return false;
        }
        fatherItem.ReplaceItemInObject(item.GetItemFiled().c_str(), item, errCode);
        if (errCode != E_OK) {
            externErrCode = (externErrCode == E_OK ? errCode : externErrCode);
            GLOGE("Find father item in source json object failed. %d", errCode);
            return false;
        }
    } else {
        JsonObject fatherItem = src.FindItem(fatherPath, errCode);
        if (errCode != E_OK) {
            externErrCode = (externErrCode == E_OK ? errCode : externErrCode);
            GLOGE("Find father item in source json object failed. %d", errCode);
            return false;
        }
        if (father.GetChild().IsNull()) {
            externErrCode = -E_NO_DATA;
            GLOGE("Replace falied, no data match");
            return false;
        }
        if (!item.GetItemFiled(errCode).empty()) {
            fatherItem.ReplaceItemInObject(item.GetItemFiled().c_str(), item, errCode);
            if (errCode != E_OK) {
                return false;
            }
        }
    }
    return true;
}

bool JsonNodeReplace(const JsonObject &src, const JsonFieldPath &itemPath, const JsonObject &father,
    const JsonObject &item, int &externErrCode)
{
    int errCode = 0;
    JsonFieldPath fatherPath = itemPath;
    fatherPath.pop_back();
    if (!fatherPath.empty()) {
        JsonObject fatherItem = src.FindItem(fatherPath, errCode);
        if (errCode != E_OK) {
            externErrCode = (externErrCode == E_OK ? errCode : externErrCode);
            GLOGE("Find father item in source json object failed. %d", errCode);
            return false;
        }
        if (fatherItem.GetType() == JsonObject::Type::JSON_ARRAY && IsNumber(itemPath.back())) {
            fatherItem.ReplaceItemInArray(std::stoi(itemPath.back()), item, errCode);
            if (errCode != E_OK) {
                externErrCode = (externErrCode == E_OK ? errCode : externErrCode);
                GLOGE("Find father item in source json object failed. %d", errCode);
            }
            return false;
        }
        fatherItem.ReplaceItemInObject(itemPath.back().c_str(), item, errCode);
        if (errCode != E_OK) {
            externErrCode = (externErrCode == E_OK ? errCode : externErrCode);
            GLOGE("Find father item in source json object failed. %d", errCode);
            return false;
        }
    } else {
        JsonObject fatherItem = src.FindItem(fatherPath, errCode);
        if (errCode != E_OK) {
            externErrCode = (externErrCode == E_OK ? errCode : externErrCode);
            GLOGE("Find father item in source json object failed. %d", errCode);
            return false;
        }
        if (father.GetChild().IsNull()) {
            externErrCode = -E_NO_DATA;
            GLOGE("Replace falied, no data match");
            return false;
        }
        fatherItem.ReplaceItemInObject(itemPath.back().c_str(), item, errCode);
        if (errCode != E_OK) {
            externErrCode = (externErrCode == E_OK ? errCode : externErrCode);
            GLOGE("Find father item in source json object failed. %d", errCode);
            return false;
        }
    }
    return true;
}
} // namespace

int JsonCommon::Append(const JsonObject &src, const JsonObject &add, bool isReplace)
{
    int externErrCode = E_OK;
    bool isAddedFlag = false;
    JsonObjectIterator(add, {},
        [&src, &externErrCode, &isReplace, &isAddedFlag](const JsonFieldPath &path, const JsonObject &father,
            const JsonObject &item) {
            bool isCollapse = false;
            JsonFieldPath itemPath = ExpendPathForField(path, isCollapse);
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
                bool ret = JsonNodeReplace(src, itemPath, father, item, externErrCode);
                if (!ret) {
                    return false;
                }
                isAddedFlag = true;
                return false;
            } else {
                if (isReplace) {
                    GLOGE("path not exist, replace failed");
                    externErrCode = -E_NO_DATA;
                    return false;
                }
                JsonObject srcFatherItem = src.FindItem(fatherPath, errCode);
                std::string lastFieldName = itemPath.back();
                if (srcFatherItem.IsNull()) {
                    isAddedFlag = true;
                    AddSpliteFiled(src, item, itemPath, externErrCode);
                    return false;
                }
                if (errCode == E_OK) {
                    if (isCollapse &&
                        (!IsNumber(itemPath.back()) || srcFatherItem.GetType() == JsonObject::Type::JSON_ARRAY)) {
                        errCode = srcFatherItem.AddItemToObject(itemPath.back(), item);
                        if (errCode != E_OK) {
                            externErrCode = (externErrCode == E_OK ? errCode : externErrCode);
                            GLOGE("Add item to object failed. %d", errCode);
                            return false;
                        }
                        isAddedFlag = true;
                        return false;
                    }
                    if (!isCollapse) {
                        bool ret = JsonValueReplace(src, fatherPath, father, item, externErrCode);
                        if (!ret) {
                            return false; // replace faild
                        }
                        isAddedFlag = true;
                        return false; // Different node types, overwrite directly, skip child node
                    }
                }
                if (!isAddedFlag) {
                    GLOGE("Add nothing because data conflict");
                    externErrCode = -E_DATA_CONFLICT;
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

bool JsonCommon::IsArrayMatch(const JsonObject &src, const JsonObject &target, int &isAlreadyMatched)
{
    JsonObject srcChild = src.GetChild();
    JsonObject targetObj = target;
    bool isMatch = false;
    int errCode = 0;
    while (!srcChild.IsNull()) {
        if (srcChild.GetType() == JsonObject::Type::JSON_OBJECT && target.GetType() == JsonObject::Type::JSON_OBJECT &&
            (IsJsonNodeMatch(srcChild, target, errCode))) {
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
    JsonObject srcItem = src.FindItemPowerMode(itemPath, errCode);
    if (srcItem == item) {
        isMatchFlag = true;
        isAlreadyMatched = 1;
        return false;
    }
    JsonFieldPath granpaPath = itemPath;
    std::string lastFiledName = granpaPath.back();
    granpaPath.pop_back();
    JsonObject granpaItem = src.FindItemPowerMode(granpaPath, errCode);
    if (granpaItem.GetType() == JsonObject::Type::JSON_ARRAY && isCollapse) {
        JsonObject fatherItem = granpaItem.GetChild();
        while (!fatherItem.IsNull()) {
            if ((fatherItem.GetObjectItem(lastFiledName, errCode) == item)) {
                isMatchFlag = true;
                isAlreadyMatched = 1;
                break;
            }
            isMatchFlag = false;
            fatherItem = fatherItem.GetNext();
        }
        return false;
    }
    if (srcItem.GetType() == JsonObject::Type::JSON_ARRAY && item.GetType() == JsonObject::Type::JSON_ARRAY &&
        !isAlreadyMatched) {
        bool isEqual = (srcItem.Print() == item.Print());
        if (!isEqual) { // Filter value is No equal with src
            isMatchFlag = isEqual;
        }
        isAlreadyMatched = isMatchFlag;
        return false; // Both leaf node, no need iterate
    }
    if (srcItem.GetType() == JsonObject::Type::JSON_LEAF && item.GetType() == JsonObject::Type::JSON_LEAF &&
        !isAlreadyMatched) {
        bool isEqual = isValueEqual(srcItem.GetItemValue(), item.GetItemValue());
        if (!isEqual) { // Filter value is No equal with src
            isMatchFlag = isEqual;
        }
        isAlreadyMatched = isMatchFlag;
        return false; // Both leaf node, no need iterate
    } else if (srcItem.GetType() != item.GetType()) {
        if (srcItem.GetType() == JsonObject::Type::JSON_ARRAY) { // srcItem Type is ARRAY, item Type is not ARRAY
            bool isEqual = IsArrayMatch(srcItem, item, isAlreadyMatched);
            if (!isEqual) {
                isMatchFlag = isEqual;
            }
            return true;
        }
        isMatchFlag = false;
        return false; // Different node types, overwrite directly, skip child node
    }
    return true; // Both array or object
}

bool JsonCommon::IsJsonNodeMatch(const JsonObject &src, const JsonObject &target, int &errCode)
{
    errCode = E_OK;
    int isMatchFlag = true;
    JsonObjectIterator(target, {}, [&src, &isMatchFlag, &errCode](JsonFieldPath &path, const JsonObject &item) {
        int isAlreadyMatched = 0;
        bool isCollapse = false;
        if (isMatchFlag == false) {
            return false;
        }
        JsonFieldPath itemPath = SplitePath(path, isCollapse);
        if (src.IsFieldExistsPowerMode(itemPath)) {
            if (isCollapse) {
                return JsonEqualJudge(itemPath, src, item, isAlreadyMatched, isCollapse, isMatchFlag);
            }
            else {
                JsonObject srcItem = src.FindItemPowerMode(itemPath, errCode);
                if (srcItem.GetType() == JsonObject::Type::JSON_ARRAY) {
                    return JsonEqualJudge(itemPath, src, item, isAlreadyMatched, isCollapse, isMatchFlag);
                }
                if (srcItem == item) {
                    isMatchFlag = true;
                    isAlreadyMatched = true;
                    return false;
                }
                isMatchFlag = false;
                return false;
            }
        } else {
            std::vector<ValueObject> ItemLeafValue = GetLeafValue(item);
            int isNULLFlag = true;
            for (auto ValueItem : ItemLeafValue) {
                if (ValueItem.GetValueType() != ValueObject::ValueType::VALUE_NULL) { // leaf value is not null
                    isNULLFlag = false;
                } else { // filter leaf is null, Src leaf is dont exist
                    isMatchFlag = true;
                    return false;
                }
            }
            if (isCollapse) { // Match failed, path not exist
                isMatchFlag = false;
                return false;
            }
            if (isAlreadyMatched == 0) { //Not match anything
                isMatchFlag = false;
            }
            return false; // Source path not exist, if leaf value is null, isMatchFlag become true, else it will become false.
        }
    });
    return isMatchFlag;
}
} // namespace DocumentDB