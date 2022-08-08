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

#define LOG_TAG "KvStoreDataService"

#include "kvstore_data_service.h"

#include <directory_ex.h>
#include <file_ex.h>
#include <ipc_skeleton.h>
#include <unistd.h>

#include <chrono>
#include <thread>

#include "accesstoken_kit.h"
#include "auth_delegate.h"
#include "auto_launch_export.h"
#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "communication_provider.h"
#include "config_factory.h"
#include "constant.h"
#include "dds_trace.h"
#include "device_kvstore_impl.h"
#include "executor_factory.h"
#include "hap_token_info.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "kvdb_service_impl.h"
#include "kvstore_account_observer.h"
#include "kvstore_app_accessor.h"
#include "kvstore_device_listener.h"
#include "kvstore_meta_manager.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/strategy_meta_data.h"
#include "object_common.h"
#include "object_manager.h"
#include "object_service_impl.h"
#include "permission_validator.h"
#include "process_communicator_impl.h"
#include "rdb_service_impl.h"
#include "route_head_handler_impl.h"
#include "system_ability_definition.h"
#include "uninstaller/uninstaller.h"
#include "upgrade_manager.h"
#include "user_delegate.h"
#include "utils/block_integer.h"
#include "utils/converter.h"
#include "string_ex.h"
#include "permit_delegate.h"
#include "utils/crypto.h"
#include "runtime_config.h"

namespace OHOS::DistributedKv {
using namespace std::chrono;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedDataDfx;
using namespace OHOS::Security::AccessToken;
using KvStoreDelegateManager = DistributedDB::KvStoreDelegateManager;
using SecretKeyMeta = DistributedData::SecretKeyMetaData;
using StrategyMetaData = DistributedData::StrategyMeta;

REGISTER_SYSTEM_ABILITY_BY_ID(KvStoreDataService, DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID, true);

constexpr size_t MAX_APP_ID_LENGTH = 256;

KvStoreDataService::KvStoreDataService(bool runOnCreate)
    : SystemAbility(runOnCreate),
      accountMutex_(),
      deviceAccountMap_(),
      clientDeathObserverMutex_(),
      clientDeathObserverMap_()
{
    ZLOGI("begin.");
}

KvStoreDataService::KvStoreDataService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate),
      accountMutex_(),
      deviceAccountMap_(),
      clientDeathObserverMutex_(),
      clientDeathObserverMap_()
{
    ZLOGI("begin");
}

KvStoreDataService::~KvStoreDataService()
{
    ZLOGI("begin.");
    deviceAccountMap_.clear();
    clientDeathObserverMap_.clear();
    deviceListeners_.clear();
}

void KvStoreDataService::Initialize()
{
    ZLOGI("begin.");
#ifndef UT_TEST
    KvStoreDelegateManager::SetProcessLabel(Bootstrap::GetInstance().GetProcessLabel(), "default");
#endif
    auto communicator = std::make_shared<AppDistributedKv::ProcessCommunicatorImpl>(RouteHeadHandlerImpl::Create);
    auto ret = KvStoreDelegateManager::SetProcessCommunicator(communicator);
    ZLOGI("set communicator ret:%{public}d.", static_cast<int>(ret));

    PermitDelegate::GetInstance().Init();
    InitSecurityAdapter();
    KvStoreMetaManager::GetInstance().InitMetaParameter();
    accountEventObserver_ = std::make_shared<KvStoreAccountObserver>(*this);
    AccountDelegate::GetInstance()->Subscribe(accountEventObserver_);
    deviceInnerListener_ = std::make_unique<KvStoreDeviceListener>(*this);
    AppDistributedKv::CommunicationProvider::GetInstance().StartWatchDeviceChange(
        deviceInnerListener_.get(), { "innerListener" });
}

sptr<IRemoteObject> KvStoreDataService::GetObjectService()
{
    ZLOGD("enter");
    if (objectService_ == nullptr) {
        std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
        if (objectService_ == nullptr) {
            objectService_ = new (std::nothrow) DistributedObject::ObjectServiceImpl();
        }
        return objectService_ == nullptr ? nullptr : objectService_->AsObject().GetRefPtr();
    }
    return objectService_->AsObject().GetRefPtr();
}

void KvStoreDataService::InitObjectStore()
{
    ZLOGI("begin.");
    if (objectService_ == nullptr) {
        std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
        if (objectService_ == nullptr) {
            objectService_ = new (std::nothrow) DistributedObject::ObjectServiceImpl();
        }
    }
    objectService_->Initialize();
    return;
}
Status KvStoreDataService::GetSingleKvStore(const Options &options, const AppId &appId, const StoreId &storeId,
                                            std::function<void(sptr<ISingleKvStore>)> callback)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    ZLOGI("begin.");
    KVSTORE_ACCOUNT_EVENT_PROCESSING_CHECKER(Status::SYSTEM_ACCOUNT_EVENT_PROCESSING);
    StoreMetaData metaData;
    Status status = FillStoreParam(options, appId, storeId, metaData);
    if (status != Status::SUCCESS) {
        callback(nullptr);
        return status;
    }

    SecretKeyPara keyPara;
    status = KvStoreDataService::GetSecretKey(options, metaData, keyPara);
    if (status != Status::SUCCESS) {
        callback(nullptr);
        return status;
    }

    std::lock_guard<std::mutex> lg(accountMutex_);
    auto it = deviceAccountMap_.find(metaData.user);
    if (it == deviceAccountMap_.end()) {
        auto result = deviceAccountMap_.emplace(std::piecewise_construct,
            std::forward_as_tuple(metaData.user), std::forward_as_tuple(metaData.user));
        if (!result.second) {
            ZLOGE("emplace failed.");
            callback(nullptr);
            return Status::ERROR;
        }
        it = result.first;
    }
    sptr<SingleKvStoreImpl> store;
    status = (it->second).GetKvStore(options, metaData, keyPara.secretKey, store);
    if (keyPara.outdated) {
        KvStoreMetaManager::GetInstance().ReKey(metaData.user, metaData.bundleName, metaData.storeId,
            KvStoreAppManager::ConvertPathType(metaData), store);
    }
    if (status == Status::SUCCESS) {
        status = UpdateMetaData(options, metaData);
        if (status != Status::SUCCESS) {
            ZLOGE("failed to write meta");
            callback(nullptr);
            return status;
        }
        callback(std::move(store));
        return status;
    }

    status =  GetSingleKvStoreFailDo(status, options, metaData, keyPara, it->second, store);
    callback(std::move(store));
    return status;
}

