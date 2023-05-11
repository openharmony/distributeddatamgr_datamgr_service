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
#include "result_set.h"

#include "log_print.h"

namespace DocumentDB {
constexpr const char *KEY_ID = "_id";

ResultSet::ResultSet() {}
ResultSet::~ResultSet() {}
int ResultSet::EraseCollection()
{
    if (store_ != nullptr) {
        store_->EraseCollection(collectionName_);
    }
    return E_OK;
}
int ResultSet::Init(DocumentStore *store, const std::string collectionName, const std::string &filter,
    std::vector<std::vector<std::string>> &path, bool ifShowId, bool viewType, bool &isOnlyId)
{
    isOnlyId_ = isOnlyId;
    store_ = store;
    collectionName_ = collectionName;
    filter_ = filter;
    projectionPath_ = path;
    if (projectionTree_.ParseTree(path) == -E_INVALID_ARGS) {
        GLOGE("Parse ProjectionTree failed");
        return -E_INVALID_ARGS;
    }
    ifShowId_ = ifShowId;
    viewType_ = viewType;
    return E_OK;
}

int ResultSet::Init(DocumentStore *store, const std::string collectionName, const std::string &filter)
{
    ifFiled_ = true;
    store_ = store;
    collectionName_ = collectionName;
    filter_ = filter;
    return E_OK;
}

int ResultSet::GetNext()
{
    if (!ifFiled_ && index_ == 0) {
        if (isOnlyId_) {
            int errCode = 0;
            JsonObject filterObj = JsonObject::Parse(filter_, errCode, true, true);
            if (errCode != E_OK) {
                GLOGE("filter Parsed failed");
                return errCode;
            }
            auto filterObjChild = filterObj.GetChild();
            auto idValue = JsonCommon::GetValueByFiled(filterObjChild, KEY_ID);
            std::string idKey = idValue.GetStringValue();
            if (idKey.empty()) {
                GLOGE("id is empty");
                return -E_NO_DATA;
            }
            Key key(idKey.begin(), idKey.end());
            Value document;
            auto coll = Collection(collectionName_, store_->GetExecutor(errCode));
            errCode = coll.GetDocument(key, document);
            if (errCode == -E_NOT_FOUND) {
                GLOGE("Cant get value from db");
                return -E_NO_DATA;
            }
            std::string jsonData(document.begin(), document.end());
            CutJsonBranch(jsonData);
            std::vector<std::pair<std::string, std::string>> values;
            values.emplace_back(std::pair(idKey, jsonData));
            matchDatas_ = values;
        } else {
            int errCode = 0;
            auto coll = Collection(collectionName_, store_->GetExecutor(errCode));
            std::vector<std::pair<std::string, std::string>> values;
            JsonObject filterObj = JsonObject::Parse(filter_, errCode, true, true);
            if (errCode != E_OK) {
                GLOGE("filter Parsed failed");
                return errCode;
            }
            errCode = coll.GetFilededDocument(filterObj, values);
            if (errCode == -E_NOT_FOUND) {
                GLOGE("Cant get value from db");
                return -E_NO_DATA;
            }
            for (size_t i = 0; i < values.size(); i++) {
                CutJsonBranch(values[i].second);
            }
            matchDatas_ = values;
        }
    } else if (index_ == 0) {
        int errCode = 0;
        auto coll = Collection(collectionName_, store_->GetExecutor(errCode));
        std::vector<std::pair<std::string, std::string>> values;
        JsonObject filterObj = JsonObject::Parse(filter_, errCode, true, true);
        if (errCode != E_OK) {
            GLOGE("filter Parsed failed");
            return errCode;
        }
        errCode = coll.GetFilededDocument(filterObj, values);
        if (errCode == -E_NOT_FOUND) {
            GLOGE("Cant get value from db");
            return -E_NO_DATA;
        }
        matchDatas_ = values;
    }
    index_++;
    if (index_ > matchDatas_.size()) {
        GLOGE("No data in The value vector");
        return -E_NO_DATA;
    }
    return E_OK;
}

int ResultSet::GetValue(char **value)
{
    if (index_ == 0 || (index_ > matchDatas_.size())) {
        GLOGE("The value vector in resultSet is empty");
        return -E_NO_DATA;
    }
    auto jsonData = matchDatas_[index_ - 1].second;
    char *jsonstr = new char[jsonData.size() + 1];
    if (jsonstr == nullptr) {
        GLOGE("Memory allocation failed!");
        return -E_FAILED_MEMORY_ALLOCATE;
    }
    errno_t err = strcpy_s(jsonstr, jsonData.size() + 1, jsonData.c_str());
    if (err != 0) {
        GLOGE("strcpy_s failed");
        delete[] jsonstr;
        return -E_NO_DATA;
    }
    *value = jsonstr;
    return E_OK;
}

int ResultSet::GetKey(std::string &key)
{
    if (index_ == 0 || (index_ > matchDatas_.size())) {
        GLOGE("The value vector in resultSet is empty");
        return -E_NO_DATA;
    }
    key = matchDatas_[index_ - 1].first;
    return E_OK;
}

int ResultSet::CheckCutNode(JsonObject *node, std::vector<std::string> singlePath,
    std::vector<std::vector<std::string>> &allCutPath)
{
    if (node == nullptr) {
        GLOGE("No node to cut");
        return -E_NO_DATA;
    }
    singlePath.emplace_back(node->GetItemFiled());
    int index = 0;
    if (!projectionTree_.SearchTree(singlePath, index) && index == 0) {
        allCutPath.emplace_back(singlePath);
    }
    if (!node->GetChild().IsNull()) {
        auto nodeNew = node->GetChild();
        CheckCutNode(&nodeNew, singlePath, allCutPath);
    }
    if (!node->GetNext().IsNull()) {
        singlePath.pop_back();
        auto nodeNew = node->GetNext();
        CheckCutNode(&nodeNew, singlePath, allCutPath);
    }
    return E_OK;
}
int ResultSet::CutJsonBranch(std::string &jsonData)
{
    int errCode;
    JsonObject cjsonObj = JsonObject::Parse(jsonData, errCode, true);
    if (errCode != E_OK) {
        GLOGE("jsonData Parsed failed");
        return errCode;
    }
    std::vector<std::vector<std::string>> allCutPath;
    if (viewType_) {
        std::vector<std::string> singlePath;
        auto cjsonObjChild = cjsonObj.GetChild();
        errCode = CheckCutNode(&cjsonObjChild, singlePath, allCutPath);
        if (errCode != E_OK) {
            GLOGE("The node in CheckCutNode is nullptr");
            return errCode;
        }
        for (auto singleCutPaht : allCutPath) {
            if (!ifShowId_ || singleCutPaht[0] != KEY_ID) {
                cjsonObj.DeleteItemDeeplyOnTarget(singleCutPaht);
            }
        }
    }
    if (!viewType_) {
        for (auto singleCutPaht : projectionPath_) {
            cjsonObj.DeleteItemDeeplyOnTarget(singleCutPaht);
        }
        if (!ifShowId_) {
            std::vector<std::string> idPath;
            idPath.emplace_back(KEY_ID);
            cjsonObj.DeleteItemDeeplyOnTarget(idPath);
        }
    }
    jsonData = cjsonObj.Print();
    return E_OK;
}
} // namespace DocumentDB
