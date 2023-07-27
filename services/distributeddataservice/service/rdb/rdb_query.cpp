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
#define LOG_TAG "RdbQuery"
#include "rdb_query.h"
#include "log_print.h"
#include "utils/anonymous.h"
#include "value_proxy.h"
namespace OHOS::DistributedRdb {
using namespace DistributedData;

RdbQuery::RdbQuery(bool isRemote) : isRemote_(isRemote) {}

bool RdbQuery::IsEqual(uint64_t tid)
{
    return tid == TYPE_ID;
}

std::vector<std::string> RdbQuery::GetTables()
{
    return tables_;
}

void RdbQuery::SetDevices(const std::vector<std::string> &devices)
{
    devices_ = devices;
}

void RdbQuery::SetSql(const std::string &sql, DistributedData::Values &&args)
{
    sql_ = sql;
    args_ = std::move(args);
}

DistributedDB::Query RdbQuery::GetQuery() const
{
    return query_;
}

std::vector<std::string> RdbQuery::GetDevices() const
{
    return devices_;
}

void RdbQuery::FromTable(const std::vector<std::string> &tables)
{
    ZLOGD("table count=%{public}zu", tables.size());
    query_.FromTable(tables);
}

void RdbQuery::MakeQuery(const PredicatesMemo &predicates)
{
    ZLOGD("table=%{public}zu", predicates.tables_.size());
    query_ = predicates.tables_.size() == 1 ? DistributedDB::Query::Select(*predicates.tables_.begin())
                                            : DistributedDB::Query::Select();
    if (predicates.tables_.size() > 1) {
        query_.FromTable(predicates.tables_);
    }
    for (const auto &operation : predicates.operations_) {
        if (operation.operator_ >= 0 && operation.operator_ < OPERATOR_MAX) {
            (this->*HANDLES[operation.operator_])(operation);
        }
    }
    devices_ = predicates.devices_;
    tables_ = predicates.tables_;
}

bool RdbQuery::IsRemoteQuery()
{
    return isRemote_;
}

DistributedDB::RemoteCondition RdbQuery::GetRemoteCondition() const
{
    auto args = args_;
    std::vector<std::string> bindArgs = ValueProxy::Convert(std::move(args));
    return { sql_, bindArgs };
}

void RdbQuery::EqualTo(const RdbPredicateOperation &operation)
{
    query_.EqualTo(operation.field_, operation.values_[0]);
}

void RdbQuery::NotEqualTo(const RdbPredicateOperation &operation)
{
    query_.NotEqualTo(operation.field_, operation.values_[0]);
}

void RdbQuery::And(const RdbPredicateOperation &operation)
{
    query_.And();
}

void RdbQuery::Or(const RdbPredicateOperation &operation)
{
    query_.Or();
}

void RdbQuery::OrderBy(const RdbPredicateOperation &operation)
{
    bool isAsc = operation.values_[0] == "true";
    query_.OrderBy(operation.field_, isAsc);
}

void RdbQuery::Limit(const RdbPredicateOperation &operation)
{
    char *end = nullptr;
    int limit = static_cast<int>(strtol(operation.field_.c_str(), &end, DECIMAL_BASE));
    int offset = static_cast<int>(strtol(operation.values_[0].c_str(), &end, DECIMAL_BASE));
    if (limit < 0) {
        limit = 0;
    }
    if (offset < 0) {
        offset = 0;
    }
    query_.Limit(limit, offset);
}
} // namespace OHOS::DistributedRdb
