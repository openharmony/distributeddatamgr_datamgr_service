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
#define LOG_TAG "BackupManager"
#include "backup_manager.h"

#include <fstream>
#include <iostream>
#include <unistd.h>
#include "backuprule/backup_rule_manager.h"
#include "communication_provider.h"
#include "crypto_manager.h"
#include "directory_manager.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "types.h"
namespace OHOS::DistributedData {
using Commu = AppDistributedKv::CommunicationProvider;
namespace {
constexpr const int COPY_SIZE = 1024;
constexpr const int MICROSEC_TO_SEC = 1000;
constexpr const char *AUTO_BACKUP_NAME = "autoBackup.bak";
constexpr const char *BACKUP_BK_POSTFIX = ".bk";
constexpr const char *BACKUP_TMP_POSTFIX = ".tmp";
}

BackupManager::BackupManager()
{
}

BackupManager::~BackupManager()
{
}

BackupManager &BackupManager::GetInstance()
{
    static BackupManager instance;
    return instance;
}

void BackupManager::Init()
{
    std::vector<StoreMetaData> metas;
    MetaDataManager::GetInstance().LoadMeta(
        StoreMetaData::GetPrefix({Commu::GetInstance().GetLocalDevice().uuid}), metas);
    for (auto &meta : metas) {
        if (!meta.isBackup || meta.isDirty) {
                continue;
        }
        auto backupPath =
            DirectoryManager::GetInstance().GetStoreBackupPath(meta) + "/" + AUTO_BACKUP_NAME;
        switch (GetClearType(meta)) {
            case ROLLBACK:
                RollBackData(backupPath);
                break;
            case CLEAN_DATA:
                CleanData(backupPath);
                break;
            case DO_NOTHING:
            default:
                break;
        }
    }
}

void BackupManager::SetBackupParam(const BackupParam &backupParam)
{
    schedularDelay_ = backupParam.schedularDelay;
    schedularInternal_ = backupParam.schedularInternal;
    backupInternal_ = backupParam.backupInternal;
    backupNumber_ = backupParam.backupNumber;
}

void BackupManager::RegisterExporter(int32_t type, Exporter exporter)
{
    if (exporters_[type] == nullptr) {
        exporters_[type] = exporter;
    } else {
        ZLOGI("Auto backup exporter has registed, type:%{public}d.", type);
    }
}

void BackupManager::BackSchedule()
{
    std::chrono::duration<int> delay(schedularDelay_);
    std::chrono::duration<int> internal(schedularInternal_);
    ZLOGI("BackupManager Schedule start.");
    scheduler_.Every(delay, internal, [this]() {
        if (!CanBackup()) {
            return;
        }
        std::vector<StoreMetaData> metas;
        MetaDataManager::GetInstance().LoadMeta(
            StoreMetaData::GetPrefix({Commu::GetInstance().GetLocalDevice().uuid}), metas);

        int64_t end = std::min(startNum_ + backupNumber_, static_cast<int64_t>(metas.size()));
        for (int64_t i = startNum_; i < end; startNum_++, i++) {
            auto &meta = metas[i];
            if (!meta.isBackup || meta.isDirty) {
                continue;
            }
            DoBackup(meta);
        }
        if (startNum_ >= static_cast<int64_t>(metas.size())) {
            startNum_ = 0;
        }
        sync();
        backupSuccessTime_ = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    });
}

void BackupManager::DoBackup(const StoreMetaData &meta)
{
    bool result = false;
    auto key = meta.GetSecretKey();
    auto backupKey = meta.GetBackupSecretKey();
    std::vector<uint8_t> decryptKey;
    SecretKeyMetaData secretKey;
    if (MetaDataManager::GetInstance().LoadMeta(key, secretKey, true)) {
        CryptoManager::GetInstance().Decrypt(secretKey.sKey, decryptKey);
    }
    auto backupPath = DirectoryManager::GetInstance().GetStoreBackupPath(meta);
    std::string backupFullPath = backupPath + "/" + AUTO_BACKUP_NAME;

    KeepData(backupFullPath);
    if (exporters_[meta.storeType] != nullptr) {
        exporters_[meta.storeType](meta, backupFullPath + BACKUP_TMP_POSTFIX, result);
    }
    if (result) {
        SaveData(backupFullPath, backupKey, secretKey);
    } else {
        CleanData(backupFullPath);
    }
    decryptKey.assign(decryptKey.size(), 0);
}

bool BackupManager::CanBackup()
{
    if (!BackupRuleManager::GetInstance().CanBackup()) {
        return false;
    }
    int64_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    if (currentTime - backupSuccessTime_ < backupInternal_ * MICROSEC_TO_SEC && backupSuccessTime_ > 0) {
        ZLOGE("no more than backup internal time since the last backup success.");
        return false;
    }
    return true;
}

void BackupManager::KeepData(const std::string &path)
{
    auto backupPath = path + BACKUP_BK_POSTFIX;
    CopyFile(path, backupPath, true);
}

void BackupManager::SaveData(
    const std::string &path, const std::string &key, const SecretKeyMetaData &secretKey)
{
    auto tmpPath = path + BACKUP_TMP_POSTFIX;
    auto backupPath = path + BACKUP_BK_POSTFIX;
    CopyFile(tmpPath, path);
    RemoveFile(tmpPath.c_str());
    if (secretKey.sKey.size() != 0) {
        MetaDataManager::GetInstance().SaveMeta(key, secretKey, true);
    }
    RemoveFile(backupPath.c_str());
}

void BackupManager::RollBackData(const std::string &path)
{
    auto tmpPath = path + BACKUP_TMP_POSTFIX;
    auto backupPath = path + BACKUP_BK_POSTFIX;
    CopyFile(backupPath, path);
    RemoveFile(tmpPath.c_str());
    RemoveFile(backupPath.c_str());
}

void BackupManager::CleanData(const std::string &path)
{
    auto backupPath = path + BACKUP_BK_POSTFIX;
    auto tmpPath = path + BACKUP_TMP_POSTFIX;
    RemoveFile(tmpPath.c_str());
    RemoveFile(backupPath.c_str());
}

/**
 *  learning by watching blow table, we can konw :
 *  as encrypt db, when backup's password same as db's password, need clean data,
 *  others if .bk file or .tmp file exist, rollback data
 *  as unencrypt db, tmp file exist, rollback, tmp file not exist, but .bk file exist, clean data
 *
 *  backup step (encrypt)   file status             key in meat         option          file num
 *  1, backup old data      autoBachup.bak          autoBachup.key      rollback        .bk = 1
 *                          autoBachup.bak.bk                                           .tmp = 0
 *
 *  2, do backup            autoBachup.bak          autoBachup.key      rollback        .bk = 1
 *                          autoBachup.bak.bk                                           .tmp = 1
 *                          autoBachup.bak.tmp
 *
 *  3, copy data            autoBachup.bak(new)     autoBachup.key      rollback        .bk = 1
 *                          autoBachup.bak.bk                                           .tmp = 1
 *                          autoBachup.bak.tmp
 *
 *  4, delet tmp data       autoBachup.bak(new)     autoBachup.key      rollback        .bk = 1
 *                          autoBachup.bak.bk                                           .tmp = 0
 *
 *  5, save key             autoBachup.bak(new)     autoBachup.key(new) clean data      .bk = 1
 *                          autoBachup.bak.bk                                           .tmp = 0
 *
 *  6, delet backup data    autoBachup.bak          autoBachup.key      do nothing      .bk = 0
 *                          -                       -                                   .tmp = 0
 *
 *  backup step (unencrypt) file status                     option                      file num
 *  1, backup old data      autoBachup.bak                  clean data                  .bk = 1
 *                          autoBachup.bak.bk                                           .tmp = 0
 *
 *  2, do backup            autoBachup.bak                  rollback data               .bk = 1
 *                          autoBachup.bak.bk,                                          .tmp = 1
 *                          autoBachup.bak.tmp
 *
 *  3, copy data            autoBachup.bak(new)             rollback data               .bk = 1
 *                          autoBachup.bak.bk,                                          .tmp = 1
 *                          autoBachup.bak.tmp
 *
 *  4, delet tmp data       autoBachup.bak                  clean data                  .bk = 1
 *                          autoBachup.bak.bk                                           .tmp = 0
 *
 *
 *  5, delet backup data    autoBachup.bak                  do nothing                  .bk = 0
 *                                                                                      .tmp =0
 * */
BackupManager::ClearType BackupManager::GetClearType(const StoreMetaData &meta)
{
    auto backupFile =
        DirectoryManager::GetInstance().GetStoreBackupPath(meta) + "/" + AUTO_BACKUP_NAME;
    auto dbKey = meta.GetSecretKey();
    auto backupKey = meta.GetBackupSecretKey();
    auto bkFile = backupFile + BACKUP_BK_POSTFIX;

    SecretKeyMetaData dbPassword;
    if (MetaDataManager::GetInstance().LoadMeta(dbKey, dbPassword, true)) {
        SecretKeyMetaData backupPassword;
        MetaDataManager::GetInstance().LoadMeta(backupKey, backupPassword, true);
        if (dbPassword.sKey != backupPassword.sKey && IsFileExist(bkFile)) {
            return ROLLBACK;
        }
        if (dbPassword.sKey == backupPassword.sKey && IsFileExist(bkFile)) {
            return CLEAN_DATA;
        }
    } else {
        auto tmpFile = backupFile + BACKUP_TMP_POSTFIX;
        if (IsFileExist(tmpFile)) {
            return ROLLBACK;
        }
        if (!IsFileExist(tmpFile) && IsFileExist(bkFile)) {
            return CLEAN_DATA;
        }
    }
    return DO_NOTHING;
}

void BackupManager::CopyFile(const std::string &oldPath, const std::string &newPath, bool isCreate)
{
    std::fstream fin, fout;
    if (!IsFileExist(oldPath)) {
        return;
    }
    fin.open(oldPath, std::ios_base::in);
    if (isCreate) {
        fout.open(newPath, std::ios_base::out | std::ios_base::ate);
    } else {
        fout.open(newPath, std::ios_base::out | std::ios_base::trunc);
    }
    char buf[COPY_SIZE] = {0};
    while (!fin.eof()) {
        fin.read(buf, COPY_SIZE);
        fout.write(buf, fin.gcount());
    }
    fin.close();
    fout.close();
}

bool BackupManager::GetPassWord(const StoreMetaData &meta, std::vector<uint8_t> &password)
{
    std::string key = meta.GetBackupSecretKey();
    SecretKeyMetaData secretKey;
    MetaDataManager::GetInstance().LoadMeta(key, secretKey, true);
    return CryptoManager::GetInstance().Decrypt(secretKey.sKey, password);
}

bool BackupManager::IsFileExist(const std::string &path)
{
    if (path.empty()) {
        return false;
    }
    if (access(path.c_str(), F_OK) != 0) {
        return false;
    }
    return true;
}

bool BackupManager::RemoveFile(const std::string &path)
{
    if (access(path.c_str(), F_OK) != 0) {
        return true;
    }
    if (remove(path.c_str()) != 0) {
        ZLOGE("remove error:%{public}d, path:%{public}s", errno, path.c_str());
        return false;
    }
    return true;
}
} // namespace OHOS::DistributedData