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

#include "doc_errno.h"
#include "cjson_object.h"
#include "log_print.h"

namespace DocumentDB {
int CjsonObject::Parse(const std::string &str)
{
    cjson_ = cJSON_Parse(str.c_str());
    if (cjson_ == nullptr) {
        GLOGE("json parse failed.");
        return -E_INVALID_JSON_FORMAT;
    }
    return E_OK;
}

std::string CjsonObject::Print()
{
    if (cjson_ == nullptr) {
        GLOGE("======================>NULL");
        return "";
    }
    char *ret = cJSON_Print(cjson_);
    std::string str = ret;
    free(ret);
    GLOGE("======================>%s", str.c_str());
    return str;
}

CjsonObject* CjsonObject::GetObjectItem(const std::string &field)
{
    if (cjson_ == nullptr) {
        return nullptr;
    }
    CjsonObject *item = new CjsonObject();
    auto cjson_item = cJSON_GetObjectItem(cjson_, field.c_str());
    if (cjson_item == nullptr) {
        return item;
    }
    item->cjson_ = cjson_item;
    return item;
}

CjsonObject* CjsonObject::GetArrayItem(const int index)
{
    if (cjson_ == nullptr) {
        return nullptr;
    }
    CjsonObject *item = new CjsonObject();
    auto cjson_item = cJSON_GetArrayItem(cjson_, index);
    if (cjson_item == nullptr) {
        return item;
    }
    item->cjson_ = cjson_item;
    return item;
}

CjsonObject* CjsonObject::GetNext()
{
    if (cjson_ == nullptr) {
        return nullptr;
    }
    CjsonObject *next = new CjsonObject();
    if (cjson_->next == nullptr) {
        return next;
    }
    next->cjson_ = cjson_->next;
    return next;
}

CjsonObject* CjsonObject::GetChild()
{
    if (cjson_ == nullptr) {
        return nullptr;
    }
    CjsonObject *child = new CjsonObject();
    if (cjson_->child == nullptr) {
        return child;
    }
    child->cjson_ = cjson_->child;
    return child;
}

int CjsonObject::DeleteItemFromObject(const std::string &field)
{
    if (field == "") {
        return E_OK;
    }
    cJSON_DeleteItemFromObject(cjson_, field.c_str());
    return E_OK;
}

ResultValue* CjsonObject::GetItemValue()
{
    if (cjson_ == nullptr) {
        return nullptr;
    }
    ResultValue* value = new ResultValue();
    switch (cjson_->type)
    {
        case cJSON_False:
            {
                value->value_type =  ResultValue::ValueType::VALUE_FALSE;
            }
            break;
        case cJSON_True:
            {
                value->value_type =  ResultValue::ValueType::VALUE_TRUE;
            }
            break;
        case cJSON_NULL:
            {
                value->value_type =  ResultValue::ValueType::VALUE_NULL;
            }
            break;
        case cJSON_Number:
            {
                value->value_type =  ResultValue::ValueType::VALUE_NUMBER;
                value->value_number = cjson_->valuedouble;
            }
            break;
        case cJSON_String:
            {
                value->value_type =  ResultValue::ValueType::VALUE_STRING;
                value->value_string = cjson_->valuestring;
            }
            break;
        case cJSON_Array:
            {
                value->value_type =  ResultValue::ValueType::VALUE_ARRAY;
                value->value_Object = GetChild();
            }
            break;
        case cJSON_Object:
            {
                value->value_type =  ResultValue::ValueType::VALUE_OBJECT;
                value->value_Object = GetChild();
            }
            break;
        default:
            break;
    }
    return value;
}

std::string CjsonObject::GetItemFiled()
{
    if (cjson_ == nullptr) {
        GLOGE("cjson is null");
        return "";
    }
    if (cjson_->string == nullptr) {
        GLOGE("cjson string is null");
        return "";
    }
    return cjson_->string;
}

int CjsonObject::Delete()
{
    if (cjson_ != nullptr) {
        cJSON_Delete(cjson_);
    }
    return E_OK;
}

bool CjsonObject::IsFieldExists(const JsonFieldPath &path)
{
    return true;
}

int CjsonObject::GetObjectByPath(const JsonFieldPath &path, ValueObject &obj)
{
    cJSON *item = cjson_;
    for (const auto &field : path) {
        item = cJSON_GetObjectItem(item, field.c_str());
        if (item == nullptr) {
            GLOGE("Invalid json field path. %s", field.c_str());
            return -E_INVALID_JSON_FIELT_PATH;
        }
        GLOGD("---->. %d", item->valueint);
    }

    if (item->type == cJSON_Number) {
        obj.valueType = ValueObject::ValueType::VALUE_NUMBER;
        obj.intValue = item->valueint;
    }

    return E_OK;
}
}