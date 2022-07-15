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

#ifndef DISTRIBUTEDDATAMGR_COMMUNICATION_FAULT_IMPL_H
#define DISTRIBUTEDDATAMGR_COMMUNICATION_FAULT_IMPL_H

#include "fault_reporter.h"
#include "hiview_adapter.h"
#include "dfx_types.h"

namespace OHOS {
namespace DistributedDataDfx {
class CommunicationFaultImpl : public FaultReporter {
public:
    virtual ~CommunicationFaultImpl() {}
    ReportStatus Report(const FaultMsg &msg) override
    {
        return ReportStatus::SUCCESS;
    };
    ReportStatus Report(const DBFaultMsg &ms) override
    {
        return ReportStatus::SUCCESS;
    };
    ReportStatus Report(const struct CommFaultMsg &msg) override;
};
}  // namespace DistributedDataDfx
}  // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_COMMUNICATION_FAULT_IMPL_H
