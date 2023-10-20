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
bool RdbQuery::IsEqual(uint64_t tid)
{
    return tid == TYPE_ID;
}

std::vector<std::string> RdbQuery::GetTables()
{
    return tables_;
}

void RdbQuery::MakeRemoteQuery(const std::string& devices, const std::string& sql, Values&& args)
{
    isRemote_ = true;
    devices_ = { devices };
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
    ZLOGD("table size:%{public}zu", tables.size());
    tables_ = tables;
    query_.FromTable(tables);
}

void RdbQuery::MakeQuery(const PredicatesMemo &predicates)
{
    ZLOGD("table size:%{public}zu, device size:%{public}zu, op size:%{public}zu", predicates.tables_.size(),
        predicates.devices_.size(), predicates.operations_.size());
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

void RdbQuery::MakeCloudQuery(const PredicatesMemo& predicates)
{
    ZLOGD("table size:%{public}zu, device size:%{public}zu, op size:%{public}zu", predicates.tables_.size(),
        predicates.devices_.size(), predicates.operations_.size());
    devices_ = predicates.devices_;
    tables_ = predicates.tables_;
    if (predicates.operations_.empty() || predicates.tables_.size() != 1) {
        query_ = DistributedDB::Query::Select();
        if (!predicates.tables_.empty()) {
            query_.FromTable(predicates.tables_);
        }
        return;
    }
    query_ = DistributedDB::Query::Select().From(*predicates.tables_.begin());
    isPriority_ = true;
    for (const auto& operation : predicates.operations_) {
        if (operation.operator_ >= 0 && operation.operator_ < OPERATOR_MAX) {
            (this->*HANDLES[operation.operator_])(operation);
        }
    }
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

void RdbQuery::SetQueryNodes(const std::string& tableName, QueryNodes&& nodes)
{
    tables_ = { tableName };
    queryNodes_ = std::move(nodes);
}

DistributedData::QueryNodes RdbQuery::GetQueryNodes(const std::string& tableName)
{
    return queryNodes_;
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

void RdbQuery::In(const RdbPredicateOperation& operation)
{
    query_.In(operation.field_, operation.values_);
}

void RdbQuery::BeginGroup(const RdbPredicateOperation& operation)
{
    query_.BeginGroup();
}

void RdbQuery::EndGroup(const RdbPredicateOperation& operation)
{
    query_.EndGroup();
}

bool RdbQuery::IsPriority()
{
    return isPriority_;
}
} // namespace OHOS::DistributedRdb