Status KvStoreDataService::FillStoreParam(
    const Options &options, const AppId &appId, const StoreId &storeId, StoreMetaData &metaData)
{
    if (!appId.IsValid() || !storeId.IsValid() || !options.IsValidType()) {
        ZLOGE("invalid argument type.");
        return Status::INVALID_ARGUMENT;
    }
    metaData.version = STORE_VERSION;
    metaData.securityLevel = options.securityLevel;
    metaData.storeType = options.kvStoreType;
    metaData.isBackup = options.backup;
    metaData.isEncrypt = options.encrypt;
    metaData.isAutoSync = options.autoSync;
    metaData.bundleName = appId.appId;
    metaData.storeId = storeId.storeId;
    metaData.uid = IPCSkeleton::GetCallingUid();
    metaData.tokenId = IPCSkeleton::GetCallingTokenID();
    metaData.appId = CheckerManager::GetInstance().GetAppId(Converter::ConvertToStoreInfo(metaData));
    ZLOGI("%{public}s, %{public}s", metaData.appId.c_str(), metaData.bundleName.c_str());
    if (metaData.appId.empty()) {
        ZLOGW("appId:%{public}s, uid:%{public}d, PERMISSION_DENIED", appId.appId.c_str(), metaData.uid);
        return PERMISSION_DENIED;
    }

    metaData.user = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(metaData.uid);
    metaData.account = AccountDelegate::GetInstance()->GetCurrentAccountId();
    return SUCCESS;
}

Status KvStoreDataService::GetSecretKey(const Options &options, const StoreMetaData &metaData,
    SecretKeyPara &secretKeyParas)
{
    std::string bundleName = metaData.bundleName;
    std::string storeIdTmp = metaData.storeId;
    std::lock_guard<std::mutex> lg(accountMutex_);
    auto metaKey = KvStoreMetaManager::GetMetaKey(metaData.user, "default", bundleName, storeIdTmp);
    if (!CheckOptions(options, metaKey)) {
        ZLOGE("encrypt type or kvStore type is not the same");
        return Status::INVALID_ARGUMENT;
    }
    std::vector<uint8_t> secretKey;
    std::unique_ptr<std::vector<uint8_t>, void (*)(std::vector<uint8_t> *)> cleanGuard(
        &secretKey, [](std::vector<uint8_t> *ptr) { ptr->assign(ptr->size(), 0); });

    std::vector<uint8_t> metaSecretKey;
    std::string secretKeyFile;
    metaSecretKey = KvStoreMetaManager::GetMetaKey(metaData.user, "default", bundleName, storeIdTmp, "SINGLE_KEY");
    secretKeyFile = KvStoreMetaManager::GetSecretSingleKeyFile(
        metaData.user, bundleName, storeIdTmp, KvStoreAppManager::ConvertPathType(metaData));

    bool outdated = false;
    Status alreadyCreated = KvStoreMetaManager::GetInstance().CheckUpdateServiceMeta(metaSecretKey, CHECK_EXIST_LOCAL);
    if (options.encrypt) {
        ZLOGI("Getting secret key");
        Status recStatus = RecoverSecretKey(alreadyCreated, outdated, metaSecretKey, secretKey, secretKeyFile);
        if (recStatus != Status::SUCCESS) {
            return recStatus;
        }
    } else {
        if (alreadyCreated == Status::SUCCESS || FileExists(secretKeyFile)) {
            ZLOGW("try to get an encrypted store with false option encrypt parameter");
            return Status::CRYPT_ERROR;
        }
    }

    SecretKeyPara kvStoreSecretKey;
    kvStoreSecretKey.metaKey = metaKey;
    kvStoreSecretKey.secretKey = secretKey;
    kvStoreSecretKey.metaSecretKey = metaSecretKey;
    kvStoreSecretKey.secretKeyFile = secretKeyFile;
    kvStoreSecretKey.alreadyCreated = alreadyCreated;
    kvStoreSecretKey.outdated = outdated;
    secretKeyParas = kvStoreSecretKey;
    return Status::SUCCESS;
}

Status KvStoreDataService::RecoverSecretKey(const Status &alreadyCreated, bool &outdated,
    const std::vector<uint8_t> &metaSecretKey, std::vector<uint8_t> &secretKey, const std::string &secretKeyFile)
{
    if (alreadyCreated != Status::SUCCESS) {
        KvStoreMetaManager::GetInstance().RecoverSecretKeyFromFile(
            secretKeyFile, metaSecretKey, secretKey, outdated);
        if (secretKey.empty()) {
            ZLOGI("new secret key");
            secretKey = Crypto::Random(32); // 32 is key length
            KvStoreMetaManager::GetInstance().WriteSecretKeyToMeta(metaSecretKey, secretKey);
            KvStoreMetaManager::GetInstance().WriteSecretKeyToFile(secretKeyFile, secretKey);
        }
    } else {
        KvStoreMetaManager::GetInstance().GetSecretKeyFromMeta(metaSecretKey, secretKey, outdated);
        if (secretKey.empty()) {
            ZLOGW("get secret key from meta failed, try to recover");
            KvStoreMetaManager::GetInstance().RecoverSecretKeyFromFile(
                secretKeyFile, metaSecretKey, secretKey, outdated);
        }
        if (secretKey.empty()) {
            ZLOGW("recover failed");
            return Status::CRYPT_ERROR;
        }
        KvStoreMetaManager::GetInstance().WriteSecretKeyToFile(secretKeyFile, secretKey);
    }
    return Status::SUCCESS;
}

Status KvStoreDataService::UpdateMetaData(const Options &options, const StoreMetaData &metaData)
{
    auto localDeviceId = DeviceKvStoreImpl::GetLocalDeviceId();
    if (localDeviceId.empty()) {
        ZLOGE("failed to get local device id");
        return Status::ERROR;
    }
    StoreMetaData saveMeta = metaData;
    saveMeta.appType = "harmony";
    saveMeta.deviceId = localDeviceId;
    saveMeta.isAutoSync = options.autoSync;
    saveMeta.isBackup = options.backup;
    saveMeta.isEncrypt = options.encrypt;
    saveMeta.storeType = options.kvStoreType;
    saveMeta.schema = options.schema;
    saveMeta.version = STORE_VERSION;
    saveMeta.securityLevel = options.securityLevel;
    saveMeta.dataDir = KvStoreAppManager::GetDbDir(metaData);
    auto saved = MetaDataManager::GetInstance().SaveMeta(saveMeta.GetKey(), saveMeta);
    return !saved ? Status::DB_ERROR : Status::SUCCESS;
}

