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
        if (node->GetItemFiled() == filed)
          {
            auto item_value = node->GetItemValue();
            return item_value;
        }
        if (node->GetNext().IsNull() == true) {
            return ResultValue();
        }
        auto node_new = node->GetNext();
        node = &node_new;
    }
    return ResultValue();
}

int JsonCommon::CheckLeafNode(JsonObject *node, std::vector<ResultValue> &leafValue)
{  
    if (node->GetChild().IsNull() == true) {
        auto item_value = node->GetItemValue();
        leafValue.emplace_back(item_value);
    } 
    if (node->GetChild().IsNull() != true) {
        auto node_new = node->GetChild();
        CheckLeafNode(&node_new, leafValue);
    }
    if (node->GetNext().IsNull() != true) {
        auto node_new = node->GetNext();
        CheckLeafNode(&node_new, leafValue);
    }
    return E_OK;
}
std::vector<ResultValue>  JsonCommon::GetLeafValue(JsonObject *node)
{
    std::vector<ResultValue> leafValue;
    CheckLeafNode(node, leafValue);
    return leafValue;
}

bool JsonCommon::CheckNode(JsonObject *node, std::set<std::string> setString, bool &errflag) {
    if (errflag == false) {
        return false;
    }
    std::string field_str; 
    if (node->GetItemValue().value_type != ResultValue::ValueType::VALUE_NULL) {
        field_str = node->GetItemFiled();
        if (setString.find(field_str) == setString.end()) {
            setString.insert(field_str);
        }
        else {
            errflag = false;
            return false;
        }
        for (int i = 0; i < field_str.size(); i++) {
            if (!(('a'<=field_str[i] && field_str[i]<='z')|| ('A'<=field_str[i] && field_str[i]<='Z') || ('0'<=field_str[i] && field_str[i]<='9') || '_' == field_str[i])) {
                errflag = false;
                return false;
            }
        } 
    }
    if (node->GetChild().IsNull() != true) {
        auto node_new = node->GetChild();
        std::set<std::string> stringSet_new;
        CheckNode(&node_new, stringSet_new, errflag);
    }
    if (node->GetNext().IsNull() != true) {
        auto node_new = node->GetNext();
        CheckNode(&node_new, setString, errflag);
    } 
    return errflag;
}

bool JsonCommon::CheckJsonField(const std::string &data) {
    JsonObject jsonObj;
    if (jsonObj.Init(data) != E_OK) {
        return false;
    }
    std::set<std::string> stringSet;
    bool errflag = true;
    return CheckNode(&jsonObj, stringSet, errflag);
}

int JsonCommon::ParseNode(JsonObject* node, std::vector<std::string> onePath, std::vector<std::vector<std::string>> &parsePath, bool isFirstFloor)
{
    std::vector<std::string> forePath;
    if (isFirstFloor) {
        std::string tempparse_name;
        std::vector<std::string> parsed_mixfiled_name;
        std::string mixfield_name = node->GetItemFiled();
        for (int j = 0; j < mixfield_name.size(); j++) {
            if (mixfield_name[j] != '.') {
                tempparse_name = tempparse_name + mixfield_name[j];
            }
            if (mixfield_name[j] == '.' || j == mixfield_name.size() - 1) {
                parsed_mixfiled_name.emplace_back(tempparse_name);
                tempparse_name.clear();
            }
        }
        forePath = onePath;
        onePath.insert(onePath.end(), parsed_mixfiled_name.begin(), parsed_mixfiled_name.end());
    } else {
        std::vector<std::string> parsed_mixfiled_name;
        parsed_mixfiled_name.emplace_back(node->GetItemFiled());
        forePath = onePath;
        onePath.insert(onePath.end(), parsed_mixfiled_name.begin(), parsed_mixfiled_name.end());
    }
    if (node->GetChild().IsNull() != true && node->GetChild().GetItemFiled() != "") {
        auto node_new = node->GetChild();
        ParseNode(&node_new, onePath, parsePath, false);
    }
    else {
        parsePath.emplace_back(onePath);
    }
    if (node->GetNext().IsNull() != true) {
        auto node_new = node->GetNext();
        ParseNode(&node_new, forePath, parsePath, isFirstFloor);
    }
    return 0;
}

std::vector<std::vector<std::string>> JsonCommon::ParsePath(JsonObject* root)
{
    std::vector<std::vector<std::string>> parsePath;
    auto projection_json = root->GetChild();
    if (projection_json.IsNull() == true) {
        GLOGE("projection_json is null");
    }
    std::vector<std::string> onePath;
    ParseNode(&projection_json, onePath, parsePath, true);
    return parsePath;
}



} // namespace DocumentDB