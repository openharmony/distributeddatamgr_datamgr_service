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

#define LOG_TAG "RdbWatcher"

#include "rdb_watcher.h"
#include "log_print.h"
#include "error/general_error.h"

namespace OHOS::DistributedRdb {
using namespace DistributedData;
using Err = DistributedData::GeneralError;
RdbWatcher::RdbWatcher(RdbServiceImpl *rdbService, uint32_t tokenId, const std::string &storeName)
    : rdbService_(rdbService), tokenId_(tokenId), storeName_(storeName)
{
}

int32_t RdbWatcher::OnChange(Origin origin, const std::string &id)
{
    if (rdbService_ == nullptr) {
        return Err::E_ERROR;
    }
    rdbService_->OnChange(tokenId_, storeName_);
    return Err::E_OK;
}

int32_t RdbWatcher::OnChange(Origin origin, const std::string &id, const std::vector<VBucket> &values)
{
    return Err::E_NOT_SUPPORT;
}
} // namespace OHOS::DistributedRdb
