/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_KVDB_QUERY_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_KVDB_QUERY_H

#include "query.h"
#include "query_helper.h"
#include "store/general_value.h"
#include "store_types.h"

namespace OHOS::DistributedKv {
class KVDBQuery : public DistributedData::GenQuery {
public:
    static constexpr uint64_t TYPE_ID = 0x20000002;

    explicit KVDBQuery(const std::string &query)
    {
        query_ = query;
        dbQuery_ = QueryHelper::StringToDbQuery(query, isSuccess_);
    }
    ~KVDBQuery() = default;
    bool IsEqual(uint64_t tid)
    {
        return tid == TYPE_ID;
    }

    std::vector<std::string> GetTables()
    {
        return {};
    }

    bool IsValidQuery()
    {
        return isSuccess_;
    }

    bool IsEmpty()
    {
        return query_.empty();
    }

    DistributedDB::Query GetDBQuery()
    {
        return dbQuery_;
    }

    bool isSuccess_ = false;
    std::string query_;
    DistributedDB::Query dbQuery_;
};
} // namespace OHOS::DistributedKv

#endif //OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_KVDB_QUERY_H
