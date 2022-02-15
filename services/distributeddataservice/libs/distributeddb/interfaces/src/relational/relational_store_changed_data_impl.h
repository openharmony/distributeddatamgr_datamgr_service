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

#ifndef RELATION_STORE_CHANGED_DATA_IMPL_H
#define RELATION_STORE_CHANGED_DATA_IMPL_H

#include <mutex>
#include "macro_utils.h"
#include "store_changed_data.h"

namespace DistributedDB {
class RelationalStoreChangedDataImpl : public StoreChangedData {
public:
    explicit RelationalStoreChangedDataImpl(const std::string &changedDevice) : changedDevice_(changedDevice) {}
    virtual ~RelationalStoreChangedDataImpl();

    DISABLE_COPY_ASSIGN_MOVE(RelationalStoreChangedDataImpl);

    std::string GetDataChangeDevice() const override;

    void GetStoreProperty(StoreProperty &storeProperty) const override;

    void SetStoreProperty(const StoreProperty &storeProperty);
private:
    mutable std::mutex mutex_;
    mutable std::string changedDevice_;
    StoreProperty storeProperty_;
};
} // namespace DistributedDB

#endif // RELATION_STORE_CHANGED_DATA_IMPL_H

