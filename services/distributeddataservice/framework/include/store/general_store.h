/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_STORE_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_STORE_H
#include <errors.h>
#include <functional>
#include <memory>
#include <optional>
#include <set>

#include "cloud/cloud_conflict_handler.h"
#include "executor_pool.h"
#include "store/cursor.h"
#include "store/general_value.h"
#include "store/general_watcher.h"

namespace OHOS::DistributedData {
class CloudDB;
class AssetLoader;
struct Database;
class Snapshot;
class GeneralStore {
public:
    using Watcher = GeneralWatcher;
    using DetailAsync = GenAsync;
    using Devices = std::vector<std::string>;
    using Executor = ExecutorPool;
    using DBProperty = std::map<std::string, std::variant<std::monostate, uint32_t, std::string>>;
    enum SyncMode {
        NEARBY_BEGIN,
        NEARBY_PUSH = NEARBY_BEGIN,
        NEARBY_PULL,
        NEARBY_PULL_PUSH,
        NEARBY_END,
        CLOUD_BEGIN = 4,
        CLOUD_TIME_FIRST = CLOUD_BEGIN,
        CLOUD_NATIVE_FIRST,
        CLOUD_CLOUD_FIRST,
        CLOUD_CUSTOM_PUSH,
        CLOUD_CUSTOM_PULL,
        CLOUD_END,
        NEARBY_SUBSCRIBE_REMOTE,
        NEARBY_UNSUBSCRIBE_REMOTE,
        MODE_BUTT,
    };
    enum HighMode : uint32_t {
        MANUAL_SYNC_MODE = 0x00000,
        AUTO_SYNC_MODE = 0x10000,
        ASSETS_SYNC_MODE = 0x20000,
    };
    enum CleanMode {
        NEARBY_DATA = 0,
        CLOUD_DATA,
        CLOUD_INFO,
        LOCAL_DATA,
        CLEAN_WATER,
        CLOUD_NONE,
        CLEAN_MODE_BUTT
    };

    enum Area : int32_t {
        EL0,
        EL1,
        EL2,
        EL3,
        EL4,
        EL5
    };

    static inline uint32_t MixMode(uint32_t syncMode, uint32_t highMode)
    {
        return syncMode | highMode;
    }

    static inline uint32_t GetSyncMode(uint32_t mixMode)
    {
        return mixMode & 0xFFFF;
    }

    static inline uint32_t GetHighMode(uint32_t mixMode)
    {
        return mixMode & ~0xFFFF;
    }

    static inline uint32_t GetPriorityLevel(uint32_t highMode)
    {
        // shift right 16 bits
        return highMode >> 16;
    }

    struct BindInfo {
        BindInfo(std::shared_ptr<CloudDB> db = nullptr, std::shared_ptr<AssetLoader> loader = nullptr)
            : db_(std::move(db)), loader_(std::move(loader))
        {
        }

        bool operator<(const BindInfo &bindInfo) const
        {
            return db_ < bindInfo.db_;
        }

        std::shared_ptr<CloudDB> db_;
        std::shared_ptr<AssetLoader> loader_;
    };

    struct CloudConfig {
        int32_t maxNumber = 30;
        int32_t maxSize = 1024 * 512 * 3; // 1.5M
        int32_t maxRetryConflictTimes = 3;     // default max retry 3 times when version conflict
        bool isSupportEncrypt = false;
    };

    enum class DistributedTableMode : int {
        COLLABORATION = 0, // Save all devices data in user table
        SPLIT_BY_DEVICE // Save device data in each table split by device
    };

    struct StoreConfig {
        bool enableCloud_ = false;
        std::optional<DistributedTableMode> tableMode;
    };

    enum ErrOffset {
        DB_MODE_ID = 1,
        CLOUD_MODE_ID = 10,
    };

    enum ConflictResolution : uint8_t {
        ON_CONFLICT_NONE = 0,
        ON_CONFLICT_ROLLBACK,
        ON_CONFLICT_ABORT,
        ON_CONFLICT_FAIL,
        ON_CONFLICT_IGNORE,
        ON_CONFLICT_REPLACE,
    };

    static const int32_t DB_ERR_OFFSET = ErrCodeOffset(SUBSYS_DISTRIBUTEDDATAMNG, DB_MODE_ID);
    static const int32_t CLOUD_ERR_OFFSET = ErrCodeOffset(SUBSYS_DISTRIBUTEDDATAMNG, CLOUD_MODE_ID);

