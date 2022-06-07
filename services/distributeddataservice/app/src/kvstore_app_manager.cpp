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

#define LOG_TAG "KvStoreAppManager"

#include "kvstore_app_manager.h"

#include <directory_ex.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread>

#include "account_delegate.h"
#include "broadcast_sender.h"
#include "checker/checker_manager.h"
#include "constant.h"
#include "device_kvstore_impl.h"
#include "directory_utils.h"
#include "kv_store_delegate.h"
#include "kvstore_app_accessor.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "permission_validator.h"
#include "reporter.h"
#include "route_head_handler_impl.h"
#include "types.h"

#define DEFAUL_RETRACT "        "

namespace OHOS {
namespace DistributedKv {
using namespace OHOS::DistributedData;
KvStoreAppManager::KvStoreAppManager(const std::string &bundleName, pid_t uid, uint32_t token)
    : bundleName_(bundleName), uid_(uid), token_(token), flowCtrl_(BURST_CAPACITY, SUSTAINED_CAPACITY)
{
    ZLOGI("begin.");
    deviceAccountId_ = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(uid);
    GetDelegateManager(PATH_DE);
    GetDelegateManager(PATH_CE);
}

KvStoreAppManager::~KvStoreAppManager()
{
    ZLOGD("begin.");
    {
        std::lock_guard<std::mutex> guard(delegateMutex_);
        delete delegateManagers_[PATH_DE];
        delete delegateManagers_[PATH_CE];
        delegateManagers_[PATH_DE] = nullptr;
        delegateManagers_[PATH_CE] = nullptr;
    }
}

Status KvStoreAppManager::ConvertErrorStatus(DistributedDB::DBStatus dbStatus, bool createIfMissing)
{
    if (dbStatus != DistributedDB::DBStatus::OK) {
        ZLOGE("delegate return error: %d.", static_cast<int>(dbStatus));
        switch (dbStatus) {
            case DistributedDB::DBStatus::INVALID_PASSWD_OR_CORRUPTED_DB:
                return Status::CRYPT_ERROR;
            case DistributedDB::DBStatus::SCHEMA_MISMATCH:
                return Status::SCHEMA_MISMATCH;
            case DistributedDB::DBStatus::INVALID_SCHEMA:
                return Status::INVALID_SCHEMA;
            case DistributedDB::DBStatus::NOT_SUPPORT:
                return Status::NOT_SUPPORT;
            case DistributedDB::DBStatus::EKEYREVOKED_ERROR: // fallthrough
            case DistributedDB::DBStatus::SECURITY_OPTION_CHECK_ERROR:
                return Status::SECURITY_LEVEL_ERROR;
            default:
                break;
        }
        if (createIfMissing) {
            return Status::DB_ERROR;
        } else {
            return Status::STORE_NOT_FOUND;
        }
    }
    return Status::SUCCESS;
}

Status KvStoreAppManager::GetKvStore(const Options &options, const StoreMetaData &metaData,
    const std::vector<uint8_t> &cipherKey, sptr<SingleKvStoreImpl> &kvStore)
{
    ZLOGI("begin");
    kvStore = nullptr;
    PathType type = ConvertPathType(metaData);
    auto *delegateManager = GetDelegateManager(type);
    if (delegateManager == nullptr) {
        ZLOGE("delegateManagers[%d] is nullptr.", type);
        return Status::ILLEGAL_STATE;
    }

    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    std::lock_guard<std::mutex> lg(storeMutex_);
    auto it = singleStores_[type].find(metaData.storeId);
    if (it != singleStores_[type].end()) {
        kvStore = it->second;
        ZLOGI("find store in map refcount: %d.", kvStore->GetSptrRefCount());
        kvStore->IncreaseOpenCount();
        return Status::SUCCESS;
    }

    if ((GetTotalKvStoreNum()) >= static_cast<size_t>(Constant::MAX_OPEN_KVSTORES)) {
        ZLOGE("limit %d KvStores can be opened.", Constant::MAX_OPEN_KVSTORES);
        return Status::ERROR;
    }

    DistributedDB::KvStoreNbDelegate::Option dbOption;
    auto status = InitNbDbOption(options, cipherKey, dbOption);
    if (status != Status::SUCCESS) {
        ZLOGE("InitNbDbOption failed.");
        return status;
    }

    DistributedDB::KvStoreNbDelegate *storeDelegate = nullptr;
    DistributedDB::DBStatus dbStatusTmp;
    delegateManager->GetKvStore(metaData.storeId, dbOption,
        [&](DistributedDB::DBStatus dbStatus, DistributedDB::KvStoreNbDelegate *kvStoreDelegate) {
            storeDelegate = kvStoreDelegate;
            dbStatusTmp = dbStatus;
        });

    if (storeDelegate == nullptr) {
        ZLOGE("storeDelegate is nullptr.");
        return ConvertErrorStatus(dbStatusTmp, options.createIfMissing);
    }
    std::string kvStorePath = GetDbDir(metaData);
    switch (options.kvStoreType) {
        case KvStoreType::DEVICE_COLLABORATION:
            kvStore = new (std::nothrow) DeviceKvStoreImpl(options, metaData.user, metaData.bundleName,
                metaData.storeId, metaData.appId, kvStorePath, storeDelegate);
            break;
        default:
            kvStore = new (std::nothrow) SingleKvStoreImpl(options, metaData.user, metaData.bundleName,
                metaData.storeId, metaData.appId, kvStorePath, storeDelegate);
            break;
    }
    if (kvStore == nullptr) {
        ZLOGE("store is nullptr.");
        delegateManager->CloseKvStore(storeDelegate);
        kvStore = nullptr;
        return Status::ERROR;
    }
    kvStore->SetCompatibleIdentify();
    auto result = singleStores_[type].emplace(metaData.storeId, kvStore);
    if (!result.second) {
        ZLOGE("emplace failed.");
        delegateManager->CloseKvStore(storeDelegate);
        kvStore = nullptr;
        return Status::ERROR;
    }

    ZLOGI("after emplace refcount: %d autoSync: %d", kvStore->GetSptrRefCount(), static_cast<int>(options.autoSync));
    if (options.autoSync) {
        bool autoSync = true;
        DistributedDB::PragmaData data = static_cast<DistributedDB::PragmaData>(&autoSync);
        auto pragmaStatus = storeDelegate->Pragma(DistributedDB::PragmaCmd::AUTO_SYNC, data);
        if (pragmaStatus != DistributedDB::DBStatus::OK) {
            ZLOGE("pragmaStatus: %d", static_cast<int>(pragmaStatus));
        }
    }

    DistributedDB::AutoLaunchOption launchOption = {
        options.createIfMissing, options.encrypt, dbOption.cipher, dbOption.passwd, dbOption.schema,
        dbOption.createDirByStoreIdOnly, kvStorePath, nullptr
    };
    launchOption.secOption = ConvertSecurity(options.securityLevel);
    AppAccessorParam accessorParam = {Constant::DEFAULT_GROUP_ID, trueAppId_, metaData.storeId, launchOption};
    KvStoreAppAccessor::GetInstance().EnableKvStoreAutoLaunch(accessorParam);
    return Status::SUCCESS;
}

Status KvStoreAppManager::CloseKvStore(const std::string &storeId)
{
    ZLOGI("CloseKvStore");
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }
    std::lock_guard<std::mutex> lg(storeMutex_);
    Status status = CloseKvStore(storeId, PATH_DE);
    if (status != Status::STORE_NOT_OPEN) {
        return status;
    }

