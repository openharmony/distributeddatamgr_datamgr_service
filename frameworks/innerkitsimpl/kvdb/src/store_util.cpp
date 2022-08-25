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
#define LOG_TAG "StoreUtil"
#include "store_util.h"
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "log_print.h"
#include "types.h"
#include "unistd.h"
namespace OHOS::DistributedKv {
constexpr mode_t DEFAULT_UMASK = 0002;
constexpr int32_t HEAD_SIZE = 3;
constexpr int32_t END_SIZE = 3;
constexpr int32_t MIN_SIZE = HEAD_SIZE + END_SIZE + 3;
constexpr const char *REPLACE_CHAIN = "***";
constexpr const char *DEFAULT_ANONYMOUS = "******";
StoreUtil::DBSecurity StoreUtil::GetDBSecurity(int32_t secLevel)
{
    if (secLevel < SecurityLevel::NO_LABEL || secLevel > SecurityLevel::S4) {
        return { DistributedDB::NOT_SET, DistributedDB::ECE };
    }
    if (secLevel == SecurityLevel::S3) {
        return { DistributedDB::S3, DistributedDB::SECE };
    }
    if (secLevel == SecurityLevel::S4) {
        return { DistributedDB::S4, DistributedDB::ECE };
    }
    return { secLevel, DistributedDB::ECE };
}

int32_t StoreUtil::GetSecLevel(StoreUtil::DBSecurity dbSec)
{
    switch (dbSec.securityLabel) {
        case DistributedDB::NOT_SET: // fallthrough
        case DistributedDB::S0:      // fallthrough
        case DistributedDB::S1:      // fallthrough
        case DistributedDB::S2:      // fallthrough
            return dbSec.securityLabel;
        case DistributedDB::S3:
            return dbSec.securityFlag ? S3 : S3_EX;
        case DistributedDB::S4:
            return S4;
        default:
            break;
    }
    return NO_LABEL;
}

StoreUtil::DBMode StoreUtil::GetDBMode(SyncMode syncMode)
{
    DBMode dbMode;
    if (syncMode == SyncMode::PUSH) {
        dbMode = DBMode::SYNC_MODE_PUSH_ONLY;
    } else if (syncMode == SyncMode::PULL) {
        dbMode = DBMode::SYNC_MODE_PULL_ONLY;
    } else {
        dbMode = DBMode::SYNC_MODE_PUSH_PULL;
    }
    return dbMode;
}

uint32_t StoreUtil::GetObserverMode(SubscribeType subType)
{
    uint32_t mode;
    if (subType == SubscribeType::SUBSCRIBE_TYPE_LOCAL) {
        mode = DistributedDB::OBSERVER_CHANGES_NATIVE;
    } else if (subType == SubscribeType::SUBSCRIBE_TYPE_REMOTE) {
        mode = DistributedDB::OBSERVER_CHANGES_FOREIGN;
    } else {
        mode = DistributedDB::OBSERVER_CHANGES_FOREIGN | DistributedDB::OBSERVER_CHANGES_NATIVE;
    }
    return mode;
}

std::string StoreUtil::Anonymous(const std::string &name)
{
    if (name.length() <= HEAD_SIZE) {
        return DEFAULT_ANONYMOUS;
    }

    if (name.length() < MIN_SIZE) {
        return (name.substr(0, HEAD_SIZE) + REPLACE_CHAIN);
    }

    return (name.substr(0, HEAD_SIZE) + REPLACE_CHAIN + name.substr(name.length() - END_SIZE, END_SIZE));
}

uint32_t StoreUtil::Anonymous(const void *ptr)
{
    uint32_t hash = (uintptr_t(ptr) & 0xFFFFFFFF);
    hash = (hash & 0xFFFF) ^ ((hash >> 16) & 0xFFFF);
    return hash;
}

Status StoreUtil::ConvertStatus(DBStatus status)
{
    switch (status) {
        case DBStatus::BUSY: // fallthrough
        case DBStatus::DB_ERROR:
            return Status::DB_ERROR;
        case DBStatus::OK:
            return Status::SUCCESS;
        case DBStatus::INVALID_ARGS:
            return Status::INVALID_ARGUMENT;
        case DBStatus::NOT_FOUND:
            return Status::NOT_FOUND;
        case DBStatus::INVALID_VALUE_FIELDS:
            return Status::INVALID_VALUE_FIELDS;
        case DBStatus::INVALID_FIELD_TYPE:
            return Status::INVALID_FIELD_TYPE;
        case DBStatus::CONSTRAIN_VIOLATION:
            return Status::CONSTRAIN_VIOLATION;
        case DBStatus::INVALID_FORMAT:
            return Status::INVALID_FORMAT;
        case DBStatus::INVALID_QUERY_FORMAT:
            return Status::INVALID_QUERY_FORMAT;
        case DBStatus::INVALID_QUERY_FIELD:
            return Status::INVALID_QUERY_FIELD;
        case DBStatus::NOT_SUPPORT:
            return Status::NOT_SUPPORT;
        case DBStatus::TIME_OUT:
            return Status::TIME_OUT;
        case DBStatus::OVER_MAX_LIMITS:
            return Status::OVER_MAX_SUBSCRIBE_LIMITS;
        case DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB:
            return Status::CRYPT_ERROR;
        case DBStatus::SCHEMA_MISMATCH:
            return Status::SCHEMA_MISMATCH;
        case DBStatus::INVALID_SCHEMA:
            return Status::INVALID_SCHEMA;
        case DBStatus::EKEYREVOKED_ERROR: // fallthrough
        case DBStatus::SECURITY_OPTION_CHECK_ERROR:
            return Status::SECURITY_LEVEL_ERROR;
        default:
            ZLOGE("unknown db error:0x%{public}x", status);
            break;
    }
    return Status::ERROR;
}
bool StoreUtil::InitPath(const std::string &path)
{
    if (access(path.c_str(), F_OK) == 0) {
        return true;
    }
    umask(DEFAULT_UMASK);
    if (mkdir(path.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) != 0 && errno != EEXIST) {
        ZLOGE("mkdir error:%{public}d, path:%{public}s", errno, path.c_str());
        return false;
    }
    return true;
}

bool StoreUtil::CreateFile(const std::string &name)
{
    if (access(name.c_str(), F_OK) == 0) {
        return true;
    }
    umask(DEFAULT_UMASK);
    int fp = open(name.c_str(), (O_WRONLY | O_CREAT), (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP));
    if (fp < 0) {
        ZLOGE("fopen error:%{public}d, path:%{public}s", errno, name.c_str());
        return false;
    }
    close(fp);
    return true;
}

std::vector<std::string> StoreUtil::GetSubPath(const std::string &path)
{
    std::vector<std::string> subPaths;
    DIR *dirp = opendir(path.c_str());
    if (dirp == nullptr) {
        ZLOGE("opendir error:%{public}d, path:%{public}s", errno, path.c_str());
        return subPaths;
    }
    struct dirent *dp;
    while ((dp = readdir(dirp)) != nullptr) {
        if (dp->d_type == DT_DIR) {
            subPaths.push_back(dp->d_name);
        }
    }
    (void)closedir(dirp);
    return subPaths;
}

std::vector<StoreUtil::FileInfo> StoreUtil::GetFiles(const std::string &path)
{
    std::vector<FileInfo> fileInfos;
    DIR *dirp = opendir(path.c_str());
    if (dirp == nullptr) {
        ZLOGE("opendir error:%{public}d, path:%{public}s", errno, path.c_str());
        return fileInfos;
    }
    struct dirent *dp;
    while ((dp = readdir(dirp)) != nullptr) {
        if (dp->d_type == DT_REG) {
            struct stat fileStat;
            auto fullName = path + "/" + dp->d_name;
            stat(fullName.c_str(), &fileStat);
            FileInfo fileInfo = { "", 0, 0 };
            fileInfo.name = dp->d_name;
            fileInfo.modifyTime = fileStat.st_mtim.tv_sec;
            fileInfo.size = fileStat.st_size;
            fileInfos.push_back(fileInfo);
        }
    }
    closedir(dirp);
    return fileInfos;
}

bool StoreUtil::Rename(const std::string &oldName, const std::string &newName)
{
    if (oldName.empty() || newName.empty()) {
        return false;
    }
    if (!Remove(newName)) {
        return false;
    }
    if (rename(oldName.c_str(), newName.c_str()) != 0) {
        ZLOGE("rename error:%{public}d, file:%{public}s->%{public}s", errno, oldName.c_str(), newName.c_str());
        return false;
    }
    return true;
}

bool StoreUtil::IsFileExist(const std::string &name)
{
    if (name.empty()) {
        return false;
    }
    if (access(name.c_str(), F_OK) != 0) {
        return false;
    }
    return true;
}

bool StoreUtil::Remove(const std::string &path)
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

void StoreUtil::DoSync()
{
    sync();
}
} // namespace OHOS::DistributedKv