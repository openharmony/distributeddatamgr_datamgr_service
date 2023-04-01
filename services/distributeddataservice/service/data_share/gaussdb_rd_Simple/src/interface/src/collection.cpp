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

int Collection::UpsertDocument(const Key &key, Value &document)
{
    if (executor_ == nullptr) {
        return -E_INVALID_ARGS;
    }
    return executor_->PutData(name_, key, document);
}

int Collection::UpdateDocument(const Key &key, Value &update)
{
    return E_OK;
}
} // namespace DocumentDB
