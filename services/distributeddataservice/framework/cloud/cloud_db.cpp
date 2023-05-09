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

#include "cloud/cloud_db.h"
namespace OHOS::DistributedData {
int32_t CloudDB::Close()
{
    return E_NOT_SUPPORT;
}

int32_t CloudDB::Execute(const std::string &table, const std::string &sql)
{
    return E_NOT_SUPPORT;
}

int32_t CloudDB::BatchInsert(const std::string &table, VBuckets &&values)
{
    return E_NOT_SUPPORT;
}

int32_t CloudDB::BatchUpdate(const std::string &table, const std::string &sql, VBuckets &&values)
{
    return E_NOT_SUPPORT;
}

int32_t CloudDB::Delete(const std::string &table, const std::string &sql, Values &&args)
{
    return E_NOT_SUPPORT;
}

std::shared_ptr<Cursor> CloudDB::Query(const std::string &table, const std::string &sql, Values &&args)
{
    return nullptr;
}

std::shared_ptr<Cursor> CloudDB::Query(const std::string &table, GenQuery &query)
{
    return nullptr;
}

int32_t CloudDB::Sync(const Devices &devices, int32_t mode, GenQuery &query, Async async, int32_t wait)
{
    return E_NOT_SUPPORT;
}

int32_t CloudDB::Watch(int32_t origin, Watcher &watcher)
{
    return E_NOT_SUPPORT;
}

int32_t CloudDB::Unwatch(int32_t origin, Watcher &watcher)
{
    return E_NOT_SUPPORT;
}

int32_t CloudDB::Execute(const std::string &table, const std::string &sql, const VBucket &extend)
{
    return E_NOT_SUPPORT;
}

int32_t CloudDB::BatchInsert(const std::string &table, VBuckets &&values, VBuckets &extends)
{
    return E_NOT_SUPPORT;
}

int32_t CloudDB::BatchUpdate(const std::string &table, VBuckets &&values, const VBuckets &extends)
{
    return E_NOT_SUPPORT;
}

int32_t CloudDB::BatchDelete(const std::string &table, const VBuckets &extends)
{
    return E_NOT_SUPPORT;
}

std::shared_ptr<Cursor> CloudDB::Query(const std::string &table, const VBucket &extend)
{
    return nullptr;
}

int32_t CloudDB::Lock()
{
    return E_NOT_SUPPORT;
}

int32_t CloudDB::Heartbeat()
{
    return E_NOT_SUPPORT;
}

int32_t CloudDB::Unlock()
{
    return E_NOT_SUPPORT;
}
} // namespace OHOS::DistributedData
