/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#ifndef LDBPROJ_GENERAL_STORE_MOCK_H
#define LDBPROJ_GENERAL_STORE_MOCK_H

#include "store/general_store.h"
#include <gmock/gmock.h>
namespace OHOS {
namespace DistributedData {
class GeneralStoreMock : public GeneralStore {
public:
    ~GeneralStoreMock() override = default;
    MOCK_METHOD(void, SetExecutor, (std::shared_ptr<Executor> executor), (override));
    MOCK_METHOD3(Bind,
        int32_t(const Database &database, const std::map<uint32_t, BindInfo> &bindInfos, const CloudConfig &config));
    MOCK_METHOD(bool, IsBound, (uint32_t user), (override));
    MOCK_METHOD(int32_t, Execute, (const std::string &table, const std::string &sql), (override));
    MOCK_METHOD(int32_t, SetDistributedTables,
        (const std::vector<std::string> &tables, int type, const std::vector<Reference> &references, int32_t tableType,
            bool isAsync),
        (override));
    MOCK_METHOD(int32_t, SetTrackerTable,
        (const std::string &tableName, const std::set<std::string> &trackerColNames,
            const std::set<std::string> &extendColNames, bool isForceUpgrade),
        (override));
    MOCK_METHOD(int32_t, Insert, (const std::string &table, VBuckets &&values), (override));
    MOCK_METHOD(int32_t, Replace, (const std::string &table, VBucket &&value), (override));
    MOCK_METHOD(int32_t, Update,
        (const std::string &table, const std::string &setSql, Values &&values, const std::string &whereSql,
            Values &&conditions),
        (override));
    MOCK_METHOD(int32_t, Delete, (const std::string &table, const std::string &sql, Values &&args), (override));
    MOCK_METHOD((std::pair<int32_t, std::shared_ptr<Cursor>>), Query,
        (const std::string &table, const std::string &sql, Values &&args), (override));
    MOCK_METHOD((std::pair<int32_t, std::shared_ptr<Cursor>>), Query, (const std::string &table, GenQuery &query),
        (override));
    MOCK_METHOD((std::pair<int32_t, int64_t>), Insert,
        (const std::string &table, VBucket &&value, ConflictResolution resolution), (override));
    MOCK_METHOD((std::pair<int32_t, int64_t>), BatchInsert,
        (const std::string &table, VBuckets &&values, ConflictResolution resolution), (override));
    MOCK_METHOD((std::pair<int32_t, int64_t>), Update,
        (GenQuery & query, VBucket &&value, ConflictResolution resolution), (override));
    MOCK_METHOD((std::pair<int32_t, int64_t>), Delete, (GenQuery & query), (override));
    MOCK_METHOD((std::pair<int32_t, Value>), Execute, (const std::string &sql, Values &&args), (override));
    MOCK_METHOD((std::pair<int32_t, std::shared_ptr<Cursor>>), Query,
        (const std::string &sql, Values &&args, bool preCount), (override));
    MOCK_METHOD((std::pair<int32_t, std::shared_ptr<Cursor>>), Query,
        (GenQuery & query, const std::vector<std::string> &columns, bool preCount), (override));
    MOCK_METHOD((std::pair<int32_t, int32_t>), Sync,
        (const Devices &devices, GenQuery &query, DetailAsync async, const SyncParam &syncParam), (override));
    MOCK_METHOD((std::pair<int32_t, std::shared_ptr<Cursor>>), PreSharing, (GenQuery & query), (override));
    MOCK_METHOD(int32_t, Clean, (const std::vector<std::string> &devices, int32_t mode, const std::string &tableName),
        (override));
    MOCK_METHOD(int32_t, Clean, (const std::string &device, int32_t mode, const std::vector<std::string> &tableList),
        (override));
    MOCK_METHOD(int32_t, Watch, (int32_t origin, Watcher &watcher), (override));
    MOCK_METHOD(int32_t, Unwatch, (int32_t origin, Watcher &watcher), (override));
    MOCK_METHOD(int32_t, RegisterDetailProgressObserver, (DetailAsync async), (override));
    MOCK_METHOD(int32_t, UnregisterDetailProgressObserver, (), (override));
    MOCK_METHOD(int32_t, Close, (bool isForce), (override));
    MOCK_METHOD(int32_t, AddRef, (), (override));
    MOCK_METHOD(int32_t, Release, (), (override));
    MOCK_METHOD1(BindSnapshots, int32_t(std::shared_ptr<std::map<std::string, std::shared_ptr<Snapshot>>> bindAssets));
    MOCK_METHOD(int32_t, MergeMigratedData, (const std::string &tableName, VBuckets &&values), (override));
    MOCK_METHOD(int32_t, CleanTrackerData, (const std::string &tableName, int64_t cursor), (override));
    MOCK_METHOD(void, SetEqualIdentifier, (const std::string &appId, const std::string &storeId, std::string account),
        (override));
    MOCK_METHOD(int32_t, SetConfig, (const StoreConfig &storeConfig), (override));
    MOCK_METHOD((std::pair<int32_t, uint32_t>), LockCloudDB, (), (override));
    MOCK_METHOD(int32_t, UnLockCloudDB, (), (override));
    MOCK_METHOD(int32_t, UpdateDBStatus, (), (override));
    MOCK_METHOD(int32_t, SetCloudConflictHandler, (const std::shared_ptr<CloudConflictHandler> &handler), (override));
    MOCK_METHOD(int32_t, StopCloudSync, (), (override));
    MOCK_METHOD(int32_t, OnSyncTrigger, (const std::string &storeId, int32_t triggerMode), (override));
};

} // namespace DistributedData
} // namespace OHOS
#endif //LDBPROJ_GENERAL_STORE_MOCK_H
