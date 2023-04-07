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
ResultValue JsonCommon::GetValueByFiled(JsonObject *node, const std::string& filed)
{
    if (node == nullptr) {
        return ResultValue();
    }
    while (node != nullptr) {
        if (node->GetItemFiled() == filed) {
            auto itemValue = node->GetItemValue();
            return itemValue;
        }
        if (node->GetNext().IsNull() == true) {
            return ResultValue();
        }
        auto nodeNew = node->GetNext();
        node = &nodeNew;
    }
    return ResultValue();
}

int JsonCommon::CheckLeafNode(JsonObject *node, std::vector<ResultValue> &leafValue)
{
    if (node->GetChild().IsNull() == true) {
        auto itemValue = node->GetItemValue();
        leafValue.emplace_back(itemValue);
    }
    if (node->GetChild().IsNull() != true) {
        auto nodeNew = node->GetChild();
        CheckLeafNode(&nodeNew, leafValue);
    }
    if (node->GetNext().IsNull() != true) {
        auto nodeNew = node->GetNext();
        CheckLeafNode(&nodeNew, leafValue);
    }
    return E_OK;
}

std::vector<ResultValue>  JsonCommon::GetLeafValue(JsonObject *node)
{
    std::vector<ResultValue> leafValue;
    CheckLeafNode(node, leafValue);
    return leafValue;
}

bool JsonCommon::CheckNode(JsonObject *node, std::set<std::string> filedSet, bool &errFlag) {
    if (errFlag == false) {
        return false;
    }
    std::string fieldName;
    if (node->GetItemValue().GetValueType() != ResultValue::ValueType::VALUE_NULL) {
        fieldName = node->GetItemFiled();
        if (filedSet.find(fieldName) == filedSet.end()) {
            filedSet.insert(fieldName);
        }
        else {
            errFlag = false;
            return false;
        }
        for (int i = 0; i < fieldName.size(); i++) {
            if (!(('a'<=fieldName[i] && fieldName[i]<='z')|| ('A'<=fieldName[i] && fieldName[i]<='Z') || ('0'<=fieldName[i] && fieldName[i]<='9') || '_' == fieldName[i])) {
                errFlag = false;
                return false;
            }
        }
    }
    if (node->GetChild().IsNull() != true) {
        auto nodeNew = node->GetChild();
        std::set<std::string> newFiledSet;
        CheckNode(&nodeNew, newFiledSet, errFlag);
    }
    if (node->GetNext().IsNull() != true) {
        auto nodeNew = node->GetNext();
        CheckNode(&nodeNew, filedSet, errFlag);
    }
    return errFlag;
}

bool JsonCommon::CheckJsonField(const std::string &data) {
    int errCode = E_OK;
    JsonObject jsonObj = JsonObject::Parse(data, errCode);
    if (errCode != E_OK) {
        return false;
    }
    std::set<std::string> filedSet;
    bool errFlag = true;
    return CheckNode(&jsonObj, filedSet, errFlag);
}

int JsonCommon::ParseNode(JsonObject* node, std::vector<std::string> singlePath, std::vector<std::vector<std::string>> &resultPath, bool isFirstFloor)
{
    std::vector<std::string> fatherPath;
    if (isFirstFloor) {
        std::string tempParseName;
        std::vector<std::string> allFiledsName;
        std::string priFieldName = node->GetItemFiled();
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
        allFiledsName.emplace_back(node->GetItemFiled());
        fatherPath = singlePath;
        singlePath.insert(singlePath.end(), allFiledsName.begin(), allFiledsName.end());
    }
    if (node->GetChild().IsNull() != true && node->GetChild().GetItemFiled() != "") {
        auto nodeNew = node->GetChild();
        ParseNode(&nodeNew, singlePath, resultPath, false);
    }
    else {
        resultPath.emplace_back(singlePath);
    }
    if (node->GetNext().IsNull() != true) {
        auto nodeNew = node->GetNext();
        ParseNode(&nodeNew, fatherPath, resultPath, isFirstFloor);
    }
    return 0;
}

std::vector<std::vector<std::string>> JsonCommon::ParsePath(const JsonObject* const root)
{
    std::vector<std::vector<std::string>> resultPath;
    auto projectionJson = root->GetChild();
    if (projectionJson.IsNull() == true) {
        GLOGE("projectionJson is null");
    }
    std::vector<std::string> singlePath;
    ParseNode(&projectionJson, singlePath, resultPath, true);
    return resultPath;
}

namespace {
JsonFieldPath ExpendPath(const JsonFieldPath &path)
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
}

int JsonCommon::Append(const JsonObject &src, const JsonObject &add)
{
    int externErrCode = E_OK;
    JsonObjectIterator(add, {},
        [&src, &externErrCode](const JsonFieldPath &path, const JsonObject &father, const JsonObject &item) {
        JsonFieldPath itemPath = ExpendPath(path);
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
                srcFatherItem.DeleteItemFromObject(item.GetItemFiled());
                srcFatherItem.AddItemToObject(item);
                return false; // Different node types, overwrite directly, skip child node
            }
            return true; // Both array or object
        } else {
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
} // namespace DocumentDB