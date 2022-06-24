/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_STORE_UTIL_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_STORE_UTIL_H
#include "visibility.h"
#include "kv_store_delegate_manager.h"
#include "store_errno.h"
#include "store_types.h"
namespace OHOS::DistributedKv {
class API_EXPORT StoreUtil final {
public:
    using DBSecurity = DistributedDB::SecurityOption;
    using DBStatus = DistributedDB::DBStatus;
    static DBSecurity GetDBSecurity(int32_t secLevel);
    static int32_t GetSecLevel(DBSecurity dbSec);
    static std::string Anonymous(const std::string &name);
    static uint32_t Anonymous(const void *ptr);
    static Status ConvertStatus(DBStatus status);
    static bool InitPath(const std::string &path);
    static bool Remove(const std::string &path);
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_STORE_UTIL_H
