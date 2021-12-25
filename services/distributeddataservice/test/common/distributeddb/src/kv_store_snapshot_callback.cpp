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
#include "kv_store_snapshot_callback.h"

#include "distributeddb_log_print.h"

using namespace DistributedDB;

void KvStoreSnapshotCallback::Callback(DBStatus status, const std::vector<Entry> &entriesVec)
{
    this->status_ = status;
    this->entriesVec_ = entriesVec;
    MST_LOG("KvStoreSnapshotCallback status: %d, entriesVec.size(): %zu", status, entriesVec.size());
}

DBStatus KvStoreSnapshotCallback::GetStatus()
{
    return status_;
}

std::vector<Entry> KvStoreSnapshotCallback::GetEntries()
{
    return entriesVec_;
}