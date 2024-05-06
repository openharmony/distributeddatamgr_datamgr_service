/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef OHOS_DISTRIBUTEDDATA_SERVICE_TEST_DB_STORE_MOCK_H
#define OHOS_DISTRIBUTEDDATA_SERVICE_TEST_DB_STORE_MOCK_H
#include "kv_store_nb_delegate.h"
#include "concurrent_map.h"
namespace OHOS {
namespace DistributedData {
class DBStoreMock : public DistributedDB::KvStoreNbDelegate {
public:
    using DBStatus = DistributedDB::DBStatus;
    using Entry = DistributedDB::Entry;
    using Key = DistributedDB::Key;
    using Value = DistributedDB::Value;
    using KvStoreResultSet = DistributedDB::KvStoreResultSet;
    using Query = DistributedDB::Query;
    using KvStoreNbPublishOnConflict = DistributedDB::KvStoreNbPublishOnConflict;
    using KvStoreObserver = DistributedDB::KvStoreObserver;
    using SyncMode = DistributedDB::SyncMode;
    using PragmaCmd = DistributedDB::PragmaCmd;
    using PragmaData = DistributedDB::PragmaData;
    using CipherPassword = DistributedDB::CipherPassword;
    using KvStoreNbConflictNotifier = DistributedDB::KvStoreNbConflictNotifier;
    using SecurityOption = DistributedDB::SecurityOption;
    using RemotePushFinishedNotifier = DistributedDB::RemotePushFinishedNotifier;
    using PushDataInterceptor = DistributedDB::PushDataInterceptor;
    using UpdateKeyCallback = DistributedDB::UpdateKeyCallback;
    using WatermarkInfo = DistributedDB::WatermarkInfo;
    using ClearMode = DistributedDB::ClearMode;
    using DataBaseSchema = DistributedDB::DataBaseSchema;
    using ICloudDb = DistributedDB::ICloudDb;
    using CloudSyncOption = DistributedDB::CloudSyncOption;
    using SyncProcessCallback = DistributedDB::SyncProcessCallback;
    using GenerateCloudVersionCallback = DistributedDB::GenerateCloudVersionCallback;
    DBStatus Get(const Key &key, Value &value) const override;
    DBStatus GetEntries(const Key &keyPrefix, std::vector<Entry> &entries) const override;
    DBStatus GetEntries(const Key &keyPrefix, KvStoreResultSet *&resultSet) const override;
    DBStatus GetEntries(const Query &query, std::vector<Entry> &entries) const override;
    DBStatus GetEntries(const Query &query, KvStoreResultSet *&resultSet) const override;
    DBStatus GetCount(const Query &query, int &count) const override;
    DBStatus CloseResultSet(KvStoreResultSet *&resultSet) override;
    DBStatus Put(const Key &key, const Value &value) override;
    DBStatus PutBatch(const std::vector<Entry> &entries) override;
    DBStatus DeleteBatch(const std::vector<Key> &keys) override;
    DBStatus Delete(const Key &key) override;
    DBStatus GetLocal(const Key &key, Value &value) const override;
    DBStatus GetLocalEntries(const Key &keyPrefix, std::vector<Entry> &entries) const override;
    DBStatus PutLocal(const Key &key, const Value &value) override;
    DBStatus DeleteLocal(const Key &key) override;
    DBStatus PublishLocal(const Key &key, bool deleteLocal, bool updateTimestamp,
        const KvStoreNbPublishOnConflict &onConflict) override;
    DBStatus UnpublishToLocal(const Key &key, bool deletePublic, bool updateTimestamp) override;
    DBStatus RegisterObserver(const Key &key, unsigned int mode, KvStoreObserver *observer) override;
    DBStatus UnRegisterObserver(const KvStoreObserver *observer) override;
    DBStatus RemoveDeviceData(const std::string &device) override;
    std::string GetStoreId() const override;
    DBStatus Sync(const std::vector<std::string> &devices, SyncMode mode,
        const std::function<void(const std::map<std::string, DBStatus> &)> &onComplete, bool wait) override;
    DBStatus Pragma(PragmaCmd cmd, PragmaData &paramData) override;
    DBStatus SetConflictNotifier(int conflictType, const KvStoreNbConflictNotifier &notifier) override;
    DBStatus Rekey(const CipherPassword &password) override;
    DBStatus Export(const std::string &filePath, const CipherPassword &passwd, bool force) override;
    DBStatus Import(const std::string &filePath, const CipherPassword &passwd) override;
    DBStatus StartTransaction() override;
    DBStatus Commit() override;
    DBStatus Rollback() override;
    DBStatus PutLocalBatch(const std::vector<Entry> &entries) override;
    DBStatus DeleteLocalBatch(const std::vector<Key> &keys) override;
    DBStatus GetSecurityOption(SecurityOption &option) const override;
    DBStatus SetRemotePushFinishedNotify(const RemotePushFinishedNotifier &notifier) override;
    DBStatus Sync(const std::vector<std::string> &devices, SyncMode mode,
        const std::function<void(const std::map<std::string, DBStatus> &)> &onComplete, const Query &query,
        bool wait) override;
    DBStatus CheckIntegrity() const override;
    DBStatus SetEqualIdentifier(const std::string &identifier, const std::vector<std::string> &targets) override;
    DBStatus SetPushDataInterceptor(const PushDataInterceptor &interceptor) override;
    DBStatus SubscribeRemoteQuery(const std::vector<std::string> &devices,
        const std::function<void(const std::map<std::string, DBStatus> &)> &onComplete, const Query &query,
        bool wait) override;
    DBStatus UnSubscribeRemoteQuery(const std::vector<std::string> &devices,
        const std::function<void(const std::map<std::string, DBStatus> &)> &onComplete, const Query &query,
        bool wait) override;
    DBStatus RemoveDeviceData() override;
    DBStatus GetKeys(const Key &keyPrefix, std::vector<Key> &keys) const override;
    size_t GetSyncDataSize(const std::string &device) const override;
    DBStatus UpdateKey(const UpdateKeyCallback &callback) override;
    std::pair<DBStatus, WatermarkInfo> GetWatermarkInfo(const std::string &device) override;
    DBStatus Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess) override;
    DBStatus SetCloudDB(const std::map<std::string, std::shared_ptr<ICloudDb>> &cloudDBs) override;
    DBStatus SetCloudDbSchema(const std::map<std::string, DataBaseSchema> &schema) override;
    DBStatus RemoveDeviceData(const std::string &device, ClearMode mode) override;
    DBStatus RemoveDeviceData(const std::string &device, const std::string &user, ClearMode mode) override;
    int32_t GetTaskCount() override;
    void SetGenCloudVersionCallback(const GenerateCloudVersionCallback &callback) override;
    std::pair<DBStatus, std::map<std::string, std::string>> GetCloudVersion(const std::string &device) override;
private:
    static const uint32_t DEFAULT_SIZE = 0;
    DBStatus Get(ConcurrentMap<Key, Value> &store, const Key &key, Value &value) const;
    DBStatus GetEntries(ConcurrentMap<Key, Value> &store, const Key &keyPrefix, std::vector<Entry> &entries) const;
    DBStatus PutBatch(ConcurrentMap<Key, Value> &store, const std::vector<Entry> &entries);
    DBStatus DeleteBatch(ConcurrentMap<Key, Value> &store, const std::vector<Key> &keys);
    mutable ConcurrentMap<Key, Value> entries_;
    mutable ConcurrentMap<Key, Value> localEntries_;
    mutable ConcurrentMap<KvStoreObserver *, std::set<Key>> observers_;
};
} // namespace DistributedData
} // namespace OHOS
#endif // OHOS_DISTRIBUTEDDATA_SERVICE_TEST_DB_STORE_MOCK_H
