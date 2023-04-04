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

#include "json_object.h"
#include <algorithm>
#include "doc_errno.h"
#include "log_print.h"
// #include "grd_format_config.h"

namespace DocumentDB {

namespace {
#define COLLECTION_LENS_MAX (512)
#define JSON_LENS_MAX (512)
#define JSON_DEEP_MAX (4)
}

ValueObject::ValueObject(bool val)
{
    valueType = ValueType::VALUE_BOOL;
    boolValue = val;
}

ValueObject::ValueObject(double val)
{
    valueType = ValueType::VALUE_NUMBER;
    doubleValue = val;
}

ValueObject::ValueObject(const char *val)
{
    valueType = ValueType::VALUE_STRING;
    stringValue = val;
}

ValueObject::ValueObject(const std::string &val)
{
    valueType = ValueType::VALUE_STRING;
    stringValue = val;
}

ValueObject::ValueType ValueObject::GetValueType() const
{
    return valueType;
}

bool ValueObject::GetBoolValue() const
{
    return boolValue;
}

int ValueObject::GetIntValue() const
{
    return static_cast<int>(doubleValue + 0.5);
}

double ValueObject::GetDoubleValue() const
{
    return doubleValue;
}

std::string ValueObject::GetStringValue() const
{
    return stringValue;
}

JsonObject JsonObject::Parse(const std::string &jsonStr, int &errCode, bool caseSensitive)
{
    JsonObject obj;
    errCode = obj.Init(jsonStr);
    obj.caseSensitive_ = caseSensitive;
    return obj;
}

JsonObject::JsonObject()
{
}

// JsonObject::JsonObject(const JsonObject &obj)
// {
//     this->cjson_ = cJSON_Duplicate(obj.cjson_, true);
//     this->isOwner_ = true;
//     this->caseSensitive_ = obj.caseSensitive_;
// }

JsonObject::~JsonObject()
{
    if (isOwner_ == true) {
        cJSON_Delete(cjson_);
    }
}

bool JsonObject::IsNull() const
{
    if (cjson_ == nullptr) {
        return true;
    }
    return false;
}

JsonObject::Type JsonObject::GetType() const
{
    if (cjson_->type == cJSON_Object) {
        return JsonObject::Type::JSON_OBJECT;
    } else if (cjson_->type == cJSON_Array) {
        return JsonObject::Type::JSON_ARRAY;
    }
    return JsonObject::Type::JSON_LEAF;
}

int JsonObject::GetDeep(cJSON *cjson)
{
    if (cjson->child == nullptr) {
        return 1; // leaf node
    }

    int depth = -1;
    cJSON *child = cjson->child;
    while (child != nullptr) {
        depth = std::max(depth, GetDeep(child) + 1);
        child = child->next;
    }
    return depth;
}

int JsonObject::Init(const std::string &str)
{
    if (str.length() + 1 > JSON_LENS_MAX) {
        return -E_INVALID_ARGS;
    }
    const char *end = NULL;
    isOwner_ = true;
    cjson_ = cJSON_ParseWithOpts(str.c_str(), &end, true);
    if (cjson_ == nullptr) {
        return -E_INVALID_ARGS;
    }

    if (cjson_->type != cJSON_Object) {
        return -E_INVALID_ARGS;
    }

    if (GetDeep(cjson_) > JSON_DEEP_MAX) {
        return -E_INVALID_ARGS;
    }
    return E_OK;
}

std::string JsonObject::Print() const
{
    if (cjson_ == nullptr) {
        return "";
    }
    char *ret = cJSON_PrintUnformatted(cjson_);
    std::string str = ret;
    free(ret);
    return str;
}

JsonObject JsonObject::GetObjectItem(const std::string &field, int &errCode)
{
    if (cjson_ == nullptr || (cjson_->type & cJSON_Object) != cJSON_Object) {
        errCode = -E_INVALID_ARGS;
        return JsonObject();
    }

    JsonObject item;
    item.caseSensitive_ = caseSensitive_;
    if (caseSensitive_) {
        item.cjson_ = cJSON_GetObjectItemCaseSensitive(cjson_, field.c_str());
    } else {
        item.cjson_ = cJSON_GetObjectItem(cjson_, field.c_str());
    }
    if (item.cjson_ == nullptr) {
        errCode = -E_NOT_FOUND;
    }
    return item;
}

JsonObject JsonObject::GetArrayItem(int index, int &errCode)
{
    if (cjson_ == nullptr || (cjson_->type & cJSON_Array) != cJSON_Array) {
        errCode = -E_INVALID_ARGS;
        return JsonObject();
    }

    JsonObject item;
    item.caseSensitive_ = caseSensitive_;
    item.cjson_ = cJSON_GetArrayItem(cjson_, index);
    if (item.cjson_ == nullptr) {
        errCode = -E_NOT_FOUND;
    }
    return item;
}

JsonObject JsonObject::GetNext() const
{
    if (cjson_ == nullptr) {
        return JsonObject();
    }
    JsonObject next;
    next.caseSensitive_ = caseSensitive_;
    if (cjson_->next == nullptr) {
        return JsonObject();
    }
    next.cjson_ = cjson_->next;
    return next;
}

JsonObject JsonObject::GetChild() const
{
    if (cjson_ == nullptr) {
        return JsonObject();
    }
    JsonObject child;
    child.caseSensitive_ = caseSensitive_;
    if (cjson_->child == nullptr) {
        return JsonObject();
    }
    child.cjson_ = cjson_->child;
    return child;
}

int JsonObject::DeleteItemFromObject(const std::string &field)
{
    if (field == "") {
        return E_OK;
    }
    cJSON_DeleteItemFromObjectCaseSensitive(cjson_, field.c_str());
    return E_OK;
}

int JsonObject::AddItemToObject(const JsonObject &item)
{
    if (item.IsNull()) {
        GLOGD("Add null object.");
        return E_OK;
    }

    cJSON *cpoyItem = cJSON_Duplicate(item.cjson_, true);
    cJSON_AddItemToObject(cjson_, item.GetItemFiled().c_str(), cpoyItem);
    return E_OK;
}

ValueObject JsonObject::GetItemValue() const
{
    if (cjson_ == nullptr) {
        return ValueObject();
    }

    ValueObject value;
    switch (cjson_->type) {
        case cJSON_False:
        case cJSON_True:
            return ValueObject(cjson_->type == cJSON_True);
        case cJSON_NULL:
            return ValueObject();
        case cJSON_Number:
            return ValueObject(cjson_->valuedouble);
        case cJSON_String:
            return ValueObject(cjson_->valuestring);
        case cJSON_Array:
        case cJSON_Object:
        default:
            GLOGW("Invalid json type: %d", cjson_->type);
            break;
    }

    return value;
}

void JsonObject::SetItemValue(const ValueObject &value) const
{
    if (cjson_ == nullptr) {
        return;
    }
    switch(value.GetValueType()) {
        case ValueObject::ValueType::VALUE_NUMBER:
            cJSON_SetNumberValue(cjson_, value.GetDoubleValue());
            break;
        case ValueObject::ValueType::VALUE_STRING:
            cJSON_SetValuestring(cjson_, value.GetStringValue().c_str());
            break;
        default:
            break;
    }
}

std::string JsonObject::GetItemFiled() const
{
    if (cjson_ == nullptr) {
        return "";
    }

    if (cjson_->string == nullptr) {
        cJSON *tail = cjson_;
        while(tail->next != nullptr) {
            tail = tail->next;
        }

        int index = 0;
        cJSON *head = cjson_;
        while (head->prev != tail) {
            head = head->prev;
            index++;

            if (index > 10) break;
        }
        return std::to_string(index);
    } else {
        return cjson_->string;
    }
}

namespace {
bool IsNumber(const std::string &str) {
    return std::all_of(str.begin(), str.end(), [](char c) {
        return std::isdigit(c);
    });
}

cJSON *GetChild(cJSON *cjson, const std::string &field, bool caseSens)
{
    if (cjson->type == cJSON_Object) {
        if (caseSens) {
            return cJSON_GetObjectItemCaseSensitive(cjson, field.c_str());
        } else {
            return cJSON_GetObjectItem(cjson, field.c_str());
        }
    } else if (cjson->type == cJSON_Array) {
        if (!IsNumber(field)) {
            GLOGW("Invalid json field path, expect array index.");
            return nullptr;
        }
        return cJSON_GetArrayItem(cjson, std::stoi(field));
    }

    GLOGW("Invalid json field type, expect object or array.");
    return nullptr;
}

cJSON *MoveToPath(cJSON *cjson, const JsonFieldPath &jsonPath, bool caseSens)
{
    for (const auto &field : jsonPath) {
        cjson = GetChild(cjson, field, caseSens);
        if (cjson == nullptr) {
            GLOGW("Invalid json field path, no such field. %s", field.c_str());
        }
    }
    return cjson;
}
}


bool JsonObject::IsFieldExists(const JsonFieldPath &jsonPath) const
{
    return (MoveToPath(cjson_, jsonPath, caseSensitive_) != nullptr);
}

JsonObject JsonObject::FindItem(const JsonFieldPath &jsonPath, int &errCode) const
{
    if (jsonPath.empty()) {
        JsonObject curr = JsonObject();
        curr.cjson_ = cjson_;
        curr.caseSensitive_ = caseSensitive_;
        curr.isOwner_ = false;
        GLOGE("return current object");
        return curr;
    }

    cJSON *findItem = MoveToPath(cjson_, jsonPath, caseSensitive_);
    if (findItem == nullptr) {
        GLOGE("Find item failed. json field path not found.");
        errCode = -E_JSON_PATH_NOT_EXISTS;
    }

    JsonObject item;
    item.caseSensitive_ = caseSensitive_;
    item.cjson_ = findItem;
    return item;
}

ValueObject JsonObject::GetObjectByPath(const JsonFieldPath &jsonPath, int &errCode) const
{
    JsonObject objGot = FindItem(jsonPath, errCode);
    if (errCode != E_OK) {
        GLOGE("Get json value object failed. %d", errCode);
        return {};
    }
    return objGot.GetItemValue();
}

int JsonObject::DeleteItemOnTarget(const JsonFieldPath &path)
{
    if (path.empty()) {
        return -E_INVALID_ARGS;
    }

    std::string fieldName = path.back();
    JsonFieldPath patherPath = path;
    patherPath.pop_back();

    int errCode = E_OK;
    cJSON *nodeFather = MoveToPath(cjson_, patherPath, caseSensitive_);
    if (nodeFather == nullptr) {
        GLOGE("Delete item failed, json field path not found.");
        return -E_JSON_PATH_NOT_EXISTS;
    }

    if (nodeFather->type == cJSON_Object) {
        if (caseSensitive_) {
            cJSON_DeleteItemFromObjectCaseSensitive(nodeFather, fieldName.c_str());
        } else {
            cJSON_DeleteItemFromObject(nodeFather, fieldName.c_str());
        }
    } else if (nodeFather->type == cJSON_Array) {
        if (!IsNumber(fieldName)) {
            GLOGW("Invalid json field path, expect array index.");
            return -E_JSON_PATH_NOT_EXISTS;
        }
        cJSON_DeleteItemFromArray(nodeFather, std::stoi(fieldName));
    }

    return E_OK;
}
