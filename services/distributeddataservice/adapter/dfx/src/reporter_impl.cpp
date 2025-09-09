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

#include "reporter_impl.h"

#include "behaviour/behaviour_reporter_impl.h"
#include "fault/cloud_sync_fault_impl.h"

namespace OHOS {
namespace DistributedDataDfx {
__attribute__((used)) static bool g_isInit = ReporterImpl::Init();
bool ReporterImpl::Init()
{
    static ReporterImpl reporterImpl;
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&]() {
        Reporter::RegisterReporterInstance(&reporterImpl);
    });
    return true;
}

FaultReporter* ReporterImpl::CloudSyncFault()
{
    static CloudSyncFaultImpl cloudSyncFault;
    cloudSyncFault.SetThreadPool(executors_);
    return &cloudSyncFault;
}

BehaviourReporter* ReporterImpl::GetBehaviourReporter()
{
    static BehaviourReporterImpl behaviourReporterImpl;
    behaviourReporterImpl.SetThreadPool(executors_);
    return &behaviourReporterImpl;
}

void ReporterImpl::SetThreadPool(std::shared_ptr<ExecutorPool> executors)
{
    executors_ = executors;
    if (executors != nullptr) {
        CloudSyncFault();
        GetBehaviourReporter();
    }
}
} // namespace DistributedDataDfx
} // namespace OHOS
