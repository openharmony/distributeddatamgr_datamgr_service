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
#ifndef OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_BACKUP_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_BACKUP_MANAGER_H
#include <string>
#include <map>
#include <vector>
#include "store_errno.h"
#include "kv_store_nb_delegate.h"
#include "kv_store_task.h"
#include "kv_store_thread_pool.h"
#include "store_util.h"
namespace OHOS::DistributedKv {
class BackupManager {
public:
    using DBStore = DistributedDB::KvStoreNbDelegate;
    struct ResidueInfo {
        size_t tmpBackupSize;
        size_t tmpKeySize;
        bool hasRawBackup;
        bool hasTmpBackup;
        bool hasRawKey;
        bool hasTmpKey;
    };
    enum ClearType {
        DO_NOTHING = 0,
        ROLLBACK_DATA,
        ROLLBACK,
        CLEAN_TMP,
    };
    static BackupManager &GetInstance();
    void Init(const std::string &baseDir);
    void Prepare(const std::string &path, const std::string &storeId);
    Status Backup(const std::string &name, const std::string &baseDir,
        const std::string &storeId, std::shared_ptr<DBStore> dbStore);
    Status Restore(const std::string &name, const std::string &baseDir,
        const std::string &appId, const std::string &storeId, std::shared_ptr<DBStore> dbStore);
    Status DeleteBackup(std::map<std::string, Status> &deleteList,
        const std::string &baseDir, const std::string &storeId);
private:
    BackupManager();
    ~BackupManager();

    void KeepData(const std::string &name, bool isCreated);
    void RollBackData(const std::string &name, bool isCreated);
    void CleanTmpData(const std::string &name);
    StoreUtil::FileInfo GetBackupFileInfo(const std::string &name,
        const std::string &baseDir, const std::string &storeId);
    bool HaveResidueFile(const std::vector<StoreUtil::FileInfo> &files);
    bool HaveResidueKey(const std::vector<StoreUtil::FileInfo> &files, std::string storeId);
    std::string GetBackupName(const std::string &fileName);
    void SetResidueInfo(ResidueInfo &residueInfo, const std::vector<StoreUtil::FileInfo> &files,
        const std::string &name, const std::string &postFix);
    std::map<std::string, ResidueInfo> BuildResidueInfo(const std::vector<StoreUtil::FileInfo> &files,
        const std::vector<StoreUtil::FileInfo> &keys, const std::string &storeId);
    ClearType GetClearType(const ResidueInfo &residueInfo);
    void ClearResidueFile(std::map<std::string, ResidueInfo> residueInfo,
        const std::string &baseDir, const std::string &storeId);
    bool IsEndWith(const std::string &fullString, const std::string &end);
    bool IsBeginWith(const std::string &fullString, const std::string &begin);

    static constexpr int MAX_BACKUP_NUM = 5;
    static constexpr int POOL_SIZE = 1;
    std::shared_ptr<DistributedKv::KvStoreThreadPool> pool_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_BACKUP_MANAGER_H
