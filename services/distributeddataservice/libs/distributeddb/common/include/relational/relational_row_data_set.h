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

#ifndef RELATIONAL_ROW_DATA_SET_H
#define RELATIONAL_ROW_DATA_SET_H

#include "parcel.h"
#include "relational_row_data.h"

namespace DistributedDB {
class RelationalRowDataSet {
public:
    RelationalRowDataSet();
    virtual ~RelationalRowDataSet();

    RelationalRowDataSet(const RelationalRowDataSet &) = delete;
    RelationalRowDataSet &operator=(const RelationalRowDataSet &) = delete;

    RelationalRowDataSet(RelationalRowDataSet &&) noexcept;
    RelationalRowDataSet &operator=(RelationalRowDataSet &&) noexcept;

    int GetSize() const;

    int CalcLength() const;

    int Serialize(Parcel &parcel) const;

    int DeSerialize(Parcel &parcel);

    void SetColNames(std::vector<std::string> &&colNames);

    void SetRowData(std::vector<RelationalRowData *> &&data);

    int Insert(RelationalRowData *rowData);

    void Clear();

    const std::vector<std::string> &GetColNames() const;

    const RelationalRowData *Get(int index) const;

    int Merge(RelationalRowDataSet &&rowDataSet);

private:
    std::vector<std::string> colNames_;
    std::vector<RelationalRowData *> data_;
    size_t serialLength_;
};
}  // namespace DistributedDB
#endif  // RELATIONAL_ROW_DATA_SET_H