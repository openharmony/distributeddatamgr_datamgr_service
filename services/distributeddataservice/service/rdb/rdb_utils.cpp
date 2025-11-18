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
#include "rdb_utils.h"
#include "rdb_errno.h"
#include "store/general_value.h"

namespace OHOS::DistributedRdb {
using namespace OHOS::DistributedData;
int32_t RdbUtils::ConvertNativeRdbStatus(int32_t status)
{
    switch (status) {
        case NativeRdb::E_OK:
            return GeneralError::E_OK;
        case NativeRdb::E_SQLITE_BUSY:
        case NativeRdb::E_DATABASE_BUSY:
        case NativeRdb::E_SQLITE_LOCKED:
            return GeneralError::E_BUSY;
        case NativeRdb::E_INVALID_ARGS:
        case NativeRdb::E_INVALID_ARGS_NEW:
            return GeneralError::E_INVALID_ARGS;
        case NativeRdb::E_ALREADY_CLOSED:
            return GeneralError::E_ALREADY_CLOSED;
        case NativeRdb::E_SQLITE_CORRUPT:
            return GeneralError::E_DB_CORRUPT;
        default:
            break;
    }
    return GeneralError::E_ERROR;
}
} // namespace OHOS::DistributedRdb