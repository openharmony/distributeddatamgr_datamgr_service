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
#include <directory_ex.h>
#include <file_ex.h>
#include <ipc_skeleton.h>
#include <thread>
#include <unistd.h>

#include "accesstoken_kit.h"
#include "auth_delegate.h"
#include "auto_launch_export.h"
#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "communication_provider.h"
#include "config_factory.h"
#include "constant.h"
#include "dds_trace.h"
#include "device_manager_adapter.h"
#include "device_matrix.h"
#include "eventcenter/event_center.h"
#include "executor_factory.h"
#include "hap_token_info.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "kvstore_account_observer.h"
#include "kvstore_app_accessor.h"
#include "log_print.h"
#include "metadata/appid_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/strategy_meta_data.h"
#include "permission_validator.h"
#include "permit_delegate.h"
#include "process_communicator_impl.h"
#include "route_head_handler_impl.h"
#include "runtime_config.h"
#include "string_ex.h"
#include "system_ability_definition.h"
#include "uninstaller/uninstaller.h"
#include "upgrade_manager.h"
#include "user_delegate.h"
#include "utils/anonymous.h"
#include "utils/block_integer.h"
#include "utils/converter.h"
#include "utils/crypto.h"
#include "upgrade.h"

namespace OHOS::DistributedKv {
using namespace std::chrono;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedDataDfx;
using namespace OHOS::Security::AccessToken;
using KvStoreDelegateManager = DistributedDB::KvStoreDelegateManager;
using SecretKeyMeta = DistributedData::SecretKeyMetaData;
using StrategyMetaData = DistributedData::StrategyMeta;
using DmAdapter = DistributedData::DeviceManagerAdapter;
using DBConfig = DistributedDB::RuntimeConfig;

REGISTER_SYSTEM_ABILITY_BY_ID(KvStoreDataService, DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID, true);

KvStoreDataService::KvStoreDataService(bool runOnCreate)
    : SystemAbility(runOnCreate), mutex_(), clients_()
{
    ZLOGI("begin.");
}

KvStoreDataService::KvStoreDataService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate), mutex_(), clients_()
{
    ZLOGI("begin");
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
    sptr<DistributedData::FeatureStubImpl> feature;
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

        value = new DistributedData::FeatureStubImpl(impl);
        feature = value;
        isFirstCreate = true;
        return true;
    });
    if (isFirstCreate) {
        feature->OnInitialize();
    }
    return feature != nullptr ? feature->AsObject() : nullptr;
}

void KvStoreDataService::InitObjectStore()
{
    ZLOGI("begin.");
    auto feature = GetFeatureInterface("data_object");
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
    AccountDelegate::GetInstance()->RegisterHashFunc(Crypto::Sha256);
    static constexpr int32_t RETRY_TIMES = 50;
    static constexpr int32_t RETRY_INTERVAL = 500 * 1000; // unit is ms
    for (BlockInteger retry(RETRY_INTERVAL); retry < RETRY_TIMES; ++retry) {
        if (!AppDistributedKv::CommunicationProvider::GetInstance().GetLocalDevice().uuid.empty()) {
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
    DeviceMatrix::GetInstance().Initialize(IPCSkeleton::GetCallingTokenID(), Bootstrap::GetInstance().GetMetaDBName());
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
    std::map<std::string, MetaData> entries;
    if (!KvStoreMetaManager::GetInstance().GetFullMetaData(entries)) {
        ZLOGE("get full meta failed");
        return false;
    }
    std::string localDeviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
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
    InitNbDbOption(options, meta.secretKeyMetaData.secretKey, dbOptions);
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

void KvStoreDataService::InitSecurityAdapter()
{
    auto ret = DATASL_OnStart();
    ZLOGI("datasl on start ret:%d", ret);
    security_ = std::make_shared<Security>();
    if (security_ == nullptr) {
        ZLOGD("Security is nullptr.");
        return;
    }

    auto dbStatus = DistributedDB::RuntimeConfig::SetProcessSystemAPIAdapter(security_);
    ZLOGD("set distributed db system api adapter: %d.", static_cast<int>(dbStatus));

    auto status = AppDistributedKv::CommunicationProvider::GetInstance().StartWatchDeviceChange(
        security_.get(), {"security"});
    if (status != AppDistributedKv::Status::SUCCESS) {
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
    DistributedDB::KvStoreNbDelegate *nbDelegate = nullptr;
    delegate_->GetKvStore(Bootstrap::GetInstance().GetMetaDBName(), option,
        [&nbDelegate, &dbStatusTmp](DistributedDB::DBStatus dbStatus, DistributedDB::KvStoreNbDelegate *delegate) {
            nbDelegate = delegate;
            dbStatusTmp = dbStatus;
        });

    if (dbStatusTmp != DistributedDB::DBStatus::OK) {
        return;
    }
    DistributedDB::Key dbKey = KvStoreMetaRow::GetKeyFor("");
    std::vector<DistributedDB::Entry> entries;
    nbDelegate->GetEntries(dbKey, entries);
    if (entries.empty()) {
        delegate_->CloseKvStore(nbDelegate);
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
    delegate_->CloseKvStore(nbDelegate);
}

int32_t KvStoreDataService::OnUninstall(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId)
{
    features_.ForEachCopies(
        [bundleName, user, index, tokenId](const auto &, sptr<DistributedData::FeatureStubImpl> &value) {
            value->OnAppUninstall(bundleName, user, index, tokenId);
            return false;
        });
    return 0;
}
} // namespace OHOS::DistributedKv
