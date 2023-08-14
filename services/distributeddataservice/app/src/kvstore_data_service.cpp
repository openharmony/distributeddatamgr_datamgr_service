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

#include <chrono>
#include <ipc_skeleton.h>
#include <thread>

#include "auth_delegate.h"
#include "auto_launch_export.h"
#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "communication_provider.h"
#include "communicator_context.h"
#include "config_factory.h"
#include "crypto_manager.h"
#include "device_manager_adapter.h"
#include "device_matrix.h"
#include "eventcenter/event_center.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "kvstore_account_observer.h"
#include "log_print.h"
#include "metadata/appid_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "permission_validator.h"
#include "permit_delegate.h"
#include "process_communicator_impl.h"
#include "reporter.h"
#include "route_head_handler_impl.h"
#include "runtime_config.h"
#include "string_ex.h"
#include "system_ability_definition.h"
#include "task_manager.h"
#include "uninstaller/uninstaller.h"
#include "upgrade.h"
#include "upgrade_manager.h"
#include "user_delegate.h"
#include "utils/anonymous.h"
#include "utils/block_integer.h"
#include "utils/crypto.h"

namespace OHOS::DistributedKv {
using namespace std::chrono;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedDataDfx;
using KvStoreDelegateManager = DistributedDB::KvStoreDelegateManager;
using SecretKeyMeta = DistributedData::SecretKeyMetaData;
using DmAdapter = DistributedData::DeviceManagerAdapter;
using DBConfig = DistributedDB::RuntimeConfig;

REGISTER_SYSTEM_ABILITY_BY_ID(KvStoreDataService, DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID, true);

KvStoreDataService::KvStoreDataService(bool runOnCreate)
    : SystemAbility(runOnCreate), mutex_(), clients_()
{
    ZLOGI("begin.");
    if (executors_ == nullptr) {
        constexpr size_t MAX = 12;
        constexpr size_t MIN = 5;
        executors_ = std::make_shared<ExecutorPool>(MAX, MIN);
        DistributedDB::RuntimeConfig::SetThreadPool(std::make_shared<TaskManager>(executors_));
    }
}

KvStoreDataService::KvStoreDataService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate), mutex_(), clients_()
{
    ZLOGI("begin");
    if (executors_ == nullptr) {
        constexpr size_t MAX = 12;
        constexpr size_t MIN = 5;
        executors_ = std::make_shared<ExecutorPool>(MAX, MIN);
        DistributedDB::RuntimeConfig::SetThreadPool(std::make_shared<TaskManager>(executors_));
    }
}

KvStoreDataService::~KvStoreDataService()
{
    ZLOGI("begin.");
    clients_.clear();
    features_.Clear();
}

void KvStoreDataService::Initialize()
{
    ZLOGI("begin.");
#ifndef UT_TEST
    KvStoreDelegateManager::SetProcessLabel(Bootstrap::GetInstance().GetProcessLabel(), "default");
#endif
    CommunicatorContext::getInstance().SetThreadPool(executors_);
    auto communicator = std::make_shared<AppDistributedKv::ProcessCommunicatorImpl>(RouteHeadHandlerImpl::Create);
    auto ret = KvStoreDelegateManager::SetProcessCommunicator(communicator);
    ZLOGI("set communicator ret:%{public}d.", static_cast<int>(ret));

    AppDistributedKv::CommunicationProvider::GetInstance();
    PermitDelegate::GetInstance().Init();
    InitSecurityAdapter(executors_);
    KvStoreMetaManager::GetInstance().BindExecutor(executors_);
    KvStoreMetaManager::GetInstance().InitMetaParameter();
    accountEventObserver_ = std::make_shared<KvStoreAccountObserver>(*this, executors_);
    AccountDelegate::GetInstance()->Subscribe(accountEventObserver_);
    deviceInnerListener_ = std::make_unique<KvStoreDeviceListener>(*this);
    DmAdapter::GetInstance().StartWatchDeviceChange(deviceInnerListener_.get(), { "innerListener" });
    auto translateCall = [](const std::string &oriDevId, const DistributedDB::StoreInfo &info) {
        StoreMetaData meta;
        AppIDMetaData appIdMeta;
        MetaDataManager::GetInstance().LoadMeta(info.appId, appIdMeta, true);
        meta.bundleName = appIdMeta.bundleName;
        meta.storeId = info.storeId;
        meta.user = info.userId;
        meta.deviceId = oriDevId;
        MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta);
        return Upgrade::GetInstance().GetEncryptedUuidByMeta(meta);
    };
    DBConfig::SetTranslateToDeviceIdCallback(translateCall);
}

