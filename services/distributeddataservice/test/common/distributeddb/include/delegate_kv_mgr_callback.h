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
#ifndef DELEGATE_KV_MGR_CALLBACK_H
#define DELEGATE_KV_MGR_CALLBACK_H

#include "kv_store_delegate.h"

// DelegateKvMgrCallback conclude the Callback implements of function< void(DBStatus, KvStoreDelegate*)>
class DelegateKvMgrCallback {
public:
    DelegateKvMgrCallback() {}
    ~DelegateKvMgrCallback() {}

    // Delete the copy and assign constructors
    DelegateKvMgrCallback(const DelegateKvMgrCallback &callback) = delete;
    DelegateKvMgrCallback& operator=(const DelegateKvMgrCallback &callback) = delete;
    DelegateKvMgrCallback(DelegateKvMgrCallback &&callback) = delete;
    DelegateKvMgrCallback& operator=(DelegateKvMgrCallback &&callback) = delete;

    void Callback(DistributedDB::DBStatus status, DistributedDB::KvStoreDelegate *kvStoreDelegate);

    DistributedDB::DBStatus GetStatus();

    const DistributedDB::KvStoreDelegate *GetKvStore();

private:
    DistributedDB::DBStatus status_ = DistributedDB::DBStatus::INVALID_ARGS;
    DistributedDB::KvStoreDelegate *kvStoreDelegate_ = nullptr;
};

#endif // DELEGATE_KV_MGR_CALLBACK_H