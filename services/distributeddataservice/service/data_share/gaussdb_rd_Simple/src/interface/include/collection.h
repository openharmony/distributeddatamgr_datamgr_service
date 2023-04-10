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

#ifndef COLLECTION_H
#define COLLECTION_H

#include <string>
#include "doc_common.h"
#include "kv_store_executor.h"

namespace DocumentDB {
class Collection {
public:
    Collection(const std::string &name, KvStoreExecutor *executor);
    ~Collection();

    int PutDocument(const Key &key, const Value &document);
    int GetDocument(const Key &key, Value &document) const;
    int DeleteDocument(const Key &key);

    int UpsertDocument(const std::string &id, const std::string &document, bool isReplace = true);
    int UpdateDocument(const Key &key, Value &update);
private:
    std::string name_;
    KvStoreExecutor *executor_ = nullptr;
};
} // namespace DocumentDB
#endif // COLLECTION_H