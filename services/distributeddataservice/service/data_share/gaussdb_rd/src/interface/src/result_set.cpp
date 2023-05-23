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
#include "document_key.h"
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
int ResultSet::Init(std::shared_ptr<QueryContext> &context, DocumentStore *store, bool isCutBranch)
{
    isCutBranch_ = isCutBranch;
    context_ = context;
    store_ = store;
    return E_OK;
}

int ResultSet::GetNextWithField()
{
    int errCode = E_OK;
    JsonObject filterObj = JsonObject::Parse(context_->filter, errCode, true, true);
    if (errCode != E_OK) {
        GLOGE("filter Parsed failed");
        return errCode;
    }
    Key key;
    if (context_->isIdExist) { // get id from filter or from previous data.
        JsonObject filterObjChild = filterObj.GetChild();
        ValueObject idValue = JsonCommon::GetValueInSameLevel(filterObjChild, KEY_ID);
        std::string idKey = idValue.GetStringValue();
        key.assign(idKey.begin(), idKey.end());
    } else {
        key.assign(lastKeyIndex_.begin(), lastKeyIndex_.end());
    }
    matchData_.first.clear(); // Delete previous data.
    matchData_.second.clear();
    std::pair<std::string, std::string> value;
    Collection coll = store_->GetCollection(context_->collectionName);
    errCode = coll.GetMatchedDocument(filterObj, key, value, context_->isIdExist);
    if (errCode == -E_NOT_FOUND) {
        return -E_NO_DATA;
    }
    std::string jsonData(value.second.begin(), value.second.end());
    std::string jsonkey(value.first.begin(), value.first.end());
    lastKeyIndex_ = jsonkey;
    if (isCutBranch_) {
        errCode = CutJsonBranch(jsonkey, jsonData);
        if (errCode != E_OK) {
            GLOGE("cut branch faild");
            return errCode;
        }
    }
    matchData_ = std::make_pair(jsonkey, jsonData);
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
    errCode = GetNextWithField();
    if (errCode != E_OK) {
        return errCode;
    }
    if (matchData_.second.empty()) {
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
        GLOGE("Start transaction faild");
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
    if (matchData_.first.empty()) {
        GLOGE("The value vector in resultSet is empty");
        return -E_NO_DATA;
    }
    std::string jsonData = matchData_.second;
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

int ResultSet::GetValue(std::string &value)
{
    if (matchData_.first.empty()) {
        GLOGE("The value vector in resultSet is empty");
        return -E_NO_DATA;
    }
    value = matchData_.second;
    return E_OK;
}

int ResultSet::GetKey(std::string &key)
{
    key = matchData_.first;
    if (key.empty()) {
        GLOGE("can not get data, because it is empty");
        return -E_NO_DATA;
    }
    return E_OK;
}

int ResultSet::CheckCutNode(JsonObject *node, std::vector<std::string> singlePath,
    std::vector<std::vector<std::string>> &allCutPath)
{
    if (node == nullptr) {
        GLOGE("No node to cut");
        return -E_NO_DATA;
    }
    singlePath.emplace_back(node->GetItemField());
    int index = 0;
    if (!context_->projectionTree.SearchTree(singlePath, index) && index == 0) {
        allCutPath.emplace_back(singlePath);
    }
    if (!node->GetChild().IsNull()) {
        JsonObject nodeNew = node->GetChild();
        CheckCutNode(&nodeNew, singlePath, allCutPath);
    }
    if (!node->GetNext().IsNull()) {
        singlePath.pop_back();
        JsonObject nodeNew = node->GetNext();
        CheckCutNode(&nodeNew, singlePath, allCutPath);
    }
    return E_OK;
}

JsonObject CreatIdObj(const std::string &idStr, int errCode)
{
    JsonObject idObj = JsonObject::Parse("{}", errCode, true); // cant be faild.
    idObj.AddItemToObject("_id");
    ValueObject idValue = ValueObject(idStr.c_str());
    idObj = idObj.GetObjectItem("_id", errCode); // cant be faild.
    (void)idObj.SetItemValue(idValue);
    return idObj;
}

int InsertRandomId(JsonObject &cjsonObj, std::string &jsonKey)
{
    std::string idStr = DocumentKey::GetIdFromKey(jsonKey);
    if (idStr.empty()) {
        GLOGE("Genalral Id faild");
        return -E_INNER_ERROR;
    }
    int errCode = E_OK;
    JsonObject idObj = CreatIdObj(idStr, errCode);
    if (errCode != E_OK) {
        GLOGE("CreatIdObj faild");
        return errCode;
    }
    cjsonObj.InsertItemObject(0, idObj);
    return E_OK;
}

int ResultSet::CutJsonBranch(std::string &jsonKey, std::string &jsonData)
{
    int errCode;
    JsonObject cjsonObj = JsonObject::Parse(jsonData, errCode, true);
    if (errCode != E_OK) {
        GLOGE("jsonData Parsed failed");
        return errCode;
    }
    bool isIdExistInValue = true; // if id exsit in the value string that get from db.
    bool isInsertIdflag = false;
    isIdExistInValue = cjsonObj.GetObjectItem("_id", errCode).IsNull() ? false : true;
    if (context_->viewType) {
        std::vector<std::vector<std::string>> allCutPath;
        std::vector<std::string> singlePath;
        JsonObject cjsonObjChild = cjsonObj.GetChild();
        errCode = CheckCutNode(&cjsonObjChild, singlePath, allCutPath);
        if (errCode != E_OK) {
            GLOGE("The node in CheckCutNode is nullptr");
            return errCode;
        }
        for (const auto &singleCutPaht : allCutPath) {
            if (!context_->ifShowId || singleCutPaht[0] != KEY_ID) {
                cjsonObj.DeleteItemDeeplyOnTarget(singleCutPaht);
            }
            if (singleCutPaht[0] == KEY_ID && !isIdExistInValue) { // projection has Id, and its showType is true.
                isInsertIdflag = true;
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
    if (context_->ifShowId && !isIdExistInValue) {
        isInsertIdflag = true; // ifShowId is true,and then the data taken out does not have IDs, insert id.
    }
    if (isInsertIdflag) {
        errCode = InsertRandomId(cjsonObj, jsonKey);
        if (errCode != E_OK) {
            return errCode;
        }
    }
    jsonData = cjsonObj.Print();
    return E_OK;
}
} // namespace DocumentDB
