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

#ifndef DISTRIBUTEDDATAMGR_COMMUNICATOR_CONTEXT_H
#define DISTRIBUTEDDATAMGR_COMMUNICATOR_CONTEXT_H

#include "executor_pool.h"
#include "visibility.h"

namespace OHOS::DistributedData {
class API_EXPORT CommunicatorContext {
public:
    API_EXPORT static CommunicatorContext &getInstance();
    API_EXPORT void SetThreadPool(std::shared_ptr<ExecutorPool> executors);
    std::shared_ptr<ExecutorPool> GetThreadPool();

private:
    CommunicatorContext() = default;
    ~CommunicatorContext() = default;
    CommunicatorContext(CommunicatorContext const &) = delete;
    void operator=(CommunicatorContext const &) = delete;
    CommunicatorContext(CommunicatorContext &&) = delete;
    CommunicatorContext &operator=(CommunicatorContext &&) = delete;

    std::shared_ptr<ExecutorPool> executors_;
};
} // namespace OHOS::DistributedData
#endif // DISTRIBUTEDDATAMGR_COMMUNICATOR_CONTEXT_H
