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

#include "log_print.h"
#include "kvstore_sync_callback_client.h"

namespace OHOS {
namespace DistributedKv {
//sptr<KvStoreSyncCallbackClient> KvStoreSyncCallbackClient::pInstance_ = new KvStoreSyncCallbackClient();
std::mutex KvStoreSyncCallbackClient::syncCallbackMutex_;
std::map<std::string, std::shared_ptr<KvStoreSyncCallback>> KvStoreSyncCallbackClient::kvStoreSyncCallbackInfo_;

KvStoreSyncCallbackClient::KvStoreSyncCallbackClient() = default;

KvStoreSyncCallbackClient::~KvStoreSyncCallbackClient() = default;

void KvStoreSyncCallbackClient::SyncCompleted(const std::map<std::string, Status> &results, const std::string &label)
{
    std::lock_guard<std::mutex> lg(syncCallbackMutex_);
    if (kvStoreSyncCallbackInfo_.find(label) != kvStoreSyncCallbackInfo_.end()) {
        ZLOGI("label = %{public}s", label.c_str());
        kvStoreSyncCallbackInfo_[label]->SyncCompleted(results);
    }
}

void KvStoreSyncCallbackClient::AddKvStoreSyncCallback(const std::shared_ptr<KvStoreSyncCallback> kvStoreSyncCallback,
                                                       const std::string &label)
{
    std::lock_guard<std::mutex> lg(syncCallbackMutex_);
    if (kvStoreSyncCallbackInfo_.find(label) == kvStoreSyncCallbackInfo_.end()) {
        kvStoreSyncCallbackInfo_.insert( {label, kvStoreSyncCallback} );
    }
}

void KvStoreSyncCallbackClient::DeleteCommonKvStoreSyncCallback()
{
    std::lock_guard<std::mutex> lg(syncCallbackMutex_);
    if (kvStoreSyncCallbackInfo_.find(CommonSyncCallbackLabel) != kvStoreSyncCallbackInfo_.end()) {
        kvStoreSyncCallbackInfo_.erase(CommonSyncCallbackLabel);
    }
}

std::string KvStoreSyncCallbackClient::GetCommonSyncCallbackLabel()
{
    return CommonSyncCallbackLabel;
}
}  // namespace DistributedKv
}  // namespace OHOS