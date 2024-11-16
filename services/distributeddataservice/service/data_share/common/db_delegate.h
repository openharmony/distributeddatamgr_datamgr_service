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

#ifndef DATASHARESERVICE_DB_DELEGATE_H
#define DATASHARESERVICE_DB_DELEGATE_H

#include <string>

#include "abs_shared_result_set.h"
#include "concurrent_map.h"
#include "datashare_predicates.h"
#include "datashare_result_set.h"
#include "datashare_values_bucket.h"
#include "executor_pool.h"
#include "metadata/store_meta_data.h"
#include "result_set.h"
#include "serializable/serializable.h"
#include "value_object.h"

namespace OHOS::DataShare {
class DBDelegate {
public:
    using Time = std::chrono::steady_clock::time_point;
    static std::shared_ptr<DBDelegate> Create(DistributedData::StoreMetaData &metaData,
        const std::string &extUri = "", const std::string &backup = "");
    virtual std::pair<int, std::shared_ptr<DataShareResultSet>> Query(const std::string &tableName,
        const DataSharePredicates &predicates, const std::vector<std::string> &columns,
        const int32_t callingPid) = 0;
    virtual std::string Query(
        const std::string &sql, const std::vector<std::string> &selectionArgs = std::vector<std::string>()) = 0;
    virtual std::shared_ptr<NativeRdb::ResultSet> QuerySql(const std::string &sql) = 0;
    virtual std::pair<int, int64_t> UpdateSql(const std::string &sql) = 0;
    virtual bool IsInvalid() = 0;
    static void SetExecutorPool(std::shared_ptr<ExecutorPool> executor);
    static void EraseStoreCache(const int32_t tokenId);
    virtual std::pair<int64_t, int64_t> InsertEx(const std::string &tableName,
        const DataShareValuesBucket &valuesBucket) = 0;
    virtual std::pair<int64_t, int64_t> UpdateEx(const std::string &tableName,
        const DataSharePredicates &predicate, const DataShareValuesBucket &valuesBucket) = 0;
    virtual std::pair<int64_t, int64_t> DeleteEx(const std::string &tableName,
        const DataSharePredicates &predicate) = 0;
private:
    static void GarbageCollect();
    static void StartTimer();
    struct Entity {
        explicit Entity(std::shared_ptr<DBDelegate> store);
        std::shared_ptr<DBDelegate> store_;
        Time time_;
    };
    static constexpr int NO_CHANGE_VERSION = -1;
    static constexpr int64_t INTERVAL = 20; //seconds
    static ConcurrentMap<uint32_t, std::map<std::string, std::shared_ptr<Entity>>> stores_;
    static std::shared_ptr<ExecutorPool> executor_;
    static ExecutorPool::TaskId taskId_;
};

class Id : public DistributedData::Serializable {
public:
    static constexpr int INVALID_USER = -1;
    Id(const std::string &id, const int32_t userId);
    ~Id() = default;
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    operator std::string()
    {
        return DistributedData::Serializable::Marshall(*this);
    }

private:
    std::string _id;
    int32_t userId;
};

class VersionData : public DistributedData::Serializable {
public:
    explicit VersionData(int version);
    bool Marshal(json &node) const override;
    bool Unmarshal(const json &node) override;
    virtual void SetVersion(int ver)
    {
        version = ver;
    };
    virtual int GetVersion() const
    {
        return version;
    };

private:
    int version;
};

class KvData {
public:
    explicit KvData(const Id &id);
    const std::string &GetId() const;
    virtual bool HasVersion() const = 0;
    virtual int GetVersion() const = 0;
    virtual std::string GetValue() const = 0;

private:
    std::string id;
};

class KvDBDelegate {
public:
    static constexpr const char *TEMPLATE_TABLE = "template_";
    static constexpr const char *DATA_TABLE = "data_";
    static std::shared_ptr<KvDBDelegate> GetInstance(
        bool reInit = false, const std::string &dir = "", const std::shared_ptr<ExecutorPool> &executors = nullptr);
    virtual ~KvDBDelegate() = default;
    virtual int32_t Upsert(const std::string &collectionName, const KvData &value) = 0;
    virtual int32_t Delete(const std::string &collectionName, const std::string &filter) = 0;
    virtual int32_t Get(const std::string &collectionName, const Id &id, std::string &value) = 0;
    virtual int32_t Get(const std::string &collectionName, const std::string &filter, const std::string &projection,
        std::string &result) = 0;
    virtual int32_t GetBatch(const std::string &collectionName, const std::string &filter,
        const std::string &projection, std::vector<std::string> &result) = 0;
    virtual void NotifyBackup() = 0;
};
} // namespace OHOS::DataShare
#endif // DATASHARESERVICE_DB_DELEGATE_H
