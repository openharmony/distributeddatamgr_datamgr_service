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

#include "database_fault_impl.h"

namespace OHOS {
namespace DistributedDataDfx {
ReportStatus DatabaseFaultImpl::Report(const DBFaultMsg &msg)
{
    int eventID;
    if (msg.errorType == Fault::DF_DB_CORRUPTED) {
        eventID = DfxCodeConstant::DATABASE_CORRUPTED_FAILED;
    } else if (msg.errorType == Fault::DF_DB_REKEY_FAILED) {
        eventID = DfxCodeConstant::DATABASE_REKEY_FAILED;
    } else if (msg.errorType == Fault::DF_DB_DAMAGE) {
        eventID = DfxCodeConstant::DATABASE_FAULT;
    } else {
        return ReportStatus::ERROR;
    }

    HiViewAdapter::ReportDBFault(eventID, msg);
    return ReportStatus::SUCCESS;
}
} // namespace DistributedDataDfx
} // namespace OHOS