sptr<IRemoteObject> KvStoreDataService::GetFeatureInterface(const std::string &name)
{
    sptr<FeatureStubImpl> feature;
    bool isFirstCreate = false;
    features_.Compute(name, [&feature, &isFirstCreate](const auto &key, auto &value) ->bool {
        if (value != nullptr) {
            feature = value;
            return true;
        }
        auto creator = FeatureSystem::GetInstance().GetCreator(key);
        if (!creator) {
            return false;
        }
        auto impl = creator();
        if (impl == nullptr) {
            return false;
        }

        value = new FeatureStubImpl(impl);
        feature = value;
        isFirstCreate = true;
        return true;
    });
    if (isFirstCreate) {
        feature->OnInitialize(executors_);
    }
    return feature != nullptr ? feature->AsObject() : nullptr;
}

void KvStoreDataService::LoadFeatures()
{
    ZLOGI("begin.");
    auto features = FeatureSystem::GetInstance().GetFeatureName(FeatureSystem::BIND_NOW);
    for (auto const &feature : features) {
        GetFeatureInterface(feature);
    }
}

/* RegisterClientDeathObserver */
Status KvStoreDataService::RegisterClientDeathObserver(const AppId &appId, sptr<IRemoteObject> observer)
{
    ZLOGD("begin.");
    KVSTORE_ACCOUNT_EVENT_PROCESSING_CHECKER(Status::SYSTEM_ACCOUNT_EVENT_PROCESSING);
    if (!appId.IsValid()) {
        ZLOGE("invalid bundleName, name:%{public}s", appId.appId.c_str());
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

    std::lock_guard<decltype(mutex_)> lg(mutex_);
    auto iter = clients_.find(info.tokenId);
    // Ignore register with same tokenId and pid
    if (iter != clients_.end() && IPCSkeleton::GetCallingPid() == iter->second.GetPid()) {
        ZLOGW("bundleName:%{public}s, uid:%{public}d, pid:%{public}d has already registered.",
            appId.appId.c_str(), info.uid, IPCSkeleton::GetCallingPid());
        return Status::SUCCESS;
    }

    clients_.erase(info.tokenId);
    auto it = clients_.emplace(std::piecewise_construct, std::forward_as_tuple(info.tokenId),
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
    std::lock_guard<decltype(mutex_)> lg(mutex_);
    clients_.erase(token);
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

void KvStoreDataService::OnStart()
{
    ZLOGI("distributeddata service onStart");
    EventCenter::Defer defer;
    Reporter::GetInstance()->SetThreadPool(executors_);
    AccountDelegate::GetInstance()->BindExecutor(executors_);
    AccountDelegate::GetInstance()->RegisterHashFunc(Crypto::Sha256);
    DmAdapter::GetInstance().Init(executors_);
    static constexpr int32_t RETRY_TIMES = 50;
    static constexpr int32_t RETRY_INTERVAL = 500 * 1000; // unit is ms
    for (BlockInteger retry(RETRY_INTERVAL); retry < RETRY_TIMES; ++retry) {
        if (!DmAdapter::GetInstance().GetLocalDevice().uuid.empty()) {
            break;
        }
        ZLOGW("GetLocalDeviceId failed, retry count:%{public}d", static_cast<int>(retry));
    }
    ZLOGI("Bootstrap configs and plugins.");
    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCheckers();
    Bootstrap::GetInstance().LoadNetworks();
    Bootstrap::GetInstance().LoadBackup(executors_);
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
    ZLOGI("add system abilityid:%{public}d", systemAbilityId);
    (void)deviceId;
    if (systemAbilityId != COMMON_EVENT_SERVICE_ID) {
        return;
    }
    AccountDelegate::GetInstance()->SubscribeAccountEvent();
    Uninstaller::GetInstance().Init(this, executors_);
}

void KvStoreDataService::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    ZLOGI("remove system abilityid:%{public}d", systemAbilityId);
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
    ZLOGI("begin.");
    KvStoreMetaManager::GetInstance().InitMetaListener();
    DeviceMatrix::GetInstance().Initialize(IPCSkeleton::GetCallingTokenID(), Bootstrap::GetInstance().GetMetaDBName());
    LoadFeatures();
    bool ret = SystemAbility::Publish(this);
    if (!ret) {
        DumpHelper::GetInstance().AddErrorInfo("StartService: Service publish failed.");
    }
    // Initialize meta db delegate manager.
    KvStoreMetaManager::GetInstance().SubscribeMeta(StoreMetaData::GetKey({}),
        [this](const std::vector<uint8_t> &key, const std::vector<uint8_t> &value, CHANGE_FLAG flag) {
            OnStoreMetaChanged(key, value, flag);
        });
    UpgradeManager::GetInstance().Init(executors_);
    UserDelegate::GetInstance().Init(executors_);

    // subscribe account event listener to EventNotificationMgr
    auto autoLaunch = [this](const std::string &identifier, DistributedDB::AutoLaunchParam &param) -> bool {
        auto status = ResolveAutoLaunchParamByIdentifier(identifier, param);
        features_.ForEachCopies([&identifier, &param](const auto &, sptr<DistributedData::FeatureStubImpl> &value) {
            value->ResolveAutoLaunch(identifier, param);
            return false;
        });
        return status;
    };
    KvStoreDelegateManager::SetAutoLaunchRequestCallback(autoLaunch);
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
        Anonymous::Change(metaData.storeId).c_str(), metaData.isDirty);
    auto deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (metaData.deviceId != deviceId || metaData.deviceId.empty()) {
        ZLOGD("ignore other device change or invalid meta device");
        return;
    }
    static constexpr const char *HARMONY_APP = "harmony";
    if (!metaData.isDirty || metaData.appType != HARMONY_APP) {
        return;
    }
    ZLOGI("dirty kv store. storeId:%{public}s", Anonymous::Change(metaData.storeId).c_str());
}

bool KvStoreDataService::ResolveAutoLaunchParamByIdentifier(
    const std::string &identifier, DistributedDB::AutoLaunchParam &param)
{
    ZLOGI("start");
    std::vector<StoreMetaData> entries;
    std::string localDeviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ localDeviceId }), entries)) {
        ZLOGE("get full meta failed");
        return false;
    }

    for (const auto &storeMeta : entries) {
        if ((!param.userId.empty() && (param.userId != storeMeta.user)) || (localDeviceId != storeMeta.deviceId) ||
            ((StoreMetaData::STORE_RELATIONAL_BEGIN <= storeMeta.storeType) &&
             (StoreMetaData::STORE_RELATIONAL_END >= storeMeta.storeType))) {
            // judge local userid and local meta
            continue;
        }
        const std::string &itemTripleIdentifier =
            DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier(storeMeta.user, storeMeta.appId,
                storeMeta.storeId, false);
        const std::string &itemDualIdentifier =
            DistributedDB::KvStoreDelegateManager::GetKvStoreIdentifier("", storeMeta.appId, storeMeta.storeId, true);
        if (identifier == itemTripleIdentifier && storeMeta.bundleName != Bootstrap::GetInstance().GetProcessLabel()) {
            // old triple tuple identifier, should SetEqualIdentifier
            ResolveAutoLaunchCompatible(storeMeta, identifier);
        }
        if (identifier == itemDualIdentifier || identifier == itemTripleIdentifier) {
            ZLOGI("identifier  find");
            DistributedDB::AutoLaunchOption option;
            option.createIfNecessary = false;
            option.isEncryptedDb = storeMeta.isEncrypt;

            SecretKeyMeta secretKey;
            if (storeMeta.isEncrypt && MetaDataManager::GetInstance().LoadMeta(storeMeta.GetSecretKey(), secretKey)) {
                std::vector<uint8_t> decryptKey;
                CryptoManager::GetInstance().Decrypt(secretKey.sKey, decryptKey);
                option.passwd.SetValue(decryptKey.data(), decryptKey.size());
                std::fill(decryptKey.begin(), decryptKey.end(), 0);
            }

            if (storeMeta.bundleName == Bootstrap::GetInstance().GetProcessLabel()) {
                param.userId = storeMeta.user;
            }
            option.schema = storeMeta.schema;
            option.createDirByStoreIdOnly = true;
            option.dataDir = storeMeta.dataDir;
            option.secOption = ConvertSecurity(storeMeta.securityLevel);
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

DistributedDB::SecurityOption KvStoreDataService::ConvertSecurity(int securityLevel)
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

void KvStoreDataService::ResolveAutoLaunchCompatible(const StoreMetaData &storeMeta, const std::string &identifier)
{
    ZLOGI("AutoLaunch:peer device is old tuple, begin to open store");
    if (storeMeta.storeType > KvStoreType::SINGLE_VERSION || storeMeta.version > STORE_VERSION) {
        ZLOGW("no longer support multi or higher version store type");
        return;
    }

    // open store and SetEqualIdentifier, then close store after 60s
    DistributedDB::KvStoreDelegateManager delegateManager(storeMeta.appId, storeMeta.user);
    delegateManager.SetKvStoreConfig({ storeMeta.dataDir });
    Options options = {
        .createIfMissing = false,
        .encrypt = storeMeta.isEncrypt,
        .autoSync = storeMeta.isAutoSync,
        .securityLevel = storeMeta.securityLevel,
        .kvStoreType = static_cast<KvStoreType>(storeMeta.storeType),
    };
    DistributedDB::KvStoreNbDelegate::Option dbOptions;
    SecretKeyMeta secretKey;
    if (storeMeta.isEncrypt && MetaDataManager::GetInstance().LoadMeta(storeMeta.GetSecretKey(), secretKey)) {
        std::vector<uint8_t> decryptKey;
        CryptoManager::GetInstance().Decrypt(secretKey.sKey, decryptKey);
        std::fill(secretKey.sKey.begin(), secretKey.sKey.end(), 0);
        secretKey.sKey = std::move(decryptKey);
        std::fill(decryptKey.begin(), decryptKey.end(), 0);
    }
    InitNbDbOption(options, secretKey.sKey, dbOptions);
    DistributedDB::KvStoreNbDelegate *store = nullptr;
    delegateManager.GetKvStore(storeMeta.storeId, dbOptions,
        [&store, &storeMeta](int status, DistributedDB::KvStoreNbDelegate *delegate) {
            ZLOGI("temporary open db for equal identifier, ret:%{public}d", status);
            if (delegate != nullptr) {
                KvStoreTuple tuple = { storeMeta.user, storeMeta.appId, storeMeta.storeId };
                UpgradeManager::SetCompatibleIdentifyByType(delegate, tuple, IDENTICAL_ACCOUNT_GROUP);
                UpgradeManager::SetCompatibleIdentifyByType(delegate, tuple, PEER_TO_PEER_GROUP);
                store = delegate;
            }
        });
    ExecutorPool::Task delayTask([store]() {
        ZLOGI("AutoLaunch:close store after 60s while autolaunch finishied");
        DistributedDB::KvStoreDelegateManager delegateManager("", "");
        delegateManager.CloseKvStore(store);
    });
    constexpr int CLOSE_STORE_DELAY_TIME = 60; // unit: seconds
    executors_->Schedule(std::chrono::seconds(CLOSE_STORE_DELAY_TIME), std::move(delayTask));
}

Status KvStoreDataService::InitNbDbOption(const Options &options, const std::vector<uint8_t> &cipherKey,
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
    dbOption.createDirByStoreIdOnly = true;
    dbOption.secOption = ConvertSecurity(options.securityLevel);
    return Status::SUCCESS;
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
    dataService_.features_.ForEachCopies([this](const auto &, sptr<DistributedData::FeatureStubImpl> &value) {
        value->OnAppExit(uid_, pid_, token_, appId_);
        return false;
    });
}

void KvStoreDataService::KvStoreClientDeathObserverImpl::NotifyClientDie()
{
    ZLOGI("appId: %{public}s uid:%{public}d tokenId:%{public}u", appId_.appId.c_str(), uid_, token_);
    dataService_.AppExit(uid_, pid_, token_, appId_);
}

pid_t KvStoreDataService::KvStoreClientDeathObserverImpl::GetPid() const
{
    return pid_;
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

void KvStoreDataService::AccountEventChanged(const AccountEventInfo &eventInfo)
{
    ZLOGI("account event %{public}d changed process, begin.", eventInfo.status);
    NotifyAccountEvent(eventInfo);
    switch (eventInfo.status) {
        case AccountStatus::DEVICE_ACCOUNT_DELETE: {
            g_kvStoreAccountEventStatus = 1;
            // delete all kvstore meta belong to this user
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
                MetaDataManager::GetInstance().DelMeta(meta.appId, true);
                MetaDataManager::GetInstance().DelMeta(meta.GetKeyLocal(), true);
                PermitDelegate::GetInstance().DelCache(meta.GetKey());
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
    features_.ForEachCopies([&eventInfo](const auto &key, sptr<DistributedData::FeatureStubImpl> &value) {
        value->OnUserChange(uint32_t(eventInfo.status), eventInfo.userId, eventInfo.harmonyAccountId);
        return false;
    });

    if (eventInfo.status == AccountStatus::DEVICE_ACCOUNT_SWITCHED) {
        features_.Erase("data_share");
    }
}

void KvStoreDataService::InitSecurityAdapter(std::shared_ptr<ExecutorPool> executors)
{
    auto ret = DATASL_OnStart();
    ZLOGI("datasl on start ret:%d", ret);
    security_ = std::make_shared<Security>(executors);
    if (security_ == nullptr) {
        ZLOGE("security is nullptr.");
        return;
    }

    auto dbStatus = DistributedDB::RuntimeConfig::SetProcessSystemAPIAdapter(security_);
    ZLOGD("set distributed db system api adapter: %d.", static_cast<int>(dbStatus));

    auto status = DmAdapter::GetInstance().StartWatchDeviceChange(security_.get(), {"security"});
    if (status != Status::SUCCESS) {
        ZLOGD("security register device change failed, status:%d", static_cast<int>(status));
    }
}

void KvStoreDataService::SetCompatibleIdentify(const AppDistributedKv::DeviceInfo &info) const
{
}

void KvStoreDataService::OnDeviceOnline(const AppDistributedKv::DeviceInfo &info)
{
    if (info.uuid.empty()) {
        return;
    }
    features_.ForEachCopies([&info](const auto &key, sptr<DistributedData::FeatureStubImpl> &value) {
        value->Online(info.uuid);
        return false;
    });
}

void KvStoreDataService::OnDeviceOffline(const AppDistributedKv::DeviceInfo &info)
{
    if (info.uuid.empty()) {
        return;
    }
    features_.ForEachCopies([&info](const auto &key, sptr<DistributedData::FeatureStubImpl> &value) {
        value->Offline(info.uuid);
        return false;
    });
}

void KvStoreDataService::OnDeviceOnReady(const AppDistributedKv::DeviceInfo &info)
{
    if (info.uuid.empty()) {
        return;
    }
    features_.ForEachCopies([&info](const auto &key, sptr<DistributedData::FeatureStubImpl> &value) {
        value->OnReady(info.uuid);
        return false;
    });
}

int32_t KvStoreDataService::OnUninstall(const std::string &bundleName, int32_t user, int32_t index)
{
    features_.ForEachCopies(
        [bundleName, user, index](const auto &, sptr<DistributedData::FeatureStubImpl> &value) {
            value->OnAppUninstall(bundleName, user, index);
            return false;
        });
    return 0;
}

int32_t KvStoreDataService::OnUpdate(const std::string &bundleName, int32_t user, int32_t index)
{
    features_.ForEachCopies(
        [bundleName, user, index](const auto &, sptr<DistributedData::FeatureStubImpl> &value) {
            value->OnAppUpdate(bundleName, user, index);
            return false;
        });
    return 0;
}
} // namespace OHOS::DistributedKv
