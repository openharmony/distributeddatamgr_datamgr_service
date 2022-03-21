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

#define LOG_TAG "RdbStoreObserverImpl"

#include "rdb_store_observer_impl.h"
#include "rdb_service_impl.h"
#include "log_print.h"

namespace OHOS::DistributedRdb {
RdbStoreObserverImpl::RdbStoreObserverImpl(RdbServiceImpl* owner, pid_t pid)
    : pid_(pid), owner_(owner)
{
    ZLOGI("construct");
}

RdbStoreObserverImpl::~RdbStoreObserverImpl()
{
    ZLOGI("destroy");
}

void RdbStoreObserverImpl::OnChange(const DistributedDB::StoreChangedData &data)
{
    ZLOGI("enter");
    if (owner_ != nullptr) {
        owner_->OnDataChange(pid_, data);
    }
}
} // namespace OHOS::DistributedRdb
