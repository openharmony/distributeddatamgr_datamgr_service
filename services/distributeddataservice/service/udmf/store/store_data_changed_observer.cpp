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

#define LOG_TAG "StoreDataChangedObserver"

#include "store_data_changed_observer.h"
#include "log_print.h"
#include "data_handler.h"
#include "delay_data_prepare_container.h"
#include "udmf_service_impl.h"
#include "synced_device_container.h"

namespace OHOS {
namespace UDMF {
void AcceptableInfoObserver::OnChange(const DistributedDB::KvStoreChangedData &data)
{
    ZLOGI("Received acceptable info changed notification.");
    auto insertedEntries = data.GetEntriesInserted();
    for (const auto &entry : insertedEntries) {
        std::string acceptableKey(entry.key.begin(), entry.key.end());
        DataLoadInfo info;
        auto status = DataHandler::UnmarshalDataLoadEntries(entry, info);
        if (status != E_OK) {
            ZLOGE("Unmarshal data load entries failed, key: %{public}s", acceptableKey.c_str());
            continue;
        }
        SyncedDeviceContainer::GetInstance().SaveSyncedDeviceInfo(info.udKey, info.deviceId);
        if (!DelayDataPrepareContainer::GetInstance().ExecDataLoadCallback(info.udKey, info)) {
            ZLOGE("Can not find data load callback, key: %{public}s", info.udKey.c_str());
        }
    }
}

void RuntimeObserver::OnChange(const DistributedDB::KvStoreChangedData &data)
{
    ZLOGI("Received runtime changed notification.");
    auto updatedEntries = data.GetEntriesUpdated();
    auto insertedEntries = data.GetEntriesInserted();
    updatedEntries.insert(updatedEntries.end(), insertedEntries.begin(), insertedEntries.end());
    for (const auto &entry : updatedEntries) {
        std::string udKey(entry.key.begin(), entry.key.end());
        UnifiedData data;
        std::vector<Entry> entries = { entry };
        auto status = DataHandler::UnmarshalEntries(udKey, entries, data);
        if (status != E_OK) {
            ZLOGE("Unmarshal runtime failed, key: %{public}s", udKey.c_str());
            continue;
        }
        auto runtime = data.GetRuntime();
        if (runtime == nullptr) {
            continue;
        }
        if (runtime->dataStatus == DataStatus::WORKING) {
            ZLOGI("Recieved delay data");
            auto service = UdmfServiceImpl::GetService();
            if (service != nullptr) {
                service->HandleRemoteDelayData(udKey);
            } else {
                ZLOGE("Get service null");
            }
        }
    }
}
} // namespace UDMF
} // namespace OHOS