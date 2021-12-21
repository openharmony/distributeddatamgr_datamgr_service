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
#ifndef I_RELATIONAL_STORE_H
#define I_RELATIONAL_STORE_H
#ifdef RELATIONAL_STORE

#include <functional>
#include <string>

#include "ref_object.h"
#include "kvdb_properties.h"
#include "relational_store_connection.h"

namespace DistributedDB {
class IRelationalStore : public virtual RefObject {
public:
    IRelationalStore() = default;
    ~IRelationalStore() override = default;
    DISABLE_COPY_ASSIGN_MOVE(IRelationalStore);

    // Open the database.
    virtual int Open(const RelationalDBProperties &kvDBProp) = 0;

    virtual void WakeUpSyncer() = 0;

    // Create a db connection.
    virtual RelationalStoreConnection *GetDBConnection(int &errCode) = 0;
};
} // namespace DistributedDB

#endif
#endif // I_RELATIONAL_STORE_H