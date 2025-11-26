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
#define LOG_TAG "DragLifeCyclePolicy"

#include "drag_lifecycle_policy.h"

#include "lifecycle_policy.h"
#include "lifecycle_manager.h"
#include "log_print.h"
#include "preprocess/preprocess_utils.h"

namespace OHOS {
namespace UDMF {
static constexpr const char *DRAG_STORE_NAME = "drag";
Status DragLifeCyclePolicy::OnTimeout(const std::string &intention)
{
    auto store = StoreCache::GetInstance().GetStore(DRAG_STORE_NAME);
    if (!store) {
        ZLOGE("Drag store get failed");
        return E_DB_ERROR;
    }
    std::unordered_map<std::string, Runtime> timeoutRuntimes;
    if (auto status = GetTimeoutRuntime(store, timeoutRuntimes); status != E_OK) {
        ZLOGE("Timeout keys get failed");
        return status;
    }
    if (timeoutRuntimes.empty()) {
        return E_OK;
    }
    std::vector<std::string> timeoutKeys;
    timeoutKeys.reserve(timeoutRuntimes.size());
    for (const auto &pair : timeoutRuntimes) {
        timeoutKeys.push_back(pair.first);
    }
    if (store->DeleteBatch(timeoutKeys) != E_OK) {
        ZLOGE("Data removal failed:%{public}s", intention.c_str());
        return E_DB_ERROR;
    }
    for (const auto &[udkey, runtime] : timeoutRuntimes) {
        LifeCycleManager::GetInstance().EraseUdkey(runtime.key.key, runtime.tokenId);
    }
    return E_OK;
}

Status DragLifeCyclePolicy::GetTimeoutRuntime(
    const std::shared_ptr<Store> &store, std::unordered_map<std::string, Runtime> &timeoutRuntimes)
{
    std::vector<Runtime> runtimes;
    auto status = store->GetBatchRuntime(DATA_PREFIX, runtimes);
    if (status != E_OK) {
        ZLOGE("Get data failed.");
        return status;
    }
    if (runtimes.empty()) {
        ZLOGI("entries is empty.");
        return E_OK;
    }
    auto curTime = PreProcessUtils::GetTimestamp();
    for (const auto &runtime : runtimes) {
        if (curTime > runtime.createTime + std::chrono::duration_cast<std::chrono::milliseconds>(INTERVAL).count()
            || curTime < runtime.createTime) {
            timeoutRuntimes.emplace(runtime.key.GetKeyCommonPrefix(), runtime);
        }
    }
    return E_OK;
}

Status DragLifeCyclePolicy::OnGotToRemote(std::shared_ptr<Store> store)
{
    if (!store) {
        ZLOGE("Store is null.");
        return E_DB_ERROR;
    }
    auto deviceIds = PreProcessUtils::GetRemoteDeviceIds();
    if (deviceIds.empty()) {
        return E_OK;
    }
    if (store->PushSync(deviceIds) != E_OK) {
        ZLOGW("Push sync failed");
    }
    return E_OK;
}

} // namespace UDMF
} // namespace OHOS
