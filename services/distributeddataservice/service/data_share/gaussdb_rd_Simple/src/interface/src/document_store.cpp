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
#include "doc_common.h"
#include "doc_errno.h"
#include "grd_base/grd_type_export.h"
#include "log_print.h"

namespace DocumentDB {
DocumentStore::DocumentStore(KvStoreExecutor *executor) : executor_(executor)
{
}

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
    errCode = executor_->DropCollection(lowerCaseName, ignoreNonExists);
    if (errCode != E_OK) {
        GLOGE("Drop collection failed. %d", errCode);
        return errCode;
    }

    std::lock_guard<std::mutex> lock(dbMutex_);
    errCode = executor_->CleanCollectionOption(lowerCaseName);
    if (errCode != E_OK) {
        GLOGE("Clean collection option failed. %d", errCode);
    }

    return errCode;
}

namespace {
bool CheckFilter(const std::string &filter, std::string &idStr, int &errCode)
{
    if (filter.empty()) {
        errCode = -E_INVALID_ARGS;
        GLOGE("Check filter invalid. %d", errCode);
        return false;
    }

    JsonObject filterObject = JsonObject::Parse(filter, errCode, true);
    if (errCode != E_OK) {
        GLOGE("Parse filter failed. %d", errCode);
        return false;
    }

    JsonObject filterId = filterObject.GetObjectItem("_id", errCode);
    if (errCode != E_OK || filterId.GetItemValue().GetValueType() != ValueObject::ValueType::VALUE_STRING) {
        GLOGE("Check filter '_id' not found or type not string.");
        errCode = -E_INVALID_ARGS;
        return false;
    }

    idStr = filterId.GetItemValue().GetStringValue();
    return true;
}

bool CheckDocument(const std::string &updateStr, int &errCode)
{
    if (updateStr.empty()) {
        errCode = -E_INVALID_ARGS;
        return false;
    }

    JsonObject updateObj = JsonObject::Parse(updateStr, errCode);
    if (updateObj.IsNull() || errCode != E_OK) {
        GLOGE("Parse update document failed. %d", errCode);
        return false;
    }

    JsonObject filterId = updateObj.GetObjectItem("_id", errCode);
    if (errCode != -E_NOT_FOUND) {
        GLOGE("Can not change '_id' with update document failed.");
        return false;
    }

    return true;
}
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

    std::string idStr;
    if (!CheckFilter(filter, idStr, errCode)) {
        GLOGE("Check update filter failed. %d", errCode);
        return errCode;
    }

    if (!CheckDocument(update, errCode)) {
        GLOGE("Check update document failed. %d", errCode);
        return errCode;
    }

    if (flags != 0) {
        GLOGE("Check flags invalid.");
        return -E_INVALID_ARGS;
    }

    std::string docId(idStr.begin(), idStr.end());

    std::lock_guard<std::mutex> lock(dbMutex_);
    auto coll = Collection(lowerCaseCollName, executor_);
    errCode = coll.UpdateDocument(docId, update);
    if (errCode == E_OK) {
        errCode = 1; // update one record.
    }
    return errCode;
}

int DocumentStore::UpsertDocument(const std::string &collection, const std::string &filter, const std::string &document,
    int flags)
{
    std::string lowerCaseCollName;
    int errCode = E_OK;
    if (!CheckCommon::CheckCollectionName(collection, lowerCaseCollName, errCode)) {
        GLOGE("Check collection name invalid. %d", errCode);
        return errCode;
    }

    std::string idStr;
    if (!CheckFilter(filter, idStr, errCode)) {
        GLOGE("Check upsert filter failed. %d", errCode);
        return errCode;
    }

    if (!CheckDocument(document, errCode)) {
        GLOGE("Check upsert document failed. %d", errCode);
        return errCode;
    }

    if (flags != GRD_DOC_APPEND && flags != GRD_DOC_REPLACE) {
        GLOGE("Check flags invalid.");
        return -E_INVALID_ARGS;
    }

    std::string docId(idStr.begin(), idStr.end());
    bool isReplace = ((flags & GRD_DOC_REPLACE) == GRD_DOC_REPLACE);

    std::lock_guard<std::mutex> lock(dbMutex_);
    auto coll = Collection(lowerCaseCollName, executor_);
    errCode = coll.UpsertDocument(docId, document, isReplace);
    if (errCode == E_OK) {
        errCode = 1; // upsert one record.
    }
    return errCode;
}
} // namespace DocumentDB