Status KvStoreDataService::GetSingleKvStoreFailDo(Status status, const Options &options, const StoreMetaData &metaData,
    SecretKeyPara &secKeyParas, KvStoreUserManager &kvUserManager, sptr<SingleKvStoreImpl> &kvStore)
{
    Status statusTmp = status;
    Status getKvStoreStatus = statusTmp;
    int32_t path = KvStoreAppManager::ConvertPathType(metaData);
    ZLOGW("getKvStore failed with status %d", static_cast<int>(getKvStoreStatus));
    if (getKvStoreStatus == Status::CRYPT_ERROR && options.encrypt) {
        if (secKeyParas.alreadyCreated != Status::SUCCESS) {
            // create encrypted store failed, remove secret key
            KvStoreMetaManager::GetInstance().RemoveSecretKey(metaData.uid, metaData.bundleName, metaData.storeId);
            return Status::ERROR;
        }
        // get existing encrypted store failed, retry with key stored in file
        Status statusRet = KvStoreMetaManager::GetInstance().RecoverSecretKeyFromFile(
            secKeyParas.secretKeyFile, secKeyParas.metaSecretKey, secKeyParas.secretKey, secKeyParas.outdated);
        if (statusRet != Status::SUCCESS) {
            kvStore = nullptr;
            return Status::CRYPT_ERROR;
        }
        // here callback is called twice
        statusTmp = kvUserManager.GetKvStore(options, metaData, secKeyParas.secretKey, kvStore);
        if (secKeyParas.outdated) {
            KvStoreMetaManager::GetInstance().ReKey(
                metaData.user, metaData.bundleName, metaData.storeId, path, kvStore);
        }
    }

    // if kvstore damaged and no backup file, then return DB_ERROR
    if (statusTmp != Status::SUCCESS && getKvStoreStatus == Status::CRYPT_ERROR) {
        // if backup file not exist, don't need recover
        if (!CheckBackupFileExist(metaData.user, metaData.bundleName, metaData.storeId, path)) {
            return Status::CRYPT_ERROR;
        }
        // remove damaged database
        if (DeleteKvStoreOnly(metaData) != Status::SUCCESS) {
            ZLOGE("DeleteKvStoreOnly failed.");
            return Status::DB_ERROR;
        }
        // recover database
        return RecoverKvStore(options, metaData, secKeyParas.secretKey, kvStore);
    }
    return statusTmp;
}

bool KvStoreDataService::CheckOptions(const Options &options, const std::vector<uint8_t> &metaKey) const
{
    ZLOGI("begin.");
    KvStoreMetaData metaData;
    metaData.version = 0;
    Status statusTmp = KvStoreMetaManager::GetInstance().GetKvStoreMeta(metaKey, metaData);
    if (statusTmp == Status::KEY_NOT_FOUND) {
        ZLOGI("get metaKey not found.");
        return true;
    }
    if (statusTmp != Status::SUCCESS) {
        ZLOGE("get metaKey failed.");
        return false;
    }
    ZLOGI("metaData encrypt is %d, kvStore type is %d, options encrypt is %d, kvStore type is %d",
          static_cast<int>(metaData.isEncrypt), static_cast<int>(metaData.kvStoreType),
          static_cast<int>(options.encrypt), static_cast<int>(options.kvStoreType));
    if (options.encrypt != metaData.isEncrypt) {
        ZLOGE("checkOptions encrypt type is not the same.");
        return false;
    }

    if (options.kvStoreType != metaData.kvStoreType && metaData.version != 0) {
        ZLOGE("checkOptions kvStoreType is not the same.");
        return false;
    }
    ZLOGI("end.");
    return true;
}

bool KvStoreDataService::CheckBackupFileExist(const std::string &userId, const std::string &bundleName,
                                              const std::string &storeId, int pathType)
{
    return false;
}
template<typename  T>
Status KvStoreDataService::RecoverKvStore(
    const Options &options, const StoreMetaData &metaData, const std::vector<uint8_t> &secretKey, sptr<T> &kvStore)
{
    // restore database
    Options optionsTmp = options;
    optionsTmp.createIfMissing = true;
    auto it = deviceAccountMap_.find(metaData.user);
    if (it == deviceAccountMap_.end()) {
        ZLOGD("deviceAccountId not found");
        return Status::INVALID_ARGUMENT;
    }

    Status statusTmp = (it->second).GetKvStore(optionsTmp, metaData, secretKey, kvStore);
    // restore database failed
    if (statusTmp != Status::SUCCESS || kvStore == nullptr) {
        ZLOGE("RecoverSingleKvStore reget GetSingleKvStore failed.");
        return Status::DB_ERROR;
    }
    // recover database from backup file
    bool importRet = kvStore->Import(metaData.bundleName);
    if (!importRet) {
        ZLOGE("RecoverSingleKvStore Import failed.");
        return Status::RECOVER_FAILED;
    }
    ZLOGD("RecoverSingleKvStore Import success.");
    return Status::RECOVER_SUCCESS;
}

void KvStoreDataService::GetAllKvStoreId(
    const AppId &appId, std::function<void(Status, std::vector<StoreId> &)> callback)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    ZLOGI("GetAllKvStoreId begin.");
    std::string bundleName = Constant::TrimCopy<std::string>(appId.appId);
    std::vector<StoreId> storeIds;
    if (bundleName.empty() || bundleName.size() > MAX_APP_ID_LENGTH) {
        ZLOGE("invalid appId.");
        callback(Status::INVALID_ARGUMENT, storeIds);
        return;
    }

    const int32_t uid = IPCSkeleton::GetCallingUid();
    const std::string userId = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(uid);
    std::string prefix =
        StoreMetaData::GetPrefix({ DeviceKvStoreImpl::GetLocalDeviceId(), userId, "default", bundleName });
    DdsTrace traceDelegate(std::string(LOG_TAG "Delegate::") + std::string(__FUNCTION__));

    std::vector<StoreMetaData> metaDatum;
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, metaDatum)) {
        ZLOGE("LoadKeys failed!");
        callback(Status::DB_ERROR, storeIds);
        return;
    }

    for (const auto &metaData : metaDatum) {
        if (metaData.storeId.empty()) {
            continue;
        }
        storeIds.push_back({ metaData.storeId });
    }
    callback(Status::SUCCESS, storeIds);
}

Status KvStoreDataService::CloseKvStore(const AppId &appId, const StoreId &storeId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    ZLOGI("begin.");
    if (!appId.IsValid() || !storeId.IsValid()) {
        ZLOGE("invalid bundleName.");
        return Status::INVALID_ARGUMENT;
    }

    CheckerManager::StoreInfo info;
    info.uid = IPCSkeleton::GetCallingUid();
    info.tokenId = IPCSkeleton::GetCallingTokenID();
    info.bundleName = appId.appId;
    info.storeId = storeId.storeId;
    std::string trueAppId = CheckerManager::GetInstance().GetAppId(info);
    if (trueAppId.empty()) {
        ZLOGW("check appId:%{public}s uid:%{public}d token:%{public}u failed.",
            appId.appId.c_str(), info.uid, info.tokenId);
        return Status::PERMISSION_DENIED;
    }
    const std::string userId = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(info.uid);
    std::lock_guard<std::mutex> lg(accountMutex_);
    auto it = deviceAccountMap_.find(userId);
    if (it != deviceAccountMap_.end()) {
        Status status = (it->second).CloseKvStore(appId.appId, storeId.storeId);
        if (status != Status::STORE_NOT_OPEN) {
            return status;
        }
    }
    ZLOGE("store not open.");
    return Status::STORE_NOT_OPEN;
}

