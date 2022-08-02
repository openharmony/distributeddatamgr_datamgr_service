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

#ifndef PREPARED_STMT_H
#define PREPARED_STMT_H

#include <string>
#include <vector>
#include "parcel.h"

namespace DistributedDB {
class PreparedStmt {
public:
    enum ExecutorOperation { MIN_LIMIT, QUERY, DELETE, UPDATE, INSERT, MAX_LIMIT };

    PreparedStmt() = default;
    PreparedStmt(ExecutorOperation opCode, const std::string &sql, const std::vector<std::string> &bindArgs);

    void SetOpCode(ExecutorOperation opCode);
    void SetSql(std::string sql);
    void SetBindArgs(std::vector<std::string> bindArgs);

    ExecutorOperation GetOpCode() const;
    const std::string &GetSql() const;
    const std::vector<std::string> &GetBindArgs() const;

    bool IsValid() const;

    int CalcLength() const;
    int Serialize(Parcel &parcel) const;
    int DeSerialize(Parcel &parcel);

private:
    static const int VERSION_1 = 1;
    static const int CURRENT_VERSION = VERSION_1;

    ExecutorOperation opCode_;
    std::string sql_;
    std::vector<std::string> bindArgs_;
};
} // namespace DistributedDB
#endif  // PREPARED_STMT_H