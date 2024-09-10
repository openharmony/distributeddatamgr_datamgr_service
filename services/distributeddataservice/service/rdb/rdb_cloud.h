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

#include <mutex>

#include "cloud/cloud_db.h"
#include "cloud/cloud_store_types.h"
#include "cloud/icloud_db.h"
#include "error/general_error.h"
#include "snapshot/snapshot.h"

namespace OHOS::DistributedRdb {
class RdbCloud : public DistributedDB::ICloudDb {
public:
    enum FLAG : uint8_t {
        SYSTEM_ABILITY = 1,
        APPLICATION
    };
    using DBStatus = DistributedDB::DBStatus;
    using DBVBucket = DistributedDB::VBucket;
    using DBQueryNodes = std::vector<DistributedDB::QueryNode>;
    using DataBucket = DistributedData::VBucket;
    using BindAssets = DistributedData::BindAssets;
    using GeneralError = DistributedData::GeneralError;

    explicit RdbCloud(std::shared_ptr<DistributedData::CloudDB> cloudDB, BindAssets* bindAssets);
    virtual ~RdbCloud() = default;
    DBStatus BatchInsert(const std::string &tableName, std::vector<DBVBucket> &&record,
        std::vector<DBVBucket> &extend) override;
    DBStatus BatchUpdate(const std::string &tableName, std::vector<DBVBucket> &&record,
        std::vector<DBVBucket> &extend) override;
    DBStatus BatchDelete(const std::string &tableName, std::vector<DBVBucket> &extend) override;
    DBStatus Query(const std::string &tableName, DBVBucket &extend, std::vector<DBVBucket> &data) override;
    DistributedData::GeneralError PreSharing(const std::string &tableName, DistributedData::VBuckets &extend);
    std::pair<DBStatus, uint32_t> Lock() override;
    DBStatus UnLock() override;
    DBStatus HeartBeat() override;
    DBStatus Close() override;
    std::pair<DBStatus, std::string> GetEmptyCursor(const std::string &tableName) override;
    static DBStatus ConvertStatus(DistributedData::GeneralError error);
    uint8_t GetLockFlag() const;
    std::pair<GeneralError, uint32_t> LockCloudDB(FLAG flag);
    GeneralError UnLockCloudDB(FLAG flag);
    void SetPrepareTraceId(const std::string &traceId) override;

private:
    static constexpr const char *TYPE_FIELD = "#_type";
    static constexpr const char *QUERY_FIELD = "#_query";
    using QueryNodes = std::vector<DistributedData::QueryNode>;
    static std::pair<QueryNodes, DistributedData::GeneralError> ConvertQuery(DBVBucket& extend);
    static QueryNodes ConvertQuery(DBQueryNodes&& nodes);
    static void ConvertErrorField(DistributedData::VBuckets& extends);
    static constexpr int32_t TO_MS = 1000; // s > ms
    std::shared_ptr<DistributedData::CloudDB> cloudDB_;
    BindAssets* snapshots_;
    uint8_t flag_ = 0;
    std::mutex mutex_;

    void PostEvent(DistributedData::VBuckets& records, std::set<std::string>& skipAssets,
        DistributedData::VBuckets& extend, DistributedData::AssetEvent eventId);
    void PostEvent(DistributedData::Value& value, DataBucket& extend, std::set<std::string>& skipAssets,
        DistributedData::AssetEvent eventId);
    void PostEventAsset(DistributedData::Asset& asset, DataBucket& extend, std::set<std::string>& skipAssets,
        DistributedData::AssetEvent eventId);
    std::pair<GeneralError, uint32_t> InnerLock(FLAG flag);
    GeneralError InnerUnLock(FLAG flag);
};
} // namespace OHOS::DistributedRdb
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_RDB_CLOUD_H