    virtual ~GeneralStore() = default;

    virtual void SetExecutor(std::shared_ptr<Executor> executor) = 0;

    virtual int32_t Bind(const Database &database, const std::map<uint32_t, BindInfo> &bindInfos,
        const CloudConfig &config) = 0;

    virtual bool IsBound(uint32_t user) = 0;

    virtual int32_t Execute(const std::string &table, const std::string &sql) = 0;

    virtual int32_t SetDistributedTables(
        const std::vector<std::string> &tables, int type, const std::vector<Reference> &references) = 0;

    virtual int32_t SetTrackerTable(const std::string &tableName, const std::set<std::string> &trackerColNames,
        const std::set<std::string> &extendColNames, bool isForceUpgrade) = 0;

    virtual int32_t Insert(const std::string &table, VBuckets &&values) = 0;

    virtual int32_t Replace(const std::string &table, VBucket &&value) = 0;

    virtual int32_t Update(const std::string &table, const std::string &setSql, Values &&values,
        const std::string &whereSql, Values &&conditions) = 0;

    virtual int32_t Delete(const std::string &table, const std::string &sql, Values &&args) = 0;

    virtual std::pair<int32_t, std::shared_ptr<Cursor>> Query(const std::string &table, const std::string &sql,
        Values &&args) = 0;

    virtual std::pair<int32_t, std::shared_ptr<Cursor>> Query(const std::string &table, GenQuery &query) = 0;

    virtual std::pair<int32_t, int64_t> Insert(const std::string &table, VBucket &&value,
        ConflictResolution resolution) = 0;
    virtual std::pair<int32_t, int64_t> BatchInsert(const std::string &table, VBuckets &&values,
        ConflictResolution resolution) = 0;
    virtual std::pair<int32_t, int64_t> Update(GenQuery &query, VBucket &&value, ConflictResolution resolution) = 0;
    virtual std::pair<int32_t, int64_t> Delete(GenQuery &query) = 0;
    virtual std::pair<int32_t, Value> Execute(const std::string &sql, Values &&args) = 0;
    virtual std::pair<int32_t, std::shared_ptr<Cursor>> Query(const std::string &sql, Values &&args = {},
        bool preCount = false) = 0;
    virtual std::pair<int32_t, std::shared_ptr<Cursor>> Query(GenQuery &query,
        const std::vector<std::string> &columns = {}, bool preCount = false) = 0;

    virtual std::pair<int32_t, int32_t> Sync(const Devices &devices, GenQuery &query,
        DetailAsync async, const SyncParam &syncParam) = 0;

    virtual std::pair<int32_t, std::shared_ptr<Cursor>> PreSharing(GenQuery &query) = 0;

    virtual int32_t Clean(const std::vector<std::string> &devices, int32_t mode, const std::string &tableName) = 0;

    virtual int32_t Clean(const std::string &device, int32_t mode, const std::vector<std::string> &tableList) = 0;

    virtual int32_t Watch(int32_t origin, Watcher &watcher) = 0;

    virtual int32_t Unwatch(int32_t origin, Watcher &watcher) = 0;

    virtual int32_t RegisterDetailProgressObserver(DetailAsync async) = 0;

    virtual int32_t UnregisterDetailProgressObserver() = 0;

    virtual int32_t Close(bool isForce = false) = 0;

    virtual int32_t AddRef() = 0;

    virtual int32_t Release() = 0;

    virtual int32_t BindSnapshots(std::shared_ptr<std::map<std::string, std::shared_ptr<Snapshot>>> bindAssets) = 0;

    virtual int32_t MergeMigratedData(const std::string &tableName, VBuckets &&values) = 0;

    virtual int32_t CleanTrackerData(const std::string &tableName, int64_t cursor) = 0;

    virtual void SetEqualIdentifier(const std::string &appId, const std::string &storeId, std::string account = "") {};

    virtual int32_t SetConfig(const StoreConfig &storeConfig) {};

    virtual std::pair<int32_t, uint32_t> LockCloudDB() = 0;

    virtual int32_t UnLockCloudDB() = 0;

    virtual int32_t UpdateDBStatus()
    {
        return 0;
    }
    virtual int32_t SetCloudConflictHandler(const std::shared_ptr<CloudConflictHandler> &handler)
    {
        return 0;
    }
    
    virtual int32_t StopCloudSync() = 0;

    virtual int32_t OnSyncTrigger(const std::string &storeId, int32_t triggerMode) = 0;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_STORE_H