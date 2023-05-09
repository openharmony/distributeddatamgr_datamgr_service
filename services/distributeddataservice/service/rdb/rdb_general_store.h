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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_GENERAL_STORE_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_GENERAL_STORE_H
#include <functional>
#include "relational_store_delegate.h"
#include "relational_store_manager.h"

#include "rdb_store.h"
#include "store/general_store.h"
#include "metadata/store_meta_data.h"
namespace OHOS::DistributedRdb {
class API_EXPORT RdbGeneralStore : public DistributedData::GeneralStore {
public:
    using Cursor = DistributedData::Cursor;
    using GenQuery = DistributedData::GenQuery;
    using VBucket = DistributedData::VBucket;
    using VBuckets = DistributedData::VBuckets;
    using Values = DistributedData::Values;
    using StoreMetaData = DistributedData::StoreMetaData;
    using RdbStore = OHOS::NativeRdb::RdbStore;
    using RdbDelegate =  DistributedDB::RelationalStoreDelegate;
    using RdbManager =  DistributedDB::RelationalStoreManager;
    RdbGeneralStore(const StoreMetaData &metaData);
    int32_t Close() override;
    int32_t Execute(const std::string &table, const std::string &sql) override;
    int32_t BatchInsert(const std::string &table, VBuckets &&values) override;
    int32_t BatchUpdate(const std::string &table, const std::string &sql, VBuckets &&values) override;
    int32_t Delete(const std::string &table, const std::string &sql, Values &&args) override;
    std::shared_ptr<Cursor> Query(const std::string &table, const std::string &sql, Values &&args) override;
    std::shared_ptr<Cursor> Query(const std::string &table, GenQuery &query) override;
    int32_t Watch(int32_t origin, Watcher &watcher) override;
    int32_t Unwatch(int32_t origin, Watcher &watcher) override;
    int32_t Sync(const Devices &devices, int32_t mode, GenQuery &query, Async async, int32_t wait) override;

private:
    NativeRdb::ValuesBucket Convert(DistributedData::VBucket &&bucket);
    NativeRdb::ValueObject Convert(DistributedData::Value &&value);
    DistributedData::Value Convert(NativeRdb::ValueObject &&rdbValue);

    RdbManager manager_;
    std::shared_ptr<RdbStore> store_;
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_RDB_GENERAL_STORE_H
