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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_ERROR_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_ERROR_H
namespace OHOS::DistributedData {
enum GeneralError : int32_t {
    E_OK = 0,
    E_ERROR,
    E_NETWORK_ERROR,
    E_CLOUD_DISABLED,
    E_LOCKED_BY_OTHERS,
    E_RECODE_LIMIT_EXCEEDED,
    E_NO_SPACE_FOR_ASSET,
    E_BLOCKED_BY_NETWORK_STRATEGY,
    E_BUSY,
    E_INVALID_ARGS,
    E_NOT_INIT,
    E_DK_NOT_INIT,
    E_NOT_LOGIN,
    E_NOT_SUPPORT,
    E_ALREADY_CONSUMED,
    E_ALREADY_CLOSED,
    E_UNOPENED,
    E_RETRY_TIMEOUT,
    E_PARTIAL_ERROR,
    E_USER_UNLOCK,
    E_VERSION_CONFLICT,
    E_RECORD_EXIST_CONFLICT,
    E_WITH_INVENTORY_DATA,
    E_SYNC_TASK_MERGED,
    E_RECORD_NOT_FOUND,
    E_RECORD_ALREADY_EXISTED,
    E_DB_ERROR,
    E_INVALID_VALUE_FIELDS,
    E_INVALID_FIELD_TYPE,
    E_CONSTRAIN_VIOLATION,
    E_INVALID_FORMAT,
    E_INVALID_QUERY_FORMAT,
    E_INVALID_QUERY_FIELD,
    E_TIME_OUT,
    E_OVER_MAX_LIMITS,
    E_SECURITY_LEVEL_ERROR,
    E_FILE_NOT_EXIST,
    E_SCREEN_LOCKED,
    E_USER_DEACTIVATING,
    E_GET_CLOUD_USER_INFO,
    E_GET_BRIEF_INFO,
    E_GET_APP_SCHEMA,
    E_CONNECT_ASSET_LOADER,
    E_CONNECT_CLOUD_DB,
    E_BUTT,
};
}
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_FRAMEWORK_STORE_GENERAL_ERROR_H