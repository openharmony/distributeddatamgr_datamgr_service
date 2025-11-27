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

#define LOG_TAG "RdbCloudConflictHandler"
#include "rdb_cloud_conflict_handler.h"
#include "value_proxy.h"

namespace OHOS::DistributedRdb {
using namespace DistributedDB;
constexpr const int32_t INSERT = 0;
constexpr const int32_t UPDATE = 1;
constexpr const int32_t DELETE = 2;
constexpr const int32_t NOT_HANDLE = 3;
RdbCloudConflictHandler::RdbCloudConflictHandler(std::shared_ptr<CloudConflictHandler> handler) : handler_(handler)
{
}
ConflictRet RdbCloudConflictHandler::HandleConflict(const std::string &table, const DBVBucket &oldData,
    const DBVBucket &newData, DBVBucket &upsert)
{
    if (handler_ == nullptr) {
        return ConflictRet::NOT_HANDLE;
    }
    VBucket rdbOldData = DistributedData::ValueProxy::Convert(DBVBucket(oldData));
    VBucket rdbNewData = DistributedData::ValueProxy::Convert(DBVBucket(newData));
    VBucket rdbUpsert = DistributedData::ValueProxy::Convert(std::move(upsert));
    auto ret = handler_->HandleConflict(table, rdbOldData, rdbNewData, rdbUpsert);
    upsert = DistributedData::ValueProxy::Convert(std::move(rdbUpsert));
    switch (ret) {
        case INSERT:
        case UPDATE:
            return ConflictRet::UPSERT;
        case DELETE:
            return ConflictRet::DELETE;
        case NOT_HANDLE:
            return ConflictRet::NOT_HANDLE;
        default:
            return ConflictRet::NOT_HANDLE;
    }
}
} // namespace OHOS::DistributedRdb