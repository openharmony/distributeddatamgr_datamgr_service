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

#include "db_change_data_mock.h"
namespace OHOS {
namespace DistributedData {
DBChangeDataMock::DBChangeDataMock(const std::vector<DBEntry> &inserts, const std::vector<DBEntry> &updates,
                                   const std::vector<DBEntry> &deletes)
{
    for (auto &entry : inserts) {
        entries_[INSERT].push_back(entry);
    }
    for (auto &entry : updates) {
        entries_[UPDATE].push_back(entry);
    }
    for (auto &entry : deletes) {
        entries_[DELETE].push_back(entry);
    }
}

const std::list<DBEntry> &DBChangeDataMock::GetEntriesInserted() const
{
    return entries_[INSERT];
}

const std::list<DBEntry> &DBChangeDataMock::GetEntriesUpdated() const
{
    return entries_[UPDATE];
}

const std::list<DBEntry> &DBChangeDataMock::GetEntriesDeleted() const
{
    return entries_[DELETE];
}

bool DBChangeDataMock::IsCleared() const
{
    return false;
}

bool DBChangeDataMock::AddEntry(DBEntry entry, int32_t type)
{
    entries_[type].push_back(std::move(entry));
    return false;
}
}
}