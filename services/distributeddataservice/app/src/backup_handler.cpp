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
constexpr const char *UPG_PREFIX = "Upgrade_";
BackupHandler::BackupHandler(IKvStoreDataService *kvStoreDataService)
{
}

BackupHandler::BackupHandler()
{
}
BackupHandler::~BackupHandler()
{
}
void BackupHandler::Initialize()
{
    uint32_t version = KvStoreDataService::STORE_VERSION;
    std::string user = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(getuid());
    std::string backupPath = GetBackupPath(user, KvStoreAppManager::PATH_DE);
    ZLOGI("backupPath is : %{public}s ", backupPath.c_str());
    if (!ForceCreateDirectory(backupPath)) {
        ZLOGE("backup create directory failed");
    }

    Upgrade::GetInstance().RegisterExporter(version, [](const StoreMeta &meta, DBPassword &pwd) {
        if (meta.isEncrypt) {
            std::string keyFile = KvStoreMetaManager::GetSecretSingleKeyFile(meta.user, meta.bundleName, meta.storeId,
                KvStoreAppManager::ConvertPathType(meta));
            auto password = KvStoreMetaManager::GetInstance().GetSecretKeyFromFile(keyFile);
            pwd.SetValue(password.data(), password.size());
            std::fill(password.begin(), password.end(), 0);
        }
        BackupPara backupPara;
        auto inited = InitBackupPara(meta, backupPara, UPG_PREFIX);
        if (!inited || FileExists(backupPara.backupFullName)) {
            return backupPara.backupFullName;
        }

        backupPara.password = pwd;
        SingleKvStoreBackup(meta, backupPara);
        return backupPara.backupFullName;
    });
    Upgrade::GetInstance().RegisterCleaner(version, [](const StoreMeta &meta) -> Status {
        DBManager manager(meta.appId, meta.user);
        manager.SetKvStoreConfig({ meta.dataDir });
        manager.DeleteKvStore(meta.storeId);

        // delete the backup file
        auto backFilePath = GetBackupPath(meta.user, KvStoreAppManager::ConvertPathType(meta));
        auto backupFileName = Constant::Concatenate({ meta.account, "_", meta.bundleName, "_", meta.storeId });
        backupFileName = GetHashedBackupName(backupFileName);
        auto backFile = Constant::Concatenate({ backFilePath, "/", backupFileName });
        auto upgFile = Constant::Concatenate({ backFilePath, "/", UPG_PREFIX + backupFileName });
        RemoveFile(backFile);
        RemoveFile(upgFile);
        if (!meta.isEncrypt) {
            return SUCCESS;
        }

        auto keyFile = KvStoreMetaManager::GetSecretSingleKeyFile(meta.user, meta.bundleName, meta.storeId,
            KvStoreAppManager::ConvertPathType(meta));
        RemoveFile(backFile);
        MetaDataManager::GetInstance().DelMeta(meta.GetSecretKey(), true);
        return SUCCESS;
    });
}

void BackupHandler::BackSchedule()
{
    std::chrono::duration<int> delay(1800);    // delay 30 minutes
    std::chrono::duration<int> internal(1800); // duration is 30 minutes
    ZLOGI("BackupHandler Schedule start.");
    scheduler_.Every(delay, internal, [&]() {
        if (!CheckNeedBackup()) {
            ZLOGE("it is not meet the condition of backup.");
            return;
        }
        auto device = CommunicationProvider::GetInstance().GetLocalDevice();
        if (device.uuid.empty()) {
            ZLOGE("local uuid is empty!");
            return;
        }

        std::vector<StoreMetaData> results;
        ZLOGI("BackupHandler Schedule Every start.");
        if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ device.uuid }), results)) {
            ZLOGE("GetFullMetaData failed.");
            return;
        }

        for (auto &entry : results) {
            if (!entry.isBackup || entry.isDirty) {
                continue;
            }

            auto type = entry.storeType;
            if (type == KvStoreType::SINGLE_VERSION) {
                DoBackup(entry);
            }
        }
        backupSuccessTime_ = TimeUtils::CurrentTimeMicros();
    });
}

void BackupHandler::DoBackup(const StoreMetaData &metaData)
{
    BackupPara backupPara;
    auto initPara = InitBackupPara(metaData, backupPara);
    if (!initPara) {
        return;
    }
    SingleKvStoreBackup(metaData, backupPara);
}

