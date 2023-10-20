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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_CLOUD_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_CLOUD_H
#include "cloud/cloud_db.h"
#include "cloud/cloud_store_types.h"
#include "cloud/icloud_db.h"
#include "error/general_error.h"

namespace OHOS::DistributedRdb {
class RdbCloud : public DistributedDB::ICloudDb {
public:
    using DBStatus = DistributedDB::DBStatus;
    using DBVBucket = DistributedDB::VBucket;
    using DBQueryNodes = std::vector<DistributedDB::QueryNode>;

    explicit RdbCloud(std::shared_ptr<DistributedData::CloudDB> cloudDB);
    virtual ~RdbCloud() = default;
    DBStatus BatchInsert(const std::string &tableName, std::vector<DBVBucket> &&record,
        std::vector<DBVBucket> &extend) override;
    DBStatus BatchUpdate(const std::string &tableName, std::vector<DBVBucket> &&record,
        std::vector<DBVBucket> &extend) override;
    DBStatus BatchDelete(const std::string &tableName, std::vector<DBVBucket> &extend) override;
    DBStatus Query(const std::string &tableName, DBVBucket &extend, std::vector<DBVBucket> &data) override;
    std::pair<DBStatus, uint32_t> Lock() override;
    DBStatus UnLock() override;
    DBStatus HeartBeat() override;
    DBStatus Close() override;
    static DBStatus ConvertStatus(DistributedData::GeneralError error);

private:
    static constexpr const char *TYPE_FIELD = "#_type";
    static constexpr const char *QUERY_FIELD = "#_query";
    using QueryNodes = std::vector<DistributedData::QueryNode>;
    static std::pair<QueryNodes, DistributedData::GeneralError> ConvertQuery(DBVBucket& extend);
    static QueryNodes ConvertQuery(DBQueryNodes&& nodes);
    static constexpr int32_t TO_MS = 1000; // s > ms
    std::shared_ptr<DistributedData::CloudDB> cloudDB_;
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_CLOUD_H
