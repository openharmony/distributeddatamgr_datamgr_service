/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef RELATIONAL_STORE_INSTANCE_H
#define RELATIONAL_STORE_INSTANCE_H
#ifdef RELATIONAL_STORE

#include <string>
#include <mutex>

#include "irelational_store.h"
#include "relationaldb_properties.h"

namespace DistributedDB {
class RelationalStoreInstance final {
public:
    RelationalStoreInstance();
    ~RelationalStoreInstance() = default;

    static RelationalStoreConnection *GetDatabaseConnection(const RelationalDBProperties &properties, int &errCode);
    static RelationalStoreInstance *GetInstance();

    static int ReleaseDataBaseConnection(RelationalStoreConnection *connection);

    int CheckDatabaseFileStatus(const std::string &id);

    // public for test mock
    static IRelationalStore *GetDataBase(const RelationalDBProperties &properties, int &errCode);

    void Dump(int fd);
private:

    IRelationalStore *OpenDatabase(const RelationalDBProperties &properties, int &errCode);

    void RemoveKvDBFromCache(const RelationalDBProperties &properties);
    void SaveRelationalDBToCache(IRelationalStore *store, const RelationalDBProperties &properties);

    void EnterDBOpenCloseProcess(const std::string &identifier);
    void ExitDBOpenCloseProcess(const std::string &identifier);

    static RelationalStoreInstance *instance_;
    static std::mutex instanceLock_;

    std::string appId_;
    std::string userId_;

    std::mutex relationalDBOpenMutex_;
    std::condition_variable relationalDBOpenCondition_;
    std::set<std::string> relationalDBOpenSet_;
};
} // namespace DistributedDB

#endif
#endif // RELATIONAL_STORE_INSTANCE_H