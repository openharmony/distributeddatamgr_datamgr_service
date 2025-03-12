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
#include "general_watcher_mock.h"

namespace OHOS::DistributedData {
bool MockQuery::IsEqual(uint64_t tid)
{
    return (tid == TYPE_ID) && lastResult;
}

std::vector<std::string> MockQuery::GetTables()
{
    return tables_;
}

const std::string GetStatement()
{
    return "AS distributed_log";
}

void MockQuery::MakeRemoteQuery(const std::string &devices, const std::string &sql, Values &&args)
{
    isRemote_ = true;
    devices_ = { devices };
    sql_ = sql;
    args_ = std::move(args);
}

void MockQuery::MakeQuery(const DistributedRdb::PredicatesMemo &predicates)
{
    devices_ = predicates.devices_;
    tables_ = predicates.tables_;
}
int32_t MockGeneralWatcher::OnChange(const Origin &origin, const PRIFields &primaryFields, ChangeInfo &&values)
{
    return GeneralError::E_OK;
}

int32_t MockGeneralWatcher::OnChange(const Origin &origin, const Fields &fields, ChangeData &&datas)
{
    return GeneralError::E_OK;
}
} // namespace OHOS::DistributedData
