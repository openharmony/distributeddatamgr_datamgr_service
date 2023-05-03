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

#include <memory>
#include <string>
#include <vector>
#include <typeinfo>

#include "cJSON.h"

namespace DocumentDB {
class ValueObject {
public:
    enum class ValueType {
        VALUE_NULL = 0,
        VALUE_BOOL,
        VALUE_NUMBER,
        VALUE_STRING,
    };

    ValueObject() = default;
    explicit ValueObject(bool val);
    explicit ValueObject(double val);
    explicit ValueObject(const char *val);
    explicit ValueObject(const std::string &val);

    ValueType GetValueType() const;
    bool GetBoolValue() const;
    int64_t GetIntValue() const;
    double GetDoubleValue() const;
    std::string GetStringValue() const;

private:
    ValueType valueType = ValueType::VALUE_NULL;
    union {
        bool boolValue;
        double doubleValue;
    };
    std::string stringValue;
};
using JsonFieldPath = std::vector<std::string>;

using ResultValue = ValueObject;
using JsonFieldPath = std::vector<std::string>;

class JsonObject {
public:
    static JsonObject Parse(const std::string &jsonStr, int &errCode, bool caseSensitive = false);

    ~JsonObject ();

    std::string Print() const;

    JsonObject GetObjectItem(const std::string &field, int &errCode);
    JsonObject GetArrayItem(int index, int &errCode);

    JsonObject GetNext() const;
    JsonObject GetChild() const;

    int DeleteItemFromObject(const std::string &field);
    int AddItemToObject(const JsonObject &item);
    int AddItemToObject(const std::string &fieldName, const JsonObject &item);

    ValueObject GetItemValue() const;
    void SetItemValue(const ValueObject &value) const;
    
    std::string GetItemFiled() const;
    std::string GetItemFiled(int &errCode) const;

    bool IsFieldExists(const JsonFieldPath &jsonPath) const;
    JsonObject FindItem(const JsonFieldPath &jsonPath, int &errCode) const;
    ValueObject GetObjectByPath(const JsonFieldPath &jsonPath, int &errCode) const;
    int DeleteItemOnTarget(const JsonFieldPath &path);
    int DeleteItemDeeplyOnTarget(const JsonFieldPath &path);
    bool IsNull() const;
    int GetDeep();
    enum class Type {
        JSON_LEAF,
        JSON_OBJECT,
        JSON_ARRAY
    };
    Type GetType() const;

private:
    JsonObject();
    int Init(const std::string &str);

    int GetDeep(cJSON *cjson);
    int CheckNumber(cJSON *cjson, int &errCode);

    cJSON *cjson_ = nullptr;
    int jsonDeep_ = 0;
    bool isOwner_ = false;
    bool caseSensitive_ = false;
};
} // DocumentDB
#endif // JSON_OBJECT_H

