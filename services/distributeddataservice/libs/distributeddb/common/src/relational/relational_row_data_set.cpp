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
#ifdef RELATIONAL_STORE
#include "relational_row_data_set.h"
#include "relational_row_data_impl.h"

namespace DistributedDB {
RelationalRowDataSet::RelationalRowDataSet() : serialLength_(Parcel::GetUInt32Len() + Parcel::GetUInt32Len()) {}

RelationalRowDataSet::~RelationalRowDataSet()
{
    RelationalRowData::Release(data_);
}

RelationalRowDataSet::RelationalRowDataSet(RelationalRowDataSet &&r) noexcept
{
    if (&r == this) {
        return;
    }

    colNames_ = std::move(r.colNames_);
    data_ = std::move(r.data_);
    serialLength_ = r.serialLength_;

    r.serialLength_ = Parcel::GetUInt32Len() + Parcel::GetUInt32Len();
}

RelationalRowDataSet &RelationalRowDataSet::operator=(RelationalRowDataSet &&r) noexcept
{
    if (&r == this) {
        return *this;
    }

    colNames_ = std::move(r.colNames_);
    data_ = std::move(r.data_);
    serialLength_ = r.serialLength_;

    r.serialLength_ = Parcel::GetUInt32Len() + Parcel::GetUInt32Len();
    return *this;
}

int RelationalRowDataSet::GetSize() const
{
    return data_.size();
}

int RelationalRowDataSet::CalcLength() const
{
    if (serialLength_ > static_cast<size_t>(INT32_MAX)) {
        return 0;
    }
    return static_cast<int>(Parcel::GetEightByteAlign(serialLength_));
}

int RelationalRowDataSet::Serialize(Parcel &parcel) const
{
    if (serialLength_ > static_cast<size_t>(INT32_MAX) || parcel.IsError()) {
        return -E_PARSE_FAIL;
    }

    (void)parcel.WriteUInt32(colNames_.size());
    for (const auto &colName : colNames_) {
        (void)parcel.WriteString(colName);
    }
    (void)parcel.WriteUInt32(data_.size());
    for (const auto &rowData : data_) {
        rowData->Serialize(parcel);
    };

    (void)parcel.EightByteAlign();
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }
    return E_OK;
};

int RelationalRowDataSet::DeSerialize(Parcel &parcel)
{
    Clear();
    uint32_t size = 0;
    parcel.ReadUInt32(size);
    if (parcel.IsError() || size > DBConstant::MAX_REMOTEDATA_SIZE / parcel.GetStringLen(std::string {})) {
        return -E_PARSE_FAIL;
    }
    while (size-- > 0) {
        std::string str;
        parcel.ReadString(str);
        if (parcel.IsError()) {
            return -E_PARSE_FAIL;
        }
        colNames_.emplace_back(std::move(str));
    }

    parcel.ReadUInt32(size);
    if (parcel.IsError() || size > DBConstant::MAX_REMOTEDATA_SIZE / parcel.GetUInt32Len()) {
        return -E_PARSE_FAIL;
    }
    while (size-- > 0) {
        auto rowData = new (std::nothrow) RelationalRowDataImpl();
        if (rowData == nullptr) {
            return -E_OUT_OF_MEMORY;
        }

        if (rowData->DeSerialize(parcel) != E_OK) {
            return -E_PARSE_FAIL;
        }
        data_.push_back(rowData);
    }
    (void)parcel.EightByteAlign();
    return E_OK;
};

void RelationalRowDataSet::SetColNames(std::vector<std::string> &&colNames)
{
    for (const auto &colName : colNames_) {
        serialLength_ -= Parcel::GetStringLen(colName);
    }
    colNames_ = std::move(colNames);
    for (const auto &colName : colNames_) {
        serialLength_ += Parcel::GetStringLen(colName);
    }
}

void RelationalRowDataSet::SetRowData(std::vector<RelationalRowData *> &&data)
{
    for (const auto &rowData : data_) {
        serialLength_ -= rowData->CalcLength();
    }
    data_ = data;
    for (const auto &rowData : data_) {
        serialLength_ += rowData->CalcLength();
    };
}

int RelationalRowDataSet::Insert(RelationalRowData *rowData)
{
    if (rowData == nullptr) {
        return -E_INVALID_ARGS;
    }
    data_.push_back(rowData);
    serialLength_ += rowData->CalcLength();
    return E_OK;
}

void RelationalRowDataSet::Clear()
{
    colNames_.clear();
    RelationalRowData::Release(data_);
    serialLength_ = Parcel::GetUInt32Len() + Parcel::GetUInt32Len();
}

const std::vector<std::string> &RelationalRowDataSet::GetColNames() const
{
    return colNames_;
}

const RelationalRowData *RelationalRowDataSet::Get(int index) const
{
    if (index < 0 || index >= static_cast<int>(data_.size())) {
        return nullptr;
    }
    return data_.at(index);
}

int RelationalRowDataSet::Merge(RelationalRowDataSet &&rowDataSet)
{
    if (this == &rowDataSet) {
        return -E_INVALID_ARGS;
    }

    if (!rowDataSet.colNames_.empty()) {
        if (colNames_.empty()) {
            colNames_ = std::move(rowDataSet.colNames_);
        } else if (colNames_ != rowDataSet.colNames_) {
            return -E_INVALID_ARGS;
        }
    }

    data_.insert(data_.end(), rowDataSet.data_.begin(), rowDataSet.data_.end());
    rowDataSet.data_.clear();
    rowDataSet.serialLength_ = Parcel::GetUInt32Len() + Parcel::GetUInt32Len();
    return E_OK;
}
}
#endif