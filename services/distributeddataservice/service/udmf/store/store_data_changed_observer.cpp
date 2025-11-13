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
void RuntimeObserver::OnChange(const DistributedDB::KvStoreChangedData &data)
{
    auto updatedEntries = data.GetEntriesUpdated();
    auto insertedEntries = data.GetEntriesInserted();
    updatedEntries.insert(updatedEntries.end(), insertedEntries.begin(), insertedEntries.end());
    for (const auto &entry : updatedEntries) {
        std::string udKey(entry.key.begin(), entry.key.end());
        UnifiedData changedData;
        std::vector<Entry> entries = { entry };
        auto status = DataHandler::UnmarshalEntries(udKey, entries, changedData);
        if (status != E_OK) {
            ZLOGE("Unmarshal runtime failed, key: %{public}s", udKey.c_str());
            continue;
        }
        auto runtime = changedData.GetRuntime();
        if (runtime == nullptr) {
            ZLOGE("Runtime is null, key: %{public}s", udKey.c_str());
            continue;
        }
        if (runtime->dataStatus == DataStatus::WORKING) {
            auto service = UdmfServiceImpl::GetService();
            if (service == nullptr) {
                ZLOGE("Get service null");
                return;
            }
            service->HandleRemoteDelayData(udKey);
        }
    }
}
} // namespace UDMF
} // namespace OHOS