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

#ifndef RELATIONAL_ROW_DATA_H
#define RELATIONAL_ROW_DATA_H

#include "parcel.h"
#include "db_types.h"

namespace DistributedDB {
class RelationalRowData {
public:
    RelationalRowData() = default;
    virtual ~RelationalRowData() = default;

    virtual std::size_t GetColSize() = 0;

    virtual int CalcLength() const = 0;
    virtual int Serialize(Parcel &parcel) const = 0;
    virtual int DeSerialize(Parcel &parcel) = 0;

    virtual int GetType(int index, StorageType &type) const = 0;
    virtual int Get(int index, int64_t &value) const = 0;
    virtual int Get(int index, double &value) const = 0;
    virtual int Get(int index, std::string &value) const = 0;
    virtual int Get(int index, std::vector<uint8_t> &value) const = 0;

    static void Release(std::vector<RelationalRowData *> &entries)
    {
        for (auto &entry : entries) {
            delete entry;
            entry = nullptr;
        }
        entries.clear();
    };
};
}  // namespace DistributedDB
#endif  // RELATIONAL_ROW_DATA_H