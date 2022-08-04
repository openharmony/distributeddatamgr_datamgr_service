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

#ifndef OHOS_DISTRIBUTED_DATA_SERVICES_BACKUP_BACKUP_MANAGER_H
#define OHOS_DISTRIBUTED_DATA_SERVICES_BACKUP_BACKUP_MANAGER_H

#include "kv_scheduler.h"
#include "metadata/store_meta_data.h"
#include "metadata/secret_key_meta_data.h"
#include "types.h"
namespace OHOS::DistributedData {
class BackupManager {
public:
    using Exporter = std::function<void(
        const StoreMetaData &, const std::vector<uint8_t> &, const std::string &, DistributedKv::Status &)>;
    struct BackupParam {
        int64_t schedularDelay;
        int64_t schedularInternal;
        int64_t backupInternal;
        int64_t backupNumber;
    };
    enum ClearType {
        DO_NOTHING = 0,
        ROLLBACK,
        CLEAN_DATA,
    };
    static BackupManager &GetInstance();
    void BackSchedule();
    void SetBackupParam(const BackupParam &backupParam);
    void RegisterExporter(int type, Exporter exporter);
    bool GetPassWord(const DistributedKv::AppId &appId,
        const DistributedKv::StoreId &storeId, std::vector<uint8_t> password);

private:
    BackupManager();
    ~BackupManager();
    void Init();
    ClearType GetClearType(const std::string &path);
    std::vector<StoreMetaData> GetMetas();
    bool CanBackup();
    void DoBackup(const StoreMetaData &meta);
    void CopyFile(const std::string &oldPath, const std::string &newPath, bool isCreate = false);
    StoreMetaData GetStoreMetaData(const DistributedKv::AppId &appId, const DistributedKv::StoreId &storeId);
    int32_t GetInstIndex(uint32_t tokenId, const DistributedKv::AppId &appId);
    void KeepData(const std::string &path, const std::string &key, const SecretKeyMetaData &secretKey);
    void SaveData(const std::string &path, const std::string &key, const SecretKeyMetaData &secretKey);
    void CleanData(const std::string &path, const std::string &key);
    void RollBackData(const std::string &path, const std::string &key);
    bool IsFileExist(const std::string &path);
    bool RemoveFile(const std::string &path);

    std::map<int32_t, Exporter> exporters_;
    int64_t schedularDelay_;
    int64_t schedularInternal_;
    int64_t backupInternal_;
    int64_t backupSuccessTime_ = 0;

    int64_t backupNumber_;
    int64_t startNum_;
    int64_t completeNum_;
    DistributedKv::KvScheduler scheduler_ {};
};
} // namespace OHOS::DistributedData
#endif // OHOS_DISTRIBUTED_DATA_SERVICES_BACKUP_BACKUP_MANAGER_H