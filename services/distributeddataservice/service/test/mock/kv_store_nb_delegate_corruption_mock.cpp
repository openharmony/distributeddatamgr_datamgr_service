/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "kv_store_nb_delegate_corruption_mock.h"

#include "store_types.h"
namespace DistributedDB {
DBStatus KvStoreNbDelegateCorruptionMock::Get(const Key &key, Value &value) const
{
    return DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB;
}

DBStatus KvStoreNbDelegateCorruptionMock::GetEntries(const Query &query, std::vector<Entry> &entries) const
{
    return DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB;
}

DBStatus KvStoreNbDelegateCorruptionMock::Put(const Key &key, const Value &value)
{
    return DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB;
}

DBStatus KvStoreNbDelegateCorruptionMock::PutBatch(const std::vector<Entry> &entries)
{
    return DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB;
}

DBStatus KvStoreNbDelegateCorruptionMock::GetLocal(const Key &key, Value &value) const
{
    return DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB;
}

DBStatus KvStoreNbDelegateCorruptionMock::PutLocal(const Key &key, const Value &value)
{
    return DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB;
}

DBStatus KvStoreNbDelegateCorruptionMock::DeleteLocal(const Key &key)
{
    return DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB;
}
} // namespace DistributedDB