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

#include "db_constant.h"
#include "log_print.h"
#include "securec.h"

namespace DocumentDB {
constexpr const char *KEY_ID = "_id";

ResultSet::ResultSet() {}
ResultSet::~ResultSet()
{
    context_ = nullptr;
}
int ResultSet::EraseCollection()
{
    if (store_ != nullptr) {
        store_->EraseCollection(context_->collectionName);
    }
    return E_OK;
}
int ResultSet::Init(std::shared_ptr<QueryContext> &context, DocumentStore *store, bool ifField)
{
    ifField_ = ifField;
    context_ = context;
    store_ = store;
    return E_OK;
}

int ResultSet::GetNextWithField()
{
    int errCode = E_OK;
    if (context_->isOnlyId) {
        JsonObject filterObj = JsonObject::Parse(context_->filter, errCode, true, true);
        if (errCode != E_OK) {
            GLOGE("filter Parsed failed");
            return errCode;
        }
        JsonObject filterObjChild = filterObj.GetChild();
        ValueObject idValue = JsonCommon::GetValueInSameLevel(filterObjChild, KEY_ID);
        std::string idKey = idValue.GetStringValue();
        if (idKey.empty()) {
            return -E_NO_DATA;
        }
        Key key(idKey.begin(), idKey.end());
        Value document;
        Collection coll = store_->GetCollection(context_->collectionName);
        errCode = coll.GetDocument(key, document);
        if (errCode == -E_NOT_FOUND) {
            return -E_NO_DATA;
        }
        std::string jsonData(document.begin(), document.end());
        CutJsonBranch(jsonData);
        std::vector<std::pair<std::string, std::string>> values;
        values.emplace_back(std::make_pair(idKey, jsonData));
        matchDatas_ = values;
    } else {
        Collection coll = store_->GetCollection(context_->collectionName);
        std::vector<std::pair<std::string, std::string>> values;
        JsonObject filterObj = JsonObject::Parse(context_->filter, errCode, true, true);
        if (errCode != E_OK) {
            GLOGE("filter Parsed failed");
            return errCode;
        }
        errCode = coll.GetMatchedDocument(filterObj, values);
        if (errCode == -E_NOT_FOUND) {
            return -E_NO_DATA;
        }
        for (size_t i = 0; i < values.size(); i++) {
            CutJsonBranch(values[i].second);
        }
        matchDatas_ = values;
    }
    return E_OK;
}

int ResultSet::GetNextInner(bool isNeedCheckTable)
{
    int errCode = E_OK;
    if (isNeedCheckTable) {
        std::string lowerCaseName = context_->collectionName;
        std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        bool isCollectionExist = store_->IsCollectionExists(DBConstant::COLL_PREFIX + lowerCaseName, errCode);
        if (errCode != E_OK) {
            return errCode;
        }
        if (!isCollectionExist) {
            return -E_INVALID_ARGS;
        }
    }
    if (!ifField_ && index_ == 0) {
        errCode = GetNextWithField();
        if (errCode != E_OK) {
            return errCode;
        }
    } else if (index_ == 0) {
        Collection coll = store_->GetCollection(context_->collectionName);
        std::vector<std::pair<std::string, std::string>> values;
        JsonObject filterObj = JsonObject::Parse(context_->filter, errCode, true, true);
        if (errCode != E_OK) {
            GLOGE("filter Parsed failed");
            return errCode;
        }
        errCode = coll.GetMatchedDocument(filterObj, values);
        if (errCode == -E_NOT_FOUND) {
            return -E_NO_DATA;
        }
        matchDatas_ = values;
    }
    index_++;
    if (index_ > matchDatas_.size()) {
        return -E_NO_DATA;
    }
    return E_OK;
}

int ResultSet::GetNext(bool isNeedTransaction, bool isNeedCheckTable)
{
    int errCode = E_OK;
    if (!isNeedTransaction) {
        return GetNextInner(isNeedCheckTable);
    }
    std::lock_guard<std::mutex> lock(store_->dbMutex_);
    errCode = store_->StartTransaction();
    if (errCode != E_OK) {
        return errCode;
    }
    errCode = GetNextInner(isNeedCheckTable);
    if (errCode == E_OK || errCode == -E_NO_DATA) {
        store_->Commit();
    } else {
        store_->Rollback();
    }
    return errCode;
}

int ResultSet::GetValue(char **value)
{
    std::lock_guard<std::mutex> lock(store_->dbMutex_);
    if (index_ == 0 || (index_ > matchDatas_.size())) {
        GLOGE("The value vector in resultSet is empty");
        return -E_NO_DATA;
    }
    std::string jsonData = matchDatas_[index_ - 1].second;
    char *jsonstr = new char[jsonData.size() + 1];
    if (jsonstr == nullptr) {
        GLOGE("Memory allocation failed!");
        return -E_FAILED_MEMORY_ALLOCATE;
    }
    int err = strcpy_s(jsonstr, jsonData.size() + 1, jsonData.c_str());
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
    JsonObject nodeInstance = *node;
    while (!nodeInstance.IsNull()) {
        singlePath.emplace_back(nodeInstance.GetItemField());
        size_t index = 0;
        if ((nodeInstance.GetType() == JsonObject::Type::JSON_ARRAY &&
                context_->projectionTree.SearchTree(singlePath, index) && index == 0) ||
            (!context_->projectionTree.SearchTree(singlePath, index) && index == 0)) {
            allCutPath.emplace_back(singlePath);
        }
        if (nodeInstance.GetType() != JsonObject::Type::JSON_ARRAY && !nodeInstance.GetChild().IsNull()) {
            JsonObject nodeChiled = nodeInstance.GetChild();
            CheckCutNode(&nodeChiled, singlePath, allCutPath);
        }
        singlePath.pop_back();
        nodeInstance = nodeInstance.GetNext();
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
    if (context_->viewType) {
        std::vector<std::string> singlePath;
        JsonObject cjsonObjChild = cjsonObj.GetChild();
        std::vector<std::vector<std::string>> allCutPath;
        errCode = CheckCutNode(&cjsonObjChild, singlePath, allCutPath);
        if (errCode != E_OK) {
            GLOGE("The node in CheckCutNode is nullptr");
            return errCode;
        }
        for (const auto &singleCutPaht : allCutPath) {
            if (!context_->ifShowId || singleCutPaht[0] != KEY_ID) {
                cjsonObj.DeleteItemDeeplyOnTarget(singleCutPaht);
            }
        }
    }
    if (!context_->viewType) {
        for (const auto &singleCutPaht : context_->projectionPath) {
            cjsonObj.DeleteItemDeeplyOnTarget(singleCutPaht);
        }
        if (!context_->ifShowId) {
            std::vector<std::string> idPath;
            idPath.emplace_back(KEY_ID);
            cjsonObj.DeleteItemDeeplyOnTarget(idPath);
        }
    }
    jsonData = cjsonObj.Print();
    return E_OK;
}
} // namespace DocumentDB
