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
#include "log_print.h"
#include "security_manager.h"

namespace OHOS::DistributedKv {
namespace {
constexpr const char *BACKUP_POSTFIX = ".backup";
constexpr const char *BACKUP_TMP_POSTFIX = ".bk";
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
        std::vector<std::string> storeIds;
        std::vector<StoreUtil::FileInfo> keyFiles;
        (void)StoreUtil::GetSubPath(topPath, storeIds);
        (void)StoreUtil::GetFiles(keyPath, keyFiles);
        for (auto &storeId : storeIds) {
            if (storeId == "." || storeId == "..") {
                continue;
            }
            std::vector<StoreUtil::FileInfo> backupFiles;
            auto backupPath = topPath + "/" + storeId;
            (void)StoreUtil::GetFiles(backupPath, backupFiles);
            if (!HaveResidueFile(backupFiles) && !HaveResidueKey(keyFiles, storeId)) {
                continue;
            } else {
                ZLOGE("Init store Id :%{public}s need to clear.", storeId.c_str());
                BuildResidueInfo(backupFiles, keyFiles, storeId);
                ClearResidueFile(baseDir, storeId);
            }
        }
    });
    pool_->AddTask(std::move(task));
}

void BackupManager::Prepare(std::string path, std::string &storeId, bool isAutoBackup)
{
    std::string topPath = path + BACKUP_TOP_PATH;
    std::string storePath = topPath + "/" + storeId;
    (void)StoreUtil::InitPath(topPath);
    (void)StoreUtil::InitPath(storePath);
    if (isAutoBackup) {
        std::string autoBackupName = storePath + "/" + AUTO_BACKUP_NAME + BACKUP_POSTFIX;
        (void)StoreUtil::CreateFile(autoBackupName);
    }
}

Status BackupManager::Backup(std::string &name, std::string &baseDir, std::string &storeId,
    std::shared_ptr<DBStore> &dbStore)
{
    if (name.size() == 0 || baseDir.size() == 0 || storeId.size() == 0 || name == AUTO_BACKUP_NAME) {
        return INVALID_ARGUMENT;
    }
    std::string path = baseDir + BACKUP_TOP_PATH + "/" + storeId;
    std::string backName = name + BACKUP_POSTFIX;
    std::string backFullName = path + "/"+ backName;
    std::string backupTmpName = backFullName + BACKUP_TMP_POSTFIX;
    std::vector<StoreUtil::FileInfo> fileInfos;
    (void)StoreUtil::GetFiles(path, fileInfos);
    bool fileExist = MatchedInFiles(backName, fileInfos);
    if (!fileExist) {
        if (fileInfos.size() >= MAX_BACKUP_NUM) {
            return ERROR;
        }
        StoreUtil::CreateFile(backupTmpName);
    } else {
        StoreUtil::Rename(backFullName, backupTmpName);
    }

    auto password = SecurityManager::GetInstance().GetKey(storeId, baseDir);
    ZLOGI("restore, password : size : %{public}zu, data : %{pubilc}s", password.GetSize(), password.GetData());
    std::string keyName = BACKUP_KEY_PREFIX + storeId + "_" + name;
    std::string keyFullName = baseDir + KEY_PATH + "/" + keyName + BACKUP_KEY_POSTFIX;
    std::string keyTmpName = keyFullName + BACKUP_TMP_POSTFIX;
    if (password.GetSize() != 0) {
        if (fileExist) {
            StoreUtil::Rename(keyFullName, keyTmpName);
        } else {
            StoreUtil::CreateFile(keyTmpName);
        }
    }

    auto dbStatus = dbStore->Export(backFullName, password);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    if (status == SUCCESS) {
        if (password.GetSize() != 0) {
            std::vector<uint8_t> pwd(password.GetData(), password.GetData() + password.GetSize());
            SecurityManager::GetInstance().SaveKey(keyName, baseDir, pwd);
            pwd.assign(pwd.size(), 0);
            StoreUtil::Remove(keyTmpName);
        }
        StoreUtil::Remove(backupTmpName);
    } else {
        if (fileExist) {
            StoreUtil::Remove(backFullName);
            StoreUtil::Rename(backupTmpName, backFullName);
            if (password.GetSize() != 0) {
                StoreUtil::Rename(keyTmpName, keyFullName);
            }
        } else {
            StoreUtil::Remove(backFullName);
            StoreUtil::Remove(backupTmpName);
            if (password.GetSize() != 0) {
                StoreUtil::Remove(keyTmpName);
            }
        }
    }
    return status;
}

