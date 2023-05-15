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

#include "check_common.h"
#include "collection_option.h"
#include "doc_errno.h"
#include "grd_base/grd_type_export.h"
#include "grd_resultset_inner.h"
#include "log_print.h"
#include "result_set.h"
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
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
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
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
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

int TranFilter(JsonObject &filterObj, std::vector<std::vector<std::string>> &filterAllPath, bool &isOnlyId)
{
    int errCode = E_OK;
    filterAllPath = JsonCommon::ParsePath(filterObj, errCode);
    if (errCode != E_OK) {
        GLOGE("filter ParsePath failed");
        return errCode;
    }
    errCode = CheckCommon::CheckFilter(filterObj, isOnlyId, filterAllPath);
    if (errCode != E_OK) {
        return errCode;
    }
    return errCode;
}

int UpdateArgsCheck(const std::string &collection, const std::string &filter, const std::string &update, uint32_t flags)
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
    return errCode;
}

int DocumentStore::UpdateDataIntoDB(std::shared_ptr<QueryContext> context, JsonObject &filterObj,
    const std::string &update, bool &isReplace)
{
    int errCode = E_OK;
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    errCode = executor_->StartTransaction();
    if (errCode != E_OK) {
        return errCode;
    }
    std::string docId;
    int count = 0;
    auto coll = Collection(context->collectionName_, executor_);
    if (context->isOnlyId_) {
        auto filterObjChild = filterObj.GetChild();
        auto idValue = JsonCommon::GetValueInSameLevel(filterObjChild, KEY_ID);
        docId = idValue.GetStringValue();
    } else {
        ResultSet resultSet;
        std::string filter = filterObj.Print();
        InitResultSet(context, this, resultSet, true);
        // no start transaction inner
        errCode = resultSet.GetNext(false, true);
        if (errCode == -E_NO_DATA) {
            // no need to set count
            errCode = E_OK;
            goto END;
        } else if (errCode != E_OK) {
            goto END;
        }
        resultSet.GetKey(docId);
    }
    errCode = coll.UpdateDocument(docId, update, isReplace);
    if (errCode == E_OK) {
        count++;
    } else if (errCode == -E_NOT_FOUND) {
        errCode = E_OK;
    }
END:
    if (errCode == E_OK) {
        executor_->Commit();
    } else {
        executor_->Rollback();
    }
    return (errCode == E_OK) ? count : errCode;
}

int DocumentStore::UpdateDocument(const std::string &collection, const std::string &filter, const std::string &update,
    uint32_t flags)
{
    int errCode = E_OK;
    errCode = UpdateArgsCheck(collection, filter, update, flags);
    if (errCode != E_OK) {
        return errCode;
    }
    JsonObject filterObj = JsonObject::Parse(filter, errCode, true, true);
    if (errCode != E_OK) {
        GLOGE("filter Parsed failed");
        return errCode;
    }
    bool isOnlyId = true;
    std::vector<std::vector<std::string>> filterAllPath;
    errCode = TranFilter(filterObj, filterAllPath, isOnlyId);
    if (errCode != E_OK) {
        return errCode;
    }
    bool isReplace = ((flags & GRD_DOC_REPLACE) == GRD_DOC_REPLACE);
    std::shared_ptr<QueryContext> context = std::make_shared<QueryContext>();
    context->collectionName_ = collection;
    context->isOnlyId_ = isOnlyId;
    context->filter_ = filter;
    errCode = UpdateDataIntoDB(context, filterObj, update, isReplace);
    return errCode;
}

int UpsertArgsCheck(const std::string &collection, const std::string &filter, const std::string &document,
    uint32_t flags)
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
    if (flags != GRD_DOC_APPEND && flags != GRD_DOC_REPLACE) {
        GLOGE("Check flags invalid.");
        return -E_INVALID_ARGS;
    }
    return errCode;
}

int DocumentStore::CheckUpsertConflict(bool &isIdExist, std::shared_ptr<QueryContext> context,
    JsonObject &filterObj, std::string &docId, Collection &coll)
{
    int errCode = E_OK;
    ResultSet resultSet;
    InitResultSet(context, this, resultSet, true);
    errCode = resultSet.GetNext(false, true);
    bool isfilterMatch = false;
    if (errCode == E_OK) {
        isfilterMatch = true;
    }
    Value ValueDocument;
    Key key(docId.begin(), docId.end());
    errCode = coll.GetDocument(key, ValueDocument);
    if (errCode == E_OK && !(isfilterMatch)) {
        GLOGE("id exist but filter does not match, data conflict");
        errCode = -E_DATA_CONFLICT;
    }
    return errCode;
}

