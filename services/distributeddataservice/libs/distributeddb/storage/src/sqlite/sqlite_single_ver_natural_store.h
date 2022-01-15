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
#ifndef SQLITE_SINGLE_VER_NATURAL_STORE_H
#define SQLITE_SINGLE_VER_NATURAL_STORE_H
#include <mutex>

#include "sync_able_kvdb.h"
#include "sqlite_single_ver_storage_engine.h"
#include "sqlite_utils.h"
#include "isyncer.h"
#include "single_ver_natural_store_commit_notify_data.h"
#include "single_ver_kvdb_sync_interface.h"
#include "kv_store_nb_conflict_data_impl.h"
#include "runtime_context.h"
#include "sqlite_single_ver_continue_token.h"

namespace DistributedDB {
class SQLiteSingleVerNaturalStore : public SyncAbleKvDB, public SingleVerKvDBSyncInterface {
public:
    SQLiteSingleVerNaturalStore();
    ~SQLiteSingleVerNaturalStore() override;

    // Delete the copy and assign constructors
    DISABLE_COPY_ASSIGN_MOVE(SQLiteSingleVerNaturalStore);

    // Open the database
    int Open(const KvDBProperties &kvDBProp) override;

    // Invoked automatically when connection count is zero
    void Close() override;

    // Create a connection object.
    GenericKvDBConnection *NewConnection(int &errCode) override;

    // Get interface type of this kvdb.
    int GetInterfaceType() const override;

    // Get the interface ref-count, in order to access asynchronously.
    void IncRefCount() override;

    // Drop the interface ref-count.
    void DecRefCount() override;

    // Get the identifier of this kvdb.
    std::vector<uint8_t> GetIdentifier() const override;

    // Get interface for syncer.
    IKvDBSyncInterface *GetSyncInterface() override;

    int GetMetaData(const Key &key, Value &value) const override;

    int PutMetaData(const Key &key, const Value &value) override;

    // Delete multiple meta data records in a transaction.
    int DeleteMetaData(const std::vector<Key> &keys) override;
    // Delete multiple meta data records with key prefix in a transaction.
    int DeleteMetaDataByPrefixKey(const Key &keyPrefix) const override;

    int GetAllMetaKeys(std::vector<Key> &keys) const override;

    int GetSyncData(TimeStamp begin, TimeStamp end, std::vector<DataItem> &dataItems, ContinueToken &continueStmtToken,
        const DataSizeSpecInfo &dataSizeInfo) const override;

    int GetSyncData(TimeStamp begin, TimeStamp end, std::vector<SingleVerKvEntry *> &entries,
        ContinueToken &continueStmtToken, const DataSizeSpecInfo &dataSizeInfo) const override;

    int GetSyncData(QueryObject &query, const SyncTimeRange &timeRange, const DataSizeSpecInfo &dataSizeInfo,
        ContinueToken &continueStmtToken, std::vector<SingleVerKvEntry *> &entries) const override;

    int GetSyncDataNext(std::vector<DataItem> &dataItems, ContinueToken &continueStmtToken,
        const DataSizeSpecInfo &dataSizeInfo) const override;

    int GetSyncDataNext(std::vector<SingleVerKvEntry *> &entries, ContinueToken &continueStmtToken,
        const DataSizeSpecInfo &dataSizeInfo) const override;

    void ReleaseContinueToken(ContinueToken &continueStmtToken) const override;

    int PutSyncDataWithQuery(const QueryObject &query, const std::vector<SingleVerKvEntry *> &entries,
        const std::string &deviceName) override;

    void GetMaxTimeStamp(TimeStamp &stamp) const override;

    int SetMaxTimeStamp(TimeStamp timestamp);

    int Rekey(const CipherPassword &passwd) override;

    int Export(const std::string &filePath, const CipherPassword &passwd) override;

    int Import(const std::string &filePath, const CipherPassword &passwd) override;

