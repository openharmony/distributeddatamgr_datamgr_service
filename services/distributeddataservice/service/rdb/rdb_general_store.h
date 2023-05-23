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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_GENERAL_STORE_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_GENERAL_STORE_H
#include <functional>
#include "relational_store_delegate.h"
#include "relational_store_manager.h"

#include "rdb_store.h"
#include "store/general_store.h"
#include "metadata/store_meta_data.h"
namespace OHOS::DistributedRdb {
class RdbGeneralStore : public DistributedData::GeneralStore {
public:
    using Cursor = DistributedData::Cursor;
    using GenQuery = DistributedData::GenQuery;
    using VBucket = DistributedData::VBucket;
    using VBuckets = DistributedData::VBuckets;
    using Value = DistributedData::Value;
    using Values = DistributedData::Values;
    using StoreMetaData = DistributedData::StoreMetaData;
    using SchemaMeta = DistributedData::SchemaMeta;
    using CloudDB = DistributedData::CloudDB;
    using RdbStore = OHOS::NativeRdb::RdbStore;
    using RdbDelegate = DistributedDB::RelationalStoreDelegate;
    using RdbManager = DistributedDB::RelationalStoreManager;

    explicit RdbGeneralStore(const StoreMetaData &meta);
    int32_t Bind(std::shared_ptr<CloudDB> cloudDb) override;
    int32_t SetSchema(const SchemaMeta &schemaMeta) override;
    int32_t Execute(const std::string &table, const std::string &sql) override;
    int32_t BatchInsert(const std::string &table, VBuckets &&values) override;
    int32_t BatchUpdate(const std::string &table, const std::string &sql, VBuckets &&values) override;
    int32_t Delete(const std::string &table, const std::string &sql, Values &&args) override;
    std::shared_ptr<Cursor> Query(const std::string &table, const std::string &sql, Values &&args) override;
    std::shared_ptr<Cursor> Query(const std::string &table, GenQuery &query) override;
    int32_t Sync(const Devices &devices, int32_t mode, GenQuery &query, Async async, int32_t wait) override;
    int32_t Watch(int32_t origin, Watcher &watcher) override;
    int32_t Unwatch(int32_t origin, Watcher &watcher) override;
    int32_t Close() override;

private:
    static constexpr uint32_t ITERATE_TIMES = 10000;

    RdbManager manager_;
    RdbDelegate *delegate_ = nullptr;
    std::shared_ptr<RdbStore> store_;
    std::shared_ptr<CloudDB> cloudDb_;
    Watcher *watcher_ = nullptr;
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_GENERAL_STORE_H
