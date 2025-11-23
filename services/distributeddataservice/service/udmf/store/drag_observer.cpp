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

#define LOG_TAG "DragObserver"

#include "drag_observer.h"
#include "log_print.h"
#include "data_handler.h"
#include "delay_data_prepare_container.h"
#include "delay_data_acquire_container.h"
#include "lifecycle_manager.h"
#include "udmf_service_impl.h"
#include "synced_device_container.h"

namespace OHOS {
namespace UDMF {
constexpr int32_t SLASH_COUNT_IN_KEY = 4;

void DragObserver::OnChange(const DistributedDB::KvStoreChangedData &data)
{
    std::vector<DistributedDB::Entry> runtimeEntries;
    std::vector<DistributedDB::Entry> deleteRuntimeEntries;
    ExtractEntries(data, runtimeEntries, deleteRuntimeEntries);
    if (!runtimeEntries.empty()) {
        ProcessRuntimeInfo(runtimeEntries);
    }
    if (!deleteRuntimeEntries.empty()) {
        ProcessDelete(deleteRuntimeEntries);
    }
}

void DragObserver::ExtractEntries(const DistributedDB::KvStoreChangedData &data,
    std::vector<DistributedDB::Entry> &runtimeEntries,
    std::vector<DistributedDB::Entry> &deleteRuntimeEntries)
{
    const auto &insertedEntries = data.GetEntriesInserted();
    const auto &updatedEntries = data.GetEntriesUpdated();
    const auto &deleteEntries = data.GetEntriesDeleted();
    runtimeEntries.reserve(updatedEntries.size() + insertedEntries.size());
    deleteRuntimeEntries.reserve(deleteEntries.size());

    CollectIfRuntimeKey(insertedEntries, runtimeEntries);
    CollectIfRuntimeKey(updatedEntries, runtimeEntries);
    CollectIfRuntimeKey(deleteEntries, deleteRuntimeEntries);
}

void DragObserver::CollectIfRuntimeKey(const std::list<DistributedDB::Entry> &srcEntries,
    std::vector<DistributedDB::Entry> &dstEntries)
{
    for (const auto &entry : srcEntries) {
        if (IsRuntimeKey(entry.key)) {
            dstEntries.emplace_back(entry);
        }
    }
}

void DragObserver::ProcessRuntimeInfo(const std::vector<DistributedDB::Entry> &runtimeEntries)
{
    std::vector<std::string> runtimeKeys;
    runtimeKeys.reserve(runtimeEntries.size());
    for (const auto &entry : runtimeEntries) {
        runtimeKeys.emplace_back(entry.key.begin(), entry.key.end());
    }

    std::vector<Runtime> runtimes;
    auto status = DataHandler::UnmarshalRuntimes(runtimeKeys, runtimeEntries, runtimes);
    if (status != E_OK) {
        ZLOGE("Unmarshal runtime failed, key count: %{public}zu", runtimeKeys.size());
        return;
    }
    auto service = UdmfServiceImpl::GetService();
    for (const auto &runtime : runtimes) {
        if (runtime.dataStatus != DataStatus::WORKING) {
            continue;
        }
        LifeCycleManager::GetInstance().InsertUdKey(runtime.tokenId, runtime.key.key);
        if (service && DelayDataAcquireContainer::GetInstance().IsContainDelayData(runtime.key.key)) {
            service->HandleRemoteDelayData(runtime.key.key);
        }
    }
    ZLOGI("Processed %{public}zu runtimes", runtimes.size());
}

void DragObserver::ProcessDelete(const std::vector<DistributedDB::Entry> &deleteEntries)
{
    std::vector<std::string> runtimeKeys;
    runtimeKeys.reserve(deleteEntries.size());
    for (const auto &entry : deleteEntries) {
        runtimeKeys.emplace_back(entry.key.begin(), entry.key.end());
    }

    std::vector<Runtime> runtimes;
    auto status = DataHandler::UnmarshalRuntimes(runtimeKeys, deleteEntries, runtimes);
    if (status != E_OK) {
        ZLOGE("Unmarshal runtime failed, key count: %{public}zu", runtimeKeys.size());
        return;
    }
    for (auto &runtime : runtimes) {
        runtime.key.GetUnifiedKey();
        LifeCycleManager::GetInstance().OnGot(runtime.key, runtime.tokenId, false);
    }
}

bool DragObserver::IsRuntimeKey(const std::vector<uint8_t> &key)
{
    return std::count(key.begin(), key.end(), '/') == SLASH_COUNT_IN_KEY &&
        std::count(key.begin(), key.end(), '#') == 0;
}

} // namespace UDMF
} // namespace OHOSF