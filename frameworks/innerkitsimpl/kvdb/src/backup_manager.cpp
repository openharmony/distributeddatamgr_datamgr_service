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
#include "kvdb_service_client.h"
#include "log_print.h"
#include "security_manager.h"

namespace OHOS::DistributedKv {
namespace {
constexpr const char *BACKUP_POSTFIX = ".bak";
constexpr const int BACKUP_POSTFIX_SIZE = 4;
constexpr const char *BACKUP_TMP_POSTFIX = ".bk";
constexpr const int BACKUP_TMP_POSTFIX_SIZE = 3;
constexpr const char *BACKUP_KEY_POSTFIX = ".key";
constexpr const char *BACKUP_KEY_PREFIX = "Prefix_backup_";
constexpr const char *AUTO_BACKUP_NAME = "autoBackup";
constexpr const char *BACKUP_TOP_PATH = "/kvdb/backup";
constexpr const char *KEY_PATH = "/key";
}

BackupManager &BackupManager::GetInstance()
{
    static BackupManager instance;
    return instance;
}

BackupManager::BackupManager()
{
    pool_ = KvStoreThreadPool::GetPool(POOL_SIZE, true);
}

BackupManager::~BackupManager()
{
    if (pool_ != nullptr) {
        pool_->Stop();
        pool_ = nullptr;
    }
}

void BackupManager::Init(const std::string &baseDir)
{
    if (pool_ == nullptr) {
        ZLOGE("Backup Init, pool is null");
        return;
    }
    KvStoreTask task([this, baseDir]() {
        auto topPath = baseDir + BACKUP_TOP_PATH;
        auto keyPath = baseDir + KEY_PATH;
        auto storeIds = StoreUtil::GetSubPath(topPath);
        auto keyFiles = StoreUtil::GetFiles(keyPath);
        for (auto &storeId : storeIds) {
            if (storeId == "." || storeId == "..") {
                continue;
            }
            auto backupPath = topPath + "/" + storeId;
            auto backupFiles = StoreUtil::GetFiles(backupPath);
            if (HaveResidueFile(backupFiles) || HaveResidueKey(keyFiles, storeId)) {
                auto ResidueInfo = BuildResidueInfo(backupFiles, keyFiles, storeId);
                ClearResidueFile(ResidueInfo, baseDir, storeId);
            }
        }
    });
    pool_->AddTask(std::move(task));
}

void BackupManager::Prepare(const std::string &path, const std::string &storeId)
{
    std::string topPath = path + BACKUP_TOP_PATH;
    std::string storePath = topPath + "/" + storeId;
    std::string autoBackupName = storePath + "/" + AUTO_BACKUP_NAME + BACKUP_POSTFIX;
    (void)StoreUtil::InitPath(topPath);
    (void)StoreUtil::InitPath(storePath);
    (void)StoreUtil::CreateFile(autoBackupName);
}

void BackupManager::KeepData(const std::string &name, bool isCreated)
{
    auto tmpName = name + BACKUP_TMP_POSTFIX;
    if (isCreated) {
        StoreUtil::CreateFile(tmpName);
    } else {
        StoreUtil::Rename(name, tmpName);
    }
}

void BackupManager::RollBackData(const std::string &name, bool isCreated)
{
    auto tmpName = name + BACKUP_TMP_POSTFIX;
    if (isCreated) {
        StoreUtil::Remove(tmpName);
    } else {
        StoreUtil::Remove(name);
        StoreUtil::Rename(tmpName, name);
    }
}

void BackupManager::CleanTmpData(const std::string &name)
{
    auto tmpName = name + BACKUP_TMP_POSTFIX;
    StoreUtil::Remove(tmpName);
}

Status BackupManager::Backup(const std::string &name, const std::string &baseDir, const std::string &storeId,
    std::shared_ptr<DBStore> dbStore)
{
    if (dbStore == nullptr) {
        return ALREADY_CLOSED;
    }
    if (name.size() == 0 || baseDir.size() == 0 || storeId.size() == 0 || name == AUTO_BACKUP_NAME) {
        return INVALID_ARGUMENT;
    }
    std::string topPath = baseDir + BACKUP_TOP_PATH;
    std::string storePath = topPath + "/" + storeId;
    std::string backupFullName = storePath + "/"+ name + BACKUP_POSTFIX;
    std::string keyName = BACKUP_KEY_PREFIX + storeId + "_" + name;
    std::string keyFullName = baseDir + KEY_PATH + "/" + keyName + BACKUP_KEY_POSTFIX;

    bool isCreate = !StoreUtil::IsFileExist(backupFullName);
    if ((StoreUtil::GetFiles(storePath).size() >= MAX_BACKUP_NUM) && isCreate) {
        return ERROR;
    }
    (void)StoreUtil::InitPath(topPath);
    (void)StoreUtil::InitPath(storePath);
    KeepData(backupFullName, isCreate);
    auto password = SecurityManager::GetInstance().GetDBPassword(storeId, baseDir);
    if (password.GetSize() != 0) {
        KeepData(keyFullName, isCreate);
    }

    auto dbStatus = dbStore->Export(backupFullName, password);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status == SUCCESS) {
        if (password.GetSize() != 0) {
            SecurityManager::GetInstance().SaveDBPassword(keyName, baseDir, password);
            CleanTmpData(keyFullName);
        }
        CleanTmpData(backupFullName);
    } else {
        RollBackData(backupFullName, isCreate);
        if (password.GetSize() != 0) {
            RollBackData(keyFullName, isCreate);
        }
    }
    return status;
}

