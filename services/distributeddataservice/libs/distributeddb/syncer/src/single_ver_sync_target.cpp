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

#include "single_ver_sync_target.h"

#include <string>

#include "db_errno.h"
#include "sync_operation.h"
#include "log_print.h"

namespace DistributedDB {
SingleVerSyncTarget::SingleVerSyncTarget()
    : endWaterMark_(0),
      query_(QuerySyncObject())
{
}

SingleVerSyncTarget::~SingleVerSyncTarget()
{
}

void SingleVerSyncTarget::SetSyncOperation(SyncOperation *operation)
{
    SyncTarget::SetSyncOperation(operation);
    if ((operation != nullptr) && !operation->IsKilled()) {
        query_ = operation->GetQuery();
        isQuerySync_ = operation->IsQuerySync();
    }
}

void SingleVerSyncTarget::SetEndWaterMark(WaterMark waterMark)
{
    endWaterMark_ = waterMark;
}

WaterMark SingleVerSyncTarget::GetEndWaterMark() const
{
    return endWaterMark_;
}

void SingleVerSyncTarget::SetResponseSessionId(uint32_t responseSessionId)
{
    responseSessionId_ = responseSessionId;
}

uint32_t SingleVerSyncTarget::GetResponseSessionId() const
{
    return responseSessionId_;
}

void SingleVerSyncTarget::SetQuery(const QuerySyncObject &query)
{
    query_ = query;
}

QuerySyncObject SingleVerSyncTarget::GetQuery() const
{
    return query_;
}

void SingleVerSyncTarget::SetQuerySync(bool isQuerySync)
{
    isQuerySync_ = isQuerySync;
}

bool SingleVerSyncTarget::IsQuerySync() const
{
    return isQuerySync_;
}
} // namespace DistributedDB