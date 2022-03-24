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

#ifndef I_SYNC_INTERFACE_H
#define I_SYNC_INTERFACE_H

#include <string>

#include "db_types.h"
#include "kvdb_properties.h"

namespace DistributedDB {
class ISyncInterface {
public:
    enum {
        SYNC_SVD = 1,   // Single version data
        SYNC_MVD,       // Multi version data
        SYNC_RELATION,  // Relation version data
    };

    // Constructor/Destructor.
    ISyncInterface() = default;
    virtual ~ISyncInterface() = default;

    // Get interface type of this kvdb.
    virtual int GetInterfaceType() const = 0;

    // Get the interface ref-count, in order to access asynchronously.
    virtual void IncRefCount() = 0;

    // Drop the interface ref-count.
    virtual void DecRefCount() = 0;

    // Get the identifier of this kvdb.
    virtual std::vector<uint8_t> GetIdentifier() const = 0;
    // Get the dual tuple identifier of this kvdb.
    virtual std::vector<uint8_t> GetDualTupleIdentifier() const = 0;

    // Get the max timestamp of all entries in database.
    virtual void GetMaxTimestamp(Timestamp &stamp) const = 0;

    // Get meta data associated with the given key.
    virtual int GetMetaData(const Key &key, Value &value) const = 0;

    // Put meta data as a key-value entry.
    virtual int PutMetaData(const Key &key, const Value &value) = 0;

    // Delete multiple meta data records in a transaction.
    virtual int DeleteMetaData(const std::vector<Key> &keys) = 0;

    // Delete multiple meta data records with key prefix in a transaction.
    virtual int DeleteMetaDataByPrefixKey(const Key &keyPrefix) const = 0;

    // Get all meta data keys.
    virtual int GetAllMetaKeys(std::vector<Key> &keys) const = 0;

    virtual const KvDBProperties &GetDbProperties() const = 0;
};
} // namespace DistributedDB

#endif // I_SYNC_INTERFACE_H