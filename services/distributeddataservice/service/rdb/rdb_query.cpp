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

#include "asset_value.h"
#include "log_print.h"
#include "utils/anonymous.h"
#include "value_proxy.h"
namespace OHOS::DistributedRdb {
using namespace DistributedData;
RdbQuery::RdbQuery(const PredicatesMemo &predicates, bool isPriority)
    : isPriority_(isPriority), devices_(predicates.devices_), tables_(predicates.tables_)
{
    ZLOGD("table size:%{public}zu, device size:%{public}zu, op size:%{public}zu", predicates.tables_.size(),
          predicates.devices_.size(), predicates.operations_.size());
    if (predicates.tables_.size() == 1) {
        if (!isPriority) {
            query_ = DistributedDB::Query::Select(*predicates.tables_.begin());
        } else if (!predicates.operations_.empty()) {
            query_.From(*predicates.tables_.begin());
        }
    }

    if (predicates.tables_.size() > 1 || predicates.operations_.empty()) {
        query_.FromTable(predicates.tables_);
    }

    if (predicates.operations_.empty() || predicates.tables_.empty()) {
        return;
    }
    predicates_ = std::make_shared<Predicates>(*predicates.tables_.begin());
    for (const auto& operation : predicates.operations_) {
        if (operation.operator_ >= 0 && operation.operator_ < OPERATOR_MAX) {
            (this->*HANDLES[operation.operator_])(operation);
        }
    }
}

RdbQuery::RdbQuery(const std::vector<std::string> &tables)
    : query_(DistributedDB::Query::Select())
{
    query_.FromTable(tables);
}

RdbQuery::RdbQuery(const std::string &device, const std::string &sql, Values &&args)
    : isRemote_(true), sql_(sql), args_(std::move(args)), devices_({device})
{
}

bool RdbQuery::IsEqual(uint64_t tid)
{
    return tid == TYPE_ID;
}

std::vector<std::string> RdbQuery::GetTables()
{
    return tables_;
}

DistributedDB::Query RdbQuery::GetQuery() const
{
    return query_;
}

std::vector<std::string> RdbQuery::GetDevices() const
{
    return devices_;
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
    if (auto strVal = std::get_if<std::string>(&operation.values_[0])) {
        query_.EqualTo(operation.field_, *strVal);
        predicates_->EqualTo(operation.field_, *strVal);
    } else if (auto intVal = std::get_if<int64_t>(&operation.values_[0])) {
        query_.EqualTo(operation.field_, *intVal);
        predicates_->EqualTo(operation.field_, *intVal);
    }
}

void RdbQuery::NotEqualTo(const RdbPredicateOperation &operation)
{
    if (operation.values_.empty()) {
        return;
    }
    if (auto strVal = std::get_if<std::string>(&operation.values_[0])) {
        query_.NotEqualTo(operation.field_, *strVal);
        predicates_->NotEqualTo(operation.field_, *strVal);
    } else if (auto intVal = std::get_if<int64_t>(&operation.values_[0])) {
        query_.NotEqualTo(operation.field_, *intVal);
        predicates_->NotEqualTo(operation.field_, *intVal);
    }
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
    if (auto strVal = std::get_if<std::string>(&operation.values_[0])) {
        bool isAsc = *strVal == "true";
        query_.OrderBy(operation.field_, isAsc);
        isAsc ? predicates_->OrderByAsc(operation.field_) : predicates_->OrderByDesc(operation.field_);
    }
}

void RdbQuery::Limit(const RdbPredicateOperation &operation)
{
    if (auto strVal = std::get_if<std::string>(&operation.values_[0])) {
        char *end = nullptr;
        int limit = static_cast<int>(strtol(operation.field_.c_str(), &end, DECIMAL_BASE));
        int offset = static_cast<int>(strtol((*strVal).c_str(), &end, DECIMAL_BASE));
        if (limit < 0) {
            limit = 0;
        }
        if (offset < 0) {
            offset = 0;
        }
        query_.Limit(limit, offset);
        predicates_->Limit(limit, offset);
    }
}

void RdbQuery::In(const RdbPredicateOperation& operation)
{
    std::vector<std::string> vals_string;
    std::vector<int64_t> vals_int;
    vals_string.reserve(operation.values_.size());
    vals_int.reserve(operation.values_.size());
    bool isString = false;
    for (const auto &value : operation.values_) {
        if (auto val = std::get_if<std::string>(&value)) {
            vals_string.emplace_back(*val);
            isString = true;
        } else if (auto val = std::get_if<int64_t>(&value)) {
            vals_int.emplace_back(*val);
        } else {
            return;
        }
    }
    if (isString) {
        query_.In(operation.field_, vals_string);
        predicates_->In(operation.field_, vals_string);
    } else {
        query_.In(operation.field_, vals_int);
    }
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
    std::vector<std::string> vals_string;
    std::vector<int64_t> vals_int;
    vals_string.reserve(operation.values_.size());
    vals_int.reserve(operation.values_.size());
    bool isString = false;
    for (const auto &value : operation.values_) {
        if (auto val = std::get_if<std::string>(&value)) {
            vals_string.emplace_back(*val);
            isString = true;
        } else if (auto val = std::get_if<int64_t>(&value)) {
            vals_int.emplace_back(*val);
        } else {
            return;
        }
    }
    if (isString) {
        query_.NotIn(operation.field_, vals_string);
        predicates_->NotIn(operation.field_, vals_string);
    } else {
        query_.NotIn(operation.field_, vals_int);
    }
}

void RdbQuery::Contain(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    if (auto strVal = std::get_if<std::string>(&operation.values_[0])) {
        query_.Like(operation.field_, "%" + *strVal + "%");
        predicates_->Contains(operation.field_, *strVal);
    }
}

void RdbQuery::BeginWith(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    if (auto strVal = std::get_if<std::string>(&operation.values_[0])) {
        query_.Like(operation.field_, *strVal + "%");
        predicates_->BeginsWith(operation.field_, *strVal);
    }
}

void RdbQuery::EndWith(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    if (auto strVal = std::get_if<std::string>(&operation.values_[0])) {
        query_.Like(operation.field_, "%" + *strVal);
        predicates_->EndsWith(operation.field_, *strVal);
    }
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
    if (auto strVal = std::get_if<std::string>(&operation.values_[0])) {
        query_.Like(operation.field_, *strVal);
        predicates_->Like(operation.field_, *strVal);
    }
}

void RdbQuery::Glob(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    auto strVal = std::get_if<std::string>(&operation.values_[0]);
    if (strVal == nullptr) {
        return;
    }
    predicates_->Glob(operation.field_, *strVal);
}

void RdbQuery::Between(const RdbPredicateOperation& operation)
{
    if (operation.values_.size() != 2) { // between a and b 2 args
        return;
    }
    auto strVal1 = std::get_if<std::string>(&operation.values_[0]);
    auto strVal2 = std::get_if<std::string>(&operation.values_[1]);
    if (strVal2 == nullptr || strVal1 == nullptr) {
        return;
    }
    predicates_->Between(operation.field_, *strVal1, *strVal2);
}

void RdbQuery::NotBetween(const RdbPredicateOperation& operation)
{
    if (operation.values_.size() != 2) { // not between a and b 2 args
        return;
    }
    auto strVal_1 = std::get_if<std::string>(&operation.values_[0]);
    auto strVal_2 = std::get_if<std::string>(&operation.values_[1]);
    if (strVal_1 == nullptr || strVal_2 == nullptr) {
        return;
    }
    predicates_->NotBetween(operation.field_, *strVal_1, *strVal_2);
}

void RdbQuery::GreaterThan(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    if (auto intVal = std::get_if<int64_t>(&operation.values_[0])) {
        query_.GreaterThan(operation.field_, *intVal);
        predicates_->GreaterThan(operation.field_, *intVal);
    }
}

void RdbQuery::GreaterThanOrEqual(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    if (auto intVal = std::get_if<int64_t>(&operation.values_[0])) {
        query_.GreaterThanOrEqualTo(operation.field_, *intVal);
        predicates_->GreaterThanOrEqualTo(operation.field_, *intVal);
    }
}

void RdbQuery::LessThan(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    if (auto intVal = std::get_if<int64_t>(&operation.values_[0])) {
        query_.LessThan(operation.field_, *intVal);
        predicates_->LessThan(operation.field_, *intVal);
    }
}

void RdbQuery::LessThanOrEqual(const RdbPredicateOperation& operation)
{
    if (operation.values_.empty()) {
        return;
    }
    if (auto intVal = std::get_if<int64_t>(&operation.values_[0])) {
        query_.LessThanOrEqualTo(operation.field_, *intVal);
        predicates_->LessThanOrEqualTo(operation.field_, *intVal);
    }
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

void RdbQuery::NotContains(const RdbPredicateOperation &operation)
{
    return;
}

void RdbQuery::NotLike(const RdbPredicateOperation &operation)
{
    if (operation.values_.empty()) {
        return;
    }
    auto strVal = std::get_if<std::string>(&operation.values_[0]);
    if (strVal == nullptr) {
        return;
    }
    query_.NotLike(operation.field_, *strVal);
    predicates_->NotLike(operation.field_, *strVal);
}

void RdbQuery::AssetsOnly(const RdbPredicateOperation &operation)
{
    if (operation.values_.empty()) {
        return;
    }
    DistributedDB::AssetsMap assetsMap;
    std::vector<NativeRdb::AssetValue> assets;
    std::set<std::string> names;
    for (const auto &value : operation.values_) {
        auto strVal = std::get_if<std::string>(&value);
        if (strVal == nullptr) {
            return;
        }
        names.insert(*strVal);
        NativeRdb::AssetValue asset{ .name = *strVal };
        assets.push_back(std::move(asset));
    }
    NativeRdb::ValueObject object(assets);
    assetsMap[operation.field_] = names;
    query_.AssetsOnly(assetsMap);
    predicates_->EqualTo(operation.field_, object);
}
} // namespace OHOS::DistributedRdb