/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#ifndef RELATIONAL_SYNC_ABLE_STORAGE_H
#define RELATIONAL_SYNC_ABLE_STORAGE_H
#ifdef RELATIONAL_STORE

#include "relational_db_sync_interface.h"
#include "relationaldb_properties.h"
#include "runtime_context.h"
#include "sqlite_single_relational_storage_engine.h"

#include "sqlite_single_ver_relational_continue_token.h"

namespace DistributedDB {
using RelationalObserverAction = std::function<void(const std::string &device)>;
class RelationalSyncAbleStorage : public RelationalDBSyncInterface, public virtual RefObject {
public:
    explicit RelationalSyncAbleStorage(StorageEngine *engine);
    ~RelationalSyncAbleStorage() override;

    // Get interface type of this kvdb.
    int GetInterfaceType() const override;

    // Get the interface ref-count, in order to access asynchronously.
    void IncRefCount() override;

    // Drop the interface ref-count.
    void DecRefCount() override;

    // Get the identifier of this kvdb.
    std::vector<uint8_t> GetIdentifier() const override;

    // Get the max timestamp of all entries in database.
    void GetMaxTimestamp(Timestamp &stamp) const override;

    // Get meta data associated with the given key.
    int GetMetaData(const Key &key, Value &value) const override;

    // Put meta data as a key-value entry.
    int PutMetaData(const Key &key, const Value &value) override;

    // Delete multiple meta data records in a transaction.
    int DeleteMetaData(const std::vector<Key> &keys) override;

    // Delete multiple meta data records with key prefix in a transaction.
    int DeleteMetaDataByPrefixKey(const Key &keyPrefix) const override;

    // Get all meta data keys.
    int GetAllMetaKeys(std::vector<Key> &keys) const override;

    const KvDBProperties &GetDbProperties() const override;

    // Get the data which would be synced with query condition
    int GetSyncData(QueryObject &query, const SyncTimeRange &timeRange,
        const DataSizeSpecInfo &dataSizeInfo, ContinueToken &continueStmtToken,
        std::vector<SingleVerKvEntry *> &entries) const override;

    int GetSyncDataNext(std::vector<SingleVerKvEntry *> &entries, ContinueToken &continueStmtToken,
        const DataSizeSpecInfo &dataSizeInfo) const override;

    int PutSyncDataWithQuery(const QueryObject &object, const std::vector<SingleVerKvEntry *> &entries,
        const DeviceID &deviceName) override;

    int RemoveDeviceData(const std::string &deviceName, bool isNeedNotify) override;

    RelationalSchemaObject GetSchemaInfo() const override;

    int GetSecurityOption(SecurityOption &option) const override;

    void NotifyRemotePushFinished(const std::string &deviceId) const override;

    // Get the timestamp when database created or imported
    int GetDatabaseCreateTimestamp(Timestamp &outTime) const override;

    // Get batch meta data associated with the given key.
    int GetBatchMetaData(const std::vector<Key> &keys, std::vector<Entry> &entries) const override;
    // Put batch meta data as a key-value entry vector
    int PutBatchMetaData(std::vector<Entry> &entries) override;

    std::vector<QuerySyncObject> GetTablesQuery() override;

    int LocalDataChanged(int notifyEvent, std::vector<QuerySyncObject> &queryObj) override;

    int InterceptData(std::vector<SingleVerKvEntry *> &entries, const std::string &sourceID,
        const std::string &targetID) const override
    {
        return E_OK;
    }

    int CheckAndInitQueryCondition(QueryObject &query) const override;
    void RegisterObserverAction(const RelationalObserverAction &action);
    void TriggerObserverAction(const std::string &deviceName);

    int CreateDistributedDeviceTable(const std::string &device, const RelationalSyncStrategy &syncStrategy) override;

    int RegisterSchemaChangedCallback(const std::function<void()> &callback) override;

    void NotifySchemaChanged();

    void RegisterHeartBeatListener(const std::function<void()> &listener);

    int GetCompressionAlgo(std::set<CompressAlgorithm> &algorithmSet) const override;

    bool CheckCompatible(const std::string &schema, uint8_t type) const override;

private:
    SQLiteSingleVerRelationalStorageExecutor *GetHandle(bool isWrite, int &errCode,
        OperatePerm perm = OperatePerm::NORMAL_PERM) const;
    void ReleaseHandle(SQLiteSingleVerRelationalStorageExecutor *&handle) const;
    int SetMaxTimestamp(Timestamp timestamp);

    // get
    int GetSyncDataForQuerySync(std::vector<DataItem> &dataItems, SQLiteSingleVerRelationalContinueToken *&token,
        const DataSizeSpecInfo &dataSizeInfo) const;

    // put
    int PutSyncData(const QueryObject &object, std::vector<DataItem> &dataItems, const std::string &deviceName);
    int SaveSyncDataItems(const QueryObject &object, std::vector<DataItem> &dataItems, const std::string &deviceName);

    // data
    SQLiteSingleRelationalStorageEngine *storageEngine_ = nullptr;
    Timestamp currentMaxTimestamp_ = 0;
    KvDBProperties properties_;
    mutable std::mutex maxTimestampMutex_;

    std::function<void()> onSchemaChanged_;
    mutable std::mutex onSchemaChangedMutex_;
    std::mutex dataChangeDeviceMutex_;
    RelationalObserverAction dataChangeDeviceCallback_;
    std::function<void()> heartBeatListener_;
    mutable std::mutex heartBeatMutex_;
};
}  // namespace DistributedDB
#endif
#endif // RELATIONAL_SYNC_ABLE_STORAGE_H