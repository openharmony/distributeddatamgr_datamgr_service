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
#include "document_type.h"
#include "kv_store_executor.h"

struct GRD_ResultSet;
namespace DocumentDB {
class DocumentStore {
public:
    DocumentStore(KvStoreExecutor *);
    ~DocumentStore();

    int CreateCollection(const std::string &name, const std::string &option, uint32_t flags);

    int DropCollection(const std::string &name, uint32_t flags);

    int UpdateDocument(const std::string &collection, const std::string &filter, const std::string &update,
        uint32_t flags);

    int UpsertDocument(const std::string &collection, const std::string &filter, const std::string &document,
        uint32_t flags);

    int InsertDocument(const std::string &collection, const std::string &document, uint32_t flags);

    int DeleteDocument(const std::string &collection, const std::string &filter, uint32_t flags);

    int FindDocument(const std::string &collection, const std::string &filter, const std::string &projection,
        uint32_t flags, GRD_ResultSet *grdResultSet);

    Collection GetCollection(std::string &collectionName);

    bool IsExistResultSet(const std::string &collection);

    int EraseCollection(const std::string &collectionName);

    void OnClose(const std::function<void(void)> &notifier);

    int Close(uint32_t flags);

    int StartTransaction();
    int Commit();
    int Rollback();

    bool IsCollectionExists(const std::string &collectionName, int &errCode);

    std::mutex dbMutex_;

private:
    int UpdateDataIntoDB(std::shared_ptr<QueryContext> &context, JsonObject &filterObj, const std::string &update,
        bool &isReplace);
    int UpsertDataIntoDB(std::shared_ptr<QueryContext> &context, JsonObject &filterObj, const std::string &document,
        JsonObject &documentObj, bool &isReplace);
    int InsertDataIntoDB(const std::string &collection, const std::string &document, JsonObject &documentObj,
        bool &isIdExist);
    int DeleteDataFromDB(std::shared_ptr<QueryContext> &context, JsonObject &filterObj);
    int InitFindResultSet(GRD_ResultSet *grdResultSet, std::shared_ptr<QueryContext> &context);
    KvStoreExecutor *executor_ = nullptr;
    std::map<std::string, Collection *> collections_;
    std::function<void(void)> closeNotifier_;
};
} // namespace DocumentDB
#endif // DOCUMENT_STORE_H