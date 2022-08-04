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
#define LOG_TAG "BackupHandler"
#include "backup_handler.h"

#include <directory_ex.h>
#include <fcntl.h>
#include <unistd.h>

#include "account_delegate.h"
#ifdef SUPPORT_POWER
#include "battery_info.h"
#include "battery_srv_client.h"
#include "power_mgr_client.h"
#endif
#include "communication_provider.h"
#include "constant.h"
#include "crypto_manager.h"
#include "kv_scheduler.h"
#include "kv_store_delegate_manager.h"
#include "kvstore_data_service.h"
#include "kvstore_meta_manager.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "time_utils.h"
#include "upgrade.h"
#include "utils/crypto.h"
namespace OHOS::DistributedKv {
using namespace DistributedData;
using namespace DistributedDataDfx;
using namespace AppDistributedKv;
using DBPassword = DistributedDB::CipherPassword;
using DBStatus = DistributedDB::DBStatus;
using DBStore = DistributedDB::KvStoreNbDelegate;
using DBManager = DistributedDB::KvStoreDelegateManager;
using StoreMeta = DistributedData::StoreMetaData;

constexpr const int64_t NANOSEC_TO_MICROSEC = 1000;

BackupHandler::BackupHandler()
{
}
BackupHandler::~BackupHandler()
{
}

bool BackupHandler::GetPassword(const StoreMetaData &metaData, DistributedDB::CipherPassword &password)
{
    if (!metaData.isEncrypt) {
        return true;
    }

    std::string key = metaData.GetSecretKey();
    SecretKeyMetaData secretKey;
    MetaDataManager::GetInstance().LoadMeta(key, secretKey, true);
    std::vector<uint8_t> decryptKey;
    CryptoManager::GetInstance().Decrypt(secretKey.sKey, decryptKey);
    if (password.SetValue(decryptKey.data(), decryptKey.size()) != DistributedDB::CipherPassword::OK) {
        std::fill(decryptKey.begin(), decryptKey.end(), 0);
        ZLOGE("Set secret key value failed. len is (%d)", int32_t(decryptKey.size()));
        return false;
    }
    std::fill(decryptKey.begin(), decryptKey.end(), 0);
    return true;
}

bool BackupHandler::SingleKvStoreRecover(StoreMetaData &metaData, DistributedDB::KvStoreNbDelegate *delegate)
{
    ZLOGI("start.");
    if (delegate == nullptr) {
        ZLOGE("SingleKvStoreRecover failed, delegate is null.");
        return false;
    }
    auto pathType = KvStoreAppManager::ConvertPathType(metaData);
    if (!BackupHandler::FileExists(BackupHandler::GetBackupPath(metaData.user, pathType))) {
        ZLOGE("SingleKvStoreRecover failed, backupDir_ file is not exist.");
        return false;
    }

    DistributedDB::CipherPassword password;
    if (!GetPassword(metaData, password)) {
        ZLOGE("Set secret key failed.");
        return false;
    }

    std::string backupName = Constant::Concatenate({ metaData.account, "_", metaData.appId, "_", metaData.storeId });
    auto backupFullName = Constant::Concatenate(
        { BackupHandler::GetBackupPath(metaData.user, pathType), "/", GetHashedBackupName(backupName) });
    DistributedDB::DBStatus dbStatus = delegate->Import(backupFullName, password);

    uint64_t currentTime = TimeUtils::CurrentTimeMicros();
    int64_t backupTime = GetBackupTime(backupFullName);
    std::string message;
    message.append(" backup name [").append(backupFullName)
        .append("], backup time [").append(std::to_string(backupTime))
        .append("], recovery time [").append(std::to_string(currentTime))
        .append("], recovery result status [").append(std::to_string(dbStatus)).append("]");
    Reporter::GetInstance()->BehaviourReporter()->Report(
        {metaData.account, metaData.appId, metaData.storeId, BehaviourType::DATABASE_RECOVERY, message});
    if (dbStatus == DistributedDB::DBStatus::OK) {
        ZLOGI("SingleKvStoreRecover success.");
        return true;
    }
    ZLOGI("SingleKvStoreRecover failed.");
    return false;
}

int64_t BackupHandler::GetBackupTime(std::string &fullName)
{
    struct stat curStat;
    stat(fullName.c_str(), &curStat);
    return curStat.st_mtim.tv_sec * SEC_TO_MICROSEC + curStat.st_mtim.tv_nsec / NANOSEC_TO_MICROSEC;
}

std::string BackupHandler::backupDirCe_;
std::string BackupHandler::backupDirDe_;
const std::string &BackupHandler::GetBackupPath(const std::string &deviceAccountId, int pathType)
{
    if (pathType == KvStoreAppManager::PATH_DE) {
        if (backupDirDe_.empty()) {
            backupDirDe_ = Constant::Concatenate({Constant::ROOT_PATH_DE, "/", Constant::SERVICE_NAME, "/",
                                                  deviceAccountId, "/", Constant::GetDefaultHarmonyAccountName(),
                                                  "/", "backup"});
        }
        return backupDirDe_;
    } else {
        if (backupDirCe_.empty()) {
            backupDirCe_ = Constant::Concatenate({Constant::ROOT_PATH_CE, "/", Constant::SERVICE_NAME, "/",
                                                  deviceAccountId, "/", Constant::GetDefaultHarmonyAccountName(),
                                                  "/", "backup"});
        }
        return backupDirCe_;
    }
}

bool BackupHandler::RemoveFile(const std::string &path)
{
    if (path.empty()) {
        ZLOGI("path is empty");
        return true;
    }

    if (unlink(path.c_str()) != 0 && (errno != ENOENT)) {
        ZLOGE("failed errno[%{public}d] path:%{public}s ", errno, path.c_str());
        return false;
    }
    return true;
}

bool BackupHandler::FileExists(const std::string &path)
{
    if (path.empty()) {
        ZLOGI("path is empty");
        return false;
    }

    if (access(path.c_str(), F_OK) != 0) {
        ZLOGI("file%{public}s is not exist", path.c_str());
        return false;
    }
    return true;
}

std::string BackupHandler::GetHashedBackupName(const std::string &bundleName)
{
    if (bundleName.empty()) {
        return bundleName;
    }
    return DistributedData::Crypto::Sha256(bundleName);
}
} // namespace OHOS::DistributedKv
