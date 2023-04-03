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
#include "log_print.h"
#include "grd_format_config.h"
#include "json_object.h"


namespace DocumentDB {

JsonObject::JsonObject()
{
    cjson_ = nullptr;
    isClone_ = true;
}

JsonObject::JsonObject(const JsonObject& newObj)
{
    cjson_ = newObj.cjson_;
    isClone_ = true;
}

JsonObject::~JsonObject()
{
    if (isClone_ == false) {
        cJSON_Delete(cjson_);
    }
}

bool JsonObject::IsNull()
{
    if (cjson_ == nullptr) {
        return true;
    }
    return false;
}

int JsonObject::GetDeep(cJSON *cjson, int deep, int &maxDeep)
{
    if (cjson->child == nullptr) {
        maxDeep = std::max(deep, maxDeep);
    } 
    if (cjson->child != nullptr) {
        GetDeep(cjson->child, deep + 1, maxDeep);
    }
    if (cjson->next != nullptr) {
        GetDeep(cjson->next, deep, maxDeep);
    } 
    return E_OK;
}

int JsonObject::Init(const std::string &str)
{
    if (str.length() + 1 > JSON_LENS_MAX) {
        return E_INVALID_ARGS;
    }
    const char *end = NULL;
    cjson_ = cJSON_ParseWithOpts(str.c_str(), &end, true);
    if (cjson_ == nullptr) {
        return E_INVALID_ARGS;
    }
    if (cjson_->type != cJSON_Object) {
        return E_INVALID_ARGS;
    }
    int deep = 0;
    GetDeep(cjson_, 0, deep);
    if (deep > JSON_DEEP_MAX) {
        return E_INVALID_ARGS;
    }
    isClone_ = false;
    return E_OK;
}

std::string JsonObject::Print()
{
    if (cjson_ == nullptr) {
        return "";
    }
    char *ret = cJSON_PrintUnformatted(cjson_);
    std::string str = ret;
    free(ret); 
    return str;
}

JsonObject JsonObject::GetObjectItem(const std::string &field, bool caseSensitive)
{
    if (cjson_ == nullptr) {
        return JsonObject();
    }
    JsonObject item;
    if (caseSensitive) {
        cJSON *cjson_item = cJSON_GetObjectItemCaseSensitive(cjson_, field.c_str());
    } else {
        cJSON *cjson_item = cJSON_GetObjectItem(cjson_, field.c_str());
        item.cjson_ = cjson_item;
    }
    return item;
}

JsonObject JsonObject::GetArrayItem(const int index)
{
    if (cjson_ == nullptr) {
        return JsonObject();
    }
    JsonObject item;
    auto cjson_item = cJSON_GetArrayItem(cjson_, index);
    if (cjson_item == nullptr) {
        return item;
    }
    item.cjson_ = cjson_item;
    return item;
}

JsonObject JsonObject::GetNext()
{
    if (cjson_ == nullptr) {
        return JsonObject();
    }
    JsonObject next;
    if (cjson_->next == nullptr) {
        return JsonObject();
    }
    next.cjson_ = cjson_->next;
    return next;
}
    
JsonObject JsonObject::GetChild()
{
    if (cjson_ == nullptr) {
        return JsonObject();
    }
    JsonObject child;
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

ResultValue JsonObject::GetItemValue()
{
    if (cjson_ == nullptr) {
        return ResultValue();
    }
    ResultValue value;
    switch (cjson_->type) {
        case cJSON_False: {
                value.value_type =  ResultValue::ValueType::VALUE_FALSE;
            }
            break;
        case cJSON_True: 
            {
                value.value_type =  ResultValue::ValueType::VALUE_TRUE;
            }
            break;
        case cJSON_NULL: 
            {
                value.value_type =  ResultValue::ValueType::VALUE_NULL;
            }
            break;
        case cJSON_Number: 
            {
                value.value_type =  ResultValue::ValueType::VALUE_NUMBER;
                value.value_number = cjson_->valuedouble;
            }
            break;
        case cJSON_String: 
            {
                value.value_type =  ResultValue::ValueType::VALUE_STRING;
                value.value_string = cjson_->valuestring;
            }
            break;
        case cJSON_Array: 
            {
                value.value_type =  ResultValue::ValueType::VALUE_ARRAY;
                if (GetChild().IsNull() != true) {
                    auto item = GetChild();
                    value.value_Object = &item;
                }
            }
            break;
        case cJSON_Object: 
            {
                value.value_type =  ResultValue::ValueType::VALUE_OBJECT;
                if (GetChild().IsNull() != true) {
                    auto item = GetChild();
                    value.value_Object = &item;
                }
            }
            break;
        default:
            break;
    }

    return value;
}

std::string JsonObject::GetItemFiled()
{
    if (cjson_ == nullptr) {
        return "";
    }
    if (cjson_->string == nullptr) {
        return "";
    }
    return cjson_->string;
}

JsonObject JsonObject::FindItem(const std::vector<std::string> json_path)
{
    cJSON *cJsondata_item = cjson_;
    cJSON *cJsondata_temp;
    if (json_path.size() != 0 && json_path[0] == "") {
        return JsonObject();
    }
    for (int i = 0; i < json_path.size(); i++) {
        if (cJsondata_item->type == cJSON_Object) {
            if (cJSON_GetObjectItem(cJsondata_item,json_path[i].c_str()) == nullptr) {
                return JsonObject();
            }
            cJsondata_temp = cJSON_GetObjectItem(cJsondata_item,json_path[i].c_str());
            cJsondata_item = cJsondata_temp;
        }
        if (cJsondata_item->type == cJSON_Array) {
            int nums = 0;
            for (int j = 0; j < json_path[i].size(); j++) {
                if (json_path[i][j]-'0' > 9 || json_path[i][j] - '0' < 0 || 
                    json_path[i][j] - '0' > cJSON_GetArraySize(cJsondata_item)) {
                    break;
                }
                auto GetArrayret = cJSON_GetArrayItem(cJsondata_item, json_path[i][j]-'0');
                if (typeid(GetArrayret) == typeid(cJSON*)) {
                    cJsondata_item = GetArrayret;
                }
                else {
                    if ( i != json_path.size() - 1) {
                        GLOGD("[cjson_object]=====>Patharg wrong");
                    }
                    else {
                        return JsonObject();
                    }
                }
            }
        }
        
    }
    JsonObject item;
    item.cjson_ = cJsondata_item;
    return item;
}

int JsonObject::DeleteItemOnTarget(std::vector<std::string> &path)
{
    cJSON *node_father;
    cJSON *cJsondata_root;
    std::string lastString;
    if (path.size()>0) {
        lastString = path.back();
        path.pop_back();
        if (path.size() == 0) {            
            std::vector<std::string> emptyPath;
            auto cjsonObj = FindItem(emptyPath);
            if  (cjsonObj.IsNull() == true) {
                return E_OK;
            }
            node_father = cjsonObj.cjson_;
            cJsondata_root = cjsonObj.cjson_;
            path.emplace_back(lastString);
        }
        else {
            std::vector<std::string> forePath;
            for (int i = 0; i < path.size() - 1; i++) {
                forePath.emplace_back(path[i]);
            }
            auto foreObj = FindItem(forePath);
            if (foreObj.IsNull() == true) {
                return E_OK;
            }
            cJsondata_root = foreObj.cjson_;
            auto cjsonObj = FindItem(path);     
            if (cjsonObj.IsNull() == true) {
                return E_OK;
            }  
            node_father = cjsonObj.cjson_;
            path.emplace_back(lastString);
        }
    }
    if (node_father->type == cJSON_Object) {
        if (cJSON_GetObjectItem(node_father, lastString.c_str()) != nullptr) {
            cJSON_DeleteItemFromObject(node_father, lastString.c_str());
        }
        else {
            GLOGE("no item that can be deleted");
        }
    }
    if (node_father->type == cJSON_Array) {
        if (cJSON_GetArrayItem(node_father, lastString[0] - '0') != nullptr) {
            cJSON_DeleteItemFromArray(node_father, lastString[0] - '0');
        }
        else {
            GLOGE("no item that can be deleted");
        }
    }
    return E_OK; 
}
}