    // In sync procedure, call this function
    int RemoveDeviceData(const std::string &deviceName, bool isNeedNotify) override;

    // In local procedure, call this function
    int RemoveDeviceData(const std::string &deviceName, bool isNeedNotify, bool isInSync);

    SQLiteSingleVerStorageExecutor *GetHandle(bool isWrite, int &errCode,
        OperatePerm perm = OperatePerm::NORMAL_PERM) const;

    void ReleaseHandle(SQLiteSingleVerStorageExecutor *&handle) const;

    int TransObserverTypeToRegisterFunctionType(int observerType, RegisterFuncType &type) const override;

    int TransConflictTypeToRegisterFunctionType(int conflictType, RegisterFuncType &type) const override;

    bool CheckWritePermission() const override;

    SchemaObject GetSchemaInfo() const override;

    bool CheckCompatible(const std::string &schema) const override;

    TimeStamp GetCurrentTimeStamp();

    SchemaObject GetSchemaObject() const;

    const SchemaObject &GetSchemaObjectConstRef() const;

    const KvDBProperties &GetDbProperties() const override;

    int RemoveKvDB(const KvDBProperties &properties) override;

    int GetKvDBSize(const KvDBProperties &properties, uint64_t &size) const override;
    KvDBProperties &GetDbPropertyForUpdate();

    int InitDatabaseContext(const KvDBProperties &kvDBProp, bool isNeedUpdateSecOpt = false);

    int RegisterLifeCycleCallback(const DatabaseLifeCycleNotifier &notifier);

    int SetAutoLifeCycleTime(uint32_t time);

    int GetSecurityOption(SecurityOption &option) const override;

    bool IsDataMigrating() const override;

    void SetConnectionFlag(bool isExisted) const override;

    int TriggerToMigrateData() const;

    int CheckValueAndAmendIfNeed(ValueSource sourceType, const Value &oriValue, Value &amendValue,
        bool &useAmendValue) const;

    int CheckReadDataControlled() const;
    bool IsCacheDBMode() const;
    bool IsExtendedCacheDBMode() const;

    void IncreaseCacheRecordVersion() const;
    uint64_t GetCacheRecordVersion() const;
    uint64_t GetAndIncreaseCacheRecordVersion() const;

    void NotifyRemotePushFinished(const std::string &targetId) const override;

    int GetDatabaseCreateTimeStamp(TimeStamp &outTime) const override;

    int CheckIntegrity() const override;

    int GetCompressionOption(bool &needCompressOnSync, uint8_t &compressionRate) const override;
    int GetCompressionAlgo(std::set<CompressAlgorithm> &algorithmSet) const override;

    // Check and init query object for query sync and subscribe, flatbuffer schema will always return E_NOT_SUPPORT.
    // return E_OK if subscribe is legal, ERROR on exception.
    int CheckAndInitQueryCondition(QueryObject &query) const override;

    int InterceptData(std::vector<SingleVerKvEntry *> &entries, const std::string &sourceID,
        const std::string &targetID) const override;

    void SetDataInterceptor(const PushDataInterceptor &interceptor) override;

    int AddSubscribe(const std::string &subscribeId, const QueryObject &query, bool needCacheSubscribe) override;

    int RemoveSubscribe(const std::string &subscribeId) override;

    int RemoveSubscribe(const std::vector<std::string> &subscribeIds) override;

    int SetMaxLogSize(uint64_t limit);

    uint64_t GetMaxLogSize() const;

private:
    int CheckDatabaseRecovery(const KvDBProperties &kvDBProp);

    void CommitAndReleaseNotifyData(SingleVerNaturalStoreCommitNotifyData *&committedData,
        bool isNeedCommit, int eventType);

    int RegisterNotification();

    void ReleaseResources();

    void InitCurrentMaxStamp();

    int SaveSyncDataItems(const QueryObject &query, std::vector<DataItem> &dataItems, const DeviceInfo &deviceInfo,
        bool checkValueContent);

