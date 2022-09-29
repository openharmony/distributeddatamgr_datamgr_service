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
#include "executor_factory.h"

namespace OHOS::DistributedData {
using namespace OHOS::DistributedKv;
ExecutorFactory &ExecutorFactory::GetInstance()
{
    static ExecutorFactory instance;
    return instance;
}

bool ExecutorFactory::Execute(KvStoreTask &&task)
{
    if (threadPool_ == nullptr) {
        return false;
    }
    threadPool_->AddTask(std::move(task));
    return true;
}

ExecutorFactory::ExecutorFactory()
{
    threadPool_ = KvStoreThreadPool::GetPool(POOL_SIZE, "Executor", true);
}

ExecutorFactory::~ExecutorFactory()
{
    if (threadPool_ != nullptr) {
        threadPool_->Stop();
        threadPool_ = nullptr;
    }
}
} // namespace OHOS::DistributedData
