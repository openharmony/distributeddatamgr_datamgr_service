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
const int JSON_LENS_MAX = 1024 * 1024;
constexpr const char *KEY_ID = "_id";

DocumentStore::DocumentStore(KvStoreExecutor *executor) : executor_(executor) {}

DocumentStore::~DocumentStore()
{
    delete executor_;
}

int DocumentStore::CreateCollection(const std::string &name, const std::string &option, uint32_t flags)
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

    if (flags != 0u && flags != CHK_EXIST_COLLECTION) {
        GLOGE("Check flags invalid.");
        return -E_INVALID_ARGS;
    }

    std::lock_guard<std::mutex> lock(dbMutex_);
    errCode = executor_->StartTransaction();
    if (errCode != E_OK) {
        return errCode;
    }

    std::string oriOptStr;
    bool ignoreExists = (flags != CHK_EXIST_COLLECTION);
    errCode = executor_->CreateCollection(lowerCaseName, oriOptStr, ignoreExists);
    if (errCode != E_OK) {
        GLOGE("Create collection failed. %d", errCode);
        goto END;
    }

END:
    if (errCode == E_OK) {
        executor_->Commit();
    } else {
        executor_->Rollback();
    }
    return errCode;
}

int DocumentStore::DropCollection(const std::string &name, uint32_t flags)
{
    std::string lowerCaseName;
    int errCode = E_OK;
    if (!CheckCommon::CheckCollectionName(name, lowerCaseName, errCode)) {
        GLOGE("Check collection name invalid. %d", errCode);
        return errCode;
    }

    if (flags != 0u && flags != CHK_NON_EXIST_COLLECTION) {
        GLOGE("Check flags invalid.");
        return -E_INVALID_ARGS;
    }

    bool ignoreNonExists = (flags != CHK_NON_EXIST_COLLECTION);
    std::lock_guard<std::mutex> lock(dbMutex_);
    errCode = executor_->StartTransaction();
    if (errCode != E_OK) {
        return errCode;
    }

    errCode = executor_->DropCollection(lowerCaseName, ignoreNonExists);
    if (errCode != E_OK) {
        GLOGE("Drop collection failed. %d", errCode);
        goto END;
    }

    errCode = executor_->CleanCollectionOption(lowerCaseName);
    if (errCode != E_OK && errCode != -E_NO_DATA) {
        GLOGE("Clean collection option failed. %d", errCode);
    } else {
        errCode = E_OK;
    }

END:
    if (errCode == E_OK) {
        executor_->Commit();
    } else {
        executor_->Rollback();
    }
    return errCode;
}

