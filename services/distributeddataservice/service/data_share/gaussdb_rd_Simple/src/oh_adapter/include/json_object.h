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

#ifndef JSON_OBJECT_H
#define JSON_OBJECT_H

#include <string>
#include <vector>
#include <typeinfo>

#include "cJSON.h"

namespace DocumentDB {
class JsonObject;
class ResultValue {
public:
    ResultValue() {};
    ResultValue(const ResultValue& newValue) {
        valueNumber = newValue.valueNumber;
        valueString = newValue.valueString;
        valueType = newValue.valueType;
        valueObject = newValue.valueObject;
    }
    enum class ValueType {
        VALUE_FALSE,
        VALUE_TRUE,
        VALUE_NULL,
        VALUE_NUMBER,
        VALUE_STRING,
        VALUE_ARRAY,
        VALUE_OBJECT
    };
    int valueNumber;
    std::string valueString;
    ValueType valueType = ValueType::VALUE_NULL;
    JsonObject *valueObject = nullptr; 
};

class JsonObject {
public:
    JsonObject();
    JsonObject(const JsonObject& newObj);
    ~JsonObject ();

    int Init(const std::string &str);
    std::string Print();
    JsonObject GetObjectItem(const std::string &field, bool caseSensitive);
    JsonObject GetArrayItem(const int index);
    JsonObject GetNext();
    JsonObject GetChild();
    int DeleteItemFromObject(const std::string &field);
    ResultValue GetItemValue();
    std::string GetItemFiled();
    JsonObject FindItem(const std::vector<std::string> jsonPath);
    int DeleteItemOnTarget(std::vector<std::string> &path);
    bool IsNull();
private:
    int GetDeep(cJSON *cjson, int deep, int &maxDeep);
    cJSON *cjson_;
    bool isClone_ = false;
};
} // DocumentDB
#endif // JSON_OBJECT_H

