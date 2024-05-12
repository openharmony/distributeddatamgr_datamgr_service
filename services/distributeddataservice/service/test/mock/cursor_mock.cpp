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
#include "cursor_mock.h"
#include "store/general_value.h"
namespace OHOS {
namespace DistributedData {
CursorMock::CursorMock(std::shared_ptr<ResultSet> resultSet) : resultSet_(std::move(resultSet)) {}
CursorMock::~CursorMock() {}
int32_t CursorMock::GetColumnNames(std::vector<std::string> &names) const
{
    if (!resultSet_->empty()) {
        for (auto &[key, value] : *resultSet_->begin()) {
            names.push_back(key);
        }
    }
    return GeneralError::E_OK;
}

int32_t CursorMock::GetColumnName(int32_t col, std::string &name) const
{
    if (!resultSet_->empty()) {
        int index = 0;
        for (auto &[key, value] : *resultSet_->begin()) {
            if (index++ == col) {
                name = key;
                break;
            }
        }
    }
    return GeneralError::E_OK;
}

int32_t CursorMock::GetColumnType(int32_t col) const
{
    if (!resultSet_->empty()) {
        int index = 0;
        for (auto &[key, value] : *resultSet_->begin()) {
            if (index++ == col) {
                return value.index();
            }
        }
    }
    return GeneralError::E_OK;
}

int32_t CursorMock::GetCount() const
{
    return resultSet_->size();
}

int32_t CursorMock::MoveToFirst()
{
    index_ = 0;
    return GeneralError::E_OK;
}

int32_t CursorMock::MoveToNext()
{
    index_++;
    return GeneralError::E_OK;
}

int32_t CursorMock::MoveToPrev()
{
    index_--;
    return GeneralError::E_OK;
}

int32_t CursorMock::GetEntry(DistributedData::VBucket &entry)
{
    GetRow(entry);
    return GeneralError::E_OK;
}

int32_t CursorMock::GetRow(DistributedData::VBucket &data)
{
    if (index_ >= 0 && index_ < resultSet_->size()) {
        data = (*resultSet_)[index_];
    }
    return GeneralError::E_OK;
}

int32_t CursorMock::Get(int32_t col, DistributedData::Value &value)
{
    if (index_ >= 0 && index_ < resultSet_->size()) {
        int i = 0;
        for (auto &[k, v] : (*resultSet_)[index_]) {
            if (i++ == col) {
                value = v;
                break;
            }
        }
    }
    return GeneralError::E_OK;
}

int32_t CursorMock::Get(const std::string &col, DistributedData::Value &value)
{
    if (index_ >= 0 && index_ < resultSet_->size()) {
        for (auto &[k, v] : (*resultSet_)[index_]) {
            if (k == col) {
                value = v;
                break;
            }
        }
    }
    return GeneralError::E_OK;
}

int32_t CursorMock::Close()
{
    resultSet_->clear();
    return GeneralError::E_OK;
}

bool CursorMock::IsEnd()
{
    return index_ == resultSet_->size() - 1;
}
} // namespace DistributedData
} // namespace OHOS
