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

#ifndef DATASHARESERVICE_HIVIEW_BASE_ADAPTER_H
#define DATASHARESERVICE_HIVIEW_BASE_ADAPTER_H

#include <cinttypes>

#include "executor_pool.h"
#include "hisysevent_c.h"

namespace OHOS {
namespace DataShare {

class HiViewBaseAdapter {
public:
    const int waitTime = 180 * 60;
    std::shared_ptr<ExecutorPool> executors_ = nullptr;
    std::atomic<bool> running_ = false;

    void SetThreadPool(std::shared_ptr<ExecutorPool> executors)
    {
        executors_ = executors;
        StartTimerThread();
    }

    void StartTimerThread()
    {
        if (executors_ == nullptr) {
            return;
        }
        if (running_.exchange(true)) {
            return;
        }
        auto interval = std::chrono::seconds(waitTime);
        auto fun = [this]() { InvokeData(); };
        executors_->Schedule(fun, interval);
    }

    virtual void InvokeData() = 0;
};
}
}
#endif // DATASHARESERVICE_HIVIEW_ADAPTER_H