/* close all opened kvstore */
Status KvStoreDataService::CloseAllKvStore(const AppId &appId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    ZLOGD("begin.");
    if (!appId.IsValid()) {
        ZLOGE("invalid bundleName.");
        return Status::INVALID_ARGUMENT;
    }
    CheckerManager::StoreInfo info;
    info.uid = IPCSkeleton::GetCallingUid();
    info.tokenId = IPCSkeleton::GetCallingTokenID();
    info.bundleName = appId.appId;
    info.storeId = "";
    std::string trueAppId = CheckerManager::GetInstance().GetAppId(info);
    if (trueAppId.empty()) {
        ZLOGW("check appId:%{public}s uid:%{public}d token:%{public}u failed.",
            appId.appId.c_str(), info.uid, info.tokenId);
        return Status::PERMISSION_DENIED;
    }

    const std::string userId = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(info.uid);
    std::lock_guard<std::mutex> lg(accountMutex_);
    auto it = deviceAccountMap_.find(userId);
    if (it != deviceAccountMap_.end()) {
        return (it->second).CloseAllKvStore(appId.appId);
    }
    ZLOGE("store not open.");
    return Status::STORE_NOT_OPEN;
}

Status KvStoreDataService::DeleteKvStore(const AppId &appId, const StoreId &storeId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    if (!appId.IsValid()) {
        ZLOGE("invalid bundleName.");
        return Status::INVALID_ARGUMENT;
    }
    CheckerManager::StoreInfo info;
    info.uid = IPCSkeleton::GetCallingUid();
    info.tokenId = IPCSkeleton::GetCallingTokenID();
    info.bundleName = appId.appId;
    info.storeId = "";
    if (!CheckerManager::GetInstance().IsValid(info)) {
        ZLOGW("check appId:%{public}s uid:%{public}d failed.", appId.appId.c_str(), info.uid);
        return Status::PERMISSION_DENIED;
    }

    HapTokenInfo tokenInfo;
    tokenInfo.instIndex = 0;
    if (AccessTokenKit::GetTokenTypeFlag(info.tokenId) == TOKEN_HAP) {
        AccessTokenKit::GetHapTokenInfo(info.tokenId, tokenInfo);
    }

    StoreMetaData storeMetaData;
    storeMetaData.deviceId = DeviceKvStoreImpl::GetLocalDeviceId();
    storeMetaData.user = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(info.uid);
    storeMetaData.bundleName = appId.appId;
    storeMetaData.storeId = storeId.storeId;
    storeMetaData.instanceId = tokenInfo.instIndex;
    auto metaKey = storeMetaData.GetKey();
    if (!MetaDataManager::GetInstance().LoadMeta(metaKey, storeMetaData)) {
        ZLOGE("load key failed, appId:%s, storeId:%s, instanceId:%u",
            appId.appId.c_str(), storeId.storeId.c_str(), storeMetaData.instanceId);
        return Status::STORE_NOT_FOUND;
    }
    return DeleteKvStore(storeMetaData);
}

Status KvStoreDataService::DeleteKvStore(StoreMetaData &metaData)
{
    std::lock_guard<std::mutex> lg(accountMutex_);
    Status status;
    auto it = deviceAccountMap_.find(metaData.user);
    if (it != deviceAccountMap_.end()) {
        status = (it->second).DeleteKvStore(metaData.bundleName, metaData.uid, metaData.tokenId, metaData.storeId);
    } else {
        KvStoreUserManager kvStoreUserManager(metaData.user);
        status = kvStoreUserManager.DeleteKvStore(
            metaData.bundleName, metaData.uid, metaData.tokenId, metaData.storeId);
    }

    if (status == Status::SUCCESS) {
        MetaDataManager::GetInstance().DelMeta(metaData.GetKey());
        MetaDataManager::GetInstance().DelMeta(metaData.GetSecretKey(), true);
        MetaDataManager::GetInstance().DelMeta(metaData.GetStrategyKey());
        auto secretKeyFile = KvStoreMetaManager::GetSecretSingleKeyFile(
            metaData.user, metaData.bundleName, metaData.storeId, KvStoreAppManager::ConvertPathType(metaData));
        if (!RemoveFile(secretKeyFile)) {
            ZLOGE("remove secretkey file single fail.");
        }
    }
    return status;
}

/* delete all kv store */
Status KvStoreDataService::DeleteAllKvStore(const AppId &appId)
{
    DdsTrace trace(std::string(LOG_TAG "::") + std::string(__FUNCTION__));
    ZLOGI("%s", appId.appId.c_str());
    if (!appId.IsValid()) {
        ZLOGE("invalid bundleName.");
        return Status::INVALID_ARGUMENT;
    }

    CheckerManager::StoreInfo info;
    info.uid = IPCSkeleton::GetCallingUid();
    info.tokenId = IPCSkeleton::GetCallingTokenID();
    info.bundleName = appId.appId;
    info.storeId = "";
    if (!CheckerManager::GetInstance().IsValid(info)) {
        ZLOGW("check appId:%{public}s uid:%{public}d failed.", appId.appId.c_str(), info.uid);
        return Status::PERMISSION_DENIED;
    }

    Status statusTmp;
    std::vector<StoreId> existStoreIds;
    GetAllKvStoreId(appId, [&statusTmp, &existStoreIds](Status status, std::vector<StoreId> &storeIds) {
        statusTmp = status;
        existStoreIds = std::move(storeIds);
    });

    if (statusTmp != Status::SUCCESS) {
        ZLOGE("%s, error: %d ", appId.appId.c_str(), static_cast<int>(statusTmp));
        return statusTmp;
    }

    for (const auto &storeId : existStoreIds) {
        statusTmp = DeleteKvStore(appId, storeId);
        if (statusTmp != Status::SUCCESS) {
            ZLOGE("%s, error: %d ", appId.appId.c_str(), static_cast<int>(statusTmp));
            return statusTmp;
        }
    }

    return statusTmp;
}

/* RegisterClientDeathObserver */
Status KvStoreDataService::RegisterClientDeathObserver(const AppId &appId, sptr<IRemoteObject> observer)
{
    ZLOGD("begin.");
    KVSTORE_ACCOUNT_EVENT_PROCESSING_CHECKER(Status::SYSTEM_ACCOUNT_EVENT_PROCESSING);
    if (!appId.IsValid()) {
        ZLOGE("invalid bundleName.");
        return Status::INVALID_ARGUMENT;
    }

    CheckerManager::StoreInfo info;
    info.uid = IPCSkeleton::GetCallingUid();
    info.tokenId = IPCSkeleton::GetCallingTokenID();
    info.bundleName = appId.appId;
    info.storeId = "";
    if (!CheckerManager::GetInstance().IsValid(info)) {
        ZLOGW("check bundleName:%{public}s uid:%{public}d failed.", appId.appId.c_str(), info.uid);
        return Status::PERMISSION_DENIED;
    }

    std::lock_guard<decltype(clientDeathObserverMutex_)> lg(clientDeathObserverMutex_);
    clientDeathObserverMap_.erase(info.tokenId);
    auto it = clientDeathObserverMap_.emplace(std::piecewise_construct, std::forward_as_tuple(info.tokenId),
        std::forward_as_tuple(appId, *this, std::move(observer)));
    ZLOGI("bundleName:%{public}s, uid:%{public}d, pid:%{public}d inserted:%{public}s.",
        appId.appId.c_str(), info.uid, IPCSkeleton::GetCallingPid(), it.second ? "success" : "failed");
    return it.second ? Status::SUCCESS : Status::ERROR;
}

