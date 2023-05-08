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

#include "document_store.h"

#include "collection_option.h"
#include "doc_errno.h"
#include "document_check.h"
#include "grd_base/grd_type_export.h"
#include "grd_resultset_inner.h"
#include "log_print.h"
#include "result_set_common.h"

namespace DocumentDB {
const int COLLECTION_LENS_MAX = 512 * 1024;
const int JSON_LENS_MAX = 1024 * 1024;
const int JSON_DEEP_MAX = 4;
constexpr const char *KEY_ID = "_id";
const bool caseSensitive = true;

DocumentStore::DocumentStore(KvStoreExecutor *executor) : executor_(executor) {}

DocumentStore::~DocumentStore()
{
    delete executor_;
}

int DocumentStore::CreateCollection(const std::string &name, const std::string &option, int flags)
{
    std::string lowerCaseName;
    int errCode = E_OK;
    if (!CheckCommon::CheckCollectionName(name, lowerCaseName, errCode)) {
        GLOGE("Check collection name invalid. %d", errCode);
        return errCode;
    }

    errCode = E_OK;
    CollectionOption collOption = CollectionOption::ReadOption(option, errCode);
    if (errCode != E_OK) {
        GLOGE("Read collection option str failed. %d", errCode);
        return errCode;
    }

    if (flags != 0 && flags != CHK_EXIST_COLLECTION) {
        GLOGE("Check flags invalid.");
        return -E_INVALID_ARGS;
    }

    std::lock_guard<std::mutex> lock(dbMutex_);
    bool ignoreExists = (flags != CHK_EXIST_COLLECTION);
    errCode = executor_->CreateCollection(lowerCaseName, ignoreExists);
    if (errCode != E_OK) {
        GLOGE("Create collection failed. %d", errCode);
        return errCode;
    }
    std::string oriOptStr;
    errCode = executor_->GetCollectionOption(lowerCaseName, oriOptStr);
    if (errCode == -E_NOT_FOUND) {
        executor_->SetCollectionOption(lowerCaseName, collOption.ToString());
        errCode = E_OK;
    } else {
        CollectionOption oriOption = CollectionOption::ReadOption(oriOptStr, errCode);
        if (collOption != oriOption) {
            GLOGE("Create collection failed, option changed.");
            return -E_INVALID_CONFIG_VALUE;
        }
    }

    return errCode;
}

int DocumentStore::DropCollection(const std::string &name, int flags)
{
    std::string lowerCaseName;
    int errCode = E_OK;
    if (!CheckCommon::CheckCollectionName(name, lowerCaseName, errCode)) {
        GLOGE("Check collection name invalid. %d", errCode);
        return errCode;
    }

    if (flags != 0 && flags != CHK_NON_EXIST_COLLECTION) {
        GLOGE("Check flags invalid.");
        return -E_INVALID_ARGS;
    }

    bool ignoreNonExists = (flags != CHK_NON_EXIST_COLLECTION);
    std::lock_guard<std::mutex> lock(dbMutex_);
    errCode = executor_->DropCollection(lowerCaseName, ignoreNonExists);
    if (errCode != E_OK) {
        GLOGE("Drop collection failed. %d", errCode);
        return errCode;
    }

    errCode = executor_->CleanCollectionOption(lowerCaseName);
    if (errCode != E_OK && errCode != -E_NO_DATA) {
        GLOGE("Clean collection option failed. %d", errCode);
        return errCode;
    }

    return E_OK;
}

int DocumentStore::UpdateDocument(const std::string &collection, const std::string &filter, const std::string &update,
    int flags)
{
    std::string lowerCaseCollName;
    int errCode = E_OK;
    if (!CheckCommon::CheckCollectionName(collection, lowerCaseCollName, errCode)) {
        GLOGE("Check collection name invalid. %d", errCode);
        return errCode;
    }
    JsonObject updateObj = JsonObject::Parse(update, errCode, true);
    if (errCode != E_OK) {
        GLOGE("update Parsed faild");
        return errCode;
    }
    std::vector<std::vector<std::string>> allPath;
    if (update != "{}") {
        allPath = JsonCommon::ParsePath(updateObj, errCode);
        if (errCode != E_OK) {
            GLOGE("updateObj ParsePath faild");
            return errCode;
        }
        if (!CheckCommon::CheckUpdata(updateObj, allPath)) {
            GLOGE("Updata format unvalid");
            return -E_INVALID_ARGS;
        }
    }
    if (flags != GRD_DOC_APPEND && flags != GRD_DOC_REPLACE) {
        GLOGE("Check flags invalid.");
        return -E_INVALID_ARGS;
    }
    JsonObject filterObj = JsonObject::Parse(filter, errCode, true);
    if (errCode != E_OK) {
        GLOGE("filter Parsed faild");
        return errCode;
    }
    std::vector<std::vector<std::string>> filterAllPath;
    filterAllPath = JsonCommon::ParsePath(filterObj, errCode);
    if (errCode != E_OK) {
        GLOGE("filter ParsePath faild");
        return errCode;
    }
    bool isOnlyId = true;
    auto coll = Collection(collection, executor_);
    errCode = CheckCommon::CheckFilter(filterObj, isOnlyId, filterAllPath);
    if (errCode != E_OK) {
        return errCode;
    }
    bool isReplace = ((flags & GRD_DOC_REPLACE) == GRD_DOC_REPLACE);
    if (isOnlyId) {
        auto filterObjChild = filterObj.GetChild();
        auto idValue = JsonCommon::GetValueByFiled(filterObjChild, KEY_ID);
        std::string docId = idValue.GetStringValue();
        std::lock_guard<std::mutex> lock(dbMutex_);
        errCode = coll.UpdateDocument(docId, update, isReplace);
        if (errCode == E_OK) {
            errCode = 1; // upsert one record.
        } else if (errCode == -E_NOT_FOUND) {
            errCode = E_OK;
        }
        return errCode;
    }
    ResultSet resultSet;
    InitResultSet(this, collection, filter, resultSet);
    std::lock_guard<std::mutex> lock(dbMutex_);
    errCode = resultSet.GetNext();
    if (errCode == -E_NO_DATA) {
        return 0; // The amount of text updated
    } else if (errCode != E_OK) {
        return errCode;
    }
    std::string docId;
    resultSet.GetKey(docId);
    errCode = coll.UpdateDocument(docId, update, isReplace);
    if (errCode == E_OK) {
        errCode = 1; // update one record.
    } else if (errCode == -E_NOT_FOUND) {
        errCode = E_OK;
    }
    return errCode;
}

int DocumentStore::UpsertDocument(const std::string &collection, const std::string &filter,
    const std::string &document, int flags)
{
    std::string lowerCaseCollName;
    int errCode = E_OK;
    if (!CheckCommon::CheckCollectionName(collection, lowerCaseCollName, errCode)) {
        GLOGE("Check collection name invalid. %d", errCode);
        return errCode;
    }
    JsonObject documentObj = JsonObject::Parse(document, errCode, true);
    if (errCode != E_OK) {
        GLOGE("document Parsed faild");
        return errCode;
    }
    std::vector<std::vector<std::string>> allPath;
    if (document != "{}") {
        allPath = JsonCommon::ParsePath(documentObj, errCode);
        if (errCode != E_OK) {
            return errCode;
        }
        if (!CheckCommon::CheckUpdata(documentObj, allPath)) {
            GLOGE("updata format unvalid");
            return -E_INVALID_ARGS;
        }
    }
    if (flags != GRD_DOC_APPEND && flags != GRD_DOC_REPLACE) {
        GLOGE("Check flags invalid.");
        return -E_INVALID_ARGS;
    }
    JsonObject filterObj = JsonObject::Parse(filter, errCode, true);
    if (errCode != E_OK) {
        GLOGE("filter Parsed faild");
        return errCode;
    }
    std::vector<std::vector<std::string>> filterAllPath;
    filterAllPath = JsonCommon::ParsePath(filterObj, errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    bool isOnlyId = true;
    bool isReplace = ((flags & GRD_DOC_REPLACE) == GRD_DOC_REPLACE);
    auto coll = Collection(collection, executor_);
    errCode = CheckCommon::CheckFilter(filterObj, isOnlyId, filterAllPath);
    if (errCode != E_OK) {
        return errCode;
    }
    if (isOnlyId) {
        std::lock_guard<std::mutex> lock(dbMutex_);
        auto filterObjChild = filterObj.GetChild();
        ValueObject idValue = JsonCommon::GetValueByFiled(filterObjChild, KEY_ID);
        std::string docId = idValue.GetStringValue();
        JsonObject idObj = filterObj.GetObjectItem(KEY_ID, errCode);
        documentObj.InsertItemObject(0, idObj);
        std::string addedIdDocument = documentObj.Print();
        errCode = coll.UpsertDocument(docId, addedIdDocument, isReplace);
        if (errCode == E_OK) {
            errCode = 1; // upsert one record.
        }
        return errCode;
    }
    bool isIdExist;
    auto filterObjChild = filterObj.GetChild();
    auto idValue = JsonCommon::GetValueByFiled(filterObjChild, KEY_ID, isIdExist);
    if (!isIdExist) {
        return -E_INVALID_ARGS;
    }
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::string docId = idValue.GetStringValue();
    JsonObject idObj = filterObj.GetObjectItem(KEY_ID, errCode);
    documentObj.InsertItemObject(0, idObj);
    std::string addedIdDocument = documentObj.Print();
    errCode = coll.UpsertDocument(docId, addedIdDocument, isReplace);
    if (errCode == E_OK) {
        errCode = 1; // upsert one record.
    } else if (errCode == -E_NOT_FOUND) {
        errCode = E_OK;
    }
    return errCode;
}

int DocumentStore::InsertDocument(const std::string &collection, const std::string &document, int flag)
{
    if (flag != 0) {
        GLOGE("InsertDocument flag is not zero");
        return -E_INVALID_ARGS;
    }
    std::string lowerCaseCollName;
    int errCode = E_OK;
    if (!CheckCommon::CheckCollectionName(collection, lowerCaseCollName, errCode)) {
        GLOGE("Check collection name invalid. %d", errCode);
        return errCode;
    }
    auto coll = Collection(collection, executor_);
    if (document.length() + 1 > JSON_LENS_MAX) {
        GLOGE("document's length is too long");
        return -E_OVER_LIMIT;
    }
    JsonObject documentObj = JsonObject::Parse(document, errCode, true);
    if (errCode != E_OK) {
        GLOGE("Document Parsed faild");
        return errCode;
    }
    errCode = CheckCommon::CheckDocument(documentObj);
    if (errCode != E_OK) {
        return errCode;
    }
    auto documentObjChild = documentObj.GetChild();
    auto idValue = JsonCommon::GetValueByFiled(documentObjChild, KEY_ID);
    std::string id = idValue.GetStringValue();
    Key key(id.begin(), id.end());
    Value value(document.begin(), document.end());
    std::lock_guard<std::mutex> lock(dbMutex_);
    return coll.PutDocument(key, value);
}

int DocumentStore::DeleteDocument(const std::string &collection, const std::string &filter, int flag)
{
    if (flag != 0) {
        GLOGE("DeleteDocument flag is not zero");
        return -E_INVALID_ARGS;
    }
    std::string lowerCaseCollName;
    int errCode = E_OK;
    if (!CheckCommon::CheckCollectionName(collection, lowerCaseCollName, errCode)) {
        GLOGE("Check collection name invalid. %d", errCode);
        return errCode;
    }
    auto coll = Collection(collection, executor_);
    if (!coll.IsCollectionExists(errCode)) {
        return -E_INVALID_ARGS;
    }
    if (filter.empty()) {
        GLOGE("Filter is empty");
        return -E_INVALID_ARGS;
    }
    if (filter.length() + 1 > JSON_LENS_MAX) {
        GLOGE("filter's length is too long");
        return -E_OVER_LIMIT;
    }
    JsonObject filterObj = JsonObject::Parse(filter, errCode, true);
    if (errCode != E_OK) {
        GLOGE("filter Parsed faild");
        return errCode;
    }
    std::vector<std::vector<std::string>> filterAllPath;
    filterAllPath = JsonCommon::ParsePath(filterObj, errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    bool isOnlyId = true;
    errCode = CheckCommon::CheckFilter(filterObj, isOnlyId, filterAllPath);
    if (errCode != E_OK) {
        return errCode;
    }
    if (isOnlyId) {
        auto filterObjChild = filterObj.GetChild();
        auto idValue = JsonCommon::GetValueByFiled(filterObjChild, KEY_ID);
        std::string id = idValue.GetStringValue();
        Key key(id.begin(), id.end());
        std::lock_guard<std::mutex> lock(dbMutex_);
        return coll.DeleteDocument(key);
    }
    ResultSet resultSet;
    InitResultSet(this, collection, filter, resultSet);
    std::lock_guard<std::mutex> lock(dbMutex_);
    errCode = resultSet.GetNext();
    if (errCode != E_OK) {
        return errCode;
    }
    std::string id;
    resultSet.GetKey(id);
    Key key(id.begin(), id.end());
    return coll.DeleteDocument(key);
}
KvStoreExecutor *DocumentStore::GetExecutor(int errCode)
{
    return executor_;
}
int DocumentStore::FindDocument(const std::string &collection, const std::string &filter,
    const std::string &projection, int flags, GRD_ResultSet *grdResultSet)
{
    if (flags != 0 && flags != GRD_DOC_ID_DISPLAY) {
        GLOGE("FindDocument flag is illegal");
        return -E_INVALID_ARGS;
    }
    std::string lowerCaseCollName;
    int errCode = E_OK;
    if (!CheckCommon::CheckCollectionName(collection, lowerCaseCollName, errCode)) {
        GLOGE("Check collection name invalid. %d", errCode);
        return errCode;
    }
    if (filter.length() + 1 > JSON_LENS_MAX) {
        GLOGE("filter's length is too long");
        return -E_OVER_LIMIT;
    }
    JsonObject filterObj = JsonObject::Parse(filter, errCode, true);
    if (errCode != E_OK) {
        GLOGE("filter Parsed faild");
        return errCode;
    }
    std::vector<std::vector<std::string>> filterAllPath;
    filterAllPath = JsonCommon::ParsePath(filterObj, errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    bool isOnlyId = true;
    errCode = CheckCommon::CheckFilter(filterObj, isOnlyId, filterAllPath);
    if (errCode != E_OK) {
        return errCode;
    }
    if (projection.length() + 1 > JSON_LENS_MAX) {
        GLOGE("projection's length is too long");
        return -E_OVER_LIMIT;
    }
    JsonObject projectionObj = JsonObject::Parse(projection, errCode, true);
    if (errCode != E_OK) {
        GLOGE("projection Parsed faild");
        return errCode;
    }
    bool viewType = false;
    std::vector<std::vector<std::string>> allPath;
    if (projection != "{}") {
        allPath = JsonCommon::ParsePath(projectionObj, errCode);
        if (errCode != E_OK) {
            return errCode;
        }
        if (!CheckCommon::CheckProjection(projectionObj, allPath)) {
            GLOGE("projection format unvalid");
            return -E_INVALID_ARGS;
        }
        if (GetViewType(projectionObj, viewType) != E_OK) {
            GLOGE("GetViewType faild");
            return -E_INVALID_ARGS;
        }
    }
    bool ifShowId = false;
    if (flags == GRD_DOC_ID_DISPLAY) {
        ifShowId = true;
    }
    auto coll = Collection(collection, executor_);
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!coll.FindDocument()) {
        GLOGE("no corresponding table name");
        return -E_INVALID_ARGS;
    }
    int ret = InitResultSet(this, collection, filter, allPath, ifShowId, viewType, grdResultSet->resultSet_, isOnlyId);
    if (ret == E_OK) {
        collections_[collection] = nullptr;
    }
    if (ret != E_OK) {
        collections_.erase(collection);
    }
    return ret;
}

bool DocumentStore::IsCollectionOpening(const std::string collection)
{
    if (collections_.find(collection) != collections_.end()) {
        GLOGE("DB is resource busy");
        return true;
    }
    return false;
}
int DocumentStore::EraseCollection(const std::string collectionName)
{
    if (collections_.find(collectionName) != collections_.end()) {
        collections_.erase(collectionName);
        return E_OK;
    }
    GLOGE("erase collection failed");
    return E_INVALID_ARGS;
}
int DocumentStore::GetViewType(JsonObject &jsonObj, bool &viewType)
{
    auto leafValue = JsonCommon::GetLeafValue(jsonObj);
    if (leafValue.size() == 0) {
        return E_INVALID_ARGS;
    }
    for (size_t i = 0; i < leafValue.size(); i++) {
        switch (leafValue[i].GetValueType()) {
            case ValueObject::ValueType::VALUE_BOOL:
                if (leafValue[i].GetBoolValue()) {
                    if (i != 0 && !viewType) {
                        return -E_INVALID_ARGS;
                    }
                    viewType = true;
                } else {
                    if (i != 0 && viewType) {
                        return E_INVALID_ARGS;
                    }
                    viewType = false;
                }
                break;
            case ValueObject::ValueType::VALUE_STRING:
                if (leafValue[i].GetStringValue() == "") {
                    if (i != 0 && !viewType) {
                        return -E_INVALID_ARGS;
                    }
                    viewType = true;
                } else {
                    return -E_INVALID_ARGS;
                }
                break;
            case ValueObject::ValueType::VALUE_NUMBER:
                if (leafValue[i].GetIntValue() == 0) {
                    if (i != 0 && viewType) {
                        return -E_INVALID_ARGS;
                    }
                    viewType = false;
                } else {
                    if (i != 0 && !viewType) {
                        return E_INVALID_ARGS;
                    }
                    viewType = true;
                }
                break;
            default:
                return E_INVALID_ARGS;
        }
    }
    return E_OK;
}
} // namespace DocumentDB