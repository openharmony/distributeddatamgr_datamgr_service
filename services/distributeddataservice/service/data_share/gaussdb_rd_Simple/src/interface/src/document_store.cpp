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
#include "doc_errno.h"

namespace DocumentDB {
DocumentStore::DocumentStore(KvStoreExecutor *executor) : executor_(executor)
{
}

DocumentStore::~DocumentStore()
{
    delete executor_;
}

int DocumentStore::CreateCollection(const std::string &name, const std::string &option, int flag)
{
    executor_->CreateCollection(name, flag);
    return E_OK;
}

int DocumentStore::DropCollection(const std::string &name, int flag)
{
    executor_->DropCollection(name, flag);
    return E_OK;
}

int DocumentStore::UpdateDocument(const std::string &collection, const std::string &filter, const std::string &update,
    int flag)
{
    return E_OK;
}

int DocumentStore::UpsertDocument(const std::string &collection, const std::string &filter, const std::string &document,
    int flag)
{
    auto coll = Collection(collection, executor_);

    Key key(filter.begin(), filter.end());
    Value value(document.begin(), document.end());

    return coll.PutDocument(key, value);
}
} // namespace DocumentDB