Status KvStoreDataService::AppExit(pid_t uid, pid_t pid, uint32_t token, const AppId &appId)
{
    ZLOGI("AppExit");
    // memory of parameter appId locates in a member of clientDeathObserverMap_ and will be freed after
    // clientDeathObserverMap_ erase, so we have to take a copy if we want to use this parameter after erase operation.
    AppId appIdTmp = appId;
    if (appId.appId == "com.ohos.medialibrary.medialibrarydata") {
        HapTokenInfo tokenInfo;
        AccessTokenKit::GetHapTokenInfo(token, tokenInfo);
        ZLOGI("not close bundle:%{public}s, tokenInfo.bundle:%{public}s, uid:%{public}d, token:%{public}u",
            appIdTmp.appId.c_str(), tokenInfo.bundleName.c_str(), uid, token);
        return Status::SUCCESS;
    }
    std::lock_guard<decltype(clientDeathObserverMutex_)> lg(clientDeathObserverMutex_);
    clientDeathObserverMap_.erase(token);

    const std::string userId = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(uid);
    std::lock_guard<std::mutex> lock(accountMutex_);
    auto it = deviceAccountMap_.find(userId);
    if (it != deviceAccountMap_.end()) {
        auto status = (it->second).CloseAllKvStore(appIdTmp.appId);
        ZLOGI("Close all kv store %{public}s uid:%{public}d, status:%{public}d.",
            appIdTmp.appId.c_str(), uid, status);
    }
    return Status::SUCCESS;
}

void KvStoreDataService::OnDump()
{
    ZLOGD("begin.");
}

int KvStoreDataService::Dump(int fd, const std::vector<std::u16string> &args)
{
    int uid = static_cast<int>(IPCSkeleton::GetCallingUid());
    const int maxUid = 10000;
    if (uid > maxUid) {
        return 0;
    }

    std::vector<std::string> argsStr;
    for (auto item : args) {
        argsStr.emplace_back(Str16ToStr8(item));
    }

    if (DumpHelper::GetInstance().Dump(fd, argsStr)) {
        return 0;
    }

    ZLOGE("DumpHelper failed");
    return ERROR;
}

void KvStoreDataService::DumpAll(int fd)
{
    dprintf(fd, "------------------------------------------------------------------\n");
    dprintf(fd, "User count : %u\n", static_cast<uint32_t>(deviceAccountMap_.size()));
    for (const auto &pair : deviceAccountMap_) {
        pair.second.Dump(fd);
    }
}

void KvStoreDataService::DumpUserInfo(int fd)
{
    dprintf(fd, "------------------------------------------------------------------\n");
    dprintf(fd, "User count : %u\n", static_cast<uint32_t>(deviceAccountMap_.size()));
    for (const auto &pair : deviceAccountMap_) {
        pair.second.DumpUserInfo(fd);
    }
}

void KvStoreDataService::DumpAppInfo(int fd, const std::string &appId)
{
    dprintf(fd, "------------------------------------------------------------------\n");
    dprintf(fd, "User count : %u\n", static_cast<uint32_t>(deviceAccountMap_.size()));
    for (const auto &pair : deviceAccountMap_) {
        pair.second.DumpAppInfo(fd, appId);
    }
}

void KvStoreDataService::DumpStoreInfo(int fd, const std::string &storeId)
{
    dprintf(fd, "------------------------------------------------------------------\n");
    dprintf(fd, "User count : %u\n", static_cast<uint32_t>(deviceAccountMap_.size()));
    for (const auto &pair : deviceAccountMap_) {
        pair.second.DumpStoreInfo(fd, storeId);
    }
}

void KvStoreDataService::OnStart()
{
    ZLOGI("distributeddata service onStart");
    AccountDelegate::GetInstance()->RegisterHashFunc(Crypto::Sha256);
    static constexpr int32_t RETRY_TIMES = 10;
    static constexpr int32_t RETRY_INTERVAL = 500 * 1000; // unit is ms
    for (BlockInteger retry(RETRY_INTERVAL); retry < RETRY_TIMES; ++retry) {
        if (!DeviceKvStoreImpl::GetLocalDeviceId().empty()) {
            break;
        }
        ZLOGE("GetLocalDeviceId failed, retry count:%{public}d", static_cast<int>(retry));
    }
    ZLOGI("Bootstrap configs and plugins.");
    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCheckers();
    Bootstrap::GetInstance().LoadNetworks();
    Bootstrap::GetInstance().LoadBackup();
    Initialize();
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr != nullptr) {
        ZLOGI("samgr exist.");
        auto remote = samgr->CheckSystemAbility(DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
        auto kvDataServiceProxy = iface_cast<IKvStoreDataService>(remote);
        if (kvDataServiceProxy != nullptr) {
            ZLOGI("service has been registered.");
            return;
        }
    }
    AddSystemAbilityListener(COMMON_EVENT_SERVICE_ID);
    StartService();
}

void KvStoreDataService::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    ZLOGI("add system abilityid:%d", systemAbilityId);
    (void)deviceId;
    if (systemAbilityId != COMMON_EVENT_SERVICE_ID) {
        return;
    }
    AccountDelegate::GetInstance()->SubscribeAccountEvent();
    Uninstaller::GetInstance().Init(this);
}

void KvStoreDataService::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    ZLOGI("remove system abilityid:%d", systemAbilityId);
    (void)deviceId;
    if (systemAbilityId != COMMON_EVENT_SERVICE_ID) {
        return;
    }
    AccountDelegate::GetInstance()->UnsubscribeAccountEvent();
    Uninstaller::GetInstance().UnsubscribeEvent();
}