int DocumentStore::UpsertDataIntoDB(std::shared_ptr<QueryContext> context, JsonObject &filterObj,
    JsonObject &documentObj, bool &isReplace)
{
    int errCode = E_OK;
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    errCode = executor_->StartTransaction();
    if (errCode != E_OK) {
        return errCode;
    }
    Collection coll = Collection(context->collectionName_, executor_);
    int count = 0;
    std::string targetDocument;
    std::string docId;
    bool isIdExist;
    auto filterObjChild = filterObj.GetChild();
    ValueObject idValue = JsonCommon::GetValueInSameLevel(filterObjChild, KEY_ID, isIdExist);
    docId = idValue.GetStringValue();
    JsonObject idObj = filterObj.GetObjectItem(KEY_ID, errCode);
    documentObj.InsertItemObject(0, idObj);
    targetDocument = documentObj.Print();
    if (!isIdExist) {
        errCode = -E_INVALID_ARGS;
        goto END;
    }
    if (!context->isOnlyId_) {
        errCode = CheckUpsertConflict(isIdExist, context, filterObj, docId, coll);
        // There are only three return values, E_ OK and - E_ NO_DATA is a normal scenario,
        // and that situation can continue to move forward
        if (errCode == -E_DATA_CONFLICT) {
            GLOGE("upsert data conflict");
            goto END;
        }
    }
    errCode = coll.UpsertDocument(docId, targetDocument, isReplace);
    if (errCode == E_OK) {
        count++;
    } else if (errCode == -E_NOT_FOUND) {
        errCode = E_OK;
    }
END:
    if (errCode == E_OK) {
        executor_->Commit();
    } else {
        executor_->Rollback();
    }
    return (errCode == E_OK) ? count : errCode;
}

int UpsertDocumentFormatCheck(const std::string &document, JsonObject &documentObj)
{
    int errCode = E_OK;
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
    return errCode;
}
int DocumentStore::UpsertDocument(const std::string &collection, const std::string &filter,
    const std::string &document, uint32_t flags)
{
    int errCode = E_OK;
    errCode = UpsertArgsCheck(collection, filter, document, flags);
    if (errCode != E_OK) {
        return errCode;
    }
    JsonObject filterObj = JsonObject::Parse(filter, errCode, true, true);
    if (errCode != E_OK) {
        GLOGE("filter Parsed failed");
        return errCode;
    }
    JsonObject documentObj = JsonObject::Parse(document, errCode, true);
    if (errCode != E_OK) {
        GLOGE("document Parsed failed");
        return errCode;
    }
    errCode = UpsertDocumentFormatCheck(document, documentObj);
    if (errCode != E_OK) {
        GLOGE("document format is illegal");
        return errCode;
    }
    bool isOnlyId = true;
    std::vector<std::vector<std::string>> filterAllPath;
    errCode = TranFilter(filterObj, filterAllPath, isOnlyId);
    if (errCode != E_OK) {
        GLOGE("filter is invalid");
        return errCode;
    }
    std::shared_ptr<QueryContext> context = std::make_shared<QueryContext>();
    context->filter_ = filter;
    context->isOnlyId_ = isOnlyId;
    context->collectionName_ = collection;
    bool isReplace = ((flags & GRD_DOC_REPLACE) == GRD_DOC_REPLACE);
    errCode = UpsertDataIntoDB(context, filterObj, documentObj, isReplace);
    return errCode;
}

int InsertArgsCheck(const std::string &collection, const std::string &document, uint32_t flags)
{
    int errCode = E_OK;
    if (flags != 0u) {
        GLOGE("InsertDocument flags is not zero");
        return -E_INVALID_ARGS;
    }
    std::string lowerCaseCollName;
    if (!CheckCommon::CheckCollectionName(collection, lowerCaseCollName, errCode)) {
        GLOGE("Check collection name invalid. %d", errCode);
        return errCode;
    }
    if (document.length() >= JSON_LENS_MAX) {
        GLOGE("document's length is too long");
        return -E_OVER_LIMIT;
    }
    return errCode;
}

int DocumentStore::InsertDataIntoDB(const std::string &collection, const std::string &document, JsonObject &documentObj)
{
    std::lock_guard<std::mutex> lock(dbMutex_);
    JsonObject documentObjChild = documentObj.GetChild();
    ValueObject idValue = JsonCommon::GetValueInSameLevel(documentObjChild, KEY_ID);
    std::string id = idValue.GetStringValue();
    Key key(id.begin(), id.end());
    Value value(document.begin(), document.end());
    Collection coll = Collection(collection, executor_);
    return coll.InsertDocument(key, value);
}

