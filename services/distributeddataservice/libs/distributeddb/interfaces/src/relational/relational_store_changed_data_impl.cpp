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

#include "relational_store_changed_data_impl.h"

namespace DistributedDB {
RelationalStoreChangedDataImpl::~RelationalStoreChangedDataImpl()
{
}

DB_API std::string RelationalStoreChangedDataImpl::GetDataChangeDevice() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    // add get changedDevice_ code;
    return changedDevice_;
}

DB_API void RelationalStoreChangedDataImpl::GetStoreProperty(StoreProperty &storeProperty) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    storeProperty = storeProperty_;
}

void RelationalStoreChangedDataImpl::SetStoreProperty(const StoreProperty &storeProperty)
{
    std::lock_guard<std::mutex> lock(mutex_);
    storeProperty_ = storeProperty;
}
} // namespace DistributedDB

