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

#include "collection.h"

#include "check_common.h"
#include "db_constant.h"
#include "doc_errno.h"
#include "document_key.h"
#include "log_print.h"

namespace DocumentDB {
constexpr int JSON_LENS_MAX = 1024 * 1024;

Collection::Collection(const std::string &name, KvStoreExecutor *executor) : executor_(executor)
{
    std::string lowerCaseName = name;
    std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    name_ = DBConstant::COLL_PREFIX + lowerCaseName;
}

Collection::Collection(const Collection &other)
{
    name_ = other.name_;
    executor_ = other.executor_;
}

Collection::~Collection()
{
    executor_ = nullptr;
}

int Collection::InsertDocument(const std::string &id, const std::string &document, bool &isIdExist)
{
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    int errCode = E_OK;
    bool isCollectionExist = IsCollectionExists(errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isCollectionExist) {
        return -E_INVALID_ARGS;
    }
    Key key;
    Value valSet(document.begin(), document.end());
    DocKey docKey;
    if (!isIdExist) {
        key.assign(id.begin(), id.end());
        errCode = executor_->InsertData(name_, key, valSet);
        while (errCode == -E_DATA_CONFLICT) { // if id alreay exist, create new one.
            DocumentKey::GetOidDocKey(docKey);
            key.assign(docKey.key.begin(), docKey.key.end());
            errCode = executor_->InsertData(name_, key, valSet);
        }
        return errCode;
    } else {
        key.assign(id.begin(), id.end());
    }
    return executor_->InsertData(name_, key, valSet);
}

bool Collection::FindDocument()
{
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    int errCode = E_OK;
    return executor_->IsCollectionExists(name_, errCode);
}

int Collection::GetDocumentByKey(Key &key, Value &document) const
{
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    return executor_->GetDataByKey(name_, key, document);
}

int Collection::GetMatchedDocument(const JsonObject &filterObj, Key &key, std::pair<std::string, std::string> &values,
    int isIdExist) const
{
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    return executor_->GetDataByFilter(name_, key, filterObj, values, isIdExist);
}

int Collection::DeleteDocument(Key &key)
{
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    int errCode = E_OK;
    bool isCollectionExist = IsCollectionExists(errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    if (!isCollectionExist) {
        return -E_INVALID_ARGS;
    }
    return executor_->DelData(name_, key);
}

int Collection::IsCollectionExists(int &errCode)
{
    return executor_->IsCollectionExists(name_, errCode);
}

int Collection::UpsertDocument(const std::string &id, const std::string &newStr, bool &isIdExist)
{
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    int errCode = E_OK;
    bool isCollExist = executor_->IsCollectionExists(name_, errCode);
    if (errCode != E_OK) {
        GLOGE("Check collection failed. %d", errCode);
        return -errCode;
    }
    if (!isCollExist) {
        GLOGE("Collection not created.");
        return -E_INVALID_ARGS;
    }
    Key key;
    Value valSet(newStr.begin(), newStr.end());
    DocKey docKey;
    if (!isIdExist) {
        key.assign(id.begin(), id.end());
        errCode = executor_->PutData(name_, key, valSet);
        while (errCode != E_OK) {
            DocumentKey::GetOidDocKey(docKey);
            key.assign(docKey.key.begin(), docKey.key.end());
            errCode = executor_->PutData(name_, key, valSet);
        }
        return errCode;
    } else {
        key.assign(id.begin(), id.end());
    }
    return executor_->PutData(name_, key, valSet);
}

int Collection::UpdateDocument(const std::string &id, const std::string &newStr)
{
    if (executor_ == nullptr) {
        return -E_INNER_ERROR;
    }
    int errCode = E_OK;
    bool isCollExist = executor_->IsCollectionExists(name_, errCode);
    if (errCode != E_OK) {
        GLOGE("Check collection failed. %d", errCode);
        return -errCode;
    }
    if (!isCollExist) {
        GLOGE("Collection not created.");
        return -E_INVALID_ARGS;
    }
    Key keyId(id.begin(), id.end());
    Value valSet(newStr.begin(), newStr.end());
    return executor_->PutData(name_, keyId, valSet);
}
} // namespace DocumentDB
