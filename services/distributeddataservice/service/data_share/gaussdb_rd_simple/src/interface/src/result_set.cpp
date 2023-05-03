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

ResultSet::ResultSet()
{

}
ResultSet::~ResultSet()
{

}
int ResultSet::EraseCollection()
{
    if (store_ != nullptr) {
        store_->EraseCollection(collectionName_);
    }
    return E_OK;
}
int ResultSet::Init(DocumentStore *store, const std::string collectionName, ValueObject &key, std::vector<std::vector<std::string>> &path, bool ifShowId, bool viewType)
{
    store_ =  store;
    collectionName_ = collectionName;
    key_ = key;
    projectionPath_ = path;
    if (projectionTree_.ParseTree(path) == -E_INVALID_ARGS) {
        GLOGE("Parse ProjectionTree failed");
        return -E_INVALID_ARGS;
    }
    ifShowId_ =  ifShowId;
    viewType_ = viewType;
    findValue_.reserve(1 + 1);
    return E_OK;
}

int ResultSet::GetNext()
{
    index_++;
    if (index_ != 1) {
        if (findValue_.size() != 0) {
            findValue_.pop_back();
        }
        return -E_NO_DATA;
    }
    std::string idValue = key_.GetStringValue();
    if (idValue.empty()) {
        GLOGE("id is empty");
        return -E_NO_DATA;
    }
    Key key(idValue.begin(), idValue.end());
    Value document;
    int errCode = 0;
    auto coll = Collection(collectionName_, store_->GetExecutor(errCode));
    errCode = coll.GetDocument(key, document);
    if (errCode == -E_NOT_FOUND) {
        GLOGE("Cant get value from db");
        return -E_NO_DATA;
    }
    std::string jsonData(document.begin(), document.end());
    CutJsonBranch(jsonData);
    findValue_.emplace_back(jsonData);
    return E_OK;
}

int  ResultSet::GetValue(char **value)
{
    if (findValue_.size() == 0) {
        GLOGE("The value vector in resultSet is empty");
        return -E_NO_DATA;
    }
    auto jsonData =  findValue_.back();
    char *jsonstr = new char[jsonData.size() + 1];
    if (jsonstr == nullptr) {
        GLOGE("Memory allocation failed!" );
        return -E_FAILED_MEMORY_ALLOCATE;
    }
    errno_t err = strcpy_s(jsonstr, jsonData.size() + 1, jsonData.c_str());
    if (err != 0) {
        GLOGE("strcpy_s failed");
        delete[] jsonstr;
        return -E_NO_DATA;;
    }
    *value = jsonstr;
    return E_OK;
}

int ResultSet::CheckCutNode(JsonObject *node, std::vector<std::string> singlePath, std::vector<std::vector<std::string>> &allCutPath)
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
        GLOGE("jsonData Parsed faild");
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
        for (size_t i = 0; i < allCutPath.size(); i++) {
            if (!ifShowId_ || allCutPath[i][0] != KEY_ID) {
                cjsonObj.DeleteItemDeeplyOnTarget(allCutPath[i]);
            }
        }
    }
    if (!viewType_) {
        for (size_t i = 0; i < projectionPath_.size(); i++) {
            cjsonObj.DeleteItemDeeplyOnTarget(projectionPath_[i]);
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
