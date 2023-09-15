/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef UDMF_STORE_CACHE_H
#define UDMF_STORE_CACHE_H

#include <memory>
#include <mutex>

#include "concurrent_map.h"
#include "executor_pool.h"
#include "store.h"
#include "unified_meta.h"

namespace OHOS {
namespace UDMF {
class StoreCache {
public:
    std::shared_ptr<Store> GetStore(std::string intention);
    static StoreCache &GetInstance();
    void SetThreadPool(std::shared_ptr<ExecutorPool> executors);

private:
    StoreCache() {}

    ~StoreCache() {}

    StoreCache(const StoreCache &obj) = delete;
    StoreCache &operator=(const StoreCache &obj) = delete;

    void GarbageCollect();

    ConcurrentMap<std::string, std::shared_ptr<Store>> stores_;
    std::mutex taskMutex_;
    ExecutorPool::TaskId taskId_ = ExecutorPool::INVALID_TASK_ID;

    static constexpr int64_t INTERVAL = 1;  // 1 min
    std::shared_ptr<ExecutorPool> executorPool_;
};
} // namespace UDMF
} // namespace OHOS
#endif // UDMF_STORE_CACHE_H