int DocumentStore::InsertDocument(const std::string &collection, const std::string &document, uint32_t flags)
{
    int errCode = E_OK;
    errCode = InsertArgsCheck(collection, document, flags);
    if (errCode != E_OK) {
        return errCode;
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
    errCode = InsertDataIntoDB(collection, document, documentObj);
    return errCode;
}

int DeleteArgsCheck(const std::string &collection, const std::string &filter, uint32_t flags)
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
    if (filter.empty()) {
        GLOGE("Filter is empty");
        return -E_INVALID_ARGS;
    }
    if (filter.length() >= JSON_LENS_MAX) {
        GLOGE("filter's length is too long");
        return -E_OVER_LIMIT;
    }
    return errCode;
}

int DocumentStore::DeleteDataFromDB(std::shared_ptr<QueryContext> context, JsonObject &filterObj)
{
    int errCode = E_OK;
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    Collection coll = Collection(context->collectionName_, executor_);
    errCode = executor_->StartTransaction();
    if (errCode != E_OK) {
        return errCode;
    }
    std::string id;
    if (context->isOnlyId_) {
        auto filterObjChild = filterObj.GetChild();
        auto idValue = JsonCommon::GetValueInSameLevel(filterObjChild, KEY_ID);
        id = idValue.GetStringValue();
    } else {
        ResultSet resultSet;
        InitResultSet(context, this, resultSet, true);
        errCode = resultSet.GetNext(false, true);
        if (errCode != E_OK) {
            goto END;
        }
        resultSet.GetKey(id);
    }
END:
    if (errCode == E_OK) {
        Key key(id.begin(), id.end());
        errCode = coll.DeleteDocument(key);
    }
    if (errCode == E_OK || errCode == E_NOT_FOUND) {
        executor_->Commit();
    } else {
        executor_->Rollback();
    }
    return errCode;
}
int DocumentStore::DeleteDocument(const std::string &collection, const std::string &filter, uint32_t flags)
{
    int errCode = E_OK;
    errCode = DeleteArgsCheck(collection, filter, flags);
    if (errCode != E_OK) {
        return errCode;
    }
    JsonObject filterObj = JsonObject::Parse(filter, errCode, true, true);
    if (errCode != E_OK) {
        return errCode;
    }
    bool isOnlyId = true;
    std::vector<std::vector<std::string>> filterAllPath;
    errCode = TranFilter(filterObj, filterAllPath, isOnlyId);
    if (errCode != E_OK) {
        return errCode;
    }
    std::shared_ptr<QueryContext> context = std::make_shared<QueryContext>();
    context->filter_ = filter;
    context->collectionName_ = collection;
    context->isOnlyId_ = isOnlyId;
    errCode = DeleteDataFromDB(context, filterObj);
    return errCode;
}
Collection DocumentStore::GetCollection(std::string &collectionName)
{
    return Collection(collectionName, executor_);
}

int JudgeViewType(size_t &index, ValueObject &leafItem, bool &viewType)
{
    switch (leafItem.GetValueType()) {
        case ValueObject::ValueType::VALUE_BOOL:
            if (leafItem.GetBoolValue()) {
                if (index != 0 && !viewType) {
                    return -E_INVALID_ARGS;
                }
                viewType = true;
            } else {
                if (index != 0 && viewType) {
                    return E_INVALID_ARGS;
                }
                viewType = false;
            }
            break;
        case ValueObject::ValueType::VALUE_STRING:
            if (leafItem.GetStringValue() == "") {
                if (index != 0 && !viewType) {
                    return -E_INVALID_ARGS;
                }
                viewType = true;
            } else {
                return -E_INVALID_ARGS;
            }
            break;
        case ValueObject::ValueType::VALUE_NUMBER:
            if (leafItem.GetIntValue() == 0) {
                if (index != 0 && viewType) {
                    return -E_INVALID_ARGS;
                }
                viewType = false;
            } else {
                if (index != 0 && !viewType) {
                    return E_INVALID_ARGS;
                }
                viewType = true;
            }
            break;
        default:
            return E_INVALID_ARGS;
    }
    return E_OK;
}

