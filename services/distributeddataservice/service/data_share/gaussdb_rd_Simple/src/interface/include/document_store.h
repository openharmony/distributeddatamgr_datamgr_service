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

#ifndef DOCUMENT_STORE_H
#define DOCUMENT_STORE_H

#include <string>
#include <map>

#include "collection.h"
#include "kv_store_executor.h"

namespace DocumentDB {
class DocumentStore {
public:
    DocumentStore(KvStoreExecutor *);
    ~DocumentStore();

    int CreateCollection(const std::string &name, const std::string &option, int flags);
    int DropCollection(const std::string &name, int flag);

    int UpdateDocument(const std::string &collection, const std::string &filter, const std::string &update, int flag);
    int UpsertDocument(const std::string &collection, const std::string &filter, const std::string &document, int flag);

private:
    KvStoreExecutor *executor_ = nullptr;
    std::map<std::string, Collection> collections_;
};
} // DocumentDB
#endif // DOCUMENT_STORE_H