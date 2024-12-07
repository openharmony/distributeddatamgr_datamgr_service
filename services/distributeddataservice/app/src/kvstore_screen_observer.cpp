/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#define LOG_TAG "KvStoreScreenObserver"

#include "kvstore_screen_observer.h"

#include "kvstore_data_service.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedKv {
void KvStoreScreenObserver::OnScreenUnlocked(int32_t user)
{
    ZLOGI("user: %{public}d screen unlock  begin.", user);
    ExecutorPool::Task task([this, user]() {
        ZLOGI("screen event processing in thread");
        kvStoreDataService_.OnScreenUnlocked(user);
    });
    executors_->Execute(std::move(task));
    ZLOGI("user: %{public}d screen unlock end.", user);
}
} // namespace DistributedKv
} // namespace OHOS