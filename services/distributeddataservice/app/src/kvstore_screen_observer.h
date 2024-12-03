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

#ifndef KVSTORE_SCREEN_OBSERVER_H
#define KVSTORE_SCREEN_OBSERVER_H

#include "executor_pool.h"
#include "screen/screen_manager.h"

namespace OHOS {
namespace DistributedKv {
using namespace DistributedData;
class KvStoreDataService;
class KvStoreScreenObserver : public ScreenManager::Observer {
public:
    explicit KvStoreScreenObserver(KvStoreDataService &kvStoreDataService, std::shared_ptr<ExecutorPool> executors)
        : kvStoreDataService_(kvStoreDataService), executors_(executors)
    {
    }
    ~KvStoreScreenObserver() override = default;

    void OnScreenUnlocked(int32_t user) override;

    std::string GetName() override
    {
        return "DistributedDataService";
    }

private:
    KvStoreDataService &kvStoreDataService_;
    std::shared_ptr<ExecutorPool> executors_;
};
} // namespace DistributedKv
} // namespace OHOS
#endif // KVSTORE_SCREEN_OBSERVER_H