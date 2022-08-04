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
#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "backuprule/backup_rule_manager.h"
#include "communication_provider.h"
#include "constant.h"
#include "crypto_manager.h"
#include "directory_manager.h"
#include "ipc_skeleton.h"
#include "kv_scheduler.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "types.h"
namespace OHOS::DistributedData {
using namespace OHOS::Security::AccessToken;
using namespace AppDistributedKv;
namespace {
constexpr const char *AUTO_BACKUP_NAME = "autoBackup.bak";
constexpr const char *BACKUP_BK_POSTFIX = ".bk";
constexpr const char *BACKUP_TMP_POSTFIX = ".tmp";
constexpr const char *BACKUP_KEY_POSTFIX = "_KeyForAutoBackup";
}

BackupManager::BackupManager()
{
    Init();
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
    auto metas = GetMetas();
    for (auto &meta : metas) {
        if (!meta.isBackup || meta.isDirty) {
                continue;
        }
        auto backupPath =
            DirectoryManager::GetInstance().GetStoreBackupPath(meta) + "/" + meta.storeId + "/" + AUTO_BACKUP_NAME;
        auto key = meta.GetSecretKey();
        switch (GetClearType(backupPath)) {
            case ROLLBACK:
                RollBackData(backupPath, key);
                break;
            case CLEAN_DATA:
                CleanData(backupPath, key);
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

void BackupManager::RegisterExporter(int type, Exporter exporter)
{
    exporters_[type] = exporter;
}

void BackupManager::BackSchedule()
{
    std::chrono::duration<int> delay(schedularDelay_);
    std::chrono::duration<int> internal(schedularInternal_);
    ZLOGI("BackupHandler Schedule start.");
    scheduler_.Every(delay, internal, [&]() {
        if (!CanBackup()) {
            return;
        }
        auto metas = GetMetas();
        completeNum_ = 0;
        for (auto &meta : metas) {
            if (!meta.isBackup || meta.isDirty) {
                continue;
            }
            DoBackup(meta);
            if (completeNum_ - startNum_ >= backupNumber_) {
                startNum_ += backupNumber_;
                break;
            }
            startNum_ = 0;
        }
        backupSuccessTime_ = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    });
}

void BackupManager::DoBackup(const StoreMetaData &meta)
{
    if (startNum_ > completeNum_) {
        completeNum_++;
        return;
    }
    DistributedKv::Status status;
    std::vector<uint8_t> decryptKey;
    std::string key = meta.GetSecretKey();
    SecretKeyMetaData secretKey;
    MetaDataManager::GetInstance().LoadMeta(key, secretKey, true);
    CryptoManager::GetInstance().Decrypt(secretKey.sKey, decryptKey);
    auto backupPath = DirectoryManager::GetInstance().GetStoreBackupPath(meta);
    std::string backupFullPath = backupPath + "/" + meta.storeId + "/" + AUTO_BACKUP_NAME;

    KeepData(backupFullPath, key, secretKey);
    exporters_[meta.storeType](meta, decryptKey, backupFullPath + BACKUP_TMP_POSTFIX, status);
    if (status == DistributedKv::Status::SUCCESS) {
        SaveData(backupFullPath, key, secretKey);
    } else {
        CleanData(backupFullPath, key);
    }
    std::fill(decryptKey.begin(), decryptKey.end(), 0);
    completeNum_++;
}

std::vector<StoreMetaData> BackupManager::GetMetas()
{
    std::vector<StoreMetaData> results;
    auto device = CommunicationProvider::GetInstance().GetLocalDevice();
    if (device.uuid.empty()) {
        ZLOGE("local uuid is empty!");
    }
    ZLOGI("BackupHandler Schedule Every start.");
    if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ device.uuid }), results)) {
        ZLOGE("GetFullMetaData failed.");
    }
    return results;
}

bool BackupManager::CanBackup()
{
    if (!BackupRuleManager::GetInstance().CanBackup()) {
        return false;
    }
    int64_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    if (currentTime - backupSuccessTime_ < backupInternal_ && backupSuccessTime_ > 0) {
        ZLOGE("no more than backup internal time since the last backup success.");
        return false;
    }
    return true;
}

