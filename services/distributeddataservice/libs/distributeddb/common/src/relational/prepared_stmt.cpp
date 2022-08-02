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
#include "prepared_stmt.h"
#include "db_constant.h"

namespace DistributedDB {
PreparedStmt::PreparedStmt(ExecutorOperation opCode, const std::string &sql, const std::vector<std::string> &bindArgs)
    : opCode_(opCode), sql_(sql), bindArgs_(bindArgs) {}


void PreparedStmt::SetOpCode(ExecutorOperation opCode)
{
    opCode_ = opCode;
}

void PreparedStmt::SetSql(std::string sql)
{
    sql_ = std::move(sql);
}

void PreparedStmt::SetBindArgs(std::vector<std::string> bindArgs)
{
    bindArgs_ = std::move(bindArgs);
}

PreparedStmt::ExecutorOperation PreparedStmt::GetOpCode() const
{
    return opCode_;
}

const std::string &PreparedStmt::GetSql() const
{
    return sql_;
}

const std::vector<std::string> &PreparedStmt::GetBindArgs() const
{
    return bindArgs_;
}

bool PreparedStmt::IsValid() const
{
    return opCode_ == ExecutorOperation::QUERY && !sql_.empty() && bindArgs_.size() <= DBConstant::MAX_SQL_ARGS_COUNT;
}

int PreparedStmt::CalcLength() const
{
    uint32_t length = Parcel::GetIntLen() +         // current version
                      Parcel::GetIntLen() +         // opcode_
                      Parcel::GetStringLen(sql_) +  // sql_
                      Parcel::GetIntLen();          // bindArgs_.size()
    for (const auto &bindArg : bindArgs_) {
        length += Parcel::GetStringLen(bindArg);    // bindArgs_
    }
    return static_cast<int>(Parcel::GetEightByteAlign(length));
}

// Before call this func. You should check if the object is valid.
int PreparedStmt::Serialize(Parcel &parcel) const
{
    // version
    (void)parcel.WriteInt(CURRENT_VERSION);

    // opcode
    (void)parcel.WriteInt(static_cast<int>(opCode_));

    // string
    (void)parcel.WriteString(sql_);

    // bindArgs
    (void)parcel.WriteInt(static_cast<int>(bindArgs_.size()));
    for (const auto &bindArg : bindArgs_) {
        (void)parcel.WriteString(bindArg);
        if (parcel.IsError()) {
            return -E_PARSE_FAIL;
        }
    }

    (void)parcel.EightByteAlign();
    if (parcel.IsError()) {
        return -E_PARSE_FAIL;
    }
    return E_OK;
}

int PreparedStmt::DeSerialize(Parcel &parcel)
{
    // clear the object
    bindArgs_.clear();

    // version
    int version = 0;
    (void)parcel.ReadInt(version);
    if (parcel.IsError() || version <= 0 || version > CURRENT_VERSION) {
        return -E_PARSE_FAIL;
    }

    // VERSION 1
    if (version >= VERSION_1) {
        // opcode
        int opCode = 0;
        (void)parcel.ReadInt(opCode);
        if (parcel.IsError() || opCode <= MIN_LIMIT || opCode >= MAX_LIMIT) {
            return -E_PARSE_FAIL;
        }
        opCode_ = static_cast<ExecutorOperation>(opCode);

        // sql
        (void)parcel.ReadString(sql_);

        // bindArgs
        int argsCount = 0;
        (void)parcel.ReadInt(argsCount);
        if (parcel.IsError() || argsCount < 0 || argsCount > static_cast<int>(DBConstant::MAX_SQL_ARGS_COUNT)) {
            return -E_PARSE_FAIL;
        }
        for (int i = 0; i < argsCount; ++i) {
            std::string bindArg;
            (void)parcel.ReadString(bindArg);
            if (parcel.IsError()) {
                return -E_PARSE_FAIL;
            }
            bindArgs_.emplace_back(std::move(bindArg));
        }
    }

    (void)parcel.EightByteAlign();
    return E_OK;
}
}
#endif