    status = CloseKvStore(storeId, PATH_CE);
    if (status != Status::STORE_NOT_OPEN) {
        return status;
    }

    ZLOGW("store not open");
    return Status::STORE_NOT_OPEN;
}

Status KvStoreAppManager::CloseAllKvStore()
{
    ZLOGI("begin.");
    std::lock_guard<std::mutex> lg(storeMutex_);
    if (GetTotalKvStoreNum() == 0) {
        return Status::STORE_NOT_OPEN;
    }

    ZLOGI("close %zu KvStores.", GetTotalKvStoreNum());
    Status status = CloseAllKvStore(PATH_DE);
    if (status == Status::DB_ERROR) {
        return status;
    }
    status = CloseAllKvStore(PATH_CE);
    if (status == Status::DB_ERROR) {
        return status;
    }
    return Status::SUCCESS;
}

Status KvStoreAppManager::DeleteKvStore(const std::string &storeId)
{
    ZLOGI("%s", storeId.c_str());
    if (!flowCtrl_.IsTokenEnough()) {
        ZLOGE("flow control denied");
        return Status::EXCEED_MAX_ACCESS_RATE;
    }

    Status statusDE = DeleteKvStore(storeId, PATH_DE);
    Status statusCE = DeleteKvStore(storeId, PATH_CE);
    if (statusDE == Status::SUCCESS || statusCE == Status::SUCCESS) {
        return Status::SUCCESS;
    }

    ZLOGE("delegate close error.");
    return Status::DB_ERROR;
}

Status KvStoreAppManager::DeleteAllKvStore()
{
    ZLOGI("begin.");
    std::lock_guard<std::mutex> lg(storeMutex_);
    if (GetTotalKvStoreNum() == 0) {
        return Status::STORE_NOT_OPEN;
    }
    ZLOGI("delete %d KvStores.", int32_t(GetTotalKvStoreNum()));

    Status status = DeleteAllKvStore(PATH_DE);
    if (status != Status::SUCCESS) {
        ZLOGE("path de delegate delete error: %d.", static_cast<int>(status));
        return Status::DB_ERROR;
    }
    status = DeleteAllKvStore(PATH_CE);
    if (status != Status::SUCCESS) {
        ZLOGE("path ce delegate delete error: %d.", static_cast<int>(status));
        return Status::DB_ERROR;
    }
    return Status::SUCCESS;
}

Status KvStoreAppManager::InitNbDbOption(const Options &options, const std::vector<uint8_t> &cipherKey,
                                         DistributedDB::KvStoreNbDelegate::Option &dbOption)
{
    DistributedDB::CipherPassword password;
    auto status = password.SetValue(cipherKey.data(), cipherKey.size());
    if (status != DistributedDB::CipherPassword::ErrorCode::OK) {
        ZLOGE("Failed to set the passwd.");
        return Status::DB_ERROR;
    }

    dbOption.syncDualTupleMode = true; // tuple of (appid+storeid)
    dbOption.createIfNecessary = options.createIfMissing;
    dbOption.isMemoryDb = (!options.persistent);
    dbOption.isEncryptedDb = options.encrypt;
    if (options.encrypt) {
        dbOption.cipher = DistributedDB::CipherType::AES_256_GCM;
        dbOption.passwd = password;
    }

    if (options.kvStoreType == KvStoreType::SINGLE_VERSION) {
        dbOption.conflictResolvePolicy = DistributedDB::LAST_WIN;
    } else if (options.kvStoreType == KvStoreType::DEVICE_COLLABORATION) {
        dbOption.conflictResolvePolicy = DistributedDB::DEVICE_COLLABORATION;
    } else {
        ZLOGE("kvStoreType is invalid");
        return Status::INVALID_ARGUMENT;
    }

    dbOption.schema = options.schema;
    dbOption.createDirByStoreIdOnly = options.dataOwnership;
    dbOption.secOption = ConvertSecurity(options.securityLevel);
    return Status::SUCCESS;
}

std::string KvStoreAppManager::GetDbDir(const StoreMetaData &metaData)
{
    return GetDataStoragePath(metaData.user, metaData.bundleName, ConvertPathType(metaData));
}

KvStoreAppManager::PathType KvStoreAppManager::ConvertPathType(const StoreMetaData &metaData)
{
    switch (metaData.securityLevel) {
        case S0:    // fallthrough
        case S1:
            return PATH_DE;
        case S2:    // fallthrough
        case S3_EX: // fallthrough
        case S3:    // fallthrough
        case S4:    // fallthrough
            return PATH_CE;
        default:
            break;
    }
    CheckerManager::StoreInfo storeInfo;
    storeInfo.uid = metaData.uid;
    storeInfo.tokenId = metaData.tokenId;
    storeInfo.bundleName = metaData.bundleName;
    storeInfo.storeId = metaData.storeId;
    if (CheckerManager::GetInstance().GetAppId(storeInfo) != metaData.bundleName) {
        return PATH_CE;
    }
    return PATH_DE;
}

DistributedDB::KvStoreDelegateManager *KvStoreAppManager::GetDelegateManager(PathType type)
{
    std::lock_guard<std::mutex> guard(delegateMutex_);
    if (delegateManagers_[type] != nullptr) {
        return delegateManagers_[type];
    }

    std::string directory = GetDataStoragePath(deviceAccountId_, bundleName_, type);
    bool ret = ForceCreateDirectory(directory);
    if (!ret) {
        ZLOGE("create directory[%s] failed, errstr=[%d].", directory.c_str(), errno);
        return nullptr;
    }
    // change mode for directories to 0755, and for files to 0600.
    DirectoryUtils::ChangeModeDirOnly(directory, Constant::DEFAULT_MODE_DIR);
    DirectoryUtils::ChangeModeFileOnly(directory, Constant::DEFAULT_MODE_FILE);

    trueAppId_ = CheckerManager::GetInstance().GetAppId({ uid_, token_, bundleName_ });
    if (trueAppId_.empty()) {
        delegateManagers_[type] = nullptr;
        ZLOGW("check bundleName:%{public}s uid:%{public}d token:%{public}u failed.",
            bundleName_.c_str(), uid_, token_);
        return nullptr;
    }

    userId_ = AccountDelegate::GetInstance()->GetCurrentAccountId();
    ZLOGD("accountId: %{public}s bundleName: %{public}s", deviceAccountId_.c_str(), bundleName_.c_str());
    delegateManagers_[type] = new (std::nothrow) DistributedDB::KvStoreDelegateManager(trueAppId_, deviceAccountId_);
    if (delegateManagers_[type] == nullptr) {
        ZLOGE("delegateManagers_[%d] is nullptr.", type);
        return nullptr;
    }

    DistributedDB::KvStoreConfig kvStoreConfig;
    kvStoreConfig.dataDir = directory;
    delegateManagers_[type]->SetKvStoreConfig(kvStoreConfig);
    return delegateManagers_[type];
}

DistributedDB::KvStoreDelegateManager *KvStoreAppManager::SwitchDelegateManager(PathType type,
    DistributedDB::KvStoreDelegateManager *delegateManager)
{
    std::lock_guard<std::mutex> guard(delegateMutex_);
    DistributedDB::KvStoreDelegateManager *oldDelegateManager = delegateManagers_[type];
    delegateManagers_[type] = delegateManager;
    return oldDelegateManager;
}

Status KvStoreAppManager::CloseKvStore(const std::string &storeId, PathType type)
{
    auto *delegateManager = GetDelegateManager(type);
    if (delegateManager == nullptr) {
        ZLOGE("delegateManager[%d] is null.", type);
        return Status::ILLEGAL_STATE;
    }

    auto itSingle = singleStores_[type].find(storeId);
    if (itSingle != singleStores_[type].end()) {
        ZLOGD("find single store and close delegate.");
        InnerStatus status = itSingle->second->Close(delegateManager);
        if (status == InnerStatus::SUCCESS) {
            singleStores_[type].erase(itSingle);
            return Status::SUCCESS;
        }
        if (status == InnerStatus::DECREASE_REFCOUNT) {
            return Status::SUCCESS;
        }
        ZLOGE("delegate close error: %d.", static_cast<int>(status));
        return Status::DB_ERROR;
    }

    return Status::STORE_NOT_OPEN;
}

Status KvStoreAppManager::CloseAllKvStore(PathType type)
{
    auto *delegateManager = GetDelegateManager(type);
    if (delegateManager == nullptr) {
        ZLOGE("delegateManager[%d] is null.", type);
        return Status::ILLEGAL_STATE;
    }

    for (auto it = singleStores_[type].begin(); it != singleStores_[type].end(); it = singleStores_[type].erase(it)) {
        SingleKvStoreImpl *currentStore = it->second.GetRefPtr();
        ZLOGI("close kvstore, refcount %d.", it->second->GetSptrRefCount());
        Status status = currentStore->ForceClose(delegateManager);
        if (status != Status::SUCCESS) {
            ZLOGE("delegate close error: %d.", static_cast<int>(status));
            return Status::DB_ERROR;
        }
    }
    singleStores_[type].clear();
    return Status::SUCCESS;
}

Status KvStoreAppManager::DeleteKvStore(const std::string &storeId, PathType type)
{
    auto *delegateManager = GetDelegateManager(type);
    if (delegateManager == nullptr) {
        ZLOGE("delegateManager[%d] is null.", type);
        return Status::ILLEGAL_STATE;
    }
    std::lock_guard<std::mutex> lg(storeMutex_);
    auto itSingle = singleStores_[type].find(storeId);
    if (itSingle != singleStores_[type].end()) {
        Status status = itSingle->second->ForceClose(delegateManager);
        if (status != Status::SUCCESS) {
            return Status::DB_ERROR;
        }
        singleStores_[type].erase(itSingle);
    }

    DistributedDB::DBStatus status = delegateManager->DeleteKvStore(storeId);
    if (singleStores_[type].empty()) {
        SwitchDelegateManager(type, nullptr);
        delete delegateManager;
    }
    return (status != DistributedDB::DBStatus::OK) ? Status::DB_ERROR : Status::SUCCESS;
}

Status KvStoreAppManager::DeleteAllKvStore(PathType type)
{
    auto *delegateManager = GetDelegateManager(type);
    if (delegateManager == nullptr) {
        ZLOGE("delegateManager[%d] is null.", type);
        return Status::ILLEGAL_STATE;
    }

    for (auto it = singleStores_[type].begin(); it != singleStores_[type].end(); it = singleStores_[type].erase(it)) {
        std::string storeId = it->first;
        SingleKvStoreImpl *currentStore = it->second.GetRefPtr();
        Status status = currentStore->ForceClose(delegateManager);
        if (status != Status::SUCCESS) {
            ZLOGE("delegate delete close failed error: %d.", static_cast<int>(status));
            return Status::DB_ERROR;
        }

        ZLOGI("close kvstore, refcount %d.", it->second->GetSptrRefCount());
        DistributedDB::DBStatus dbStatus = delegateManager->DeleteKvStore(storeId);
        if (dbStatus != DistributedDB::DBStatus::OK) {
            ZLOGE("delegate delete error: %d.", static_cast<int>(dbStatus));
            return Status::DB_ERROR;
        }
    }
    singleStores_[type].clear();
    SwitchDelegateManager(type, nullptr);
    delete delegateManager;
    return Status::SUCCESS;
}

size_t KvStoreAppManager::GetTotalKvStoreNum() const
{
    size_t total = singleStores_[PATH_DE].size();
    total += singleStores_[PATH_CE].size();
    return int(total);
};

std::string KvStoreAppManager::GetDataStoragePath(const std::string &userId, const std::string &bundleName,
                                                  PathType type)
{
    std::string miscPath = (type == PATH_DE) ? Constant::ROOT_PATH_DE : Constant::ROOT_PATH_CE;
    return Constant::Concatenate(
        {miscPath, "/", Constant::SERVICE_NAME, "/", userId, "/", Constant::GetDefaultHarmonyAccountName(),
        "/", bundleName, "/"});
}

DistributedDB::SecurityOption KvStoreAppManager::ConvertSecurity(int securityLevel)
{
    if (securityLevel < SecurityLevel::NO_LABEL || securityLevel > SecurityLevel::S4) {
        return {DistributedDB::NOT_SET, DistributedDB::ECE};
    }
    switch (securityLevel) {
        case SecurityLevel::S3:
            return {DistributedDB::S3, DistributedDB::SECE};
        case SecurityLevel::S4:
            return {DistributedDB::S4, DistributedDB::ECE};
        default:
            return {securityLevel, DistributedDB::ECE};
    }
}

void KvStoreAppManager::Dump(int fd) const
{
    size_t singleStoreNum = singleStores_[PATH_DE].size() + singleStores_[PATH_CE].size();
    dprintf(fd, DEFAUL_RETRACT"----------------------------------------------------------\n");
    dprintf(fd, DEFAUL_RETRACT"AppID         : %s\n", trueAppId_.c_str());
    dprintf(fd, DEFAUL_RETRACT"BundleName    : %s\n", bundleName_.c_str());
    dprintf(fd, DEFAUL_RETRACT"Store count   : %u\n", static_cast<uint32_t>(singleStoreNum));
    for (const auto &singleStoreMap : singleStores_) {
        for (const auto &pair : singleStoreMap) {
            pair.second->OnDump(fd);
        }
    }
}

void KvStoreAppManager::DumpUserInfo(int fd) const
{
    dprintf(fd, DEFAUL_RETRACT"----------------------------------------------------------\n");
    dprintf(fd, DEFAUL_RETRACT"AppID         : %s\n", trueAppId_.c_str());
}

void KvStoreAppManager::DumpAppInfo(int fd) const
{
    size_t singleStoreNum = singleStores_[PATH_DE].size() + singleStores_[PATH_CE].size();
    dprintf(fd, DEFAUL_RETRACT"----------------------------------------------------------\n");
    dprintf(fd, DEFAUL_RETRACT"AppID         : %s\n", trueAppId_.c_str());
    dprintf(fd, DEFAUL_RETRACT"BundleName    : %s\n", bundleName_.c_str());
    dprintf(fd, DEFAUL_RETRACT"Store count   : %u\n", static_cast<uint32_t>(singleStoreNum));
    for (const auto &singleStoreMap : singleStores_) {
        for (const auto &pair : singleStoreMap) {
            pair.second->DumpStoreName(fd);
        }
    }
}

void KvStoreAppManager::DumpStoreInfo(int fd, const std::string &storeId) const
{
    if (storeId != "") {
        for (const auto &singleStoreMap : singleStores_) {
            auto it = singleStoreMap.find(storeId);
            if (it != singleStoreMap.end()) {
                dprintf(fd, DEFAUL_RETRACT"----------------------------------------------------------\n");
                dprintf(fd, DEFAUL_RETRACT"AppID         : %s\n", trueAppId_.c_str());
                it->second->OnDump(fd);
            }
            return;
        }
    }

    dprintf(fd, DEFAUL_RETRACT"AppID         : %s\n", trueAppId_.c_str());
    for (const auto &singleStoreMap : singleStores_) {
        for (const auto &pair : singleStoreMap) {
            pair.second->OnDump(fd);
        }
    }
}

bool KvStoreAppManager::IsStoreOpened(const std::string &storeId) const
{
    return (!singleStores_[PATH_DE].empty() && singleStores_->find(storeId) != singleStores_->end())
           || (!singleStores_[PATH_CE].empty() && singleStores_->find(storeId) != singleStores_->end());
}

void KvStoreAppManager::SetCompatibleIdentify(const std::string &deviceId) const
{
    for (const auto &storeType : singleStores_) {
        for (const auto &item : storeType) {
            if (item.second == nullptr) {
                continue;
            }
            item.second->SetCompatibleIdentify(deviceId);
        }
    }
}
}  // namespace DistributedKv
}  // namespace OHOS