StoreUtil::FileInfo BackupManager::GetBackupFileInfo(
    const std::string &name, const std::string &baseDir, const std::string &storeId)
{
    StoreUtil::FileInfo backupFile;
    std::string path = baseDir + BACKUP_TOP_PATH + "/" + storeId;
    std::string backupName = name + BACKUP_POSTFIX;

    auto files = StoreUtil::GetFiles(path);
    time_t modifyTime = 0;
    for (auto &file : files) {
        if (file.name == backupName) {
            backupFile = file;
            break;
        }
        if ((file.modifyTime > modifyTime) && (file.size != 0)) {
            modifyTime = file.modifyTime;
            backupFile = file;
        }
    }
    return backupFile;
}

Status BackupManager::Restore(const std::string &name, const std::string &baseDir, const std::string &appId,
    const std::string &storeId, std::shared_ptr<DBStore> dbStore)
{
    if (dbStore == nullptr) {
        return ALREADY_CLOSED;
    }
    auto service = KVDBServiceClient::GetInstance();
    if (service == nullptr) {
        return SERVER_UNAVAILABLE;
    }
    if (storeId.size() == 0 || baseDir.size() == 0) {
        return INVALID_ARGUMENT;
    }
    auto backupFile = GetBackupFileInfo(name, baseDir, storeId);
    if (backupFile.name.size() == 0) {
        return INVALID_ARGUMENT;
    }
    std::string keyName = BACKUP_KEY_PREFIX + storeId + "_" + name;
    std::string fullName = baseDir + BACKUP_TOP_PATH + "/" + storeId + "/" + backupFile.name;
    SecurityManager::DBPassword password;
    if (backupFile.name != AUTO_BACKUP_NAME) {
        password = SecurityManager::GetInstance().GetDBPassword(keyName, baseDir);
    } else {
        std::vector<uint8_t> pwd;
        service->GetBackupPassword({appId}, {storeId}, pwd);
        password.SetValue(pwd.data(), pwd.size());
        pwd.assign(pwd.size(), 0);
    }
    auto dbStatus = dbStore->Import(fullName, password);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    return status;
}

