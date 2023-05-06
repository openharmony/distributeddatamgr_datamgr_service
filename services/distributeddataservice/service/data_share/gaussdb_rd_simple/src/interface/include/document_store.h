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

#include <map>
#include <mutex>
#include <string>

#include "collection.h"
#include "kv_store_executor.h"

struct GRD_ResultSet;
namespace DocumentDB {
class DocumentStore {
public:
    DocumentStore(KvStoreExecutor *);
    ~DocumentStore();

    int CreateCollection(const std::string &name, const std::string &option, int flags);
    int DropCollection(const std::string &name, int flags);

    int UpdateDocument(const std::string &collection, const std::string &filter, const std::string &update, int flags);
    int UpsertDocument(const std::string &collection, const std::string &filter, const std::string &document,
        int flags);
    int InsertDocument(const std::string &collection, const std::string &document, int flag);
    int DeleteDocument(const std::string &collection, const std::string &filter, int flag);
    int FindDocument(const std::string &collection, const std::string &filter, const std::string &projection,
        int flags, GRD_ResultSet *grdResultSet);
    KvStoreExecutor *GetExecutor(int errCode);
    bool IsCollectionOpening(const std::string collection);
    int EraseCollection(const std::string collectionName);

    void OnClose(const std::function<void(void)> &notifier);

    void Close();

private:
    int GetViewType(JsonObject &jsonObj, bool &viewType);
    std::mutex dbMutex_;
    KvStoreExecutor *executor_ = nullptr;
    std::map<std::string, Collection *> collections_;
    std::function<void(void)> closeNotifier_;
};
} // namespace DocumentDB
#endif // DOCUMENT_STORE_H