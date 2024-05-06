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
#include <functional>
#include <memory>
#include <set>

#include "snapshot/snapshot.h"
#include "store/cursor.h"
#include "store/general_value.h"
#include "store/general_watcher.h"
namespace OHOS::DistributedData {
class CloudDB;
class AssetLoader;
struct Database;
class GeneralStore {
public:
    using Watcher = GeneralWatcher;
    using DetailAsync = GenAsync;
    using Devices = std::vector<std::string>;
    enum SyncMode {
        NEARBY_BEGIN,
        NEARBY_PUSH = NEARBY_BEGIN,
        NEARBY_PULL,
        NEARBY_PULL_PUSH,
        NEARBY_END,
        CLOUD_BEGIN = 4,
        CLOUD_TIME_FIRST = CLOUD_BEGIN,
        CLOUD_NATIVE_FIRST,
        CLOUD_ClOUD_FIRST,
        CLOUD_END,
        NEARBY_SUBSCRIBE_REMOTE,
        NEARBY_UNSUBSCRIBE_REMOTE,
        MODE_BUTT,
    };
    enum HighMode : uint32_t {
        MANUAL_SYNC_MODE = 0x00000,
        AUTO_SYNC_MODE = 0x10000,
    };
    enum CleanMode {
        NEARBY_DATA = 0,
        CLOUD_DATA,
        CLOUD_INFO,
        LOCAL_DATA,
        CLEAN_MODE_BUTT
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
    virtual ~GeneralStore() = default;

    virtual int32_t Bind(Database &database, const std::map<uint32_t, BindInfo> &bindInfos) = 0;

    virtual bool IsBound() = 0;

    virtual int32_t Execute(const std::string &table, const std::string &sql) = 0;

    virtual int32_t SetDistributedTables(
        const std::vector<std::string> &tables, int type, const std::vector<Reference> &references) = 0;

    virtual int32_t SetTrackerTable(const std::string &tableName, const std::set<std::string> &trackerColNames,
        const std::string &extendColName) = 0;

    virtual int32_t Insert(const std::string &table, VBuckets &&values) = 0;

    virtual int32_t Update(const std::string &table, const std::string &setSql, Values &&values,
        const std::string &whereSql, Values &&conditions) = 0;

    virtual int32_t Replace(const std::string &table, VBucket &&value) = 0;

    virtual int32_t Delete(const std::string &table, const std::string &sql, Values &&args) = 0;

    virtual std::shared_ptr<Cursor> Query(const std::string &table, const std::string &sql, Values &&args) = 0;

    virtual std::shared_ptr<Cursor> Query(const std::string &table, GenQuery &query) = 0;

    virtual int32_t Sync(const Devices &devices, GenQuery &query, DetailAsync async, SyncParam &syncParm) = 0;

    virtual std::shared_ptr<Cursor> PreSharing(GenQuery &query) = 0;

    virtual int32_t Clean(const std::vector<std::string> &devices, int32_t mode, const std::string &tableName) = 0;

    virtual int32_t Watch(int32_t origin, Watcher &watcher) = 0;

    virtual int32_t Unwatch(int32_t origin, Watcher &watcher) = 0;

    virtual int32_t RegisterDetailProgressObserver(DetailAsync async) = 0;

    virtual int32_t UnregisterDetailProgressObserver() = 0;

    virtual int32_t Close() = 0;

    virtual int32_t AddRef() = 0;

    virtual int32_t Release() = 0;

    virtual int32_t BindSnapshots(std::shared_ptr<std::map<std::string, std::shared_ptr<Snapshot>>> bindAssets) = 0;

    virtual int32_t MergeMigratedData(const std::string &tableName, VBuckets &&values) = 0;

    virtual std::vector<std::string> GetWaterVersion(const std::string &deviceId) = 0;
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_STORE_H
