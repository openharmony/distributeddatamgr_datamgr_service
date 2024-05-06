/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef OHOS_DISTRIBUTEDDATA_SERVICE_TEST_GENERAL_STORE_MOCK_H
#define OHOS_DISTRIBUTEDDATA_SERVICE_TEST_GENERAL_STORE_MOCK_H

#include "cursor_mock.h"
#include "store/general_store.h"
namespace OHOS {
namespace DistributedData {
class GeneralStoreMock : public GeneralStore {
public:
    int32_t Bind(Database &database, const std::map<uint32_t, BindInfo> &bindInfos) override;
    bool IsBound() override;
    int32_t Execute(const std::string &table, const std::string &sql) override;
    int32_t SetDistributedTables(
        const std::vector<std::string> &tables, int32_t type, const std::vector<Reference> &references) override;
    int32_t SetTrackerTable(const std::string &tableName, const std::set<std::string> &trackerColNames,
        const std::string &extendColName) override;
    int32_t Insert(const std::string &table, VBuckets &&values) override;
    int32_t Update(const std::string &table, const std::string &setSql, Values &&values, const std::string &whereSql,
        Values &&conditions) override;
    int32_t Replace(const std::string &table, VBucket &&value) override;
    int32_t Delete(const std::string &table, const std::string &sql, Values &&args) override;
    std::shared_ptr<Cursor> Query(const std::string &table, const std::string &sql, Values &&args) override;
    std::shared_ptr<Cursor> Query(const std::string &table, GenQuery &query) override;
    int32_t Sync(const Devices &devices, GenQuery &query, DetailAsync async, SyncParam &syncParm) override;
    std::shared_ptr<Cursor> PreSharing(GenQuery &query) override;
    int32_t Clean(const std::vector<std::string> &devices, int32_t mode, const std::string &tableName) override;
    int32_t Watch(int32_t origin, Watcher &watcher) override;
    int32_t Unwatch(int32_t origin, Watcher &watcher) override;
    int32_t RegisterDetailProgressObserver(DetailAsync async) override;
    int32_t UnregisterDetailProgressObserver() override;
    int32_t Close() override;
    int32_t AddRef() override;
    int32_t Release() override;
    int32_t BindSnapshots(std::shared_ptr<std::map<std::string, std::shared_ptr<Snapshot>>> bindAssets) override;
    int32_t MergeMigratedData(const std::string &tableName, VBuckets &&values) override;
    std::vector<std::string> GetWaterVersion(const std::string &deviceId) override;
    void MakeCursor(const std::map<std::string, Value> &entry);

private:
    std::shared_ptr<Cursor> cursor_ = nullptr;
};
} // namespace DistributedData
} // namespace OHOS
#endif