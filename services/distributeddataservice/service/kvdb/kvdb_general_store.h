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

#ifndef OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_KVDB_GENERAL_STORE_H
#define OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_KVDB_GENERAL_STORE_H

#include <atomic>
#include <functional>
#include <shared_mutex>

#include "kv_store_changed_data.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_nb_delegate.h"
#include "kvstore_sync_callback.h"
#include "kvstore_sync_manager.h"
#include "metadata/store_meta_data.h"
#include "store/general_store.h"
#include "store/general_value.h"
#include "store_observer.h"

namespace OHOS::DistributedKv {
using namespace DistributedData;
class KVDBGeneralStore : public DistributedData::GeneralStore {
public:
    using Value = DistributedData::Value;
    using GenErr = DistributedData::GeneralError;
    using DBStatus = DistributedDB::DBStatus;
    using DBOption = DistributedDB::KvStoreNbDelegate::Option;
    using DBSecurity = DistributedDB::SecurityOption;
    using DBPassword = DistributedDB::CipherPassword;

    explicit KVDBGeneralStore(const StoreMetaData &meta);
    ~KVDBGeneralStore();
    int32_t Bind(const Database &database,
        const std::map<uint32_t, BindInfo> &bindInfos, const CloudConfig &config) override;
    bool IsBound(uint32_t user) override;
    bool IsValid();
    int32_t Execute(const std::string &table, const std::string &sql) override;
    int32_t SetDistributedTables(
        const std::vector<std::string> &tables, int32_t type, const std::vector<Reference> &references) override;
    int32_t SetTrackerTable(const std::string &tableName, const std::set<std::string> &trackerColNames,
        const std::set<std::string> &extendColNames, bool isForceUpgrade = false) override;
    int32_t Insert(const std::string &table, VBuckets &&values) override;
    int32_t Update(const std::string &table, const std::string &setSql, Values &&values, const std::string &whereSql,
        Values &&conditions) override;
    int32_t Replace(const std::string &table, VBucket &&value) override;
    int32_t Delete(const std::string &table, const std::string &sql, Values &&args) override;
    std::pair<int32_t, std::shared_ptr<Cursor>> Query(
        const std::string &table, const std::string &sql, Values &&args) override;
    std::pair<int32_t, std::shared_ptr<Cursor>> Query(const std::string &table, GenQuery &query) override;
    std::pair<int32_t, int32_t> Sync(const Devices &devices, GenQuery &query, DetailAsync async,
        const SyncParam &syncParam) override;
    std::pair<int32_t, std::shared_ptr<Cursor>> PreSharing(GenQuery &query) override;
    int32_t Clean(const std::vector<std::string> &devices, int32_t mode, const std::string &tableName) override;
    int32_t Watch(int32_t origin, Watcher &watcher) override;
    int32_t Unwatch(int32_t origin, Watcher &watcher) override;
    int32_t RegisterDetailProgressObserver(DetailAsync async) override;
    int32_t UnregisterDetailProgressObserver() override;
    int32_t Close(bool isForce = false) override;
    int32_t AddRef() override;
    int32_t Release() override;
    int32_t BindSnapshots(std::shared_ptr<std::map<std::string, std::shared_ptr<Snapshot>>> bindAssets) override;
    int32_t MergeMigratedData(const std::string &tableName, VBuckets &&values) override;
    int32_t CleanTrackerData(const std::string &tableName, int64_t cursor) override;
    void SetEqualIdentifier(const std::string &appId, const std::string &storeId, std::string account = "") override;
    void SetConfig(const StoreConfig &storeConfig) override;
    void SetExecutor(std::shared_ptr<Executor> executor) override;
    static DBPassword GetDBPassword(const StoreMetaData &data);
    static DBOption GetDBOption(const StoreMetaData &data, const DBPassword &password);
    static DBSecurity GetDBSecurity(int32_t secLevel);
    std::pair<int32_t, uint32_t> LockCloudDB() override;
    int32_t UnLockCloudDB() override;

private:
    using KvDelegate = DistributedDB::KvStoreNbDelegate;
    using KvManager = DistributedDB::KvStoreDelegateManager;
    using SyncProcess = DistributedDB::SyncProcess;
    using DBSyncCallback = std::function<void(const std::map<std::string, DBStatus> &status)>;
    using DBProcessCB = std::function<void(const std::map<std::string, SyncProcess> &processes)>;
    static GenErr ConvertStatus(DBStatus status);
    void SetDBPushDataInterceptor(int32_t storeType);
    void SetDBReceiveDataInterceptor(int32_t storeType);
    std::vector<uint8_t> GetNewKey(std::vector<uint8_t> &key, const std::string &uuid);
    DBSyncCallback GetDBSyncCompleteCB(DetailAsync async);
    DBProcessCB GetDBProcessCB(DetailAsync async);
    DBStatus CloudSync(const Devices &devices, DistributedDB::SyncMode cloudSyncMode, DetailAsync async, int64_t wait,
        const std::string &prepareTraceId);
    void GetIdentifierParams(
        std::vector<std::string> &devices, const std::vector<std::string> &uuids, int32_t authType);
    void Report(const std::string &faultType, int32_t errCode, const std::string &appendix);
    class ObserverProxy : public DistributedDB::KvStoreObserver {
    public:
        using DBOrigin = DistributedDB::Origin;
        using DBChangeData = DistributedDB::ChangedData;
        using DBEntry = DistributedDB::Entry;
        using GenOrigin = Watcher::Origin;
        ~ObserverProxy() = default;
        void OnChange(DBOrigin origin, const std::string &originalId, DBChangeData &&data) override;
        void OnChange(const DistributedDB::KvStoreChangedData &data) override;
        void ConvertChangeData(const std::list<DBEntry> &entries, std::vector<Values> &values);
        bool HasWatcher() const
        {
            return watcher_ != nullptr;
        }

    private:
        friend KVDBGeneralStore;
        Watcher *watcher_ = nullptr;
        std::string storeId_;
    };

    static constexpr uint8_t META_COMPRESS_RATE = 10;
    ObserverProxy observer_;
    KvManager manager_;
    KvDelegate *delegate_ = nullptr;
    std::shared_mutex bindMutex_;
    std::map<uint32_t, BindInfo> bindInfos_{};
    std::mutex mutex_;
    int32_t ref_ = 1;
    mutable std::shared_timed_mutex rwMutex_;
    StoreInfo storeInfo_;

    static constexpr int32_t NO_ACCOUNT = 0;
    static constexpr int32_t IDENTICAL_ACCOUNT = 1;
    static constexpr const char *defaultAccountId = "ohosAnonymousUid";
    bool enableCloud_ = false;
    bool isPublic_ = false;
    static const std::map<DBStatus, GenErr> dbStatusMap_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_DATAMGR_SERVICE_KVDB_GENERAL_STORE_H