Status BackupManager::DeleteBackup(std::map<std::string, Status> &deleteList, const std::string &baseDir,
    const std::string &storeId)
{
    if (deleteList.empty() || baseDir.size() == 0 || storeId.size() == 0) {
        return INVALID_ARGUMENT;
    }

    std::string path = baseDir + BACKUP_TOP_PATH + "/" + storeId;
    auto fileInfos = StoreUtil::GetFiles(path);
    for (auto &info : fileInfos) {
        auto it = deleteList.find(info.name.substr(0, info.name.length() - BACKUP_POSTFIX_SIZE));
        if (it == deleteList.end()) {
            continue;
        }
        if (info.name == AUTO_BACKUP_NAME) {
            it->second = INVALID_ARGUMENT;
            continue;
        }
        std::string keyName = BACKUP_KEY_PREFIX + storeId + "_" + info.name;
        SecurityManager::GetInstance().DelDBPassword(keyName, baseDir);
        it->second = (StoreUtil::Remove(path + "/" + info.name)) ?  SUCCESS : ERROR;
    }
    return SUCCESS;
}

bool BackupManager::HaveResidueFile(const std::vector<StoreUtil::FileInfo> &files)
{
    for (auto &file : files) {
        if (IsEndWith(file.name, BACKUP_TMP_POSTFIX)) {
            return true;
        }
    }
    return false;
}

bool BackupManager::HaveResidueKey(const std::vector<StoreUtil::FileInfo> &files, std::string storeId)
{
    for (auto &file : files) {
        auto prefix = BACKUP_KEY_PREFIX + storeId;
        if (IsBeginWith(file.name, prefix) && IsEndWith(file.name, BACKUP_TMP_POSTFIX)) {
            return true;
        }
    }
    return false;
}

std::string BackupManager::GetBackupName(const std::string &fileName)
{
    int postFixLen = IsEndWith(fileName, BACKUP_TMP_POSTFIX) ?
        BACKUP_POSTFIX_SIZE + BACKUP_TMP_POSTFIX_SIZE : BACKUP_POSTFIX_SIZE;
    return fileName.substr(0, fileName.length() - postFixLen);
}

void BackupManager::SetResidueInfo(BackupManager::ResidueInfo &residueInfo,
    const std::vector<StoreUtil::FileInfo> &files, const std::string &name, const std::string &postFix)
{
    for (auto &file : files) {
        if (IsBeginWith(file.name, name)) {
            if (IsEndWith(file.name, postFix + BACKUP_TMP_POSTFIX) && (postFix == BACKUP_POSTFIX)) {
                residueInfo.hasTmpBackup = true;
                residueInfo.tmpBackupSize = file.size;
            }
            if (IsEndWith(file.name, postFix) && (postFix == BACKUP_POSTFIX)) {
                residueInfo.hasRawBackup = true;
            }
            if (IsEndWith(file.name, postFix + BACKUP_TMP_POSTFIX) && (postFix == BACKUP_KEY_POSTFIX)) {
                residueInfo.hasTmpKey = true;
                residueInfo.tmpKeySize = file.size;
            }
            if (IsEndWith(file.name, postFix) && (postFix == BACKUP_KEY_POSTFIX)) {
                residueInfo.hasRawKey = true;
            }
        }
    }
}

std::map<std::string, BackupManager::ResidueInfo> BackupManager::BuildResidueInfo(
    const std::vector<StoreUtil::FileInfo> &files,
    const std::vector<StoreUtil::FileInfo> &keys, const std::string &storeId)
{
    std::map<std::string, ResidueInfo> residueInfoList;
    for (auto &file : files) {
        auto backupName = GetBackupName(file.name);
        auto it = residueInfoList.find(backupName);
        if (it == residueInfoList.end()) {
            ResidueInfo residueInfo = { 0, 0, false, false, false, false };
            SetResidueInfo(residueInfo, files, backupName, BACKUP_POSTFIX);
            SetResidueInfo(residueInfo, keys, BACKUP_KEY_PREFIX + storeId + "_" + backupName, BACKUP_KEY_POSTFIX);
            residueInfoList.emplace(backupName, residueInfo);
        }
    }
    return residueInfoList;
}

