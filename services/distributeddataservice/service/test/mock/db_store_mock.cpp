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
#include "db_store_mock.h"
#include "db_change_data_mock.h"
namespace OHOS {
namespace DistributedData {
using namespace DistributedDB;
DBStatus DBStoreMock::Get(const Key &key, Value &value) const
{
    return Get(entries_, key, value);
}

DBStatus DBStoreMock::GetEntries(const Key &keyPrefix, std::vector<Entry> &entries) const
{
    return GetEntries(entries_, keyPrefix, entries);
}

DBStatus DBStoreMock::GetEntries(const Key &keyPrefix, KvStoreResultSet *&resultSet) const
{
    resultSet = nullptr;
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::GetEntries(const Query &query, std::vector<Entry> &entries) const
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::GetEntries(const Query &query, KvStoreResultSet *&resultSet) const
{
    resultSet = nullptr;
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::GetCount(const Query &query, int &count) const
{
    count = 0;
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::CloseResultSet(KvStoreResultSet *&resultSet)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::Put(const Key &key, const Value &value)
{
    return PutBatch(entries_, { { key, value } });
}

DBStatus DBStoreMock::PutBatch(const std::vector<Entry> &entries)
{
    return PutBatch(entries_, entries);
}

DBStatus DBStoreMock::DeleteBatch(const std::vector<Key> &keys)
{
    return DeleteBatch(entries_, keys);
}

DBStatus DBStoreMock::Delete(const Key &key)
{
    return DeleteBatch(entries_, { key });
}

DBStatus DBStoreMock::GetLocal(const Key &key, Value &value) const
{
    return Get(localEntries_, key, value);
}

DBStatus DBStoreMock::GetLocalEntries(const Key &keyPrefix, std::vector<Entry> &entries) const
{
    return GetEntries(localEntries_, keyPrefix, entries);
}

DBStatus DBStoreMock::PutLocal(const Key &key, const Value &value)
{
    return PutBatch(localEntries_, { { key, value } });
}

DBStatus DBStoreMock::DeleteLocal(const Key &key)
{
    return DeleteBatch(localEntries_, { key });
}

DBStatus DBStoreMock::PublishLocal(const Key &key, bool deleteLocal, bool updateTimestamp,
    const KvStoreNbPublishOnConflict &onConflict)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::UnpublishToLocal(const Key &key, bool deletePublic, bool updateTimestamp)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::RegisterObserver(const Key &key, unsigned int mode, KvStoreObserver *observer)
{
    observers_.Compute(observer, [key](auto &, std::set<Key> &prefixes) {
        prefixes.insert(key);
        return !prefixes.empty();
    });
    return OK;
}

DBStatus DBStoreMock::UnRegisterObserver(const KvStoreObserver *observer)
{
    KvStoreObserver *key = const_cast<KvStoreObserver *>(observer);
    observers_.Erase(key);
    return OK;
}

DBStatus DBStoreMock::RemoveDeviceData(const std::string &device)
{
    return NOT_SUPPORT;
}

std::string DBStoreMock::GetStoreId() const
{
    return std::string();
}

DBStatus DBStoreMock::Sync(const std::vector<std::string> &devices, SyncMode mode,
    const std::function<void(const std::map<std::string, DBStatus> &)> &onComplete, bool wait)
{
    std::map<std::string, DBStatus> result;
    for (const auto &device : devices) {
        result[device] = OK;
    }
    onComplete(result);
    return OK;
}

DBStatus DBStoreMock::Pragma(PragmaCmd cmd, PragmaData &paramData)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::SetConflictNotifier(int conflictType, const KvStoreNbConflictNotifier &notifier)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::Rekey(const CipherPassword &password)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::Export(const std::string &filePath, const CipherPassword &passwd, bool force)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::Import(const std::string &filePath, const CipherPassword &passwd)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::StartTransaction()
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::Commit()
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::Rollback()
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::PutLocalBatch(const std::vector<Entry> &entries)
{
    return PutBatch(localEntries_, entries);
}

DBStatus DBStoreMock::DeleteLocalBatch(const std::vector<Key> &keys)
{
    return DeleteBatch(localEntries_, keys);
}

DBStatus DBStoreMock::GetSecurityOption(SecurityOption &option) const
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::SetRemotePushFinishedNotify(const RemotePushFinishedNotifier &notifier)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::Sync(const std::vector<std::string> &devices, SyncMode mode,
    const std::function<void(const std::map<std::string, DBStatus> &)> &onComplete, const Query &query, bool wait)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::CheckIntegrity() const
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::SetEqualIdentifier(const std::string &identifier, const std::vector<std::string> &targets)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::SetPushDataInterceptor(const PushDataInterceptor &interceptor)
{
    return NOT_SUPPORT;
}
DBStatus DBStoreMock::SubscribeRemoteQuery(const std::vector<std::string> &devices,
    const std::function<void(const std::map<std::string, DBStatus> &)> &onComplete, const Query &query, bool wait)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::UnSubscribeRemoteQuery(const std::vector<std::string> &devices,
    const std::function<void(const std::map<std::string, DBStatus> &)> &onComplete, const Query &query, bool wait)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::RemoveDeviceData()
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::GetKeys(const Key &keyPrefix, std::vector<Key> &keys) const
{
    return NOT_SUPPORT;
}

size_t DBStoreMock::GetSyncDataSize(const std::string &device) const
{
    return DEFAULT_SIZE;
}

DBStatus DBStoreMock::Get(ConcurrentMap<Key, Value> &store, const Key &key, Value &value) const
{
    auto it = store.Find(key);
    value = std::move(it.second);
    return it.first ? OK : NOT_FOUND;
}

DBStatus DBStoreMock::GetEntries(ConcurrentMap<Key, Value> &store, const Key &keyPrefix,
    std::vector<Entry> &entries) const
{
    store.ForEach([&entries, &keyPrefix](const Key &key, Value &value) {
        auto it = std::search(key.begin(), key.end(), keyPrefix.begin(), keyPrefix.end());
        if (it == key.begin()) {
            entries.push_back({key, value});  
        }
        return false;
    });
    return entries.size() > 0 ? OK : NOT_FOUND;
}

DBStatus DBStoreMock::PutBatch(ConcurrentMap<Key, Value> &store, const std::vector<Entry> &entries)
{
    for (auto &entry : entries) {
        store.InsertOrAssign(entry.key, entry.value);
    }
    DBChangeDataMock changeData({}, entries, {});
    observers_.ForEachCopies([&changeData](const auto &observer, auto &keys) mutable {
        if (observer) {
            observer->OnChange(changeData);
        }
        return false;
    });
    return OK;
}

DBStatus DBStoreMock::DeleteBatch(ConcurrentMap<Key, Value> &store, const std::vector<Key> &keys)
{
    DBChangeDataMock changeData({}, {}, {});
    for (auto &key : keys) {
        auto it = store.Find(key);
        if (!it.first) {
            continue;
        }
        changeData.AddEntry({key, std::move(it.second)});
        store.Erase(key);
    }

    observers_.ForEachCopies([&changeData](const auto &observer, auto &keys) {
        if (observer) {
            observer->OnChange(changeData);
        }
        return false;
    });
    return OK;
}

DBStatus DBStoreMock::UpdateKey(const UpdateKeyCallback &callback)
{
    return NOT_SUPPORT;
}

std::pair<DBStatus, WatermarkInfo> DBStoreMock::GetWatermarkInfo(const std::string &device)
{
    WatermarkInfo mark;
    return { DBStatus::OK, mark };
}

DBStatus DBStoreMock::Sync(const CloudSyncOption &option, const SyncProcessCallback &onProcess)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::SetCloudDB(const std::map<std::string, std::shared_ptr<ICloudDb>> &cloudDBs)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::SetCloudDbSchema(const std::map<std::string, DataBaseSchema> &schema)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::RemoveDeviceData(const std::string &device, ClearMode mode)
{
    return NOT_SUPPORT;
}

DBStatus DBStoreMock::RemoveDeviceData(const std::string &device, const std::string &user,
    ClearMode mode)
{
    return NOT_SUPPORT;
}

int32_t DBStoreMock::GetTaskCount()
{
    return 0;
}

void DBStoreMock::SetGenCloudVersionCallback(const GenerateCloudVersionCallback &callback)
{
}

std::pair<DBStatus, std::map<std::string, std::string>> DBStoreMock::GetCloudVersion(const std::string &device)
{
    return { NOT_SUPPORT, {} };
}
} // namespace DistributedData
} // namespace OHOS