int DocumentStore::UpdateDocument(const std::string &collection, const std::string &filter, const std::string &update,
    uint32_t flags)
{
    std::string lowerCaseCollName;
    int errCode = E_OK;
    if (!CheckCommon::CheckCollectionName(collection, lowerCaseCollName, errCode)) {
        GLOGE("Check collection name invalid. %d", errCode);
        return errCode;
    }
    if (update.length() >= JSON_LENS_MAX || filter.length() >= JSON_LENS_MAX) {
        GLOGE("args document's length is too long");
        return -E_OVER_LIMIT;
    }
    JsonObject updateObj = JsonObject::Parse(update, errCode, true);
    if (errCode != E_OK) {
        GLOGE("update Parsed failed");
        return errCode;
    }
    std::vector<std::vector<std::string>> allPath;
    if (update != "{}") {
        allPath = JsonCommon::ParsePath(updateObj, errCode);
        if (errCode != E_OK) {
            GLOGE("updateObj ParsePath failed");
            return errCode;
        }
        errCode = CheckCommon::CheckUpdata(updateObj, allPath);
        if (errCode != E_OK) {
            GLOGE("Updata format is illegal");
            return errCode;
        }
    }
    if (flags != GRD_DOC_APPEND && flags != GRD_DOC_REPLACE) {
        GLOGE("Check flags invalid.");
        return -E_INVALID_ARGS;
    }
    JsonObject filterObj = JsonObject::Parse(filter, errCode, true, true);
    if (errCode != E_OK) {
        GLOGE("filter Parsed failed");
        return errCode;
    }
    std::vector<std::vector<std::string>> filterAllPath;
    filterAllPath = JsonCommon::ParsePath(filterObj, errCode);
    if (errCode != E_OK) {
        GLOGE("filter ParsePath failed");
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
        bool isCollectionExist = coll.IsCollectionExists(errCode);
        if (errCode != E_OK) {
            return errCode;
        }
        if (!isCollectionExist) {
            return -E_INVALID_ARGS;
        }
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
    bool isCollectionExist = coll.IsCollectionExists(errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isCollectionExist) {
        return -E_INVALID_ARGS;
    }
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
    const std::string &document, uint32_t flags)
{
    std::string lowerCaseCollName;
    int errCode = E_OK;
    if (!CheckCommon::CheckCollectionName(collection, lowerCaseCollName, errCode)) {
        GLOGE("Check collection name invalid. %d", errCode);
        return errCode;
    }
    if (document.length() >= JSON_LENS_MAX || filter.length() >= JSON_LENS_MAX) {
        GLOGE("args length is too long");
        return -E_OVER_LIMIT;
    }
    JsonObject documentObj = JsonObject::Parse(document, errCode, true);
    if (errCode != E_OK) {
        GLOGE("document Parsed failed");
        return errCode;
    }
    std::vector<std::vector<std::string>> allPath;
    if (document != "{}") {
        allPath = JsonCommon::ParsePath(documentObj, errCode);
        if (errCode != E_OK) {
            return errCode;
        }
        errCode = CheckCommon::CheckUpdata(documentObj, allPath);
        if (errCode != E_OK) {
            GLOGE("UpsertDocument document format is illegal");
            return errCode;
        }
    }
    if (flags != GRD_DOC_APPEND && flags != GRD_DOC_REPLACE) {
        GLOGE("Check flags invalid.");
        return -E_INVALID_ARGS;
    }
    JsonObject filterObj = JsonObject::Parse(filter, errCode, true, true);
    if (errCode != E_OK) {
        GLOGE("filter Parsed failed");
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
        bool isCollectionExist = coll.IsCollectionExists(errCode);
        if (errCode != E_OK) {
            return errCode;
        }
        if (!isCollectionExist) {
            return -E_INVALID_ARGS;
        }
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
    bool isCollectionExist = coll.IsCollectionExists(errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isCollectionExist) {
        return -E_INVALID_ARGS;
    }
    ResultSet resultSet;
    InitResultSet(this, collection, filter, resultSet);
    errCode = resultSet.GetNext();
    bool isfilterMatch = false;
    if (errCode == E_OK) {
        isfilterMatch = true;
    }
    std::string docId = idValue.GetStringValue();
    JsonObject idObj = filterObj.GetObjectItem(KEY_ID, errCode);
    documentObj.InsertItemObject(0, idObj);
    std::string addedIdDocument = documentObj.Print();
    Value ValueDocument;
    Key key(docId.begin(), docId.end());
    errCode = coll.GetDocument(key, ValueDocument);
    if (errCode == E_OK && !(isfilterMatch)) {
        GLOGE("id exist but filter does not match, data conflict");
        return -E_DATA_CONFLICT;
    }
    errCode = coll.UpsertDocument(docId, addedIdDocument, isReplace);
    if (errCode == E_OK) {
        errCode = 1; // upsert one record.
    } else if (errCode == -E_NOT_FOUND) {
        errCode = E_OK;
    }
    return errCode;
}

int DocumentStore::InsertDocument(const std::string &collection, const std::string &document, uint32_t flags)
{
    if (flags != 0u) {
        GLOGE("InsertDocument flags is not zero");
        return -E_INVALID_ARGS;
    }
    std::string lowerCaseCollName;
    int errCode = E_OK;
    if (!CheckCommon::CheckCollectionName(collection, lowerCaseCollName, errCode)) {
        GLOGE("Check collection name invalid. %d", errCode);
        return errCode;
    }
    auto coll = Collection(collection, executor_);
    if (document.length() >= JSON_LENS_MAX) {
        GLOGE("document's length is too long");
        return -E_OVER_LIMIT;
    }
    JsonObject documentObj = JsonObject::Parse(document, errCode, true);
    if (errCode != E_OK) {
        GLOGE("Document Parsed failed");
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
    bool isCollectionExist = coll.IsCollectionExists(errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isCollectionExist) {
        return -E_INVALID_ARGS;
    }
    Value ValueDocument;
    errCode = coll.GetDocument(key, ValueDocument);
    switch (errCode) {
        case -E_NOT_FOUND:
            return coll.PutDocument(key, value);
        case -E_ERROR:
            GLOGE("collection dont exsited");
            return -E_INVALID_ARGS;
        default:
            GLOGE("id already exsited, data conflict");
            return -E_DATA_CONFLICT;
    }
}

int DocumentStore::DeleteDocument(const std::string &collection, const std::string &filter, uint32_t flags)
{
    if (flags != 0u) {
        GLOGE("DeleteDocument flags is not zero");
        return -E_INVALID_ARGS;
    }
    std::string lowerCaseCollName;
    int errCode = E_OK;
    if (!CheckCommon::CheckCollectionName(collection, lowerCaseCollName, errCode)) {
        GLOGE("Check collection name invalid. %d", errCode);
        return errCode;
    }
    auto coll = Collection(collection, executor_);
    if (filter.empty()) {
        GLOGE("Filter is empty");
        return -E_INVALID_ARGS;
    }
    if (filter.length() >= JSON_LENS_MAX) {
        GLOGE("filter's length is too long");
        return -E_OVER_LIMIT;
    }
    JsonObject filterObj = JsonObject::Parse(filter, errCode, true, true);
    if (errCode != E_OK) {
        GLOGE("filter Parsed failed");
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
        bool isCollectionExist = coll.IsCollectionExists(errCode);
        if (errCode != E_OK) {
            return errCode;
        }
        if (!isCollectionExist) {
            return -E_INVALID_ARGS;
        }
        return coll.DeleteDocument(key);
    }
    ResultSet resultSet;
    InitResultSet(this, collection, filter, resultSet);
    std::lock_guard<std::mutex> lock(dbMutex_);
    bool isCollectionExist = coll.IsCollectionExists(errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isCollectionExist) {
        return -E_INVALID_ARGS;
    }
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
    const std::string &projection, uint32_t flags, GRD_ResultSet *grdResultSet)
{
    if (flags != 0u && flags != GRD_DOC_ID_DISPLAY) {
        GLOGE("FindDocument flags is illegal");
        return -E_INVALID_ARGS;
    }
    std::string lowerCaseCollName;
    int errCode = E_OK;
    if (!CheckCommon::CheckCollectionName(collection, lowerCaseCollName, errCode)) {
        GLOGE("Check collection name invalid. %d", errCode);
        return errCode;
    }
    if (filter.length() >= JSON_LENS_MAX || projection.length() >= JSON_LENS_MAX) {
        GLOGE("args length is too long");
        return -E_OVER_LIMIT;
    }
    JsonObject filterObj = JsonObject::Parse(filter, errCode, true, true);
    if (errCode != E_OK) {
        GLOGE("filter Parsed failed");
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
    if (projection.length() >= JSON_LENS_MAX) {
        GLOGE("projection's length is too long");
        return -E_OVER_LIMIT;
    }
    JsonObject projectionObj = JsonObject::Parse(projection, errCode, true);
    if (errCode != E_OK) {
        GLOGE("projection Parsed failed");
        return errCode;
    }
    bool viewType = false;
    std::vector<std::vector<std::string>> allPath;
    if (projection != "{}") {
        allPath = JsonCommon::ParsePath(projectionObj, errCode);
        if (errCode != E_OK) {
            return errCode;
        }
        if (GetViewType(projectionObj, viewType) != E_OK) {
            GLOGE("GetViewType failed");
            return -E_INVALID_ARGS;
        }
        errCode = CheckCommon::CheckProjection(projectionObj, allPath);
        if (errCode != E_OK) {
            GLOGE("projection format unvalid");
            return errCode;
        }
    }
    bool ifShowId = false;
    if (flags == GRD_DOC_ID_DISPLAY) {
        ifShowId = true;
    }
    auto coll = Collection(collection, executor_);
    std::lock_guard<std::mutex> lock(dbMutex_);
    bool isCollectionExist = coll.IsCollectionExists(errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isCollectionExist) {
        return -E_INVALID_ARGS;
    }
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
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (collections_.find(collection) != collections_.end()) {
        GLOGE("DB is resource busy");
        return true;
    }
    return false;
}

int DocumentStore::EraseCollection(const std::string collectionName)
{
    std::lock_guard<std::mutex> lock(dbMutex_);
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

void DocumentStore::OnClose(const std::function<void(void)> &notifier)
{
    closeNotifier_ = notifier;
}

int DocumentStore::Close(uint32_t flags)
{
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (flags == GRD_DB_CLOSE && !collections_.empty()) {
        GLOGE("Close store failed with result set not closed.");
        return -E_RESOURCE_BUSY;
    }

    if (closeNotifier_) {
        closeNotifier_();
    }
    return E_OK;
}
} // namespace DocumentDB