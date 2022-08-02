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
#ifdef RELATIONAL_STORE
#include "relational_row_data_impl.h"

namespace DistributedDB {
std::size_t RelationalRowDataImpl::GetColSize()
{
    return data_.size();
}

int RelationalRowDataImpl::CalcLength() const
{
    size_t length = Parcel::GetUInt32Len();  // For save the count.
    for (const auto &dataValue : data_) {
        length += Parcel::GetUInt32Len();  // For save the dataValue's type.
        length += DataTransformer::CalDataValueLength(dataValue);
        if (length > static_cast<size_t>(INT_MAX)) {
            return 0;
        }
    }
    return static_cast<int>(Parcel::GetEightByteAlign(length));
}

int RelationalRowDataImpl::Serialize(Parcel &parcel) const
{
    parcel.WriteUInt32(data_.size());
    for (const auto &dataValue : data_) {
        if (DataTransformer::SerializeDataValue(dataValue, parcel) != E_OK) {
            return -E_PARSE_FAIL;
        }
    }
    parcel.EightByteAlign();
    return E_OK;
}

int RelationalRowDataImpl::DeSerialize(Parcel &parcel)
{
    uint32_t size = 0;
    (void)parcel.ReadUInt32(size);
    if (parcel.IsError() ||
        size > DBConstant::MAX_REMOTEDATA_SIZE / DataTransformer::CalDataValueLength(DataValue {})) {
        return -E_PARSE_FAIL;
    }
    while (size-- > 0u) {
        DataValue value;
        if (DataTransformer::DeserializeDataValue(value, parcel) != E_OK) {
            return -E_PARSE_FAIL;
        }
        data_.emplace_back(std::move(value));
    }
    parcel.EightByteAlign();
    return E_OK;
}

int RelationalRowDataImpl::GetType(int index, StorageType &type) const
{
    if (index < 0 || index >= static_cast<int>(data_.size())) {
        return -E_NONEXISTENT;
    }
    type = data_.at(index).GetType();
    return E_OK;
}

int RelationalRowDataImpl::Get(int index, int64_t &value) const
{
    if (index < 0 || index >= static_cast<int>(data_.size())) {
        return -E_NONEXISTENT;
    }
    if (data_.at(index).GetInt64(value) != E_OK) {
        return -E_TYPE_MISMATCH;
    }
    return E_OK;
}

int RelationalRowDataImpl::Get(int index, double &value) const
{
    if (index < 0 || index >= static_cast<int>(data_.size())) {
        return -E_NONEXISTENT;
    }
    if (data_.at(index).GetDouble(value) != E_OK) {
        return -E_TYPE_MISMATCH;
    }
    return E_OK;
}

int RelationalRowDataImpl::Get(int index, std::string &value) const
{
    if (index < 0 || index >= static_cast<int>(data_.size())) {
        return -E_NONEXISTENT;
    }
    if (data_.at(index).GetText(value) != E_OK) {
        return -E_TYPE_MISMATCH;
    }
    return E_OK;
}

int RelationalRowDataImpl::Get(int index, std::vector<uint8_t> &value) const
{
    if (index < 0 || index >= static_cast<int>(data_.size())) {
        return -E_NONEXISTENT;
    }
    Blob blob;
    int errCode = data_.at(index).GetBlob(blob);
    if (errCode != E_OK) {
        return errCode == -E_NOT_SUPPORT ? -E_TYPE_MISMATCH : errCode;
    }
    value = blob.ToVector();
    return errCode;
}
}  // namespace DistributedDB
#endif