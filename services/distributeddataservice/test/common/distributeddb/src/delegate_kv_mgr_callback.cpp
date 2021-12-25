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
#include "delegate_kv_mgr_callback.h"

void DelegateKvMgrCallback::Callback(DistributedDB::DBStatus status, DistributedDB::KvStoreDelegate *kvStoreDelegate)
{
    this->status_ = status;
    this->kvStoreDelegate_ = kvStoreDelegate;
}

DistributedDB::DBStatus DelegateKvMgrCallback::GetStatus()
{
    return status_;
}

const DistributedDB::KvStoreDelegate *DelegateKvMgrCallback::GetKvStore()
{
    return kvStoreDelegate_;
}