Status BackupManager::Restore(std::string &name, std::string &baseDir, std::string &storeId,
    std::shared_ptr<DBStore> &dbStore)
{
    if (storeId.size() == 0 || baseDir.size() == 0) {
        return INVALID_ARGUMENT;
    }
    std::string path = baseDir + BACKUP_TOP_PATH + "/" + storeId;
    std::string fullName;
    if (name.size() != 0) {
        fullName = path + "/" + name + BACKUP_POSTFIX;
        if (!StoreUtil::IsFileExist(fullName)) {
            return INVALID_ARGUMENT;
        }
    } else {
        std::vector<StoreUtil::FileInfo> files;
        (void)StoreUtil::GetFiles(path, files);
        if (files.size() == 0) {
            return INVALID_ARGUMENT;
        }
        GetLatestFile(name, files);
        fullName = path + "/" + name;
    }

    std::string keyName = BACKUP_KEY_PREFIX + storeId + "_" + name;
    auto password = SecurityManager::GetInstance().GetKey(keyName, baseDir);
    ZLOGI("restore, password : size : %{public}zu, data : %{pubilc}s", password.GetSize(), password.GetData());
    auto dbStatus = dbStore->Import(fullName, password);
    auto status = StoreUtil::ConvertStatus(dbStatus);
    return status;
}

Status BackupManager::DeleteBackup(std::vector<std::string> &files, std::string &baseDir, std::string &storeId,
    std::map<std::string, Status> &results)
{
    if (files.empty() || baseDir.size() == 0 || storeId.size() == 0) {
        return INVALID_ARGUMENT;
    }
    std::string path = baseDir + BACKUP_TOP_PATH + "/" + storeId;
    std::vector<StoreUtil::FileInfo> fileInfos;
    (void)StoreUtil::GetFiles(path, fileInfos);
    for (auto &file : files) {
        std::string fullName = file + BACKUP_POSTFIX;
        if (file == AUTO_BACKUP_NAME) {
            results.emplace(file, INVALID_ARGUMENT);
        } else if (MatchedInFiles(fullName, fileInfos)) {
            std::string keyName = BACKUP_KEY_PREFIX + storeId + "_" + file;
            SecurityManager::GetInstance().DelKey(keyName, baseDir);
            if (StoreUtil::Remove(path + "/" + fullName)) {
                results.emplace(file, SUCCESS);
            } else {
                results.emplace(file, ERROR);
                ZLOGE("DeleteBackup, name: %{public}s, ret : %{pubilc}d", file.c_str(), ERROR);
            }
        } else {
            results.emplace(file, STORE_NOT_FOUND);
            ZLOGE("DeleteBackup, name: %{public}s, ret : %{pubilc}d", file.c_str(), STORE_NOT_FOUND);
        }
    }
    return SUCCESS;
}

bool BackupManager::MatchedInFiles(std::string &file, std::vector<StoreUtil::FileInfo> &fileInfos)
{
    for (auto &fileInfo : fileInfos) {
        if (file == fileInfo.name) {
            return true;
        }
    }
    return false;
}

void BackupManager::GetLatestFile(std::string &name, std::vector<StoreUtil::FileInfo> &fileInfos)
{
    time_t temTime = 0;
    for (auto &file : fileInfos) {
        if (file.modifyTime > temTime) {
            temTime = file.modifyTime;
            name = file.name;
        }
    }
}