void KvStoreDataService::StartService()
{
    // register this to ServiceManager.
    KvStoreMetaManager::GetInstance().InitMetaListener();
    InitObjectStore();
    bool ret = SystemAbility::Publish(this);
    if (!ret) {
        DumpHelper::GetInstance().AddErrorInfo("StartService: Service publish failed.");
    }
    Uninstaller::GetInstance().Init(this);
    // Initialize meta db delegate manager.
    KvStoreMetaManager::GetInstance().SubscribeMeta(KvStoreMetaRow::KEY_PREFIX,
        [this](const std::vector<uint8_t> &key, const std::vector<uint8_t> &value, CHANGE_FLAG flag) {
            OnStoreMetaChanged(key, value, flag);
        });
    UpgradeManager::GetInstance().Init();
    UserDelegate::GetInstance().Init();

    // subscribe account event listener to EventNotificationMgr
    AccountDelegate::GetInstance()->SubscribeAccountEvent();
    auto autoLaunch = [this](const std::string &identifier, DistributedDB::AutoLaunchParam &param) -> bool {
        auto status = ResolveAutoLaunchParamByIdentifier(identifier, param);
        if (kvdbService_) {
            kvdbService_->ResolveAutoLaunch(identifier, param);
        }
        return status;
    };
    KvStoreDelegateManager::SetAutoLaunchRequestCallback(autoLaunch);
    DumpHelper::GetInstance().AddDumpOperation(
        std::bind(&KvStoreDataService::DumpAll, this, std::placeholders::_1),
        std::bind(&KvStoreDataService::DumpUserInfo, this, std::placeholders::_1),
        std::bind(&KvStoreDataService::DumpAppInfo, this, std::placeholders::_1, std::placeholders::_2),
        std::bind(&KvStoreDataService::DumpStoreInfo, this, std::placeholders::_1, std::placeholders::_2)
    );
    ZLOGI("Publish ret: %{public}d", static_cast<int>(ret));
}

void KvStoreDataService::OnStoreMetaChanged(
    const std::vector<uint8_t> &key, const std::vector<uint8_t> &value, CHANGE_FLAG flag)
{
    if (flag != CHANGE_FLAG::UPDATE) {
        return;
    }
    StoreMetaData metaData;
    metaData.Unmarshall({ value.begin(), value.end() });
    ZLOGD("meta data info appType:%{public}s, storeId:%{public}s isDirty:%{public}d", metaData.appType.c_str(),
        metaData.storeId.c_str(), metaData.isDirty);
    if (metaData.deviceId != DeviceKvStoreImpl::GetLocalDeviceId() || metaData.deviceId.empty()) {
        ZLOGD("ignore other device change or invalid meta device");
        return;
    }
    static constexpr const char *HARMONY_APP = "harmony";
    if (!metaData.isDirty || metaData.appType != HARMONY_APP) {
        return;
    }
    ZLOGI("dirty kv store. storeId:%{public}s", metaData.storeId.c_str());
    CloseKvStore({ metaData.bundleName }, { metaData.storeId });
    DeleteKvStore({ metaData.bundleName }, { metaData.storeId });
}

bool KvStoreDataService::ResolveAutoLaunchParamByIdentifier(
    const std::string &identifier, DistributedDB::AutoLaunchParam &param)
{
    ZLOGI("start");
    std::map<std::string, MetaData> entries;
    if (!KvStoreMetaManager::GetInstance().GetFullMetaData(entries)) {
        ZLOGE("get full meta failed");
        return false;
    }
    std::string localDeviceId = DeviceKvStoreImpl::GetLocalDeviceId();
    for (const auto &entry : entries) {
        auto &storeMeta = entry.second.kvStoreMetaData;
        if ((!param.userId.empty() && (param.userId != storeMeta.deviceAccountId))
            || (localDeviceId != storeMeta.deviceId)) {
            // judge local userid and local meta
            continue;
        }
        const std::string &itemTripleIdentifier = DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier(
            storeMeta.userId, storeMeta.appId, storeMeta.storeId, false);
        const std::string &itemDualIdentifier =
            DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier("", storeMeta.appId, storeMeta.storeId, true);
        if (identifier == itemTripleIdentifier && storeMeta.bundleName != Bootstrap::GetInstance().GetProcessLabel()) {
            // old triple tuple identifier, should SetEqualIdentifier
            ResolveAutoLaunchCompatible(entry.second, identifier);
        }
        if (identifier == itemDualIdentifier || identifier == itemTripleIdentifier) {
            ZLOGI("identifier  find");
            DistributedDB::AutoLaunchOption option;
            option.createIfNecessary = false;
            option.isEncryptedDb = storeMeta.isEncrypt;
            DistributedDB::CipherPassword password;
            const std::vector<uint8_t> &secretKey = entry.second.secretKeyMetaData.secretKey;
            if (password.SetValue(secretKey.data(), secretKey.size()) != DistributedDB::CipherPassword::OK) {
                ZLOGE("Get secret key failed.");
            }
            if (storeMeta.bundleName == Bootstrap::GetInstance().GetProcessLabel()) {
                param.userId = storeMeta.deviceAccountId;
            }
            option.passwd = password;
            option.schema = storeMeta.schema;
            option.createDirByStoreIdOnly = true;
            option.dataDir = storeMeta.dataDir;
            option.secOption = KvStoreAppManager::ConvertSecurity(storeMeta.securityLevel);
            option.isAutoSync = storeMeta.isAutoSync;
            option.syncDualTupleMode = true; // dual tuple flag
            param.appId = storeMeta.appId;
            param.storeId = storeMeta.storeId;
            param.option = option;
            return true;
        }
    }
    ZLOGI("not find identifier");
    return false;
}

void KvStoreDataService::ResolveAutoLaunchCompatible(const MetaData &meta, const std::string &identifier)
{
    ZLOGI("AutoLaunch:peer device is old tuple, begin to open store");
    if (meta.kvStoreType > KvStoreType::SINGLE_VERSION || meta.kvStoreMetaData.version > STORE_VERSION) {
        ZLOGW("no longer support multi or higher version store type");
        return;
    }

    // open store and SetEqualIdentifier, then close store after 60s
    auto &storeMeta = meta.kvStoreMetaData;
    auto *delegateManager = new (std::nothrow)
        DistributedDB::KvStoreDelegateManager(storeMeta.appId, storeMeta.deviceAccountId);
    if (delegateManager == nullptr) {
        ZLOGE("get store delegate manager failed");
        return;
    }
    delegateManager->SetKvStoreConfig({ storeMeta.dataDir });
    Options options = {
        .createIfMissing = false,
        .encrypt = storeMeta.isEncrypt,
        .autoSync = storeMeta.isAutoSync,
        .securityLevel = storeMeta.securityLevel,
        .kvStoreType = static_cast<KvStoreType>(storeMeta.kvStoreType),
    };
    DistributedDB::KvStoreNbDelegate::Option dbOptions;
    KvStoreAppManager::InitNbDbOption(options, meta.secretKeyMetaData.secretKey, dbOptions);
    DistributedDB::KvStoreNbDelegate *store = nullptr;
    delegateManager->GetKvStore(storeMeta.storeId, dbOptions,
        [&identifier, &store, &storeMeta](int status, DistributedDB::KvStoreNbDelegate *delegate) {
            ZLOGI("temporary open db for equal identifier, ret:%{public}d", status);
            if (delegate != nullptr) {
                KvStoreTuple tuple = { storeMeta.deviceAccountId, storeMeta.appId, storeMeta.storeId };
                UpgradeManager::SetCompatibleIdentifyByType(delegate, tuple, IDENTICAL_ACCOUNT_GROUP);
                UpgradeManager::SetCompatibleIdentifyByType(delegate, tuple, PEER_TO_PEER_GROUP);
                store = delegate;
            }
        });
    KvStoreTask delayTask([delegateManager, store]() {
        constexpr const int CLOSE_STORE_DELAY_TIME = 60; // unit: seconds
        std::this_thread::sleep_for(std::chrono::seconds(CLOSE_STORE_DELAY_TIME));
        ZLOGI("AutoLaunch:close store after 60s while autolaunch finishied");
        delegateManager->CloseKvStore(store);
        delete delegateManager;
    });
    ExecutorFactory::GetInstance().Execute(std::move(delayTask));
}