void BackupManager::KeepData(
    const std::string &path, const std::string &key, const SecretKeyMetaData &secretKey)
{
    auto backupPath = path + BACKUP_BK_POSTFIX;
    auto backupKey = key + BACKUP_KEY_POSTFIX + BACKUP_BK_POSTFIX;
    CopyFile(path, backupPath, true);
    MetaDataManager::GetInstance().SaveMeta(backupKey, secretKey, true);
}

void BackupManager::SaveData(
    const std::string &path, const std::string &key, const SecretKeyMetaData &secretKey)
{
    auto tmpPath = path + BACKUP_TMP_POSTFIX;
    auto backupPath = path + BACKUP_BK_POSTFIX;
    auto saveKey = key + BACKUP_KEY_POSTFIX;
    auto backupKey = saveKey + BACKUP_BK_POSTFIX;
    CopyFile(tmpPath, path);
    MetaDataManager::GetInstance().SaveMeta(saveKey, secretKey, true);
    RemoveFile(tmpPath.c_str());
    MetaDataManager::GetInstance().DelMeta(backupKey, true);
    RemoveFile(backupPath.c_str());
}

void BackupManager::RollBackData(const std::string &path, const std::string &key)
{
    auto tmpPath = path + BACKUP_TMP_POSTFIX;
    auto backupPath = path + BACKUP_BK_POSTFIX;
    CopyFile(backupPath, path);
    RemoveFile(tmpPath.c_str());
    RemoveFile(backupPath.c_str());
    auto saveKey = key + BACKUP_KEY_POSTFIX;
    auto backupKey = saveKey + BACKUP_BK_POSTFIX;
    SecretKeyMetaData secretKey;
    if (MetaDataManager::GetInstance().LoadMeta(backupKey, secretKey, true)) {
        MetaDataManager::GetInstance().SaveMeta(saveKey, secretKey, true);
        MetaDataManager::GetInstance().DelMeta(backupKey, true);
    };
}

void BackupManager::CleanData(const std::string &path, const std::string &key)
{
    auto backupPath = path + BACKUP_BK_POSTFIX;
    auto tmpPath = path + BACKUP_TMP_POSTFIX;
    auto backupKey = path + BACKUP_KEY_POSTFIX + BACKUP_BK_POSTFIX;
    RemoveFile(tmpPath.c_str());
    RemoveFile(backupPath.c_str());
    MetaDataManager::GetInstance().DelMeta(backupKey, true);
}

/**
 *  learning by watching blow table, we can konw :
 *  when tmp file exist, interrupt happend druing backup or copy data to client's file
 *  when tmp file not exist but bk file exist, interrupt happend before backup or copy data completed
 *
 *  backup step (encrypt)   file status             key in meat         option          file num
 *  1, backup old data      autoBachup.bak          autoBachup.key      clean data      raw = 2
 *                          autoBachup.bak.bk       -                                   .bk = 1
 *                                                                                      .tmp = 0
 *
 *  2, backup old key       autoBachup.bak          autoBachup.key      clean data      raw = 2
 *                          autoBachup.bak.bk       autoBachup.key.bk                   .bk = 2
 *                                                                                      .tmp = 0
 *
 *  3, do backup            autoBachup.bak          autoBachup.key      rollback        raw = 2
 *                          autoBachup.bak.bk       autoBachup.key.bk                   .bk = 2
 *                          autoBachup.bak.tmp                                          .tmp = 1
 *
 *  4, copy data            autoBachup.bak(new)     autoBachup.key      rollback        raw = 2
 *                          autoBachup.bak.bk       autoBachup.key.bk                   .bk = 2
 *                          autoBachup.bak.tmp                                          .tmp = 1
 *
 *  5, save key             autoBachup.bak(new)     autoBachup.key(new) rollback        raw = 2
 *                          autoBachup.bak.bk       autoBachup.key.bk                   .bk = 2
 *                          autoBachup.bak.tmp                                          .tmp = 1
 *
 *  6, delet tmp data       autoBachup.bak(new)     autoBachup.key(new) clean data       raw = 2
 *                          autoBachup.bak.bk       autoBachup.key.bk                   .bk = 1
 *                          -                                                           .tmp = 0
 *
 *  7, delet backup key     autoBachup.bak(new)     autoBachup.key(new) clean data      raw = 2
 *                          autoBachup.bak.bk                                           .bk = 1
 *                          -                                                           .tmp = 0
 *
 *  8, delet backup data    autoBachup.bak          autoBachup.key         do nothing   raw = 2
 *                          -                       -                                   .bk = 0
 *                          -                                                           .tmp = 0
 *
 *  backup step (unencrypt) file status                     option                      file num
 *  1, backup old data      autoBachup.bak                  clean data                  raw = 1
 *                          autoBachup.bak.bk                                           .bk = 1
 *                                                                                      .tmp = 0
 *
 *  2, do backup            autoBachup.bak                  rollback data               raw = 1
 *                          autoBachup.bak.bk,                                          .bk = 1
 *                          autoBachup.bak.tmp                                          .tmp = 1
 *
 *  3, copy data            autoBachup.bak(new)             rollback data               raw = 1
 *                          autoBachup.bak.bk,                                          .bk = 1
 *                          autoBachup.bak.tmp                                          .tmp = 1
 *
 *  4, delet tmp data       autoBachup.bak                  clean data                  raw = 1
 *                          autoBachup.bak.bk                                           .bk = 1
 *                                                                                      .tmp =0
 *
 *  5, delet backup data    autoBachup.bak                  do nothing                  raw = 1
 *                                                                                      .bk = 0
 *                                                                                      .tmp =0
 * */
