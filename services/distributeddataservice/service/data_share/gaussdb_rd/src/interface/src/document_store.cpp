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
#include "document_key.h"
#include "grd_base/grd_type_export.h"
#include "grd_resultset_inner.h"
#include "log_print.h"
#include "result_set.h"
#include "result_set_common.h"

namespace DocumentDB {
constexpr int JSON_LENS_MAX = 1024 * 1024;
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

int TranFilter(JsonObject &filterObj, std::vector<std::vector<std::string>> &filterAllPath, bool &isIdExist)
{
    int errCode = E_OK;
    filterAllPath = JsonCommon::ParsePath(filterObj, errCode);
    if (errCode != E_OK) {
        GLOGE("filter ParsePath failed");
        return errCode;
    }
    return CheckCommon::CheckFilter(filterObj, filterAllPath, isIdExist);
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
    if (update != "{}") {
        std::vector<std::vector<std::string>> allPath;
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

int GetUpDataRePlaceData(ResultSet &resultSet, const std::string &id, const std::string &update, std::string &valStr,
    bool isReplace)
{
    std::string valueGotStr;
    int errCode = resultSet.GetValue(valueGotStr);
    if (errCode == -E_NO_DATA) {
        GLOGW("Get original document not found.");
        return -E_NOT_FOUND;
    } else if (errCode != E_OK) {
        GLOGE("Get original document failed. %d", errCode);
        return errCode;
    }
    JsonObject updateValue = JsonObject::Parse(update, errCode, true);
    if (errCode != E_OK) {
        GLOGD("Parse upsert value failed. %d", errCode);
        return errCode;
    }
    JsonObject originValue = JsonObject::Parse(valueGotStr, errCode, true);
    if (errCode != E_OK) {
        GLOGD("Parse original value failed. %d %s", errCode, valueGotStr.c_str());
        return errCode;
    }
    errCode = JsonCommon::Append(originValue, updateValue, isReplace);
    if (errCode != E_OK) {
        GLOGD("Append value failed. %d", errCode);
        return errCode;
    }
    valStr = originValue.Print();
    if (valStr.length() >= JSON_LENS_MAX) {
        GLOGE("document's length is too long");
        return -E_OVER_LIMIT;
    }
    return errCode;
}

int DocumentStore::UpdateDataIntoDB(std::shared_ptr<QueryContext> &context, JsonObject &filterObj,
    const std::string &update, bool &isReplace)
{
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    int errCode = executor_->StartTransaction();
    if (errCode != E_OK) {
        return errCode;
    }
    std::string docId;
    int count = 0;
    std::string valStr;
    auto coll = Collection(context->collectionName, executor_);
    ResultSet resultSet;
    errCode = InitResultSet(context, this, resultSet, false);
    if (errCode != E_OK) {
        goto END;
    }
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
    errCode = GetUpDataRePlaceData(resultSet, docId, update, valStr, isReplace);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = coll.UpdateDocument(docId, valStr);
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
    int errCode = UpdateArgsCheck(collection, filter, update, flags);
    if (errCode != E_OK) {
        return errCode;
    }
    JsonObject filterObj = JsonObject::Parse(filter, errCode, true, true);
    if (errCode != E_OK) {
        GLOGE("filter Parsed failed");
        return errCode;
    }
    bool isIdExist = false;
    std::vector<std::vector<std::string>> filterAllPath;
    errCode = TranFilter(filterObj, filterAllPath, isIdExist);
    if (errCode != E_OK) {
        return errCode;
    }
    bool isReplace = ((flags & GRD_DOC_REPLACE) == GRD_DOC_REPLACE);
    std::shared_ptr<QueryContext> context = std::make_shared<QueryContext>();
    context->isIdExist = isIdExist;
    context->collectionName = collection;
    context->filter = filter;
    context->ifShowId = true;
    return UpdateDataIntoDB(context, filterObj, update, isReplace);
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

int CheckUpsertConflict(ResultSet &resultSet, JsonObject &filterObj, std::string &docId, Collection &coll,
    bool &isDataExist)
{
    std::string val; // use to know whether there is data in the resultSet or not.
    int errCode = resultSet.GetValue(val);
    if (errCode == E_OK) {
        isDataExist = true;
    }
    Value ValueDocument;
    Key id(docId.begin(), docId.end());
    errCode = coll.GetDocumentById(id, ValueDocument);
    if (errCode == E_OK && !(isDataExist)) {
        GLOGE("id exist but filter does not match, data conflict");
        errCode = -E_DATA_CONFLICT;
    }
    return errCode;
}

int GetUpsertRePlaceData(ResultSet &resultSet, JsonObject &documentObj, bool isReplace, std::string &valStr)
{
    int errCode = resultSet.GetValue(valStr);
    if (errCode != E_OK || isReplace) {
        valStr = documentObj.Print(); // If cant not find data, insert it.
        return E_OK;
    }
    if (errCode != E_OK && errCode != -E_NOT_FOUND) {
        GLOGW("Get original document failed. %d", errCode);
        return errCode;
    } else if (errCode == E_OK) { // document has been inserted
        JsonObject originValue = JsonObject::Parse(valStr, errCode, true);
        if (errCode != E_OK) {
            GLOGD("Parse original value failed. %d %s", errCode, valStr.c_str());
            return errCode;
        }
        errCode = JsonCommon::Append(originValue, documentObj, isReplace);
        if (errCode != E_OK) {
            GLOGD("Append value failed. %d", errCode);
            return errCode;
        }
        valStr = originValue.Print();
        if (valStr.length() >= JSON_LENS_MAX) {
            GLOGE("document's length is too long");
            return -E_OVER_LIMIT;
        }
    }
    return errCode;
}

int InsertIdToDocument(ResultSet &resultSet, JsonObject &filterObj, JsonObject &documentObj, std::string &docId)
{
    auto filterObjChild = filterObj.GetChild();
    bool isIdExist;
    ValueObject idValue = JsonCommon::GetValueInSameLevel(filterObjChild, KEY_ID, isIdExist);
    int errCode = E_OK;
    int ret = resultSet.GetNext(false, true); // All anomalies will be judged later
    if (ret != E_OK && ret != -E_NO_DATA) {
        return ret;
    }
    if (isIdExist) {
        docId = idValue.GetStringValue();
        JsonObject idObj = filterObj.GetObjectItem(KEY_ID, errCode); // this errCode will always be E_OK.
        documentObj.InsertItemObject(0, idObj);
    } else {
        if (ret == E_OK) { // E_OK means find data.
            (void)resultSet.GetKey(docId); // This errCode will always be E_OK.
        } else {
            DocKey docKey;
            DocumentKey::GetOidDocKey(docKey);
            docId = docKey.key;
        }
    }
    return errCode;
}

int DocumentStore::UpsertDataIntoDB(std::shared_ptr<QueryContext> &context, JsonObject &filterObj,
    const std::string &document, JsonObject &documentObj, bool &isReplace)
{
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    int errCode = executor_->StartTransaction();
    if (errCode != E_OK) {
        return errCode;
    }
    Collection coll = Collection(context->collectionName, executor_);
    int count = 0;
    std::string docId;
    ResultSet resultSet;
    std::string newDocument;
    bool isDataExist = false;
    errCode = InitResultSet(context, this, resultSet, false);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = InsertIdToDocument(resultSet, filterObj, documentObj, docId);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = CheckUpsertConflict(resultSet, filterObj, docId, coll, isDataExist);
    // There are only three return values, the two other situation can continue to move forward.
    if (errCode == -E_DATA_CONFLICT) {
        GLOGE("upsert data conflict");
        goto END;
    }
    errCode = GetUpsertRePlaceData(resultSet, documentObj, isReplace, newDocument);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = coll.UpsertDocument(docId, newDocument, isDataExist);
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
    if (document != "{}") {
        std::vector<std::vector<std::string>> allPath;
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
    int errCode = UpsertArgsCheck(collection, filter, document, flags);
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
    bool isIdExist = false;
    std::vector<std::vector<std::string>> filterAllPath;
    errCode = TranFilter(filterObj, filterAllPath, isIdExist);
    if (errCode != E_OK) {
        GLOGE("filter is invalid");
        return errCode;
    }
    std::shared_ptr<QueryContext> context = std::make_shared<QueryContext>();
    context->filter = filter;
    context->collectionName = collection;
    context->ifShowId = true;
    context->isIdExist = isIdExist;
    bool isReplace = ((flags & GRD_DOC_REPLACE) == GRD_DOC_REPLACE);
    return UpsertDataIntoDB(context, filterObj, document, documentObj, isReplace);
}

int InsertArgsCheck(const std::string &collection, const std::string &document, uint32_t flags)
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
    if (document.length() >= JSON_LENS_MAX) {
        GLOGE("document's length is too long");
        return -E_OVER_LIMIT;
    }
    return errCode;
}

int DocumentStore::InsertDataIntoDB(const std::string &collection, const std::string &document,
    JsonObject &documentObj, bool &isIdExist)
{
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::string id;
    if (isIdExist) {
        JsonObject documentObjChild = documentObj.GetChild();
        ValueObject idValue = JsonCommon::GetValueInSameLevel(documentObjChild, KEY_ID);
        id = idValue.GetStringValue();
    } else {
        DocKey docKey;
        DocumentKey::GetOidDocKey(docKey);
        id = docKey.key;
    }
    Collection coll = Collection(collection, executor_);
    return coll.InsertDocument(id, document, isIdExist);
}

int DocumentStore::InsertDocument(const std::string &collection, const std::string &document, uint32_t flags)
{
    int errCode = InsertArgsCheck(collection, document, flags);
    if (errCode != E_OK) {
        return errCode;
    }
    JsonObject documentObj = JsonObject::Parse(document, errCode, true);
    if (errCode != E_OK) {
        GLOGE("Document Parsed failed");
        return errCode;
    }
    bool isIdExist = true;
    errCode = CheckCommon::CheckDocument(documentObj, isIdExist);
    if (errCode != E_OK) {
        return errCode;
    }
    return InsertDataIntoDB(collection, document, documentObj, isIdExist);
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

int DocumentStore::DeleteDataFromDB(std::shared_ptr<QueryContext> &context, JsonObject &filterObj)
{
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    Collection coll = Collection(context->collectionName, executor_);
    int errCode = executor_->StartTransaction();
    if (errCode != E_OK) {
        return errCode;
    }
    std::string id;
    ResultSet resultSet;
    errCode = InitResultSet(context, this, resultSet, false);
    if (errCode != E_OK) {
        goto END;
    }
    errCode = resultSet.GetNext(false, true);
    if (errCode != E_OK) {
        goto END;
    }
    resultSet.GetKey(id);
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
    int errCode = DeleteArgsCheck(collection, filter, flags);
    if (errCode != E_OK) {
        return errCode;
    }
    JsonObject filterObj = JsonObject::Parse(filter, errCode, true, true);
    if (errCode != E_OK) {
        return errCode;
    }
    bool isIdExist = false;
    std::vector<std::vector<std::string>> filterAllPath;
    errCode = TranFilter(filterObj, filterAllPath, isIdExist);
    if (errCode != E_OK) {
        return errCode;
    }
    std::shared_ptr<QueryContext> context = std::make_shared<QueryContext>();
    context->isIdExist = isIdExist;
    context->filter = filter;
    context->collectionName = collection;
    return DeleteDataFromDB(context, filterObj);
}
Collection DocumentStore::GetCollection(std::string &collectionName)
{
    return Collection(collectionName, executor_);
}

int JudgeViewType(const size_t &index, ValueObject &leafItem, bool &viewType)
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
    return ret;
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

int FindProjectionInit(const std::string &projection, const std::shared_ptr<QueryContext> &context)
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
    context->projectionPath = std::move(allPath);
    context->viewType = viewType;
    return errCode;
}

int DocumentStore::InitFindResultSet(GRD_ResultSet *grdResultSet, std::shared_ptr<QueryContext> &context)
{
    std::lock_guard<std::mutex> lock(dbMutex_);
    int errCode = E_OK;
    Collection coll = Collection(context->collectionName, executor_);
    if (IsExistResultSet(context->collectionName)) {
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
    errCode = InitResultSet(context, this, grdResultSet->resultSet_, true);
    if (errCode == E_OK) {
        collections_[context->collectionName] = nullptr;
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
    int errCode = FindArgsCheck(collection, filter, projection, flags);
    if (errCode != E_OK) {
        GLOGE("delete arg is illegal");
        return errCode;
    }
    JsonObject filterObj = JsonObject::Parse(filter, errCode, true, true);
    if (errCode != E_OK) {
        GLOGE("filter Parsed failed");
        return errCode;
    }
    std::shared_ptr<QueryContext> context = std::make_shared<QueryContext>();
    errCode = FindProjectionInit(projection, context);
    if (errCode != E_OK) {
        return errCode;
    }
    bool isIdExist = false;
    std::vector<std::vector<std::string>> filterAllPath;
    errCode = TranFilter(filterObj, filterAllPath, isIdExist);
    if (errCode != E_OK) {
        GLOGE("filter is invalid");
        return errCode;
    }
    if (flags == GRD_DOC_ID_DISPLAY) {
        context->ifShowId = true;
    } else {
        context->ifShowId = false;
    }
    context->collectionName = collection;
    context->filter = filter;
    context->isIdExist = isIdExist;
    return InitFindResultSet(grdResultSet, context);
}

bool DocumentStore::IsExistResultSet(const std::string &collection)
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
        errCode = -E_INNER_ERROR;
        return false;
    }
    return executor_->IsCollectionExists(collectionName, errCode);
}
} // namespace DocumentDB