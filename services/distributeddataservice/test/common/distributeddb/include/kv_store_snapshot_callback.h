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
#ifndef KV_STORE_SNAPSHOT_CALLBACK_H
#define KV_STORE_SNAPSHOT_CALLBACK_H

#include "store_types.h"
#include "types_export.h"

class KvStoreSnapshotCallback {
public:
    KvStoreSnapshotCallback() {}
    ~KvStoreSnapshotCallback() {}
    /**
     * @tc.steps: step1. Delete the copy and assign constructors.
     * @tc.expected: step1. operate successfully.
     */
    KvStoreSnapshotCallback(const KvStoreSnapshotCallback &callback) = delete;
    KvStoreSnapshotCallback& operator=(const KvStoreSnapshotCallback &callback) = delete;
    KvStoreSnapshotCallback(KvStoreSnapshotCallback &&callback) = delete;
    KvStoreSnapshotCallback& operator=(KvStoreSnapshotCallback &&callback) = delete;

    void Callback(DistributedDB::DBStatus status, const std::vector<DistributedDB::Entry> &entriesVec);
    DistributedDB::DBStatus GetStatus();
    std::vector<DistributedDB::Entry> GetEntries();

private:
    DistributedDB::DBStatus status_ = DistributedDB::DBStatus::INVALID_ARGS;
    std::vector<DistributedDB::Entry> entriesVec_ = {};
};
#endif // KV_STORE_SNAPSHOT_CALLBACK_H