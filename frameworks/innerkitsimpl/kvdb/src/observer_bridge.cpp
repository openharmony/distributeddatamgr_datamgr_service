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
ObserverBridge::ObserverBridge(AppId appId, StoreId store, std::shared_ptr<Observer> observer, const Convertor &cvt)
    : appId_(std::move(appId)), storeId_(std::move(store)), observer_(std::move(observer)), convert_(cvt)
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
    auto status = service->Subscribe(appId_, storeId_, remote_);
    if (status != SUCCESS) {
        remote_ = nullptr;
    }
    return status;
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
    auto inserted = ConvertDB(data.GetEntriesInserted(), deviceId, convert_);
    auto updated = ConvertDB(data.GetEntriesUpdated(), deviceId, convert_);
    auto deleted = ConvertDB(data.GetEntriesDeleted(), deviceId, convert_);
    ChangeNotification notice(std::move(inserted), std::move(updated), std::move(deleted), deviceId, false);
    observer_->OnChange(notice);
}

ObserverBridge::ObserverClient::ObserverClient(std::shared_ptr<Observer> observer, const Convertor &cvt)
    : KvStoreObserverClient(observer), convert_(cvt)
{
}

void ObserverBridge::ObserverClient::OnChange(const ChangeNotification &data)
{
    std::string deviceId;
    auto inserted = ObserverBridge::ConvertDB(data.GetInsertEntries(), deviceId, convert_);
    auto updated = ObserverBridge::ConvertDB(data.GetUpdateEntries(), deviceId, convert_);
    auto deleted = ObserverBridge::ConvertDB(data.GetDeleteEntries(), deviceId, convert_);
    ChangeNotification notice(std::move(inserted), std::move(updated), std::move(deleted), deviceId, false);
    KvStoreObserverClient::OnChange(notice);
}

template<class T>
std::vector<Entry> ObserverBridge::ConvertDB(const T &dbEntries, std::string &deviceId, const Convertor &convert)
{
    std::vector<Entry> entries(dbEntries.size());
    auto it = entries.begin();
    for (const auto &dbEntry : dbEntries) {
        Entry &entry = *it;
        entry.key = convert.ToKey(DBKey(dbEntry.key), deviceId);
        entry.value = dbEntry.value;
        ++it;
    }
    return entries;
}
} // namespace OHOS::DistributedKv
