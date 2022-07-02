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
#include "observer_bridge.h"

#include "kvdb_service_client.h"
#include "kvstore_observer_client.h"
namespace OHOS::DistributedKv {
ObserverBridge::ObserverBridge(const AppId &app, const StoreId &store, std::shared_ptr<Observer> observer, Convert cvt)
    : appId_(app), storeId_(store), observer_(std::move(observer)), convert_(std::move(cvt))
{
}

ObserverBridge::~ObserverBridge()
{
    if (remote_ == nullptr) {
        return;
    }
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return;
    }
    service->Unsubscribe(appId_, storeId_, remote_);
}

Status ObserverBridge::RegisterRemoteObserver()
{
    if (remote_ != nullptr) {
        return SUCCESS;
    }

    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }

    remote_ = new (std::nothrow) ObserverClient(observer_, convert_);
    return service->Subscribe(appId_, storeId_, remote_);
}

Status ObserverBridge::UnregisterRemoteObserver()
{
    if (remote_ == nullptr) {
        return SUCCESS;
    }

    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }

    auto status = service->Unsubscribe(appId_, storeId_, remote_);
    remote_ = nullptr;
    return status;
}

void ObserverBridge::OnChange(const DBChangedData &data)
{
    std::string deviceId;
    auto inserted = ConvertDB(data.GetEntriesInserted(), deviceId);
    auto updated = ConvertDB(data.GetEntriesUpdated(), deviceId);
    auto deleted = ConvertDB(data.GetEntriesDeleted(), deviceId);
    ChangeNotification notice(std::move(inserted), std::move(updated), std::move(deleted), deviceId, false);
    observer_->OnChange(notice);
}

ObserverBridge::ObserverClient::ObserverClient(std::shared_ptr<Observer> observer, Convert &convert)
    : KvStoreObserverClient(observer), convert_(convert)
{
}

void ObserverBridge::ObserverClient::OnChange(const ChangeNotification &data)
{
    if (convert_ == nullptr) {
        KvStoreObserverClient::OnChange(data);
        return;
    }

    std::string deviceId;
    auto inserted = ConvertDB(data.GetInsertEntries(), deviceId);
    auto updated = ConvertDB(data.GetUpdateEntries(), deviceId);
    auto deleted = ConvertDB(data.GetDeleteEntries(), deviceId);
    ChangeNotification notice(std::move(inserted), std::move(updated), std::move(deleted), deviceId, false);
    KvStoreObserverClient::OnChange(notice);
}

std::vector<Entry> ObserverBridge::ObserverClient::ConvertDB(const std::vector<Entry> &dbEntries,
    std::string &deviceId) const
{
    std::vector<Entry> entries(dbEntries.size());
    auto it = entries.begin();
    for (const auto &dbEntry : dbEntries) {
        Entry &entry = *it;
        entry.key = convert_ ? convert_(dbEntry.key, deviceId) : Key(dbEntry.key);
        entry.value = dbEntry.value;
        ++it;
    }
    return entries;
}

std::vector<Entry> ObserverBridge::ConvertDB(const std::list<DBEntry> &dbEntries, std::string &deviceId) const
{
    std::vector<Entry> entries(dbEntries.size());
    auto it = entries.begin();
    for (const auto &dbEntry : dbEntries) {
        Entry &entry = *it;
        entry.key = convert_ ? convert_(dbEntry.key, deviceId) : Key(dbEntry.key);
        entry.value = dbEntry.value;
        ++it;
    }
    return entries;
}
} // namespace OHOS::DistributedKv
