/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef RELATIONAL_STORE_DELEGATE_MOCK_H
#define RELATIONAL_STORE_DELEGATE_MOCK_H
#include "rdb_general_store.h"
#include "resultset_mock.h"
#include <memory>
namespace DistributedDB {
class MockRelationalStoreDelegate : public DistributedDB::RelationalStoreDelegate {
public:
    ~MockRelationalStoreDelegate() = default;

    DBStatus Sync(const std::vector<std::string> &devices, DistributedDB::SyncMode mode, const Query &query,
        const SyncStatusCallback &onComplete, bool wait) override
    {
        return DBStatus::OK;
    }
    static inline int32_t cloudSyncTask = 0;
    static bool SetCloudSyncTaskCount(int32_t task)
    {
        cloudSyncTask = task;
        return true;
    }
    static inline int32_t deviceSyncTask = 0;
    static bool SetDeviceTaskCount(int32_t task)
    {
        deviceSyncTask = task;
        return true;
    }
    static inline int32_t downloadingAssetsCount = 0;
    static bool SetDownloadingAssetsCount(int32_t task)
    {
        downloadingAssetsCount = task;
        return true;
    }
    int32_t GetCloudSyncTaskCount() override
    {
        return cloudSyncTask;
    }
    std::pair<DBStatus, int32_t> GetDownloadingAssetsCount() override
    {
        return {DBStatus::OK, downloadingAssetsCount};
    }
    int32_t GetDeviceSyncTaskCount() override
    {
        return deviceSyncTask;
    }
    DBStatus RemoveDeviceData(const std::string &device, const std::string &tableName) override
    {
        return DBStatus::OK;
    }

    DBStatus RemoteQuery(const std::string &device, const RemoteCondition &condition, uint64_t timeout,
        std::shared_ptr<ResultSet> &result) override
    {
        if (device == "test") {
            result = nullptr;
            return DBStatus::DB_ERROR;
        }
        if (device == "device001") {
            result = std::make_shared<MockResultSet>();
            return DBStatus::OK;
        }
        return DBStatus::OK;
    }

    DBStatus RemoveDeviceData() override
    {
        return DBStatus::OK;
    }

    DBStatus Sync(const std::vector<std::string> &devices, DistributedDB::SyncMode mode, const Query &query,
        const SyncProcessCallback &onProcess, int64_t waitTime) override
    {
        return DBStatus::OK;
    }

    DBStatus SetCloudDB(const std::shared_ptr<ICloudDb> &cloudDb) override
    {
        return DBStatus::OK;
    }

    DBStatus SetCloudDbSchema(const DataBaseSchema &schema) override
    {
        return DBStatus::OK;
    }

    DBStatus RegisterObserver(StoreObserver *observer) override
    {
        return DBStatus::OK;
    }

    DBStatus UnRegisterObserver() override
    {
        return DBStatus::OK;
    }

    DBStatus UnRegisterObserver(StoreObserver *observer) override
    {
        return DBStatus::OK;
    }

    DBStatus SetIAssetLoader(const std::shared_ptr<IAssetLoader> &loader) override
    {
        return DBStatus::OK;
    }

    DBStatus Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess) override
    {
        return DBStatus::OK;
    }

    DBStatus SetTrackerTable(const TrackerSchema &schema) override
    {
        if (schema.tableName == "WITH_INVENTORY_DATA") {
            return DBStatus::WITH_INVENTORY_DATA;
        }
        if (schema.tableName == "test") {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus ExecuteSql(const SqlCondition &condition, std::vector<DistributedDB::VBucket> &records) override
    {
        if (condition.sql == "") {
            return DBStatus::DB_ERROR;
        }

        std::string sqls = "INSERT INTO test ( #flag, #float, #gid, #value) VALUES  ( ?, ?, ?, ?)";
        std::string sqlIn = " UPDATE test SET setSql WHERE whereSql";
        std::string sql = "REPLACE INTO test ( #flag, #float, #gid, #value) VALUES  ( ?, ?, ?, ?)";
        if (condition.sql == sqls || condition.sql == sqlIn || condition.sql == sql) {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus SetReference(const std::vector<TableReferenceProperty> &tableReferenceProperty) override
    {
        if (gTestResult) {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus CleanTrackerData(const std::string &tableName, int64_t cursor) override
    {
        return DBStatus::OK;
    }

    DBStatus Pragma(PragmaCmd cmd, PragmaData &pragmaData) override
    {
        return DBStatus::OK;
    }

    DBStatus UpsertData(const std::string &tableName, const std::vector<DistributedDB::VBucket> &records,
        RecordStatus status = RecordStatus::WAIT_COMPENSATED_SYNC) override
    {
        return DBStatus::OK;
    }

    DBStatus SetCloudSyncConfig(const CloudSyncConfig &config) override
    {
        return DBStatus::OK;
    }
    static bool gTestResult;
protected:
    DBStatus RemoveDeviceDataInner(const std::string &device, ClearMode mode) override
    {
        if (gTestResult) {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }

    DBStatus CreateDistributedTableInner(const std::string &tableName, TableSyncType type) override
    {
        if (tableName == "test") {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::OK;
    }
};
} // namespace DistributedDB
#endif