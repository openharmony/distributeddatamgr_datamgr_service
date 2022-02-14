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

#include <atomic>
#include "log_print.h"
#include "kvstore_sync_callback_client.h"

namespace OHOS {
namespace DistributedKv {
KvStoreSyncCallbackClient::~KvStoreSyncCallbackClient() = default;
void KvStoreSyncCallbackClient::SyncCompleted(const std::map<std::string, Status> &results, uint64_t sequenceId)
{
    if (SyncCallbackInfo_.Contains(sequenceId)) {
        SyncCallbackInfo_[sequenceId]->SyncCompleted(results);
        DeleteSyncCallback(sequenceId);
    }
}

void KvStoreSyncCallbackClient::AddSyncCallback(const std::shared_ptr<KvStoreSyncCallback> SyncCallback,
                                                uint64_t sequenceId)
{
    if (!SyncCallbackInfo_.Contains(sequenceId)) {
        SyncCallbackInfo_.Insert(sequenceId, SyncCallback);
    }
}

void KvStoreSyncCallbackClient::DeleteSyncCallback(uint64_t sequenceId)
{
    if (SyncCallbackInfo_.Contains(sequenceId)) {
        SyncCallbackInfo_.Erase(sequenceId);
    }
}
}  // namespace DistributedKv
}  // namespace OHOS