void KvStoreDataService::OnStop()
{
    ZLOGI("begin.");
}

KvStoreDataService::KvStoreClientDeathObserverImpl::KvStoreClientDeathObserverImpl(
    const AppId &appId, KvStoreDataService &service, sptr<IRemoteObject> observer)
    : appId_(appId), dataService_(service), observerProxy_(std::move(observer)),
      deathRecipient_(new KvStoreDeathRecipient(*this))
{
    ZLOGI("KvStoreClientDeathObserverImpl");
    uid_ = IPCSkeleton::GetCallingUid();
    pid_ = IPCSkeleton::GetCallingPid();
    token_ = IPCSkeleton::GetCallingTokenID();
    if (observerProxy_ != nullptr) {
        ZLOGI("add death recipient");
        observerProxy_->AddDeathRecipient(deathRecipient_);
    } else {
        ZLOGW("observerProxy_ is nullptr");
    }
}

KvStoreDataService::KvStoreClientDeathObserverImpl::~KvStoreClientDeathObserverImpl()
{
    ZLOGI("~KvStoreClientDeathObserverImpl");
    if (deathRecipient_ != nullptr && observerProxy_ != nullptr) {
        ZLOGI("remove death recipient");
        observerProxy_->RemoveDeathRecipient(deathRecipient_);
    }
    sptr<KVDBServiceImpl> kvdbService;
    {
        std::lock_guard<decltype(dataService_.mutex_)> lockGuard(dataService_.mutex_);
        kvdbService = dataService_.kvdbService_;
    }
    if (kvdbService) {
        kvdbService->AppExit(uid_, pid_, token_, appId_);
    }
}

void KvStoreDataService::KvStoreClientDeathObserverImpl::NotifyClientDie()
{
    ZLOGI("appId: %{public}s uid:%{public}d tokenId:%{public}u", appId_.appId.c_str(), uid_, token_);
    dataService_.AppExit(uid_, pid_, token_, appId_);
}

KvStoreDataService::KvStoreClientDeathObserverImpl::KvStoreDeathRecipient::KvStoreDeathRecipient(
    KvStoreClientDeathObserverImpl &kvStoreClientDeathObserverImpl)
    : kvStoreClientDeathObserverImpl_(kvStoreClientDeathObserverImpl)
{
    ZLOGI("KvStore Client Death Observer");
}

KvStoreDataService::KvStoreClientDeathObserverImpl::KvStoreDeathRecipient::~KvStoreDeathRecipient()
{
    ZLOGI("KvStore Client Death Observer");
}

void KvStoreDataService::KvStoreClientDeathObserverImpl::KvStoreDeathRecipient::OnRemoteDied(
    const wptr<IRemoteObject> &remote)
{
    (void) remote;
    ZLOGI("begin");
    kvStoreClientDeathObserverImpl_.NotifyClientDie();
}

Status KvStoreDataService::DeleteKvStoreOnly(const StoreMetaData &metaData)
{
    ZLOGI("DeleteKvStoreOnly begin.");
    auto it = deviceAccountMap_.find(metaData.user);
    if (it != deviceAccountMap_.end()) {
        return (it->second).DeleteKvStore(metaData.bundleName, metaData.uid, metaData.tokenId, metaData.storeId);
    }
    KvStoreUserManager kvStoreUserManager(metaData.user);
    return kvStoreUserManager.DeleteKvStore(metaData.bundleName, metaData.uid, metaData.tokenId, metaData.storeId);
}

void KvStoreDataService::AccountEventChanged(const AccountEventInfo &eventInfo)
{
    ZLOGI("account event %{public}d changed process, begin.", eventInfo.status);
    NotifyAccountEvent(eventInfo);
    std::lock_guard<std::mutex> lg(accountMutex_);
    switch (eventInfo.status) {
        case AccountStatus::DEVICE_ACCOUNT_DELETE: {
            g_kvStoreAccountEventStatus = 1;
            // delete all kvstore belong to this device account
            auto it = deviceAccountMap_.find(eventInfo.userId);
            if (it != deviceAccountMap_.end()) {
                (it->second).DeleteAllKvStore();
                deviceAccountMap_.erase(eventInfo.userId);
            } else {
                KvStoreUserManager kvStoreUserManager(eventInfo.userId);
                kvStoreUserManager.DeleteAllKvStore();
            }
            std::initializer_list<std::string> dirList = { Constant::ROOT_PATH_DE, "/", Constant::SERVICE_NAME, "/",
                eventInfo.userId };
            std::string userDir = Constant::Concatenate(dirList);
            ForceRemoveDirectory(userDir);
            dirList = { Constant::ROOT_PATH_CE, "/", Constant::SERVICE_NAME, "/", eventInfo.userId };
            userDir = Constant::Concatenate(dirList);
            ForceRemoveDirectory(userDir);

            std::vector<StoreMetaData> metaData;
            MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({""}), metaData);
            for (const auto &meta : metaData) {
                if (meta.user != eventInfo.userId) {
                    continue;
                }
                ZLOGI("bundlname:%s, user:%s", meta.bundleName.c_str(), meta.user.c_str());
                MetaDataManager::GetInstance().DelMeta(meta.GetKey());
                MetaDataManager::GetInstance().DelMeta(meta.GetStrategyKey());
                MetaDataManager::GetInstance().DelMeta(meta.GetSecretKey(), true);
            }
            g_kvStoreAccountEventStatus = 0;
            break;
        }
        case AccountStatus::DEVICE_ACCOUNT_SWITCHED: {
            auto ret = DistributedDB::KvStoreDelegateManager::NotifyUserChanged();
            ZLOGI("notify delegate manager result:%{public}d", ret);
            break;
        }
        default: {
            break;
        }
    }
    ZLOGI("account event %{public}d changed process, end.", eventInfo.status);
}
void KvStoreDataService::NotifyAccountEvent(const AccountEventInfo &eventInfo)
{
    sptr<DistributedRdb::RdbServiceImpl> rdbService;
    sptr<KVDBServiceImpl> kvdbService;
    sptr<DistributedObject::ObjectServiceImpl> objectService;
    {
        std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
        rdbService = rdbService_;
        kvdbService = kvdbService_;
        objectService = objectService_;
    }
    if (kvdbService) {
        kvdbService->OnUserChanged();
    }
    if (eventInfo.status == AccountStatus::DEVICE_ACCOUNT_SWITCHED && objectService) {
        objectService.clear();
    }
}

