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
#ifndef RELATIONAL_REMOTE_QUERY_CONTINUE_TOKEN_H
#define RELATIONAL_REMOTE_QUERY_CONTINUE_TOKEN_H
#ifdef RELATIONAL_STORE
#include <string>
#include <vector>
#include "relational_row_data_set.h"
#include "relational_row_data.h"

namespace DistributedDB {
class RelationalRemoteQueryContinueToken {
public:
    RelationalRemoteQueryContinueToken(std::vector<std::string> &&colNames, std::vector<RelationalRowData *> &&data);
    ~RelationalRemoteQueryContinueToken();

    // Check the magic number at the beginning and end of the RelationalRemoteQueryContinueToken.
    bool CheckValid() const;

    // if succeed, return E_UNFINISHED, E_OK; if error happened, return other errCode
    int GetData(int packetSize, RelationalRowDataSet &dataSet);

private:
    static const unsigned int MAGIC_BEGIN = 0x600D0AC7;  // for token guard
    static const unsigned int MAGIC_END = 0x0AC7600D;    // for token guard
    unsigned int magicBegin_ = MAGIC_BEGIN;
    std::vector<std::string> colNames_;
    std::vector<RelationalRowData *> data_;
    unsigned int magicEnd_ = MAGIC_END;
};
}  // namespace DistributedDB
#endif  // RELATIONAL_STORE
#endif  // RELATIONAL_REMOTE_QUERY_CONTINUE_TOKEN_H