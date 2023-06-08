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

    DistributedDB::Query query_;
    std::string sql_;
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_QUERY_H
