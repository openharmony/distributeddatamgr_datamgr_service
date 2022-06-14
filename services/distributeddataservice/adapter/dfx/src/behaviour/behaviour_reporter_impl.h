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

#ifndef DISTRIBUTEDDATAMGR_BEHAVIOUR_REPORTER_IMPL_H
#define DISTRIBUTEDDATAMGR_BEHAVIOUR_REPORTER_IMPL_H

#include "behaviour_reporter.h"
#include "hiview_adapter.h"

namespace OHOS {
namespace DistributedDataDfx {
class BehaviourReporterImpl : public BehaviourReporter {
public:
    virtual ~BehaviourReporterImpl() {}
    ReportStatus Report(const struct BehaviourMsg &msg) override;
};
}  // namespace DistributedDataDfx
}  // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_BEHAVIOUR_REPORTER_IMPL_H

