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

#ifndef OHOS_DISTRIBUTEDDATA_SERVICE_TEST_DB_CHANGE_DATA_MOCK_H
#define OHOS_DISTRIBUTEDDATA_SERVICE_TEST_DB_CHANGE_DATA_MOCK_H
#include "kv_store_changed_data.h"
namespace OHOS {
namespace DistributedData {
using DBEntry = DistributedDB::Entry;
class DBChangeDataMock : public DistributedDB::KvStoreChangedData {
public:
    enum Action : int32_t {
        INSERT,
        UPDATE,
        DELETE,
        BUTT
    };
    DBChangeDataMock(const std::vector<DBEntry> &inserts, const std::vector<DBEntry> &updates,
        const std::vector<DBEntry> &deletes);
    const std::list<DBEntry> &GetEntriesInserted() const override;
    const std::list<DBEntry> &GetEntriesUpdated() const override;
    const std::list<DBEntry> &GetEntriesDeleted() const override;
    bool IsCleared() const override;

    bool AddEntry(DBEntry entry, int32_t type = DELETE);

private:
    std::list<DBEntry> entries_[BUTT];
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTEDDATA_SERVICE_TEST_DB_CHANGE_DATA_MOCK_H
