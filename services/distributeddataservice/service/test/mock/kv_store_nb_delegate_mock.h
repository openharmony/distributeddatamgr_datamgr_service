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

#ifndef KV_STORE_NB_DELEGATE_H_MOCK
#define KV_STORE_NB_DELEGATE_H_MOCK

#include <functional>
#include <map>
#include <string>

#include "cloud/cloud_store_types.h"
#include "cloud/icloud_db.h"
#include "intercepted_data.h"
#include "iprocess_system_api_adapter.h"
#include "kv_store_nb_delegate.h"
#include "kv_store_nb_conflict_data.h"
#include "kv_store_observer.h"
#include "kv_store_result_set.h"
#include "query.h"
#include "store_types.h"

namespace DistributedDB {
class KvStoreNbDelegateMock : public DistributedDB::KvStoreNbDelegate {
public:
    int32_t taskCountMock_ = 0;
    ~KvStoreNbDelegateMock() = default;
    DBStatus Get(const Key &key, Value &value) const;
    DBStatus GetEntries(const Key &keyPrefix, std::vector<Entry> &entries) const;
    DBStatus GetEntries(const Key &keyPrefix, KvStoreResultSet *&resultSet) const;
    DBStatus GetEntries(const Query &query, std::vector<Entry> &entries) const;
    DBStatus GetEntries(const Query &query, KvStoreResultSet *&resultSet) const;
    DBStatus GetCount(const Query &query, int &count) const;
    DBStatus CloseResultSet(KvStoreResultSet *&resultSet);
    DBStatus Put(const Key &key, const Value &value);
    DBStatus PutBatch(const std::vector<Entry> &entries);
    DBStatus DeleteBatch(const std::vector<Key> &keys);
    DBStatus Delete(const Key &key);
    DBStatus GetLocal(const Key &key, Value &value) const;
    DBStatus GetLocalEntries(const Key &keyPrefix, std::vector<Entry> &entries) const;
    DBStatus PutLocal(const Key &key, const Value &value);
    DBStatus DeleteLocal(const Key &key);
    DBStatus PublishLocal(const Key &key, bool deleteLocal, bool updateTimestamp,
        const KvStoreNbPublishOnConflict &onConflict);
    DBStatus UnpublishToLocal(const Key &key, bool deletePublic, bool updateTimestamp);
    DBStatus RegisterObserver(const Key &key, unsigned int mode, KvStoreObserver *observer);
    DBStatus UnRegisterObserver(const KvStoreObserver *observer);
    DBStatus RemoveDeviceData(const std::string &device);
    std::string GetStoreId() const;
    DBStatus Sync(const std::vector<std::string> &devices, SyncMode mode,
        const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
        bool wait = false);
    DBStatus Pragma(PragmaCmd cmd, PragmaData &paramData);
    DBStatus SetConflictNotifier(int conflictType,
        const KvStoreNbConflictNotifier &notifier);
    DBStatus Rekey(const CipherPassword &password);
    DBStatus Export(const std::string &filePath, const CipherPassword &passwd, bool force = false);
    DBStatus Import(const std::string &filePath, const CipherPassword &passwd);
    DBStatus StartTransaction();
    DBStatus Commit();
    DBStatus Rollback();
    DBStatus PutLocalBatch(const std::vector<Entry> &entries);
    DBStatus DeleteLocalBatch(const std::vector<Key> &keys);
    DBStatus GetSecurityOption(SecurityOption &option) const;
    DBStatus SetRemotePushFinishedNotify(const RemotePushFinishedNotifier &notifier);
    DBStatus Sync(const std::vector<std::string> &devices, SyncMode mode,
        const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
        const Query &query, bool wait);
    DBStatus CheckIntegrity() const;
    DBStatus SetEqualIdentifier(const std::string &identifier,
        const std::vector<std::string> &targets);
    DBStatus SetPushDataInterceptor(const PushDataInterceptor &interceptor);
    DBStatus SubscribeRemoteQuery(const std::vector<std::string> &devices,
        const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
        const Query &query, bool wait);
    DBStatus UnSubscribeRemoteQuery(const std::vector<std::string> &devices,
        const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
        const Query &query, bool wait);
    DBStatus RemoveDeviceData();
    DBStatus GetKeys(const Key &keyPrefix, std::vector<Key> &keys) const;
    size_t GetSyncDataSize(const std::string &device) const;
    DBStatus UpdateKey(const UpdateKeyCallback &callback);
    std::pair<DBStatus, WatermarkInfo> GetWatermarkInfo(const std::string &device);
    DBStatus Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess);
    DBStatus SetCloudDB(const std::map<std::string, std::shared_ptr<ICloudDb>> &cloudDBs);
    DBStatus SetCloudDbSchema(const std::map<std::string, DataBaseSchema> &schema);
    DBStatus RemoveDeviceData(const std::string &device, ClearMode mode);
    DBStatus RemoveDeviceData(const std::string &device, const std::string &user, ClearMode mode);
    int32_t GetTaskCount();
    void SetGenCloudVersionCallback(const GenerateCloudVersionCallback &callback);
    std::pair<DBStatus, std::map<std::string, std::string>> GetCloudVersion(
        const std::string &device);
    DBStatus SetReceiveDataInterceptor(const DataInterceptor &interceptor);
    DBStatus SetCloudSyncConfig(const CloudSyncConfig &config);
    DBStatus GetDeviceEntries(const std::string &device, std::vector<Entry> &entries) const;
    DBStatus Sync(const DeviceSyncOption &option, const DeviceSyncProcessCallback &onProcess);
    DBStatus CancelSync(uint32_t syncId);
};
} // namespace DistributedDB
#endif // KV_STORE_NB_DELEGATE_H_MOCK