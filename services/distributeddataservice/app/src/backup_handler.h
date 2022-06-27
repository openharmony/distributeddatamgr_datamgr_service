/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDATAMGR_BACKUP_HANDLER_H
#define DISTRIBUTEDDATAMGR_BACKUP_HANDLER_H

#include "ikvstore_data_service.h"
#include "kv_store_nb_delegate.h"
#include "kv_store_delegate.h"
#include "kvstore_app_manager.h"
#include "kv_scheduler.h"
#include "kvstore_meta_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/secret_key_meta_data.h"
#include "types.h"

namespace OHOS::DistributedKv {
class BackupHandler {
public:
    using StoreMetaData = DistributedData::StoreMetaData;
    using SecretKeyMetaData = DistributedData::SecretKeyMetaData;
    static BackupHandler &GetInstance();
    void BackSchedule();
    void SingleKvStoreBackup(const StoreMetaData &metaData);
    bool SingleKvStoreRecover(StoreMetaData &metaData, DistributedDB::KvStoreNbDelegate *delegate);

    const std::string &GetBackupPath(const std::string &deviceAccountId, int pathType);
    bool RenameFile(const std::string &oldPath, const std::string &newPath);
    bool RemoveFile(const std::string &path);
    bool FileExists(const std::string &path);
    std::string GetHashedBackupName(const std::string &bundleName);

    struct BackupPara {
        KvStoreAppManager::PathType pathType;
        DistributedDB::CipherPassword password;
        std::string backupFullName;
        std::string backupBackFullName;
    };

private:
    BackupHandler();
    ~BackupHandler();
    bool CheckNeedBackup();
    bool InitBackupPara(const StoreMetaData &metaData, BackupPara &backupPara);
    bool GetPassword(const StoreMetaData &metaData, DistributedDB::CipherPassword &password);
    int64_t GetBackupTime(std::string &fullName);
    static std::string backupDirCe_;
    static std::string backupDirDe_;

    void SetDBOptions(DistributedDB::KvStoreNbDelegate::Option &dbOption,
                      const BackupPara &backupPara, const StoreMetaData &metaData);
    KvScheduler scheduler_ {};
    static constexpr uint64_t BACKUP_INTERVAL = 3600 * 1000 * 10; // 10 hours
    int64_t backupSuccessTime_ = 0;
};
} // namespace OHOS::DistributedKv
#endif // DISTRIBUTEDDATAMGR_BACKUP_HANDLER_H
