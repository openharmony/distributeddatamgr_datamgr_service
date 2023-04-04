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
#include "doc_common.h"
#include "doc_errno.h"
#include "log_print.h"

namespace DocumentDB {
Collection::Collection(std::string name, KvStoreExecutor *executor) : name_(COLL_PREFIX + name), executor_(executor)
{
}

Collection::~Collection()
{
    executor_ = nullptr;
}

int Collection::PutDocument(const Key &key, const Value &document)
{
    if (executor_ == nullptr) {
        return -E_INVALID_ARGS;
    }
    return executor_->PutData(name_, key, document);
}

int Collection::GetDocument(const Key &key, Value &document) const
{
    return E_OK;
}

int Collection::DeleteDocument(const Key &key)
{
    return E_OK;
}

int Collection::UpsertDocument(const std::string &id, const std::string &document, bool isReplace)
{
    if (executor_ == nullptr) {
        return -E_INVALID_ARGS;
    }

    int errCode = E_OK;
    bool isCollExist = executor_->IsCollectionExists(name_, errCode);
    if (errCode != E_OK) {
        GLOGE("Check collection failed. %d", errCode);
        return -errCode;
    }
    if (!isCollExist) {
        GLOGE("Collection not created.");
        return -E_NO_DATA;
    }

    Key keyId(id.begin(), id.end());
    Value valSet(document.begin(), document.end());

    if (!isReplace) {
        Value valueGot;
        errCode = executor_->GetData(name_, keyId, valueGot);
        std::string valueGotStr = std::string(valueGot.begin(), valueGot.end());

        if (errCode != E_OK && errCode != -E_NOT_FOUND) {
            GLOGE("Get original document failed. %d", errCode);
            return errCode;
        } else if (errCode == E_OK) { // document has been inserted
            GLOGD("Document has been inserted, append value.");
            JsonObject originValue = JsonObject::Parse(valueGotStr, errCode);

            JsonObject upsertValue = JsonObject::Parse(document, errCode);

            // TOOD:: Join document and update valSet
        }
    }

    return executor_->PutData(name_, keyId, valSet);
}

int Collection::UpdateDocument(const Key &key, Value &update)
{
    return E_OK;
}
} // namespace DocumentDB
