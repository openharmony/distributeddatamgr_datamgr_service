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
        cJSON *cjsonItem = cJSON_GetObjectItemCaseSensitive(cjson_, field.c_str());
    } else {
        cJSON *cjsonItem = cJSON_GetObjectItem(cjson_, field.c_str());
        item.cjson_ = cjsonItem;
    }
    return item;
}

JsonObject JsonObject::GetArrayItem(const int index)
{
    if (cjson_ == nullptr) {
        return JsonObject();
    }
    JsonObject item;
    auto cjsonItem = cJSON_GetArrayItem(cjson_, index);
    if (cjsonItem == nullptr) {
        return item;
    }
    item.cjson_ = cjsonItem;
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
                value.valueType =  ResultValue::ValueType::VALUE_FALSE;
            }
            break;
        case cJSON_True: 
            {
                value.valueType =  ResultValue::ValueType::VALUE_TRUE;
            }
            break;
        case cJSON_NULL: 
            {
                value.valueType =  ResultValue::ValueType::VALUE_NULL;
            }
            break;
        case cJSON_Number: 
            {
                value.valueType =  ResultValue::ValueType::VALUE_NUMBER;
                value.valueNumber = cjson_->valuedouble;
            }
            break;
        case cJSON_String: 
            {
                value.valueType =  ResultValue::ValueType::VALUE_STRING;
                value.valueString = cjson_->valuestring;
            }
            break;
        case cJSON_Array: 
            {
                value.valueType =  ResultValue::ValueType::VALUE_ARRAY;
                if (GetChild().IsNull() != true) {
                    auto item = GetChild();
                    value.valueObject = &item;
                }
            }
            break;
        case cJSON_Object: 
            {
                value.valueType =  ResultValue::ValueType::VALUE_OBJECT;
                if (GetChild().IsNull() != true) {
                    auto item = GetChild();
                    value.valueObject = &item;
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

JsonObject JsonObject::FindItem(const std::vector<std::string> jsonPath)
{
    cJSON *cJsondataItem = cjson_;
    if (jsonPath.size() != 0 && jsonPath[0] == "") {
        return JsonObject();
    }
    for (int i = 0; i < jsonPath.size(); i++) {
        if (cJsondataItem->type == cJSON_Object) {
            if (cJSON_GetObjectItem(cJsondataItem,jsonPath[i].c_str()) == nullptr) {
                return JsonObject();
            }
            cJSON *cJsondataTemp;
            cJsondataTemp = cJSON_GetObjectItem(cJsondataItem,jsonPath[i].c_str());
            cJsondataItem = cJsondataTemp;
        }
        if (cJsondataItem->type == cJSON_Array) {
            int nums = 0;
            for (int j = 0; j < jsonPath[i].size(); j++) {
                if (jsonPath[i][j]-'0' > 9 || jsonPath[i][j] - '0' < 0 || 
                    jsonPath[i][j] - '0' > cJSON_GetArraySize(cJsondataItem)) {
                    break;
                }
                auto GetArrayRet = cJSON_GetArrayItem(cJsondataItem, jsonPath[i][j]-'0');
                if (typeid(GetArrayRet) == typeid(cJSON*)) {
                    cJsondataItem = GetArrayRet;
                }
                else {
                    if ( i != jsonPath.size() - 1) {
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
    item.cjson_ = cJsondataItem;
    return item;
}

int JsonObject::DeleteItemOnTarget(std::vector<std::string> &path)
{
    cJSON *nodeFather;
    cJSON *cJsondataRoot;
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
            nodeFather = cjsonObj.cjson_;
            cJsondataRoot = cjsonObj.cjson_;
            path.emplace_back(lastString);
        }
        else {
            std::vector<std::string> fatherPath;
            for (int i = 0; i < path.size() - 1; i++) {
                fatherPath.emplace_back(path[i]);
            }
            auto foreObj = FindItem(fatherPath);
            if (foreObj.IsNull() == true) {
                return E_OK;
            }
            cJsondataRoot = foreObj.cjson_;
            auto cjsonObj = FindItem(path);     
            if (cjsonObj.IsNull() == true) {
                return E_OK;
            }  
            nodeFather = cjsonObj.cjson_;
            path.emplace_back(lastString);
        }
    }
    if (nodeFather->type == cJSON_Object) {
        if (cJSON_GetObjectItem(nodeFather, lastString.c_str()) != nullptr) {
            cJSON_DeleteItemFromObject(nodeFather, lastString.c_str());
        }
        else {
            GLOGE("no item that can be deleted");
        }
    }
    if (nodeFather->type == cJSON_Array) {
        if (cJSON_GetArrayItem(nodeFather, lastString[0] - '0') != nullptr) {
            cJSON_DeleteItemFromArray(nodeFather, lastString[0] - '0');
        }
        else {
            GLOGE("no item that can be deleted");
        }
    }
    return E_OK; 
}
}