/**
 *  in function NeedRollBack, use the number of tmp and raw file to charge who to do when start,
 *  learning by watching blow table,
 *  we can konw when the num of tmp file greater than or equal raw, interrupt happend druing backup
 *
 *  backup step (encrypt)   file status                         option          file num
 *  1, backup old data      -               storeId.key         rollback data   raw = 1
 *                          storeId.bak.bk  -                                   tmp = 1
 *
 *  2, backup old key       -               -                   rollback        raw = 0
 *                          storeId.bak.bk, storeId.key.bk                      tmp = 2
 *
 *  3, do backup            storeId.bak     -                   rollback        raw = 1
 *                          storeId.bak.bk, storeId.key.bk                      tmp = 2
 *
 *  4, store key            storeId.bak     storeId.key         rollback        raw = 2
 *                          storeId.bak.bk, storeId.key.bk                      tmp = 2
 *
 *  5, delet tmp key        storeId.bak     storeId.key         clean data      raw = 2
 *                          storeId.bak.bk  -                                   tmp = 1
 *
 *  6, delet tmp data       storeId.bak     storeId.key         do nothing      raw = 2
 *                          -               -                                   tmp = 0
 *
 *
 *  backup step (unencrypt) file status                         option          file num
 *  1, backup old data      -                                   rollback data   raw = 0
 *                          storeId.bak.bk  -                                   tmp = 1
 *
 *  2, do backup            storeId.bak     -                   rollback data   raw = 1
 *                          storeId.bak.bk, -                                   tmp = 1
 *
 *  6, delet tmp data       storeId.bak     -                   do nothing      raw = 1
 *                          -               -                                   tmp = 0
 *
 * */
BackupManager::ClearType BackupManager::GetClearType(const BackupManager::ResidueInfo &residueInfo)
{
    int rawFile = 0;
    int tmpFile = 0;
    if (residueInfo.hasRawBackup) {
        rawFile++;
    }
    if (residueInfo.hasRawKey) {
        rawFile++;
    }
    if (residueInfo.hasTmpBackup) {
        tmpFile++;
    }
    if (residueInfo.hasTmpKey) {
        tmpFile++;
    }
    if (tmpFile == 0) {
        return DO_NOTHING;
    }
    if ((tmpFile >= rawFile) && (tmpFile ==1)) {
        return ROLLBACK_DATA;
    }
    return (tmpFile >= rawFile) ? ROLLBACK : CLEAN_TMP;
}

void BackupManager::ClearResidueFile(std::map<std::string, ResidueInfo> residueInfo,
    const std::string &baseDir, const std::string &storeId)
{
    for (auto &info : residueInfo) {
        auto backupFullName = baseDir + BACKUP_TOP_PATH + "/" + storeId + "/" + info.first + BACKUP_POSTFIX;
        auto keyFullName =
            baseDir + KEY_PATH + "/" + BACKUP_KEY_PREFIX + storeId + "_" + info.first + BACKUP_KEY_POSTFIX;
        switch (GetClearType(info.second)) {
            case ROLLBACK_DATA:
                RollBackData(backupFullName, (info.second.tmpBackupSize == 0));
                break;
            case ROLLBACK:
                RollBackData(backupFullName, (info.second.tmpBackupSize == 0));
                RollBackData(keyFullName, (info.second.tmpKeySize == 0));
                break;
            case CLEAN_TMP:
                CleanTmpData(backupFullName);
                CleanTmpData(keyFullName);
                break;
            case DO_NOTHING:
            default:
                break;
        }
    }
}

bool BackupManager::IsEndWith(const std::string &fullString, const std::string &end)
{
    if (fullString.length() >= end.length()) {
        return (fullString.compare(fullString.length() - end.length(), end.length(), end) == 0);
    } else {
        return false;
    }
}

bool BackupManager::IsBeginWith(const std::string &fullString, const std::string &begin)
{
    if (fullString.length() >= begin.length()) {
        return (fullString.compare(0, begin.length(), begin) == 0);
    } else {
        return false;
    }
}
} // namespace OHOS::DistributedKv