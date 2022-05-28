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
namespace OHOS::DistributedKv {
void ObserverBridge::OnChange(const DistributedDB::KvStoreChangedData &data)
{
    std::string deviceId;
    ChangeNotification notification(ConvertDB(data.GetEntriesInserted(), deviceId),
        ConvertDB(data.GetEntriesUpdated(), deviceId), ConvertDB(data.GetEntriesDeleted(), deviceId), deviceId, false);

    observer_->OnChange(notification);
}

ObserverBridge::ObserverBridge(std::shared_ptr<DistributedKv::KvStoreObserver> observer, Convert convert)
    : observer_(std::move(observer)), convert_(std::move(convert))
{
}

std::vector<Entry> ObserverBridge::ConvertDB(
    const std::list<DistributedDB::Entry> &dbEntries, std::string &deviceId) const
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