BackupManager::ClearType BackupManager::GetClearType(const std::string &path)
{

    std::string tmpFile = path + BACKUP_TMP_POSTFIX;
    std::string bkFile = path + BACKUP_BK_POSTFIX;
    if (IsFileExist(tmpFile)) {
        return ROLLBACK;
    }
    if (!IsFileExist(tmpFile) && IsFileExist(bkFile)) {
        return CLEAN_DATA;
    }
    return DO_NOTHING;
}

void BackupManager::CopyFile(const std::string &oldPath, const std::string &newPath, bool isCreate)
{
    std::fstream fin,fout;
    fin.open(oldPath, std::ios_base::in);
    if (isCreate) {
        fout.open(newPath, std::ios_base::out | std::ios_base::ate);
    } else {
        fout.open(newPath, std::ios_base::out | std::ios_base::trunc);
    }
    char buf[1024] = {0};
    while( !fin.eof() )
    {
        fin.read(buf,1024);
        fout.write(buf,fin.gcount());
    }
    fin.close();
    fout.close();
}

bool BackupManager::GetPassWord(const DistributedKv::AppId &appId, const DistributedKv::StoreId &storeId, std::vector<uint8_t> password)
{
    auto meta = GetStoreMetaData(appId, storeId);
    std::string key = meta.GetSecretKey();
    std::string backupKey = key + BACKUP_KEY_POSTFIX;
    SecretKeyMetaData secretKey;
    MetaDataManager::GetInstance().LoadMeta(backupKey, secretKey, true);
    return CryptoManager::GetInstance().Decrypt(secretKey.sKey, password);
}

StoreMetaData BackupManager::GetStoreMetaData(const DistributedKv::AppId &appId, const DistributedKv::StoreId &storeId)
{
    StoreMetaData metaData;
    metaData.uid = IPCSkeleton::GetCallingUid();
    metaData.tokenId = IPCSkeleton::GetCallingTokenID();
    metaData.instanceId = GetInstIndex(metaData.tokenId, appId);
    metaData.bundleName = appId.appId;
    metaData.deviceId = CommunicationProvider::GetInstance().GetLocalDevice().uuid;
    metaData.storeId = storeId.storeId;
    metaData.user = DistributedKv::AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(metaData.uid);
    return metaData;
}

int32_t BackupManager::GetInstIndex(uint32_t tokenId, const DistributedKv::AppId &appId)
{
    if (Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(tokenId) != TOKEN_HAP) {
        return 0;
    }
    HapTokenInfo tokenInfo;
    tokenInfo.instIndex = -1;
    int errCode = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (errCode != RET_SUCCESS) {
        ZLOGE("GetHapTokenInfo error:%{public}d, tokenId:0x%{public}x appId:%{public}s", errCode, tokenId,
            appId.appId.c_str());
        return -1;
    }
    return tokenInfo.instIndex;
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