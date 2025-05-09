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

#include "kv_store_nb_delegate_mock.h"

#include "store_types.h"
namespace DistributedDB {
DBStatus KvStoreNbDelegateMock::Get(const Key &key, Value &value) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::GetEntries(const Key &keyPrefix, std::vector<Entry> &entries) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::GetEntries(const Key &keyPrefix, KvStoreResultSet *&resultSet) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::GetEntries(const Query &query, std::vector<Entry> &entries) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::GetEntries(const Query &query, KvStoreResultSet *&resultSet) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::GetCount(const Query &query, int &count) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::CloseResultSet(KvStoreResultSet *&resultSet)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::Put(const Key &key, const Value &value)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::PutBatch(const std::vector<Entry> &entries)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::DeleteBatch(const std::vector<Key> &keys)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::Delete(const Key &key)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::GetLocal(const Key &key, Value &value) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::GetLocalEntries(const Key &keyPrefix, std::vector<Entry> &entries) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::PutLocal(const Key &key, const Value &value)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::DeleteLocal(const Key &key)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::PublishLocal(const Key &key, bool deleteLocal, bool updateTimestamp,
    const KvStoreNbPublishOnConflict &onConflict)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::UnpublishToLocal(const Key &key, bool deletePublic, bool updateTimestamp)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::RegisterObserver(const Key &key, unsigned int mode,
    KvStoreObserver *observer)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::UnRegisterObserver(const KvStoreObserver *observer)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::RemoveDeviceData(const std::string &device)
{
    return DBStatus::OK;
}

std::string KvStoreNbDelegateMock::GetStoreId() const
{
    return "ok";
}

DBStatus KvStoreNbDelegateMock::Sync(const std::vector<std::string> &devices, SyncMode mode,
    const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
    bool wait)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::Pragma(PragmaCmd cmd, PragmaData &paramData)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::SetConflictNotifier(int conflictType,
    const KvStoreNbConflictNotifier &notifier)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::Rekey(const CipherPassword &password)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::Export(const std::string &filePath,
    const CipherPassword &passwd, bool force)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::Import(const std::string &filePath, const CipherPassword &passwd,
    bool isNeedIntegrityCheck)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::StartTransaction()
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::Commit()
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::Rollback()
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::PutLocalBatch(const std::vector<Entry> &entries)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::DeleteLocalBatch(const std::vector<Key> &keys)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::GetSecurityOption(SecurityOption &option) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::SetRemotePushFinishedNotify(const RemotePushFinishedNotifier &notifier)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::Sync(const std::vector<std::string> &devices, SyncMode mode,
    const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
    const Query &query, bool wait)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::CheckIntegrity() const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::SetEqualIdentifier(const std::string &identifier,
    const std::vector<std::string> &targets)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::SetPushDataInterceptor(const PushDataInterceptor &interceptor)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::SubscribeRemoteQuery(const std::vector<std::string> &devices,
    const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
    const Query &query, bool wait)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::UnSubscribeRemoteQuery(const std::vector<std::string> &devices,
    const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
    const Query &query, bool wait)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::RemoveDeviceData()
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::GetKeys(const Key &keyPrefix, std::vector<Key> &keys) const
{
    return DBStatus::OK;
}

size_t KvStoreNbDelegateMock::GetSyncDataSize(const std::string &device) const
{
    size_t size = 0;
    return size;
}

DBStatus KvStoreNbDelegateMock::UpdateKey(const UpdateKeyCallback &callback)
{
    return DBStatus::OK;
}

std::pair<DBStatus, WatermarkInfo> KvStoreNbDelegateMock::GetWatermarkInfo(const std::string &device)
{
    std::pair<DBStatus, WatermarkInfo> ret;
    return ret;
}

DBStatus KvStoreNbDelegateMock::Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::SetCloudDB(const std::map<std::string, std::shared_ptr<ICloudDb>> &cloudDBs)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::SetCloudDbSchema(const std::map<std::string, DataBaseSchema> &schema)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::RemoveDeviceData(const std::string &device, ClearMode mode)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::RemoveDeviceData(const std::string &device, const std::string &user, ClearMode mode)
{
    return DBStatus::OK;
}

int32_t KvStoreNbDelegateMock::GetTaskCount()
{
    int32_t taskCount = taskCountMock_;
    return taskCount;
}

void KvStoreNbDelegateMock::SetGenCloudVersionCallback(const GenerateCloudVersionCallback &callback)
{
    auto callback_ = callback;
}

std::pair<DBStatus, std::map<std::string, std::string>> KvStoreNbDelegateMock::GetCloudVersion(
    const std::string &device)
{
    if (device.empty()) {
        return { DBStatus::OK, {} };
    } else if (device == "test") {
        return { DBStatus::DB_ERROR, {} };
    } else if (device == "device") {
        return { DBStatus::DB_ERROR, {{device, device}} };
    } else {
        return { DBStatus::OK, {{device, device}} };
    }
}

DBStatus KvStoreNbDelegateMock::SetReceiveDataInterceptor(const DataInterceptor &interceptor)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::SetCloudSyncConfig(const CloudSyncConfig &config)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::GetDeviceEntries(const std::string &device, std::vector<Entry> &entries) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::Sync(const DeviceSyncOption &option, const DeviceSyncProcessCallback &onProcess)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateMock::CancelSync(uint32_t syncId)
{
    return DBStatus::OK;
}

KvStoreNbDelegate::DatabaseStatus KvStoreNbDelegateMock::GetDatabaseStatus() const
{
    return {};
}

DBStatus KvStoreNbDelegateMock::ClearMetaData(ClearKvMetaDataOption option)
{
    return DBStatus::OK;
}
} // namespace DistributedDB