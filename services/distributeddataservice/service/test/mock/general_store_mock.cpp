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
#include "general_store_mock.h"
namespace OHOS {
namespace DistributedData {
int32_t GeneralStoreMock::Bind(const Database &database, const std::map<uint32_t, BindInfo> &bindInfos,
    const CloudConfig &config)
{
    return 0;
}

bool GeneralStoreMock::IsBound(uint32_t user)
{
    return false;
}

int32_t GeneralStoreMock::Execute(const std::string &table, const std::string &sql)
{
    return 0;
}

int32_t GeneralStoreMock::SetDistributedTables(
    const std::vector<std::string> &tables, int32_t type, const std::vector<Reference> &references)
{
    return 0;
}

int32_t GeneralStoreMock::SetTrackerTable(const std::string &tableName,
    const std::set<std::string> &trackerColNames, const std::set<std::string> &extendColNames, bool isForceUpgrade)
{
    return 0;
}

int32_t GeneralStoreMock::Insert(const std::string &table, VBuckets &&values)
{
    return 0;
}

int32_t GeneralStoreMock::Update(const std::string &table, const std::string &setSql, Values &&values,
    const std::string &whereSql, Values &&conditions)
{
    return 0;
}

int32_t GeneralStoreMock::Replace(const std::string &table, VBucket &&value)
{
    return 0;
}

int32_t GeneralStoreMock::Delete(const std::string &table, const std::string &sql, Values &&args)
{
    return 0;
}

std::pair<int32_t, std::shared_ptr<Cursor>> GeneralStoreMock::Query(const std::string &table, GenQuery &query)
{
    return {GeneralError::E_NOT_SUPPORT, nullptr};
}

std::pair<int32_t, int32_t> GeneralStoreMock::Sync(const Devices &devices, GenQuery &query,
    DetailAsync async, const SyncParam &syncParm)
{
    if (!async) {
        return { GeneralError::E_OK, 0 };
    }
    std::map<std::string, GenProgressDetail> details;
    for (auto &device : devices) {
        details[device] = { .progress = SYNC_FINISH, .code = 0, .dbCode = 0 };
    }
    async(details);
    return { GeneralError::E_OK, 0 };
}

std::pair<int32_t, std::shared_ptr<Cursor>> GeneralStoreMock::PreSharing(GenQuery &query)
{
    return {GeneralError::E_NOT_SUPPORT, nullptr};
}

int32_t GeneralStoreMock::Clean(const std::vector<std::string> &devices, int32_t mode, const std::string &tableName)
{
    return 0;
}

int32_t GeneralStoreMock::Clean(const std::string &device, int32_t mode, const std::vector<std::string> &tableList)
{
    return 0;
}

int32_t GeneralStoreMock::Watch(int32_t origin, Watcher &watcher)
{
    return 0;
}

int32_t GeneralStoreMock::Unwatch(int32_t origin, Watcher &watcher)
{
    return 0;
}

int32_t GeneralStoreMock::RegisterDetailProgressObserver(DetailAsync async)
{
    return 0;
}

int32_t GeneralStoreMock::UnregisterDetailProgressObserver()
{
    return 0;
}

int32_t GeneralStoreMock::Close(bool isForce)
{
    return 0;
}

int32_t GeneralStoreMock::AddRef()
{
    return 0;
}

int32_t GeneralStoreMock::Release()
{
    return 0;
}

int32_t GeneralStoreMock::BindSnapshots(std::shared_ptr<std::map<std::string, std::shared_ptr<Snapshot>>> bindAssets)
{
    return 0;
}

int32_t GeneralStoreMock::MergeMigratedData(const std::string &tableName, VBuckets &&values)
{
    return 0;
}

int32_t GeneralStoreMock::CleanTrackerData(const std::string &tableName, int64_t cursor)
{
    return 0;
}

std::pair<int32_t, std::shared_ptr<Cursor>> GeneralStoreMock::Query(const std::string &table, const std::string &sql,
    Values &&args)
{
    return {GeneralError::E_OK, cursor_};
}

void GeneralStoreMock::SetExecutor(std::shared_ptr<Executor> executor) {}

std::pair<int32_t, uint32_t> GeneralStoreMock::LockCloudDB()
{
    return { E_OK, 0 };
}

int32_t GeneralStoreMock::UnLockCloudDB()
{
    return E_OK;
}

int32_t GeneralStoreMock::UpdateDBStatus()
{
    return dbStatus_;
}

void GeneralStoreMock::SetMockCursor(const std::map<std::string, Value> &entry)
{
    auto resultSet = std::make_shared<CursorMock::ResultSet>(1, entry);
    cursor_ = std::make_shared<CursorMock>(resultSet);
}

void GeneralStoreMock::SetMockDBStatus(int32_t dbStatus)
{
    dbStatus_ = dbStatus;
}

std::pair<int32_t, int64_t> GeneralStoreMock::Insert(const std::string &table, VBucket &&value,
    ConflictResolution resolution)
{
    return { E_OK, E_OK };
}

std::pair<int32_t, int64_t> GeneralStoreMock::BatchInsert(const std::string &table, VBuckets &&values,
    ConflictResolution resolution)
{
    return { E_OK, E_OK };
}

std::pair<int32_t, int64_t> GeneralStoreMock::Update(GenQuery &query, VBucket &&value, ConflictResolution resolution)
{
    return { E_OK, E_OK };
}

std::pair<int32_t, int64_t> GeneralStoreMock::Delete(GenQuery &query)
{
    return { E_OK, E_OK };
}
std::pair<int32_t, Value> GeneralStoreMock::Execute(const std::string &sql, Values &&args)
{
    return { E_OK, E_OK };
}
std::pair<int32_t, std::shared_ptr<Cursor>> GeneralStoreMock::Query(const std::string &sql, Values &&args,
    bool preCount)
{
    return { E_OK, nullptr };
}
std::pair<int32_t, std::shared_ptr<Cursor>> GeneralStoreMock::Query(GenQuery &query,
    const std::vector<std::string> &columns, bool preCount)
{
    return { E_OK, nullptr };
}

int32_t GeneralStoreMock::StopCloudSync()
{
    return 0;
}

int32_t GeneralStoreMock::OnSyncTrigger(const std::string &storeId, int32_t triggerMode)
{
    return E_OK;
}
} // namespace DistributedData
} // namespace OHOS