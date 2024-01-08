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

void RdbQuery::MakeQuery(const PredicatesMemo &predicates)
{
    ZLOGD("table size:%{public}zu, device size:%{public}zu, op size:%{public}zu", predicates.tables_.size(),
        predicates.devices_.size(), predicates.operations_.size());
    query_ = predicates.tables_.size() == 1 ? DistributedDB::Query::Select(*predicates.tables_.begin())
                                            : DistributedDB::Query::Select();
    if (predicates.tables_.size() > 1) {
        query_.FromTable(predicates.tables_);
    }
    if (!predicates.tables_.empty()) {
        predicates_ = std::make_shared<Predicates>(*predicates.tables_.begin());
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
    predicates_ = std::make_shared<Predicates>(*predicates.tables_.begin());
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

std::string RdbQuery::GetStatement() const
{
    return predicates_ != nullptr ? predicates_->GetStatement() : "";
}

DistributedData::Values RdbQuery::GetBindArgs() const
{
    return predicates_ != nullptr ? ValueProxy::Convert(predicates_->GetBindArgs()) : DistributedData::Values();
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
    if (operation.values_.empty()) {
        return;
    }
    query_.EqualTo(operation.field_, operation.values_[0]);
    predicates_->EqualTo(operation.field_, operation.values_[0]);
}

void RdbQuery::NotEqualTo(const RdbPredicateOperation &operation)
{
    if (operation.values_.empty()) {
        return;
    }
    query_.NotEqualTo(operation.field_, operation.values_[0]);
    predicates_->NotEqualTo(operation.field_, operation.values_[0]);
}

void RdbQuery::And(const RdbPredicateOperation &operation)
{
    query_.And();
    predicates_->And();
}

void RdbQuery::Or(const RdbPredicateOperation &operation)
{
    query_.Or();
    predicates_->Or();
}

void RdbQuery::OrderBy(const RdbPredicateOperation &operation)
{
    if (operation.values_.empty()) {
        return;
    }
    bool isAsc = operation.values_[0] == "true";
    query_.OrderBy(operation.field_, isAsc);
    isAsc ? predicates_->OrderByAsc(operation.field_) : predicates_->OrderByDesc(operation.field_);
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
    predicates_->Limit(limit, offset);
}

void RdbQuery::In(const RdbPredicateOperation& operation)
{
    query_.In(operation.field_, operation.values_);
    predicates_->In(operation.field_, operation.values_);
}

void RdbQuery::BeginGroup(const RdbPredicateOperation& operation)
{
    query_.BeginGroup();
    predicates_->BeginWrap();
}

void RdbQuery::EndGroup(const RdbPredicateOperation& operation)
{
    query_.EndGroup();
    predicates_->EndWrap();
}

void RdbQuery::NotIn(const RdbPredicateOperation& operation)
{
    query_.NotIn(operation.field_, operation.values_);
    predicates_->NotIn(operation.field_, operation.values_);
}

void RdbQuery::Contain(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    query_.Like(operation.field_, "%" + operation.values_[0] + "%");
    predicates_->Contains(operation.field_, operation.values_[0]);
}

void RdbQuery::BeginWith(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    query_.Like(operation.field_, operation.values_[0] + "%");
    predicates_->BeginsWith(operation.field_, operation.values_[0]);
}

void RdbQuery::EndWith(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    query_.Like(operation.field_, "%" + operation.values_[0]);
    predicates_->EndsWith(operation.field_, operation.values_[0]);
}

void RdbQuery::IsNull(const RdbPredicateOperation& operation)
{
    query_.IsNull(operation.field_);
    predicates_->IsNull(operation.field_);
}

void RdbQuery::IsNotNull(const RdbPredicateOperation& operation)
{
    query_.IsNotNull(operation.field_);
    predicates_->IsNotNull(operation.field_);
}

void RdbQuery::Like(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    query_.Like(operation.field_, operation.values_[0]);
    predicates_->Like(operation.field_, operation.values_[0]);
}

void RdbQuery::Glob(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    predicates_->Glob(operation.field_, operation.values_[0]);
}

void RdbQuery::Between(const RdbPredicateOperation& operation)
{
    if (operation.values_.size() != 2) { // between a and b 2 args
        return;
    }
    predicates_->Between(operation.field_, operation.values_[0], operation.values_[1]);
}

void RdbQuery::NotBetween(const RdbPredicateOperation& operation)
{
    if (operation.values_.size() != 2) { // not between a and b 2 args
        return;
    }
    predicates_->NotBetween(operation.field_, operation.values_[0], operation.values_[1]);
}

void RdbQuery::GreaterThan(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    query_.GreaterThan(operation.field_, operation.field_[0]);
    predicates_->GreaterThan(operation.field_, operation.field_[0]);
}

void RdbQuery::GreaterThanOrEqual(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    query_.GreaterThanOrEqualTo(operation.field_, operation.field_[0]);
    predicates_->GreaterThanOrEqualTo(operation.field_, operation.field_[0]);
}

void RdbQuery::LessThan(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    query_.LessThan(operation.field_, operation.field_[0]);
    predicates_->LessThan(operation.field_, operation.field_[0]);
}

void RdbQuery::LessThanOrEqual(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    query_.LessThanOrEqualTo(operation.field_, operation.field_[0]);
    predicates_->LessThanOrEqualTo(operation.field_, operation.field_[0]);
}

void RdbQuery::Distinct(const RdbPredicateOperation& operation)
{
    predicates_->Distinct();
}

void RdbQuery::IndexedBy(const RdbPredicateOperation& operation)
{
    predicates_->IndexedBy(operation.field_);
    query_.SuggestIndex(operation.field_);
}

bool RdbQuery::IsPriority()
{
    return isPriority_;
}

void RdbQuery::SetColumns(std::vector<std::string> &&columns)
{
    columns_ = std::move(columns);
}

void RdbQuery::SetColumns(const std::vector<std::string> &columns)
{
    columns_ = columns;
}

std::vector<std::string> RdbQuery::GetColumns() const
{
    return columns_;
}
} // namespace OHOS::DistributedRdb
