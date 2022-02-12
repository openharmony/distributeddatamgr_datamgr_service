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

#ifndef STORE_CHANGED_DATA_H
#define STORE_CHANGED_DATA_H

#include <list>
#include "store_types.h"

namespace DistributedDB {
struct StoreProperty {
    std::string userId;
    std::string appId;
    std::string storeId;
};
class StoreChangedData {
public:
    StoreChangedData() {}
    DB_API virtual ~StoreChangedData() {}

    // Interface for Getting the device whose data changed.
    DB_API virtual const std::string GetDataChangeDevice() const = 0;

    // Interface for Getting the store whose data changed.
    DB_API virtual void GetStoreProperty(StoreProperty &storeProperty) const = 0;
};
} // namespace DistributedDB

#endif // STORE_CHANGED_DATA_H