int GetViewType(JsonObject &jsonObj, bool &viewType)
{
    std::vector<ValueObject> leafValue = JsonCommon::GetLeafValue(jsonObj);
    if (leafValue.size() == 0) {
        return E_INVALID_ARGS;
    }
    int ret = E_OK;
    for (size_t i = 0; i < leafValue.size(); i++) {
        ret = JudgeViewType(i, leafValue[i], viewType);
        if (ret != E_OK) {
            return ret;
        }
    }
    return E_OK;
}

int FindArgsCheck(const std::string &collection, const std::string &filter, const std::string &projection,
    uint32_t flags)
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
    if (projection.length() >= JSON_LENS_MAX) {
        GLOGE("projection's length is too long");
        return -E_OVER_LIMIT;
    }
    return errCode;
}

int FindProjectionInit(const std::string &projection, std::shared_ptr<QueryContext> context)
{
    int errCode = E_OK;
    std::vector<std::vector<std::string>> allPath;
    JsonObject projectionObj = JsonObject::Parse(projection, errCode, true);
    if (errCode != E_OK) {
        GLOGE("projection Parsed failed");
        return errCode;
    }
    bool viewType = false;
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
    context->path_ = std::move(allPath);
    context->viewType_ = viewType;
    return errCode;
}

int DocumentStore::InitFindResultSet(GRD_ResultSet *grdResultSet, std::shared_ptr<QueryContext> context)
{
    std::lock_guard<std::mutex> lock(dbMutex_);
    int errCode = E_OK;
    Collection coll = Collection(context->collectionName_, executor_);
    if (IsCollectionOpening(context->collectionName_)) {
        return -E_RESOURCE_BUSY;
    }
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    errCode = executor_->StartTransaction();
    if (errCode != E_OK) {
        return errCode;
    }
    bool isCollectionExist = coll.IsCollectionExists(errCode);
    if (!isCollectionExist) {
        errCode = -E_INVALID_ARGS;
    }
    if (errCode != E_OK) {
        goto END;
    }
    errCode = InitResultSet(context, this, grdResultSet->resultSet_, false);
    if (errCode == E_OK) {
        collections_[context->collectionName_] = nullptr;
    }
END:
    if (errCode == E_OK) {
        executor_->Commit();
    } else {
        executor_->Rollback();
    }
    return errCode;
}

int DocumentStore::FindDocument(const std::string &collection, const std::string &filter,
    const std::string &projection, uint32_t flags, GRD_ResultSet *grdResultSet)
{
    std::shared_ptr<QueryContext> context = std::make_shared<QueryContext>();
    int errCode = E_OK;
    errCode = FindArgsCheck(collection, filter, projection, flags);
    if (errCode != E_OK) {
        GLOGE("delete arg is illegal");
        return errCode;
    }
    context->collectionName_ = collection;
    context->filter_ = filter;
    JsonObject filterObj = JsonObject::Parse(filter, errCode, true, true);
    if (errCode != E_OK) {
        GLOGE("filter Parsed failed");
        return errCode;
    }
    errCode = FindProjectionInit(projection, context);
    if (errCode != E_OK) {
        return errCode;
    }
    bool isOnlyId = true;
    std::vector<std::vector<std::string>> filterAllPath;
    errCode = TranFilter(filterObj, filterAllPath, isOnlyId);
    if (errCode != E_OK) {
        GLOGE("filter is invalid");
        return errCode;
    }
    context->isOnlyId_ = isOnlyId;
    bool ifShowId = false;
    if (flags == GRD_DOC_ID_DISPLAY) {
        ifShowId = true;
    }
    context->ifShowId_ = ifShowId;
    errCode = InitFindResultSet(grdResultSet, context);
    return errCode;
}

bool DocumentStore::IsCollectionOpening(const std::string &collection)
{
    if (collections_.find(collection) != collections_.end()) {
        GLOGE("DB is resource busy");
        return true;
    }
    return false;
}

int DocumentStore::EraseCollection(const std::string &collectionName)
{
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (collections_.find(collectionName) != collections_.end()) {
        collections_.erase(collectionName);
        return E_OK;
    }
    GLOGE("erase collection failed");
    return E_INVALID_ARGS;
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

int DocumentStore::StartTransaction()
{
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    return executor_->StartTransaction();
}
int DocumentStore::Commit()
{
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    return executor_->Commit();
}
int DocumentStore::Rollback()
{
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    return executor_->Rollback();
}

bool DocumentStore::IsCollectionExists(const std::string &collectionName, int &errCode)
{
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    return executor_->IsCollectionExists(collectionName, errCode);
}
} // namespace DocumentDB