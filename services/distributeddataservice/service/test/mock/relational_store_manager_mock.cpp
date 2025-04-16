/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#define LOG_TAG "MockRelationalStoreManager"
#include <memory>

#include "relational_store_delegate_mock.h"
#include "relational_store_manager.h"
namespace DistributedDB {
DB_API DBStatus RelationalStoreManager::OpenStore(const std::string &path, const std::string &storeId,
    const RelationalStoreDelegate::Option &option, RelationalStoreDelegate *&delegate)
{
    delegate = std::make_shared<MockRelationalStoreDelegate>().get();
    if (storeId == "mock") {
        return OK;
    }
    return DB_ERROR;
}

DB_API DBStatus RelationalStoreManager::CloseStore(RelationalStoreDelegate *store)
{
    store = nullptr;
    return OK;
}
} // namespace DistributedDB