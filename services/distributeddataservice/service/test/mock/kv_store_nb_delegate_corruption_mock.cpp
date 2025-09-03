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

#include "kv_store_nb_delegate_corruption_mock.h"

#include "store_types.h"
namespace DistributedDB {
DBStatus KvStoreNbDelegateCorruptionMock::Get(const Key &key, Value &value) const
{
    return DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB;
}

DBStatus KvStoreNbDelegateCorruptionMock::GetEntries(const Key &keyPrefix, std::vector<Entry> &entries) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::GetEntries(const Key &keyPrefix, KvStoreResultSet *&resultSet) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::GetEntries(const Query &query, std::vector<Entry> &entries) const
{
    return DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB;
}

DBStatus KvStoreNbDelegateCorruptionMock::GetEntries(const Query &query, KvStoreResultSet *&resultSet) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::GetCount(const Query &query, int &count) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::CloseResultSet(KvStoreResultSet *&resultSet)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::Put(const Key &key, const Value &value)
{
    return DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB;
}

DBStatus KvStoreNbDelegateCorruptionMock::PutBatch(const std::vector<Entry> &entries)
{
    return DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB;
}

DBStatus KvStoreNbDelegateCorruptionMock::DeleteBatch(const std::vector<Key> &keys)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::Delete(const Key &key)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::GetLocal(const Key &key, Value &value) const
{
    return DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB;
}

DBStatus KvStoreNbDelegateCorruptionMock::GetLocalEntries(const Key &keyPrefix, std::vector<Entry> &entries) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::PutLocal(const Key &key, const Value &value)
{
    return DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB;
}

DBStatus KvStoreNbDelegateCorruptionMock::DeleteLocal(const Key &key)
{
    return DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB;
}

DBStatus KvStoreNbDelegateCorruptionMock::PublishLocal(const Key &key, bool deleteLocal, bool updateTimestamp,
    const KvStoreNbPublishOnConflict &onConflict)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::UnpublishToLocal(const Key &key, bool deletePublic, bool updateTimestamp)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::RegisterObserver(const Key &key, unsigned int mode,
    KvStoreObserver *observer)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::UnRegisterObserver(const KvStoreObserver *observer)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::RemoveDeviceData(const std::string &device)
{
    return DBStatus::OK;
}

std::string KvStoreNbDelegateCorruptionMock::GetStoreId() const
{
    return "ok";
}

DBStatus KvStoreNbDelegateCorruptionMock::Sync(const std::vector<std::string> &devices, SyncMode mode,
    const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
    bool wait)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::Pragma(PragmaCmd cmd, PragmaData &paramData)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::SetConflictNotifier(int conflictType,
    const KvStoreNbConflictNotifier &notifier)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::Rekey(const CipherPassword &password)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::Export(const std::string &filePath,
    const CipherPassword &passwd, bool force)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::Import(const std::string &filePath, const CipherPassword &passwd,
    bool isNeedIntegrityCheck)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::StartTransaction()
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::Commit()
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::Rollback()
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::PutLocalBatch(const std::vector<Entry> &entries)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::DeleteLocalBatch(const std::vector<Key> &keys)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::GetSecurityOption(SecurityOption &option) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::SetRemotePushFinishedNotify(const RemotePushFinishedNotifier &notifier)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::Sync(const std::vector<std::string> &devices, SyncMode mode,
    const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
    const Query &query, bool wait)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::CheckIntegrity() const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::SetEqualIdentifier(const std::string &identifier,
    const std::vector<std::string> &targets)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::SetPushDataInterceptor(const PushDataInterceptor &interceptor)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::SubscribeRemoteQuery(const std::vector<std::string> &devices,
    const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
    const Query &query, bool wait)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::UnSubscribeRemoteQuery(const std::vector<std::string> &devices,
    const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete,
    const Query &query, bool wait)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::RemoveDeviceData()
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::GetKeys(const Key &keyPrefix, std::vector<Key> &keys) const
{
    return DBStatus::OK;
}

size_t KvStoreNbDelegateCorruptionMock::GetSyncDataSize(const std::string &device) const
{
    size_t size = 0;
    return size;
}

DBStatus KvStoreNbDelegateCorruptionMock::UpdateKey(const UpdateKeyCallback &callback)
{
    return DBStatus::OK;
}

std::pair<DBStatus, WatermarkInfo> KvStoreNbDelegateCorruptionMock::GetWatermarkInfo(const std::string &device)
{
    std::pair<DBStatus, WatermarkInfo> ret;
    return ret;
}

DBStatus KvStoreNbDelegateCorruptionMock::Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::SetCloudDB(const std::map<std::string, std::shared_ptr<ICloudDb>> &cloudDBs)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::SetCloudDbSchema(const std::map<std::string, DataBaseSchema> &schema)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::RemoveDeviceData(const std::string &device, ClearMode mode)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::RemoveDeviceData(const std::string &device,
    const std::string &user, ClearMode mode)
{
    return DBStatus::OK;
}

int32_t KvStoreNbDelegateCorruptionMock::GetTaskCount()
{
    int32_t taskCount = taskCountMock_;
    return taskCount;
}

void KvStoreNbDelegateCorruptionMock::SetGenCloudVersionCallback(const GenerateCloudVersionCallback &callback)
{
    auto callback_ = callback;
}

std::pair<DBStatus, std::map<std::string, std::string>> KvStoreNbDelegateCorruptionMock::GetCloudVersion(
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

DBStatus KvStoreNbDelegateCorruptionMock::SetReceiveDataInterceptor(const DataInterceptor &interceptor)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::SetCloudSyncConfig(const CloudSyncConfig &config)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::GetDeviceEntries(const std::string &device, std::vector<Entry> &entries) const
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::Sync(const DeviceSyncOption &option,
    const DeviceSyncProcessCallback &onProcess)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::Sync(const DeviceSyncOption &option,
    const std::function<void(const std::map<std::string, DBStatus> &devicesMap)> &onComplete)
{
    return DBStatus::OK;
}

DBStatus KvStoreNbDelegateCorruptionMock::CancelSync(uint32_t syncId)
{
    return DBStatus::OK;
}

KvStoreNbDelegate::DatabaseStatus KvStoreNbDelegateCorruptionMock::GetDatabaseStatus() const
{
    return {};
}

DBStatus KvStoreNbDelegateCorruptionMock::ClearMetaData(ClearKvMetaDataOption option)
{
    return DBStatus::OK;
}
} // namespace DistributedDB