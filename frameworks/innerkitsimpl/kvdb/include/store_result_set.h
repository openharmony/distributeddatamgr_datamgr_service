/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_STORE_RESULT_SET_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_STORE_RESULT_SET_H
#include <memory>
#include <shared_mutex>
#include "convertor.h"
#include "kv_store_nb_delegate.h"
#include "kv_store_result_set.h"
#include "kvstore_result_set.h"
namespace OHOS::DistributedKv {
class StoreResultSet : public KvStoreResultSet {
public:
    using DBResultSet = DistributedDB::KvStoreResultSet;
    using DBStore = DistributedDB::KvStoreNbDelegate;
    using DBEntry = DistributedDB::Entry;
    StoreResultSet(DBResultSet *impl, std::shared_ptr<DBStore> dbStore, const Convertor &convert);
    ~StoreResultSet();
    int GetCount() const override;
    int GetPosition() const override;
    bool MoveToFirst() override;
    bool MoveToLast() override;
    bool MoveToNext() override;
    bool MoveToPrevious() override;
    bool Move(int offset) override;
    bool MoveToPosition(int position) override;
    bool IsFirst() const override;
    bool IsLast() const override;
    bool IsBeforeFirst() const override;
    bool IsAfterLast() const override;
    Status GetEntry(Entry &entry) const override;
    Status Close() override;

private:
    mutable std::shared_mutex mutex_;
    DBResultSet *impl_;
    std::shared_ptr<DBStore> dbStore_;
    const Convertor &convert_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_STORE_RESULT_SET_H
