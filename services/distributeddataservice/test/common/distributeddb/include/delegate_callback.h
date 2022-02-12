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
#ifndef DELEGATE_CALLBACK_H
#define DELEGATE_CALLBACK_H
#include "kv_store_snapshot_delegate.h"
#include "types_export.h"

// DelegateCallback conclude the Callback implements of function< void(DBStatus, KvStoreSnapshotDelegate*)>
class DelegateCallback {
public:
    DelegateCallback() {}
    ~DelegateCallback() {}

    // Delete the copy and assign constructors
    DelegateCallback(const DelegateCallback &callback) = delete;
    DelegateCallback& operator=(const DelegateCallback &callback) = delete;
    DelegateCallback(DelegateCallback &&callback) = delete;
    DelegateCallback& operator=(DelegateCallback &&callback) = delete;

    void Callback(DistributedDB::DBStatus status, DistributedDB::KvStoreSnapshotDelegate *kvStoreSnapshotDelegate);
    void CallbackKv(DistributedDB::DBStatus status, const DistributedDB::Value &value);

    DistributedDB::DBStatus GetStatus();
    const DistributedDB::Value &GetValue();

    const DistributedDB::KvStoreSnapshotDelegate *GetKvStoreSnapshot()
    {
        return kvStoreSnapshotDelegate_;
    }

private:
    DistributedDB::DBStatus status_ = DistributedDB::DBStatus::INVALID_ARGS;
    DistributedDB::Value value_ = {};
    DistributedDB::KvStoreSnapshotDelegate *kvStoreSnapshotDelegate_ = nullptr;
};

#endif // DELEGATE_CALLBACK_H