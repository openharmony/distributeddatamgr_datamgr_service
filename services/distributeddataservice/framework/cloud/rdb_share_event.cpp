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

#include "cloud/rdb_share_event.h"

namespace OHOS::DistributedData {
RdbShareEvent::RdbShareEvent(CloudEvent::StoreInfo storeInfo,
    std::shared_ptr<DistributedRdb::PredicatesMemo> predicates, const std::vector<std::string>& columns,
    Callback callback)
    : CloudEvent(RDB_SHARE, std::move(storeInfo)), predicates_(std::move(predicates)), columns_(columns),
      callback_(std::move(callback))
{
}
std::shared_ptr<DistributedRdb::PredicatesMemo> RdbShareEvent::GetPredicts() const
{
    return predicates_;
}
std::vector<std::string> RdbShareEvent::GetColumns() const
{
    return columns_;
}

RdbShareEvent::Callback RdbShareEvent::GetCallback() const
{
    return callback_;
}
} // namespace OHOS::DistributedData