void BackupManager::BuildResidueInfo(std::vector<StoreUtil::FileInfo> &fileList,
    std::vector<StoreUtil::FileInfo> &keyList, std::string &storeId)
{
    for (auto &file : fileList) {
        std::string backupName;
        if (IsEndWith(file.name, BACKUP_TMP_POSTFIX)) {
            backupName = file.name.substr(0, file.name.length() - 10);
        } else {
            backupName = file.name.substr(0, file.name.length() - 7);
        }

        auto it = backupResidueInfo_.find(backupName);
        if (it != backupResidueInfo_.end()) {
            if (IsEndWith(file.name, BACKUP_TMP_POSTFIX)) {
                it->second.haveTmpBackupFile = true;
                it->second.tmpBackupFileSize = file.size;
            } else {
                it->second.haveRawBackupFile = true;
            }
        } else {
            ResidueInfo residueInfo = {0};
            if (IsEndWith(file.name, BACKUP_TMP_POSTFIX)) {
                residueInfo.haveTmpBackupFile = true;
                residueInfo.tmpBackupFileSize = file.size;
            } else {
                residueInfo.haveRawBackupFile = true;
            }
            for (auto key : keyList) {
                auto keyName = BACKUP_KEY_PREFIX + storeId + "_" + backupName + BACKUP_KEY_POSTFIX;
                if (IsBeginWith(key.name, keyName)) {
                    if (IsEndWith(key.name, BACKUP_TMP_POSTFIX)) {
                        residueInfo.haveTmpKeyFile = true;
                    } else {
                        residueInfo.haveRawKeyFile = true;
                    }
                }
            }
            backupResidueInfo_.emplace(backupName, residueInfo);
        }
    }
}

void BackupManager::ClearResidueFile(const std::string &baseDir, std::string &storeId)
{
   for (auto it : backupResidueInfo_) {
        auto backupFullName = baseDir + BACKUP_TOP_PATH + "/" + storeId + "/" + it.first + BACKUP_POSTFIX;
        auto keyFullName = baseDir + KEY_PATH + "/" + BACKUP_KEY_PREFIX + storeId + "_" + it.first + BACKUP_KEY_POSTFIX;
       if (NeedRollBack(it.second)) {
            if (it.second.haveTmpBackupFile) {
                StoreUtil::Remove(backupFullName);
                if (it.second.tmpBackupFileSize == 0) {
                    StoreUtil::Remove(backupFullName + BACKUP_TMP_POSTFIX);
                } else {
                    StoreUtil::Rename(backupFullName + BACKUP_TMP_POSTFIX, backupFullName);
                }
            }
            if (it.second.haveTmpKeyFile) {
                StoreUtil::Remove(keyFullName);
                if (it.second.tmpKeyFileSize == 0 ) {
                    StoreUtil::Remove(keyFullName + BACKUP_TMP_POSTFIX);
                } else {
                    StoreUtil::Rename(keyFullName + BACKUP_TMP_POSTFIX, keyFullName);
                }
            }
       } else {
            if (it.second.haveTmpBackupFile) {
                StoreUtil::Remove(backupFullName + BACKUP_TMP_POSTFIX);
            }
            if (it.second.haveTmpKeyFile) {
                StoreUtil::Remove(keyFullName + BACKUP_TMP_POSTFIX);
            }
       }
    }
}

bool BackupManager::HaveResidueFile(std::vector<StoreUtil::FileInfo> fileList)
{
    for (auto &file : fileList) {
        if (IsEndWith(file.name, BACKUP_TMP_POSTFIX)) {
            return true;
        }
    }
    return false;
}

bool BackupManager::HaveResidueKey(std::vector<StoreUtil::FileInfo> fileList, std::string &storeId)
{
    for (auto &file : fileList) {
        auto prefix = BACKUP_KEY_PREFIX + storeId;
        if (IsBeginWith(file.name, prefix) && IsEndWith(file.name, BACKUP_TMP_POSTFIX)) {
            return true;
        }
    }
    return false;
}

bool BackupManager::IsEndWith(const std::string &fullString, const std::string &end)
{
    if (fullString.length() >= end.length()) {
        return (0 == fullString.compare(fullString.length() - end.length(), end.length(), end));
    } else {
        return false;
    }
}

bool BackupManager::IsBeginWith(const std::string &fullString, const std::string &begin)
{
    if (fullString.length() >= begin.length()) {
        return (0 == fullString.compare(0, begin.length(), begin));
    } else {
        return false;
    }
}

bool BackupManager::NeedRollBack(BackupManager::ResidueInfo residueInfo)
{
    int rawFile = 0;
    int tmpFile = 0;
    bool needRollBack = false;
    if (residueInfo.haveRawBackupFile) {
        rawFile++;
    }
    if (residueInfo.haveRawKeyFile) {
        rawFile++;
    }
    if (residueInfo.haveTmpBackupFile) {
        tmpFile++;
    }
    if (residueInfo.haveTmpKeyFile) {
        tmpFile++;
    }

    if (tmpFile >= rawFile) {
        needRollBack =  true;
    }
    return needRollBack;
}
} // namespace OHOS::DistributedKv