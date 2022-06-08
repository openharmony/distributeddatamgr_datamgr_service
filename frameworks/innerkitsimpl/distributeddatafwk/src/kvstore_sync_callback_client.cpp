/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#define LOG_TAG "KvStoreSyncCallbackClient"

#include "kvstore_sync_callback_client.h"
#include <cinttypes>
#include <atomic>
#include "dds_trace.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
KvStoreSyncCallbackClient::~KvStoreSyncCallbackClient()
{
    syncCallbackInfo_.Clear();
}

void KvStoreSyncCallbackClient::SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__),
        TraceSwitch::BYTRACE_ON | TraceSwitch::API_PERFORMANCE_TRACE_ON);
    auto finded = syncCallbackInfo_.Find(sequenceId);
    if (finded.first) {
        finded.second->SyncCompleted(results);
        DeleteSyncCallback(sequenceId);
    }
}

void KvStoreSyncCallbackClient::AddSyncCallback(
    const std::shared_ptr<KvStoreSyncCallback> callback, uint64_t sequenceId)
{
    if (callback == nullptr) {
        ZLOGE("callback is nullptr");
        return;
    }
    auto inserted = syncCallbackInfo_.Insert(sequenceId, callback);
    if (!inserted) {
        ZLOGE("The sequeuceId %{public}" PRIu64 "is repeat!", sequenceId);
    }
}

void KvStoreSyncCallbackClient::DeleteSyncCallback(uint64_t sequenceId)
{
    syncCallbackInfo_.Erase(sequenceId);
}
}  // namespace DistributedKv
}  // namespace OHOS
