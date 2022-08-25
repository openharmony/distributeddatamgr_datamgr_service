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
#include "kv_store_delegate_manager.h"
#include "store_errno.h"
#include "store_types.h"
#include "types.h"
namespace OHOS::DistributedKv {
class StoreUtil final {
public:
    using DBSecurity = DistributedDB::SecurityOption;
    using DBStatus = DistributedDB::DBStatus;
    using DBMode = DistributedDB::SyncMode;
    struct FileInfo {
        std::string name;
        size_t size;
        time_t modifyTime;
    };
    static DBSecurity GetDBSecurity(int32_t secLevel);
    static int32_t GetSecLevel(DBSecurity dbSec);
    static DBMode GetDBMode(SyncMode syncMode);
    static uint32_t GetObserverMode(SubscribeType subType);
    static std::string Anonymous(const std::string &name);
    static uint32_t Anonymous(const void *ptr);
    static Status ConvertStatus(DBStatus status);
    static bool InitPath(const std::string &path);
    static bool CreateFile(const std::string &name);
    static std::vector<std::string> GetSubPath(const std::string &path);
    static std::vector<FileInfo> GetFiles(const std::string &path);
    static bool Rename(const std::string &oldName, const std::string &newName);
    static bool IsFileExist(const std::string &name);
    static bool Remove(const std::string &path);
    static void DoSync();
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_STORE_UTIL_H
