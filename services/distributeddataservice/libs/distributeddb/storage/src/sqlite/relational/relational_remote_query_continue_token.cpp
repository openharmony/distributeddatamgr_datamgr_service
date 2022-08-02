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
#include "relational_remote_query_continue_token.h"

namespace DistributedDB {
RelationalRemoteQueryContinueToken::RelationalRemoteQueryContinueToken(std::vector<std::string> &&colNames,
    std::vector<RelationalRowData *> &&data) : colNames_(std::move(colNames)), data_(std::move(data)) {}

RelationalRemoteQueryContinueToken::~RelationalRemoteQueryContinueToken()
{
    RelationalRowData::Release(data_);
}

// Check the magic number at the beginning and end of the RelationalRemoteQueryContinueToken.
bool RelationalRemoteQueryContinueToken::CheckValid() const
{
    bool isValid = (magicBegin_ == MAGIC_BEGIN && magicEnd_ == MAGIC_END);
    if (!isValid) {
        LOGE("Invalid continue token.");
    }
    return isValid;
}

// if succeed, return E_UNFINISHED, E_OK; if error happened, return other errCode
int RelationalRemoteQueryContinueToken::GetData(int packetSize, RelationalRowDataSet &dataSet)
{
    bool isEmpty = true;
    if (!colNames_.empty()) {
        dataSet.SetColNames(std::move(colNames_));
        isEmpty = false;
    }
    while (!data_.empty()) {
        if (isEmpty) {  // at least get one data
            dataSet.Insert(data_.at(0));
            data_.erase(data_.begin());
            isEmpty = false;
            continue;
        }
        if (dataSet.CalcLength() + data_.at(0)->CalcLength() > packetSize) {
            return -E_UNFINISHED;
        }
        dataSet.Insert(data_.at(0));
        data_.erase(data_.begin());
    }
    return E_OK;
}
}
#endif