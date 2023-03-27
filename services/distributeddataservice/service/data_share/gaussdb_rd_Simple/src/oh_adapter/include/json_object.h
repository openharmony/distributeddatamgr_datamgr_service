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

namespace DocumentDB {

class JsonObject;
class ResultValue {
public:
    enum class ValueType {
        VALUE_FALSE,
        VALUE_TRUE,
        VALUE_NULL,
        VALUE_NUMBER,
        VALUE_STRING,
        VALUE_ARRAY,
        VALUE_OBJECT
    };
    int value_number;
    std::string value_string;
    ValueType value_type = ValueType::VALUE_NULL;
    JsonObject *value_Object = nullptr; 
};

class JsonObject {
public:
    JsonObject () = default;
    virtual ~JsonObject () = default;

    virtual int Parse(const std::string &str) = 0;
    virtual std::string Print() = 0;
    virtual JsonObject* GetObjectItem(const std::string &field) = 0;
    virtual JsonObject* GetArrayItem(const int index) = 0;
    virtual ResultValue* GetItemValue() = 0;
    virtual std::string GetItemFiled() = 0;
    virtual int DeleteItemFromObject(const std::string &field) = 0;
    virtual int Delete() = 0;
};
} // DocumentDB
#endif // JSON_OBJECT_H