Status KvStoreDataService::GetLocalDevice(DeviceInfo &device)
{
    auto tmpDevice = AppDistributedKv::CommunicationProvider::GetInstance().GetLocalBasicInfo();
    device = { tmpDevice.networkId, tmpDevice.deviceName, std::to_string(tmpDevice.deviceType) };
    return Status::SUCCESS;
}

Status KvStoreDataService::GetRemoteDevices(std::vector<DeviceInfo> &deviceInfoList, DeviceFilterStrategy strategy)
{
    auto devices = AppDistributedKv::CommunicationProvider::GetInstance().GetRemoteDevices();
    for (auto const &device : devices) {
        DeviceInfo deviceInfo = { device.networkId, device.deviceName, std::to_string(device.deviceType) };
        deviceInfoList.push_back(deviceInfo);
    }
    ZLOGD("strategy is %{public}d.", strategy);
    return Status::SUCCESS;
}

void KvStoreDataService::InitSecurityAdapter()
{
    auto ret = DATASL_OnStart();
    ZLOGI("datasl on start ret:%d", ret);
    security_ = std::make_shared<Security>();
    if (security_ == nullptr) {
        ZLOGD("Security is nullptr.");
        return;
    }

    auto dbStatus = DistributedDB::KvStoreDelegateManager::SetProcessSystemAPIAdapter(security_);
    ZLOGD("set distributed db system api adapter: %d.", static_cast<int>(dbStatus));

    auto status = AppDistributedKv::CommunicationProvider::GetInstance().StartWatchDeviceChange(
        security_.get(), {"security"});
    if (status != AppDistributedKv::Status::SUCCESS) {
        ZLOGD("security register device change failed, status:%d", static_cast<int>(status));
    }
}

Status KvStoreDataService::StartWatchDeviceChange(sptr<IDeviceStatusChangeListener> observer,
                                                  DeviceFilterStrategy strategy)
{
    if (observer == nullptr) {
        ZLOGD("observer is null");
        return Status::INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lck(deviceListenerMutex_);
    if (deviceListener_ == nullptr) {
        deviceListener_ = std::make_shared<DeviceChangeListenerImpl>(deviceListeners_);
        AppDistributedKv::CommunicationProvider::GetInstance().StartWatchDeviceChange(
            deviceListener_.get(), {"serviceWatcher"});
    }
    IRemoteObject *objectPtr = observer->AsObject().GetRefPtr();
    auto listenerPair = std::make_pair(objectPtr, observer);
    deviceListeners_.insert(listenerPair);
    ZLOGD("strategy is %{public}d.", strategy);
    return Status::SUCCESS;
}

Status KvStoreDataService::StopWatchDeviceChange(sptr<IDeviceStatusChangeListener> observer)
{
    if (observer == nullptr) {
        ZLOGD("observer is null");
        return Status::INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lck(deviceListenerMutex_);
    IRemoteObject *objectPtr = observer->AsObject().GetRefPtr();
    auto it = deviceListeners_.find(objectPtr);
    if (it == deviceListeners_.end()) {
        return Status::ILLEGAL_STATE;
    }
    deviceListeners_.erase(it->first);
    return Status::SUCCESS;
}

void KvStoreDataService::SetCompatibleIdentify(const AppDistributedKv::DeviceInfo &info) const
{
    for (const auto &item : deviceAccountMap_) {
        item.second.SetCompatibleIdentify(info.uuid);
    }
}

sptr<IRemoteObject> KvStoreDataService::GetRdbService()
{
    if (rdbService_ == nullptr) {
        std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
        if (rdbService_ == nullptr) {
            rdbService_ = new (std::nothrow) DistributedRdb::RdbServiceImpl();
        }
        return rdbService_ == nullptr ? nullptr : rdbService_->AsObject().GetRefPtr();
    }
    return rdbService_->AsObject().GetRefPtr();
}

sptr<IRemoteObject> KvStoreDataService::GetKVdbService()
{
    if (kvdbService_ == nullptr) {
        std::lock_guard<decltype(mutex_)> lockGuard(mutex_);
        if (kvdbService_ == nullptr) {
            kvdbService_ = new (std::nothrow) KVDBServiceImpl();
        }
        return kvdbService_ == nullptr ? nullptr : kvdbService_->AsObject().GetRefPtr();
    }
    return kvdbService_->AsObject().GetRefPtr();
}

bool DbMetaCallbackDelegateMgr::GetKvStoreDiskSize(const std::string &storeId, uint64_t &size)
{
    if (IsDestruct()) {
        return false;
    }
    DistributedDB::DBStatus ret = delegate_->GetKvStoreDiskSize(storeId, size);
    return (ret == DistributedDB::DBStatus::OK);
}

void DbMetaCallbackDelegateMgr::GetKvStoreKeys(std::vector<StoreInfo> &dbStats)
{
    if (IsDestruct()) {
        return;
    }
    DistributedDB::DBStatus dbStatusTmp;
    Option option {.createIfNecessary = true, .isMemoryDb = false, .isEncryptedDb = false};
    DistributedDB::KvStoreNbDelegate *kvStoreNbDelegatePtr = nullptr;
    delegate_->GetKvStore(
        Constant::SERVICE_META_DB_NAME, option,
        [&kvStoreNbDelegatePtr, &dbStatusTmp](DistributedDB::DBStatus dbStatus,
                                              DistributedDB::KvStoreNbDelegate *kvStoreNbDelegate) {
            kvStoreNbDelegatePtr = kvStoreNbDelegate;
            dbStatusTmp = dbStatus;
        });

    if (dbStatusTmp != DistributedDB::DBStatus::OK) {
        return;
    }
    DistributedDB::Key dbKey = KvStoreMetaRow::GetKeyFor("");
    std::vector<DistributedDB::Entry> entries;
    kvStoreNbDelegatePtr->GetEntries(dbKey, entries);
    if (entries.empty()) {
        delegate_->CloseKvStore(kvStoreNbDelegatePtr);
        return;
    }
    for (auto const &entry : entries) {
        std::string key = std::string(entry.key.begin(), entry.key.end());
        std::vector<std::string> out;
        Split(key, Constant::KEY_SEPARATOR, out);
        if (out.size() >= VECTOR_SIZE) {
            StoreInfo storeInfo = {out[USER_ID], out[APP_ID], out[STORE_ID]};
            dbStats.push_back(std::move(storeInfo));
        }
    }
    delegate_->CloseKvStore(kvStoreNbDelegatePtr);
}

int32_t KvStoreDataService::DeleteObjectsByAppId(const std::string &appId)
{
    return objectService_->DeleteByAppId(appId);
}
} // namespace OHOS::DistributedKv
