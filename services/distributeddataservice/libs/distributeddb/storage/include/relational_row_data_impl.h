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

#ifndef RELATIONAL_ROW_DATA_IMPL_H
#define RELATIONAL_ROW_DATA_IMPL_H

#include "data_transformer.h"
#include "relational_row_data.h"

namespace DistributedDB {
class RelationalRowDataImpl : public RelationalRowData {
public:
    RelationalRowDataImpl() = default;
    explicit RelationalRowDataImpl(RowData &&rowData) : data_(std::move(rowData)) {}
    ~RelationalRowDataImpl() override {}

    std::size_t GetColSize() override;

    int CalcLength() const override;
    int Serialize(Parcel &parcel) const override;
    int DeSerialize(Parcel &parcel) override;

    int GetType(int index, StorageType &type) const override;
    int Get(int index, int64_t &value) const override;
    int Get(int index, double &value) const override;
    int Get(int index, std::string &value) const override;
    int Get(int index, std::vector<uint8_t> &value) const override;

private:
    RowData data_;
};
}  // namespace DistributedDB
#endif  // RELATIONAL_ROW_DATA_IMPL_H