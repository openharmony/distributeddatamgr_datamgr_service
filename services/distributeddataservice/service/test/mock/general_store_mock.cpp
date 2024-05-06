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
int32_t GeneralStoreMock::Bind(Database &database, const std::map<uint32_t, BindInfo> &bindInfos)
{
    return 0;
}

bool GeneralStoreMock::IsBound()
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

int32_t GeneralStoreMock::SetTrackerTable(
    const std::string &tableName, const std::set<std::string> &trackerColNames, const std::string &extendColName)
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

std::shared_ptr<Cursor> GeneralStoreMock::Query(const std::string &table, GenQuery &query)
{
    return nullptr;
}

int32_t GeneralStoreMock::Sync(const Devices &devices, GenQuery &query, DetailAsync async, SyncParam &syncParm)
{
    return 0;
}

std::shared_ptr<Cursor> GeneralStoreMock::PreSharing(GenQuery &query)
{
    return nullptr;
}

int32_t GeneralStoreMock::Clean(const std::vector<std::string> &devices, int32_t mode, const std::string &tableName)
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

int32_t GeneralStoreMock::Close()
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

std::shared_ptr<Cursor> GeneralStoreMock::Query(const std::string &table, const std::string &sql, Values &&args)
{
    return cursor_;
}

std::vector<std::string> GeneralStoreMock::GetWaterVersion(const std::string &deviceId)
{
    return std::vector<std::string>();
}

void GeneralStoreMock::MakeCursor(const std::map<std::string, Value> &entry)
{
    auto resultSet = std::make_shared<CursorMock::ResultSet>(1, entry);
    cursor_ = std::make_shared<CursorMock>(resultSet);
}
} // namespace DistributedData
} // namespace OHOS