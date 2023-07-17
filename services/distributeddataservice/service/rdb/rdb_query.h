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
    explicit RdbQuery(bool isRemote);

    ~RdbQuery() override = default;

    bool IsEqual(uint64_t tid) override;
    std::vector<std::string> GetTables() override;
    std::vector<std::string> GetDevices() const;
    DistributedDB::Query GetQuery() const;
    DistributedDB::RemoteCondition GetRemoteCondition() const;
    bool IsRemoteQuery();
    void SetDevices(const std::vector<std::string> &devices);
    void SetSql(const std::string &sql, DistributedData::Values &&args);
    void MakeQuery(const PredicatesMemo &predicates);

private:
    void EqualTo(const RdbPredicateOperation& operation);
    void NotEqualTo(const RdbPredicateOperation& operation);
    void And(const RdbPredicateOperation& operation);
    void Or(const RdbPredicateOperation& operation);
    void OrderBy(const RdbPredicateOperation& operation);
    void Limit(const RdbPredicateOperation& operation);
    using PredicateHandle = void (RdbQuery::*)(const RdbPredicateOperation &operation);
    static constexpr inline PredicateHandle HANDLES[OPERATOR_MAX] = {
        [EQUAL_TO] = &RdbQuery::EqualTo,
        [NOT_EQUAL_TO] = &RdbQuery::NotEqualTo,
        [AND] = &RdbQuery::And,
        [OR] = &RdbQuery::Or,
        [ORDER_BY] = &RdbQuery::OrderBy,
        [LIMIT] = &RdbQuery::Limit,
    };
    static constexpr inline uint32_t DECIMAL_BASE = 10;

    DistributedDB::Query query_;
    bool isRemote_ = false;
    std::string sql_;
    DistributedData::Values args_;
    std::vector<std::string> devices_;
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_QUERY_H