void BackupHandler::SingleKvStoreBackup(const StoreMetaData &metaData, const BackupPara &backupPara)
{
    ZLOGI("SingleKvStoreBackup start.");
    DistributedDB::KvStoreNbDelegate::Option dbOption;
    SetDBOptions(dbOption, backupPara, metaData);
    DistributedDB::KvStoreDelegateManager manager(metaData.appId, metaData.user);
    DistributedDB::KvStoreConfig kvStoreConfig = { metaData.dataDir };
    manager.SetKvStoreConfig(kvStoreConfig);
    auto fun = [&](DistributedDB::DBStatus status, DistributedDB::KvStoreNbDelegate *delegate) {
        if (delegate == nullptr) {
            ZLOGE("SingleKvStoreBackup delegate is null");
            return;
        }
        if (metaData.isAutoSync) {
            bool autoSync = true;
            DistributedDB::PragmaData data = static_cast<DistributedDB::PragmaData>(&autoSync);
            auto pragmaStatus = delegate->Pragma(DistributedDB::PragmaCmd::AUTO_SYNC, data);
            if (pragmaStatus != DistributedDB::DBStatus::OK) {
                ZLOGE("pragmaStatus: %d", static_cast<int>(pragmaStatus));
            }
        }
        ZLOGW("SingleKvStoreBackup export");
        if (status == DistributedDB::DBStatus::OK) {
            auto backupFullName = backupPara.backupFullName;
            auto backupBackFullName = backupPara.backupBackFullName;
            RenameFile(backupFullName, backupBackFullName);
            status = delegate->Export(backupFullName, dbOption.passwd);
            if (status == DistributedDB::DBStatus::OK) {
                ZLOGD("SingleKvStoreBackup export success.");
                RemoveFile(backupBackFullName);
            } else {
                ZLOGE("SingleKvStoreBackup export failed, status is %d.", status);
                RenameFile(backupBackFullName, backupFullName);
            }
            std::string message;
            message.append(" backup name [")
                .append(backupFullName)
                .append("], isEncryptedDb [")
                .append(std::to_string(dbOption.isEncryptedDb))
                .append("]")
                .append("], backup result status [")
                .append(std::to_string(status))
                .append("]");
            Reporter::GetInstance()->BehaviourReporter()->Report(
                { metaData.account, metaData.appId, metaData.storeId, BehaviourType::DATABASE_BACKUP, message });
        }
        manager.CloseKvStore(delegate);
    };
    manager.GetKvStore(metaData.storeId, dbOption, fun);
}

void BackupHandler::SetDBOptions(DBOption &dbOption, const BackupPara &backupPara, const StoreMetaData &meta)
{
    dbOption.syncDualTupleMode = true;
    dbOption.createIfNecessary = false;
    dbOption.isEncryptedDb = backupPara.password.GetSize() > 0;
    dbOption.passwd = backupPara.password;
    dbOption.createDirByStoreIdOnly = true;
    dbOption.secOption = KvStoreAppManager::ConvertSecurity(meta.securityLevel);
}

bool BackupHandler::InitBackupPara(const StoreMetaData &metaData, BackupPara &backupPara, const std::string &prefix)
{
    auto pathType = KvStoreAppManager::ConvertPathType(metaData);
    if (!ForceCreateDirectory(BackupHandler::GetBackupPath(metaData.user, pathType))) {
        ZLOGE("create directory failed[%{public}d].", errno);
        return false;
    }

    std::initializer_list<std::string> backList = { metaData.account, "_", metaData.appId, "_", metaData.storeId };
    std::string backupName = Constant::Concatenate(backList);
    std::initializer_list<std::string> backFullList = { BackupHandler::GetBackupPath(metaData.user, pathType), "/",
        prefix + GetHashedBackupName(backupName) };
    auto backupFullName = Constant::Concatenate(backFullList);
    std::initializer_list<std::string> backNameList = { backupFullName, ".", "backup" };
    auto backupBackFullName = Constant::Concatenate(backNameList);

    backupPara.pathType = pathType;
    backupPara.backupFullName = backupFullName;
    backupPara.backupBackFullName = backupBackFullName;
    backupPara.password = {};
    if (metaData.isEncrypt) {
        GetPassword(metaData, backupPara.password);
    }
    return true;
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

    int64_t currentTime = TimeUtils::CurrentTimeMicros();
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

bool BackupHandler::RenameFile(const std::string &oldPath, const std::string &newPath)
{
    if (oldPath.empty() || newPath.empty()) {
        ZLOGE("RenameFile failed: path is empty");
        return false;
    }
    if (!RemoveFile(newPath)) {
        ZLOGE("RenameFile failed: newPath file is already exist");
        return false;
    }
    if (rename(oldPath.c_str(), newPath.c_str()) != 0) {
        ZLOGE("RenameFile: rename error, errno[%d].", errno);
        return false;
    }
    return true;
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

bool BackupHandler::CheckNeedBackup()
{
#ifdef SUPPORT_POWER
    auto &batterySrvClient = PowerMgr::BatterySrvClient::GetInstance();
    auto chargingStatus = batterySrvClient.GetChargingStatus();
    if (chargingStatus != PowerMgr::BatteryChargeState::CHARGE_STATE_ENABLE) {
        if (chargingStatus != PowerMgr::BatteryChargeState::CHARGE_STATE_FULL) {
            ZLOGE("the device is not in charge state, chargingStatus=%{public}d.", chargingStatus);
            return false;
        }
        auto batteryPluggedType = batterySrvClient.GetPluggedType();
        if (batteryPluggedType != PowerMgr::BatteryPluggedType::PLUGGED_TYPE_AC &&
            batteryPluggedType != PowerMgr::BatteryPluggedType::PLUGGED_TYPE_USB &&
            batteryPluggedType != PowerMgr::BatteryPluggedType::PLUGGED_TYPE_WIRELESS) {
            ZLOGE("the device is not in charge full statue, the batteryPluggedType is %{public}d.", batteryPluggedType);
            return false;
        }
    }
    auto &powerMgrClient = PowerMgr::PowerMgrClient::GetInstance();
    if (powerMgrClient.IsScreenOn()) {
        ZLOGE("the device screen is on.");
        return false;
    }
    uint64_t currentTime = TimeUtils::CurrentTimeMicros();
    if (currentTime - backupSuccessTime_ < BACKUP_INTERVAL && backupSuccessTime_ > 0) {
        ZLOGE("no more than 10 hours since the last backup success.");
        return false;
    }
#endif
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