    int InitStorageEngine(const KvDBProperties &kvDBProp, bool isNeedUpdateSecOpt);

    void InitialLocalDataTimestamp();

    int GetSchema(SchemaObject &schema) const;

    static void InitDataBaseOption(const KvDBProperties &kvDBProp, OpenDbProperties &option);

    static int SetUserVer(const KvDBProperties &kvDBProp, int version);

    static std::string GetDatabasePath(const KvDBProperties &kvDBProp);
    static std::string GetSubDirPath(const KvDBProperties &kvDBProp);
    void NotifyRemovedData(std::vector<Entry> &entries);

    // Decide read only based on schema situation
    int DecideReadOnlyBaseOnSchema(const KvDBProperties &kvDBProp, bool &isReadOnly,
        SchemaObject &savedSchemaObj) const;

    void HeartBeatForLifeCycle() const;

    int StartLifeCycleTimer(const DatabaseLifeCycleNotifier &notifier) const;

    int ResetLifeCycleTimer() const;

    int StopLifeCycleTimer() const;
    void InitConflictNotifiedFlag(SingleVerNaturalStoreCommitNotifyData *committedData);

    void AsyncDataMigration() const;

    // Change value that should be amended, and neglect value that is incompatible
    void CheckAmendValueContentForSyncProcedure(std::vector<DataItem> &dataItems) const;

    int RemoveDeviceDataInCacheMode(const std::string &deviceName, bool isNeedNotify);

    int RemoveDeviceDataNormally(const std::string &deviceName, bool isNeedNotify);

    int SaveSyncDataToMain(const QueryObject &query, std::vector<DataItem> &dataItems, const DeviceInfo &deviceInfo);

    // Currently, this function only suitable to be call from sync in insert_record_from_sync procedure
    // Take attention if future coder attempt to call it in other situation procedure
    int SaveSyncItems(const QueryObject& query, std::vector<DataItem> &dataItems, const DeviceInfo &deviceInfo,
        TimeStamp &maxTimestamp, SingleVerNaturalStoreCommitNotifyData *commitData) const;

    int SaveSyncDataToCacheDB(const QueryObject &query, std::vector<DataItem> &dataItems,
        const DeviceInfo &deviceInfo);

    int SaveSyncItemsInCacheMode(SQLiteSingleVerStorageExecutor *handle, const QueryObject &query,
        std::vector<DataItem> &dataItems, const DeviceInfo &deviceInfo, TimeStamp &maxTimestamp) const;

    int ClearIncompleteDatabase(const KvDBProperties &kvDBPro) const;

    int GetSyncDataForQuerySync(std::vector<DataItem> &dataItems, SQLiteSingleVerContinueToken *&continueStmtToken,
        const DataSizeSpecInfo &dataSizeInfo) const;

    int SaveCreateDBTime();
    int SaveCreateDBTimeIfNotExisted();

    int GetAndInitStorageEngine(const KvDBProperties &kvDBProp);

    int RemoveAllSubscribe();

    DECLARE_OBJECT_TAG(SQLiteSingleVerNaturalStore);

    TimeStamp currentMaxTimeStamp_ = 0;
    SQLiteSingleVerStorageEngine *storageEngine_;
    bool notificationEventsRegistered_;
    bool notificationConflictEventsRegistered_;
    bool isInitialized_;
    bool isReadOnly_;
    mutable std::mutex syncerMutex_;
    mutable std::mutex initialMutex_;
    mutable std::mutex maxTimeStampMutex_;
    mutable std::mutex lifeCycleMutex_;
    mutable DatabaseLifeCycleNotifier lifeCycleNotifier_;
    mutable TimerId lifeTimerId_;
    uint32_t autoLifeTime_;
    mutable TimeStamp createDBTime_;
    mutable std::mutex createDBTimeMutex_;

    mutable std::shared_mutex dataInterceptorMutex_;
    PushDataInterceptor dataInterceptor_;
    std::atomic<uint64_t> maxLogSize_;
};
}
#endif
