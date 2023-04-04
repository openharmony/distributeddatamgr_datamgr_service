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

std::vector<std::vector<std::string>> JsonCommon::ParsePath(JsonObject* root)
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

std::string PrintJsonPath(const JsonFieldPath &path) {
    std::string str;
    for (const auto &field : path) {
        str += field + ".";
    }
    return str;
}
}

void JsonCommon::Append(const JsonObject &src, const JsonObject &add)
{
    JsonObjectIterator(add, {}, [&src](const JsonFieldPath &path, const JsonObject &father, const JsonObject &item) {
        GLOGD("---->path: %s", PrintJsonPath(path).c_str());
        JsonFieldPath patherPath = path;
        patherPath.pop_back();
        int errCode = E_OK;
        if (src.IsFieldExists(path)) {
            JsonObject srcItem = src.FindItem(path, errCode);
            if (srcItem.GetType() == JsonObject::Type::JSON_LEAF && item.GetType() == JsonObject::Type::JSON_LEAF) {
                srcItem.SetItemValue(item.GetItemValue());
                return true;
            } else if (srcItem.GetType() == JsonObject::Type::JSON_OBJECT && item.GetType() == JsonObject::Type::JSON_OBJECT) {
                return true;
            } else if (srcItem.GetType() == JsonObject::Type::JSON_ARRAY && item.GetType() == JsonObject::Type::JSON_ARRAY) {
                return true;
            } else {
                JsonObject srcFatherItem = src.FindItem(patherPath, errCode);
                srcFatherItem.DeleteItemFromObject(item.GetItemFiled());
                srcFatherItem.AddItemToObject(item);
                return false;
            }
        } else {
            GLOGD("Append value to source: %s - %d.", item.GetItemFiled().c_str(), item.GetItemValue().GetIntValue());
            GLOGD("Append valie : %s", item.Print().c_str());
            JsonObject srcFatherItem = src.FindItem(patherPath, errCode);
            srcFatherItem.AddItemToObject(item);
            return false; // skip child
        }
    });
}
} // namespace DocumentDB