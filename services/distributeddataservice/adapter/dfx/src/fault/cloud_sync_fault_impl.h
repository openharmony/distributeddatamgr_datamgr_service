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

#ifndef DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_CLOUD_SYNC_FAULT_IMPL_H
#define DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_CLOUD_SYNC_FAULT_IMPL_H

#include "dfx/dfx_types.h"
#include "dfx/fault_reporter.h"
#include "hiview_adapter.h"

namespace OHOS {
namespace DistributedDataDfx {
class CloudSyncFaultImpl : public FaultReporter {
public:
    virtual ~CloudSyncFaultImpl() {}
    ReportStatus Report(const ArkDataFaultMsg &msg) override;
    void SetThreadPool(std::shared_ptr<ExecutorPool> executors);

private:
    std::shared_ptr<ExecutorPool> executors_;
};
} // namespace DistributedDataDfx
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_DATAMGR_SERVICE_CLOUD_SYNC_FAULT_IMPL_H