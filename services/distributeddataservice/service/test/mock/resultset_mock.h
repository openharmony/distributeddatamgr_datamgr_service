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
#ifndef RESULT_SET_MOCK_H
#define RESULT_SET_MOCK_H
#include "rdb_general_store.h"
#include <gmock/gmock.h>
#include <memory>
namespace DistributedDB {
class MockResultSet : public DistributedDB::ResultSet {
    MOCK_METHOD((int), GetCount, (), (const));
    MOCK_METHOD((int), GetPosition, (), (const));
    MOCK_METHOD((bool), MoveToFirst, ());
    MOCK_METHOD((bool), MoveToLast, ());
    MOCK_METHOD((bool), MoveToNext, ());
    MOCK_METHOD((bool), MoveToPrevious, ());
    MOCK_METHOD((bool), Move, (int));
    MOCK_METHOD((bool), MoveToPosition, (int));
    MOCK_METHOD((bool), IsFirst, (), (const));
    MOCK_METHOD((bool), IsLast, (), (const));
    MOCK_METHOD((bool), IsBeforeFirst, (), (const));
    MOCK_METHOD((bool), IsAfterLast, (), (const));
    MOCK_METHOD((bool), IsClosed, (), (const));
    MOCK_METHOD((void), Close, ());
    MOCK_METHOD((DBStatus), GetEntry, (Entry &), (const));
    MOCK_METHOD((void), GetColumnNames, (std::vector<std::string> &), (const));
    MOCK_METHOD((DBStatus), GetColumnType, (int, ColumnType &), (const));
    MOCK_METHOD((DBStatus), GetColumnIndex, (const std::string &, int &), (const));
    MOCK_METHOD((DBStatus), GetColumnName, (int, std::string &), (const));
    MOCK_METHOD((DBStatus), Get, (int, std::vector<uint8_t> &), (const));
    MOCK_METHOD((DBStatus), Get, (int, std::string &), (const));
    MOCK_METHOD((DBStatus), Get, (int, int64_t &), (const));
    MOCK_METHOD((DBStatus), Get, (int, double &), (const));
    MOCK_METHOD((DBStatus), IsColumnNull, (int, bool &), (const));
    DBStatus GetRow(std::map<std::string, VariantData> &data) const override
    {
        return DBStatus::OK;
    }
};
} // namespace DistributedDB
#endif