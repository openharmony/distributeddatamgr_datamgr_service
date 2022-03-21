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

#include "sync_observer.h"

namespace OHOS::DistributedKv {
SyncObserver::SyncObserver(const std::vector<std::shared_ptr<KvStoreSyncCallback>> &callbacks)
    :callbacks_(callbacks)
{};

SyncObserver::SyncObserver()
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    callbacks_.clear();
}

bool SyncObserver::Add(const std::shared_ptr<KvStoreSyncCallback> callback)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    callbacks_.push_back(callback);
    return true;
}

bool SyncObserver::Clean()
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    callbacks_.clear();
    return true;
}

void SyncObserver::SyncCompleted(const std::map<std::string, DistributedKv::Status> &results)
{
    std::lock_guard<decltype(mutex_)> lock(mutex_);
    for (auto &callback : callbacks_) {
        callback->SyncCompleted(results);
    }
}
} // namespace OHOS::DistributedKv
