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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_QUERY_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_QUERY_H
#include "rdb_predicates.h"
#include "store/general_value.h"
#include "store_types.h"
#include "query.h"
namespace OHOS::DistributedRdb {
class RdbQuery : public DistributedData::GenQuery {
public:
    using Predicates = NativeRdb::RdbPredicates;
    static constexpr uint64_t TYPE_ID = 0x20000001;
    RdbQuery() = default;

    ~RdbQuery() override = default;

    bool IsEqual(uint64_t tid) override;
    std::vector<std::string> GetTables() override;
    void SetQueryNodes(const std::string& tableName, DistributedData::QueryNodes&& nodes) override;
    DistributedData::QueryNodes GetQueryNodes(const std::string& tableName) override;
    std::vector<std::string> GetDevices() const;
    std::string GetStatement() const;
    DistributedData::Values GetBindArgs() const;
    void SetColumns(std::vector<std::string> &&columns);
    void SetColumns(const std::vector<std::string> &columns);
    std::vector<std::string> GetColumns() const;
    DistributedDB::Query GetQuery() const;
    DistributedDB::RemoteCondition GetRemoteCondition() const;
    bool IsRemoteQuery();
    bool IsPriority();
    void MakeQuery(const PredicatesMemo &predicates);
    void MakeRemoteQuery(const std::string &devices, const std::string &sql, DistributedData::Values &&args);
    void MakeCloudQuery(const PredicatesMemo &predicates);

private:
    void EqualTo(const RdbPredicateOperation& operation);
    void NotEqualTo(const RdbPredicateOperation& operation);
    void And(const RdbPredicateOperation& operation);
    void Or(const RdbPredicateOperation& operation);
    void OrderBy(const RdbPredicateOperation& operation);
    void Limit(const RdbPredicateOperation& operation);
    void In(const RdbPredicateOperation& operation);
    void NotIn(const RdbPredicateOperation& operation);
    void Contain(const RdbPredicateOperation& operation);
    void BeginWith(const RdbPredicateOperation& operation);
    void EndWith(const RdbPredicateOperation& operation);
    void IsNull(const RdbPredicateOperation& operation);
    void IsNotNull(const RdbPredicateOperation& operation);
    void Like(const RdbPredicateOperation& operation);
    void Glob(const RdbPredicateOperation& operation);
    void Between(const RdbPredicateOperation& operation);
    void NotBetween(const RdbPredicateOperation& operation);
    void GreaterThan(const RdbPredicateOperation& operation);
    void GreaterThanOrEqual(const RdbPredicateOperation& operation);
    void LessThan(const RdbPredicateOperation& operation);
    void LessThanOrEqual(const RdbPredicateOperation& operation);
    void Distinct(const RdbPredicateOperation& operation);
    void IndexedBy(const RdbPredicateOperation& operation);
    void BeginGroup(const RdbPredicateOperation& operation);
    void EndGroup(const RdbPredicateOperation& operation);
    using PredicateHandle = void (RdbQuery::*)(const RdbPredicateOperation &operation);
    static constexpr inline PredicateHandle HANDLES[OPERATOR_MAX] = {
        &RdbQuery::EqualTo,
        &RdbQuery::NotEqualTo,
        &RdbQuery::And,
        &RdbQuery::Or,
        &RdbQuery::OrderBy,
        &RdbQuery::Limit,
        &RdbQuery::BeginGroup,
        &RdbQuery::EndGroup,
        &RdbQuery::In,
        &RdbQuery::NotIn,
        &RdbQuery::Contain,
        &RdbQuery::BeginWith,
        &RdbQuery::EndWith,
        &RdbQuery::IsNull,
        &RdbQuery::IsNotNull,
        &RdbQuery::Like,
        &RdbQuery::Glob,
        &RdbQuery::Between,
        &RdbQuery::NotBetween,
        &RdbQuery::GreaterThan,
        &RdbQuery::GreaterThanOrEqual,
        &RdbQuery::LessThan,
        &RdbQuery::LessThanOrEqual,
        &RdbQuery::Distinct,
        &RdbQuery::IndexedBy
    };
    static constexpr inline uint32_t DECIMAL_BASE = 10;

    DistributedDB::Query query_;
    std::shared_ptr<Predicates> predicates_;
    std::vector<std::string> columns_;
    bool isRemote_ = false;
    bool isPriority_ = false;
    std::string sql_;
    DistributedData::Values args_;
    std::vector<std::string> devices_;
    std::vector<std::string> tables_;
    DistributedData::QueryNodes queryNodes_;
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_QUERY_H
