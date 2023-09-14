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
#include "general_store_mock.h"

namespace OHOS {
namespace DistributedData {
StoreMetaData GeneralStoreMock::GetStoreMetaData()
{
    return StoreMetaData();
}
int32_t GeneralStoreMock::Clean(const std::vector<std::string> &devices, int32_t mode, const std::string &tableName)
{
    return 0;
}
GeneralStoreMock::GeneralStoreMock(const StoreMetaData &meta) {}
GeneralStoreMock::~GeneralStoreMock() {}
int32_t GeneralStoreMock::Bind(const Database &database, GeneralStore::BindInfo bindInfo)
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
int32_t GeneralStoreMock::SetDistributedTables(const std::vector<std::string> &tables, int32_t type)
{
    return 0;
}
int32_t GeneralStoreMock::BatchInsert(const std::string &table, VBuckets &&values)
{
    return 0;
}
int32_t GeneralStoreMock::BatchUpdate(const std::string &table, const std::string &sql, VBuckets &&values)
{
    return 0;
}
int32_t GeneralStoreMock::Delete(const std::string &table, const std::string &sql, Values &&args)
{
    return 0;
}
std::shared_ptr<Cursor> GeneralStoreMock::Query(const std::string &table, const std::string &sql, Values &&args)
{
    return std::shared_ptr<Cursor>();
}
std::shared_ptr<Cursor> GeneralStoreMock::Query(const std::string &table, GenQuery &query)
{
    return std::shared_ptr<Cursor>();
}
int32_t GeneralStoreMock::Sync(const GeneralStore::Devices &devices, int32_t mode, GenQuery &query,
    GeneralStore::DetailAsync async, int32_t wait)
{
    return 0;
}

int32_t GeneralStoreMock::Watch(int32_t origin, Watcher &watcher)
{
    if (origin != Watcher::Origin::ORIGIN_ALL || watcher_ != nullptr) {
        return GeneralError::E_INVALID_ARGS;
    }

    watcher_ = &watcher;
    return GeneralError::E_OK;
}

int32_t GeneralStoreMock::Unwatch(int32_t origin, Watcher &watcher)
{
    if (origin != Watcher::Origin::ORIGIN_ALL || watcher_ != &watcher) {
        return GeneralError::E_INVALID_ARGS;
    }

    watcher_ = nullptr;
    return GeneralError::E_OK;
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

int32_t GeneralStoreMock::OnChange(const GeneralWatcher::Origin &origin, const GeneralWatcher::PRIFields &primaries,
    GeneralWatcher::ChangeInfo &&changeInfo)
{
    if (watcher_ != nullptr) {
        watcher_->OnChange(origin, primaries, std::move(changeInfo));
        return GeneralError::E_OK;
    }
    return GeneralError::E_ERROR;
}
} // namespace DistributedData
} // namespace OHOS