/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "cache_cursor.h"
#include "value_proxy.h"

namespace OHOS::DistributedRdb {
using namespace OHOS::DistributedData;

CacheCursor::CacheCursor(std::vector<DistributedData::VBucket> &&records)
    : row_(0), maxCol_(0), records_(std::move(records))
{
    maxRow_ = records_.size();
    if (maxRow_ > 0) {
        for (auto it = records_[0].begin(); it != records_[0].end(); it++) {
            colNames_.push_back(it->first);
            colTypes_.push_back(it->second.index());
        }
        maxCol_ = colNames_.size();
    }
}

int32_t CacheCursor::GetColumnNames(std::vector<std::string> &names) const
{
    names = colNames_;
    return GeneralError::E_OK;
}


int32_t CacheCursor::GetColumnName(int32_t col, std::string &name) const
{
    if (col < 0 || col >= maxCol_) {
        return GeneralError::E_INVALID_ARGS;
    }
    name = colNames_[col];
    return GeneralError::E_OK;
}

int32_t CacheCursor::GetColumnType(int32_t col) const
{
    if (col < 0 || col >= maxCol_) {
        return DistributedDB::ResultSet::ColumnType::INVALID_TYPE;
    }
    return colTypes_[col];
}

int32_t CacheCursor::GetCount() const
{
    return maxRow_;
}

int32_t CacheCursor::MoveToFirst()
{
    row_ = 0;
    return GeneralError::E_OK;
}

int32_t CacheCursor::MoveToNext()
{
    auto row = row_;
    if (row >= maxRow_ - 1) {
        return GeneralError::E_ERROR;
    }
    row++;
    row_ = row;
    return GeneralError::E_OK;
}

int32_t CacheCursor::MoveToPrev()
{
    return GeneralError::E_NOT_SUPPORT;
}

int32_t CacheCursor::GetEntry(DistributedData::VBucket &entry)
{
    return GetRow(entry);
}

int32_t CacheCursor::GetRow(DistributedData::VBucket &data)
{
    auto row = row_;
    if (row >= maxRow_) {
        return GeneralError::E_RECODE_LIMIT_EXCEEDED;
    }
    data = records_[row];
    return GeneralError::E_OK;
}

int32_t CacheCursor::Get(int32_t col, DistributedData::Value &value)
{
    if (col < 0 || col >= maxCol_) {
        return GeneralError::E_INVALID_ARGS;
    }
    return Get(colNames_[col], value);
}

int32_t CacheCursor::Get(const std::string &col, DistributedData::Value &value)
{
    auto row = row_;
    if (row >= maxRow_) {
        return GeneralError::E_RECODE_LIMIT_EXCEEDED;
    }
    auto it = records_[row].find(col);
    if (it == records_[row].end()) {
        return GeneralError::E_INVALID_ARGS;
    }
    value = it->second;
    return GeneralError::E_OK;
}

int32_t CacheCursor::Close()
{
    return GeneralError::E_OK;
}

bool CacheCursor::IsEnd()
{
    return false;
}
}