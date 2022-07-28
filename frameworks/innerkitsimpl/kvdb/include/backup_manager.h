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
    struct ResidueInfo
    {
        size_t tmpBackupFileSize;
        size_t tmpKeyFileSize;
        bool haveRawBackupFile;
        bool haveTmpBackupFile;
        bool haveRawKeyFile;
        bool haveTmpKeyFile;
    };
    static constexpr int MAX_BACKUP_NUM = 5;
    static BackupManager &GetInstance();
    void Init(const std::string &baseDir);
    void Prepare(std::string path, std::string &storeId, bool isAutoBackup = false);
    Status Backup(std::string &name, std::string &baseDir, std::string &storeId, std::shared_ptr<DBStore> &dbStore);
    Status Restore(std::string &name, std::string &baseDir, std::string &storeId, std::shared_ptr<DBStore> &dbStore);
    Status DeleteBackup(std::vector<std::string> &files, std::string &baseDir, std::string &storeId,
        std::map<std::string, Status> &status);
private:
    BackupManager();
    ~BackupManager();
    bool MatchedInFiles(std::string &file, std::vector<StoreUtil::FileInfo> &fileInfos);
    void GetLatestFile(std::string &name, std::vector<StoreUtil::FileInfo> &fileInfos);
    bool HaveResidueFile(std::vector<StoreUtil::FileInfo> fileList);
    bool HaveResidueKey(std::vector<StoreUtil::FileInfo> fileList, std::string &storeId);
    void BuildResidueInfo(std::vector<StoreUtil::FileInfo> &fileList,
        std::vector<StoreUtil::FileInfo> &keyList, std::string &storeId);
    bool NeedRollBack(BackupManager::ResidueInfo residueInfo);
    void ClearResidueFile(const std::string &baseDir, std::string &storeId);
    bool IsEndWith(const std::string &fullString, const std::string &end);
    bool IsBeginWith(const std::string &fullString, const std::string &begin);

    static constexpr int POOL_SIZE = 1;
    std::map<std::string, ResidueInfo> backupResidueInfo_;
    std::shared_ptr<DistributedKv::KvStoreThreadPool> pool_;
};
} // namespace OHOS::DistributedKv
#endif // OHOS_DISTRIBUTED_DATA_FRAMEWORKS_KVDB_BACKUP_MANAGER_H
