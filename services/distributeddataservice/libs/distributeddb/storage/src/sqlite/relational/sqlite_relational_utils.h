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

#ifndef SQLITE_RELATIONAL_UTILS_H
#define SQLITE_RELATIONAL_UTILS_H

#include <vector>
#include "sqlite_import.h"
#include "data_value.h"

namespace DistributedDB {
class SQLiteRelationalUtils {
public:
    static int GetDataValueByType(sqlite3_stmt *statement, int cid, DataValue &value);

    static std::vector<DataValue> GetSelectValues(sqlite3_stmt *stmt);
};
} // namespace DistributedDB
#endif // SQLITE_RELATIONAL_UTILS_H
