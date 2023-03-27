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
ResultValue* JsonCommon::GetValue(CjsonObject *root, std::vector<std::string> path) 
{
    return nullptr;
}

int JsonCommon::GetIdValue(CjsonObject *root, std::vector<std::string> &id) 
{
    auto &node = root;
    if (root == nullptr) {
        return E_OK;
    }
    while (node->GetNext() != nullptr) {
        if (node->GetItemFiled() == "_id") {
            auto item_value = node->GetItemValue();
            if (item_value->value_type != ResultValue::ValueType::VALUE_STRING) {
                return E_ERROR;
            }
            id.emplace_back(item_value->value_string);
        }
        node = node->GetNext();
    }
    return E_OK;
}

bool JsonCommon::CheckIsJson(const std::string &data) {
    CjsonObject cjsonObj;
    if (cjsonObj.Parse(data) == E_ERROR) {
        return false;
    }
    return true;
}

int JsonCommon::GetJsonDeep(const std::string &data) {
    int lens = 0;
    int deep = 0;
    for (int i = 0; i < data.size(); i++) {
        if (data[i] == '[' || data[i] == '{') {
            lens++;
        }
        else if (data[i] == ']' || data[i] == '}') {
            deep = std::max(deep, lens);
            lens--;
        }
    }
    return deep;
}

} // namespace DocumentDB