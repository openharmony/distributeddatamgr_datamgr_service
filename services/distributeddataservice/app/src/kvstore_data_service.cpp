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

#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <chrono>
#include <ipc_skeleton.h>
#include <thread>
#include <unistd.h>

#include "accesstoken_kit.h"
#include "app_id_mapping/app_id_mapping_config_manager.h"
#include "auth_delegate.h"
#include "auto_launch_export.h"
#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "communication_provider.h"
#include "communicator_context.h"
#include "config_factory.h"
#include "crypto_manager.h"
#include "db_info_handle_impl.h"
#include "device_manager_adapter.h"
#include "device_matrix.h"
#include "dump/dump_manager.h"
#include "dump_helper.h"
#include "eventcenter/event_center.h"
#include "if_system_ability_manager.h"
#include "installer/installer.h"
#include "iservice_registry.h"
#include "kvstore_account_observer.h"
#include "kvstore_screen_observer.h"
#include "log_print.h"
#include "mem_mgr_client.h"
#include "mem_mgr_proxy.h"
#include "metadata/appid_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "permission_validator.h"
#include "permit_delegate.h"
#include "process_communicator_impl.h"
#include "reporter.h"
#include "route_head_handler_impl.h"
#include "runtime_config.h"
#include "store/auto_cache.h"
#include "string_ex.h"
#include "system_ability_definition.h"
#include "task_manager.h"
#include "thread/thread_manager.h"
#include "upgrade.h"
#include "upgrade_manager.h"
#include "user_delegate.h"
#include "utils/anonymous.h"
#include "utils/base64_utils.h"
#include "utils/block_integer.h"
#include "utils/crypto.h"
#include "xcollie.h"

namespace OHOS::DistributedKv {
using namespace std::chrono;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedDataDfx;
using namespace OHOS::Security::AccessToken;
using KvStoreDelegateManager = DistributedDB::KvStoreDelegateManager;
using SecretKeyMeta = DistributedData::SecretKeyMetaData;
using DmAdapter = DistributedData::DeviceManagerAdapter;
const std::string EXTENSION_BACKUP = "backup";
const std::string EXTENSION_RESTORE = "restore";
const std::string SECRET_KEY_BACKUP_PATH =
    "/data/service/el1/public/database/distributeddata/"
    "secret_key_backup.conf";

REGISTER_SYSTEM_ABILITY_BY_ID(KvStoreDataService, DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID, true);

constexpr char FOUNDATION_PROCESS_NAME[] = "foundation";
constexpr int MAX_DOWNLOAD_ASSETS_COUNT = 50;
constexpr int MAX_DOWNLOAD_TASK = 5;

KvStoreDataService::KvStoreDataService(bool runOnCreate)
    : SystemAbility(runOnCreate), clients_()
{
    ZLOGI("begin.");
}

KvStoreDataService::KvStoreDataService(int32_t systemAbilityId, bool runOnCreate)
    : SystemAbility(systemAbilityId, runOnCreate), clients_()
{
    ZLOGI("begin");
}

KvStoreDataService::~KvStoreDataService()
{
    ZLOGI("begin.");
    clients_.Clear();
    features_.Clear();
}

void KvStoreDataService::Initialize()
{
    ZLOGI("begin.");
#ifndef UT_TEST
    KvStoreDelegateManager::SetProcessLabel(Bootstrap::GetInstance().GetProcessLabel(), "default");
#endif
    CommunicatorContext::GetInstance().SetThreadPool(executors_);
    auto communicator = std::shared_ptr<AppDistributedKv::ProcessCommunicatorImpl>(
        AppDistributedKv::ProcessCommunicatorImpl::GetInstance());
    communicator->SetRouteHeadHandlerCreator(RouteHeadHandlerImpl::Create);
    DistributedDB::RuntimeConfig::SetDBInfoHandle(std::make_shared<DBInfoHandleImpl>());
    DistributedDB::RuntimeConfig::SetAsyncDownloadAssetsConfig({ MAX_DOWNLOAD_TASK, MAX_DOWNLOAD_ASSETS_COUNT });
    auto ret = KvStoreDelegateManager::SetProcessCommunicator(communicator);
    ZLOGI("set communicator ret:%{public}d.", static_cast<int>(ret));

    AppDistributedKv::CommunicationProvider::GetInstance();
    PermitDelegate::GetInstance().Init();
    InitSecurityAdapter(executors_);
    KvStoreMetaManager::GetInstance().BindExecutor(executors_);
    KvStoreMetaManager::GetInstance().InitMetaParameter();
    accountEventObserver_ = std::make_shared<KvStoreAccountObserver>(*this, executors_);
    AccountDelegate::GetInstance()->Subscribe(accountEventObserver_);
    screenEventObserver_ = std::make_shared<KvStoreScreenObserver>(*this, executors_);
    ScreenManager::GetInstance()->Subscribe(screenEventObserver_);
    deviceInnerListener_ = std::make_unique<KvStoreDeviceListener>(*this);
    DmAdapter::GetInstance().StartWatchDeviceChange(deviceInnerListener_.get(), { "innerListener" });
    CommunicatorContext::GetInstance().RegSessionListener(deviceInnerListener_.get());
    auto translateCall = [](const std::string &oriDevId, const DistributedDB::StoreInfo &info) {
        StoreMetaData meta;
        AppIDMetaData appIdMeta;
        MetaDataManager::GetInstance().LoadMeta(info.appId, appIdMeta, true);
        meta.bundleName = appIdMeta.bundleName;
        meta.storeId = info.storeId;
        meta.user = info.userId;
        meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
        MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true);
        std::string uuid;
        if (OHOS::Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(meta.tokenId) ==
            OHOS::Security::AccessToken::TOKEN_HAP) {
            uuid = DmAdapter::GetInstance().CalcClientUuid(meta.appId, oriDevId);
        } else {
            uuid = DmAdapter::GetInstance().CalcClientUuid(" ", oriDevId);
        }
        return uuid;
    };
    DistributedDB::RuntimeConfig::SetTranslateToDeviceIdCallback(translateCall);
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
    auto pid = IPCSkeleton::GetCallingPid();
    clients_.Compute(
        info.tokenId, [&appId, &info, pid, this, obs = std::move(observer)](const auto tokenId, auto &clients) {
            auto res = clients.try_emplace(pid, appId, *this, std::move(obs));
            ZLOGI("bundleName:%{public}s, uid:%{public}d, pid:%{public}d, inserted:%{public}s.", appId.appId.c_str(),
                info.uid, pid, res.second ? "success" : "failed");
            return !clients.empty();
        });
    return Status::SUCCESS;
}

Status KvStoreDataService::AppExit(pid_t uid, pid_t pid, uint32_t token, const AppId &appId)
{
    ZLOGI("AppExit");
    // memory of parameter appId locates in a member of clientDeathObserverMap_ and will be freed after
    // clientDeathObserverMap_ erase, so we have to take a copy if we want to use this parameter after erase operation.
    AppId appIdTmp = appId;
    KvStoreClientDeathObserverImpl impl(*this);
    clients_.ComputeIfPresent(token, [&impl, pid](auto &, auto &value) {
        auto it = value.find(pid);
        if (it == value.end()) {
            return !value.empty();
        }
        impl = std::move(it->second);
        value.erase(it);
        return !value.empty();
    });
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

void KvStoreDataService::InitExecutor()
{
    if (executors_ == nullptr) {
        executors_ = std::make_shared<ExecutorPool>(ThreadManager::GetInstance().GetMaxThreadNum(),
            ThreadManager::GetInstance().GetMinThreadNum());
        DistributedDB::RuntimeConfig::SetThreadPool(std::make_shared<TaskManager>(executors_));
    }
    IPCSkeleton::SetMaxWorkThreadNum(ThreadManager::GetInstance().GetIpcThreadNum());
}

void KvStoreDataService::OnStart()
{
    ZLOGI("distributeddata service onStart");
    LoadConfigs();
    EventCenter::Defer defer;
    Reporter::GetInstance()->SetThreadPool(executors_);
    AccountDelegate::GetInstance()->BindExecutor(executors_);
    ScreenManager::GetInstance()->BindExecutor(executors_);
    AccountDelegate::GetInstance()->RegisterHashFunc(Crypto::Sha256);
    DmAdapter::GetInstance().Init(executors_);
    AutoCache::GetInstance().Bind(executors_);
    static constexpr int32_t RETRY_TIMES = 50;
    static constexpr int32_t RETRY_INTERVAL = 500 * 1000; // unit is ms
    for (BlockInteger retry(RETRY_INTERVAL); retry < RETRY_TIMES; ++retry) {
        if (!DmAdapter::GetInstance().GetLocalDevice().uuid.empty()) {
            break;
        }
        ZLOGW("GetLocalDeviceId failed, retry count:%{public}d", static_cast<int>(retry));
    }
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
    AddSystemAbilityListener(MEMORY_MANAGER_SA_ID);
    RegisterStoreInfo();
    Handler handlerStoreInfo = std::bind(&KvStoreDataService::DumpStoreInfo, this, std::placeholders::_1,
        std::placeholders::_2);
    DumpManager::GetInstance().AddHandler("STORE_INFO", uintptr_t(this), handlerStoreInfo);
    RegisterUserInfo();
    Handler handlerUserInfo = std::bind(&KvStoreDataService::DumpUserInfo, this, std::placeholders::_1,
        std::placeholders::_2);
    DumpManager::GetInstance().AddHandler("USER_INFO", uintptr_t(this), handlerUserInfo);
    RegisterBundleInfo();
    Handler handlerBundleInfo = std::bind(&KvStoreDataService::DumpBundleInfo, this, std::placeholders::_1,
        std::placeholders::_2);
    DumpManager::GetInstance().AddHandler("BUNDLE_INFO", uintptr_t(this), handlerBundleInfo);
    StartService();
}

void KvStoreDataService::LoadConfigs()
{
    ZLOGI("Bootstrap configs and plugins.");
    Bootstrap::GetInstance().LoadThread();
    InitExecutor();
    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCheckers();
    Bootstrap::GetInstance().LoadNetworks();
    Bootstrap::GetInstance().LoadBackup(executors_);
    Bootstrap::GetInstance().LoadCloud();
    Bootstrap::GetInstance().LoadAppIdMappings();
}

void KvStoreDataService::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    ZLOGI("add system abilityid:%{public}d", systemAbilityId);
    (void)deviceId;
    if (systemAbilityId == COMMON_EVENT_SERVICE_ID) {
        AccountDelegate::GetInstance()->SubscribeAccountEvent();
        Installer::GetInstance().Init(this, executors_);
        ScreenManager::GetInstance()->SubscribeScreenEvent();
    } else if (systemAbilityId == MEMORY_MANAGER_SA_ID) {
        Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), 1, 1,
                                                                DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
    }
    return;
}

void KvStoreDataService::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    ZLOGI("remove system abilityid:%{public}d", systemAbilityId);
    (void)deviceId;
    if (systemAbilityId != COMMON_EVENT_SERVICE_ID) {
        return;
    }
    AccountDelegate::GetInstance()->UnsubscribeAccountEvent();
    ScreenManager::GetInstance()->UnsubscribeScreenEvent();
    Installer::GetInstance().UnsubscribeEvent();
}

int32_t KvStoreDataService::OnExtension(const std::string& extension, MessageParcel& data, MessageParcel& reply)
{
    ZLOGI("extension is %{public}s.", extension.c_str());
    if (extension == EXTENSION_BACKUP) {
        return KvStoreDataService::OnBackup(data, reply);
    } else if (extension == EXTENSION_RESTORE) {
        return KvStoreDataService::OnRestore(data, reply);
    }
    return 0;
}

std::string KvStoreDataService::SetBackupReplyCode(int replyCode, const std::string &info = "")
{
    CloneReplyCode reply;
    CloneReplyResult result;
    result.errorCode = std::to_string(replyCode);
    result.errorInfo = info;
    reply.resultInfo.push_back(result);
    return Serializable::Marshall(reply);
}

bool KvStoreDataService::CheckBackupInfo(MessageParcel &data, CloneBackupInfo &backupInfo) {
    std::string info = data.ReadString();
    bool success = backupInfo.Unmarshal(DistributedData::Serializable::ToJson(info));
    if (!success) {
        ZLOGE("parse json failed.");
        return false;
    }
    auto bundleInfos = backupInfo.bundleInfos;
    auto userId = backupInfo.userId;
    if (bundleInfos.empty()) {
        ZLOGE("bundle empty.");
        return false;
    }
    if (userId.empty()) {
        ZLOGE("userId invalid");
        return false;
    }
    return true;
}

ErrCode KvStoreDataService::OnBackup(MessageParcel& data, MessageParcel& reply)
{
    CloneBackupInfo backupInfo;
    if (!CheckBackupInfo(data, backupInfo)) {
        return -1;
    }

    if (!CryptoManager::GetInstance().ImportBackupKey(backupInfo.encryptionInfo.symkey,
        backupInfo.encryptionInfo.iv)) {
        return -1;
    };

    std::string content;
    if (!KvStoreDataService::GetSecretKeyBackup(backupInfo.bundleInfos, backupInfo.userId, content)) {
        return -1;
    };

    FILE *fp = fopen(SECRET_KEY_BACKUP_PATH.c_str(), "w");

    if (!fp) {
        ZLOGE("Secret key backup file open failed");
        (void)fclose(fp);
        return -1;
    }
    std::lock_guard<std::mutex> lock(valueMutex_);
    size_t ret = fwrite(content.c_str(), 1, content.length(), fp);
    if (ret != content.length()) {
        ZLOGE("Save config file fwrite() failed!");
        (void)fclose(fp);
        return -1;
    }

    (void)fflush(fp);
    (void)fsync(fileno(fp));
    (void)fclose(fp);

    UniqueFd fd(-1);
    fd = UniqueFd(open(SECRET_KEY_BACKUP_PATH.c_str(), O_RDONLY));
    std::string replyCode = KvStoreDataService::SetBackupReplyCode(0);
    if (reply.WriteFileDescriptor(fd) == false ||
        reply.WriteString(replyCode) == false) {
        close(fd.Release());
        ZLOGI("OnBackup fail: reply wirte fail");
        return -1;
    }

    close(fd.Release());
    return 0;
}

bool KvStoreDataService::GetSecretKeyBackup(
    const std::vector<DistributedData::CloneBundleInfo> &bundleInfos,
    const std::string &userId, std::string &content)
{
    SecretKeyBackupData backupInfos;
    for (const auto& bundleInfo : bundleInfos) {
        std::string deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
        std::string metaPrefix = StoreMetaData::GetKey({ deviceId, userId, "default", bundleInfo.bundleName });
        std::vector<StoreMetaData> StoreMetas;
        if (!MetaDataManager::GetInstance().LoadMeta(metaPrefix, StoreMetas, true)) {
            ZLOGE("Store meta load failed, bundleName: %{public}s",
                  bundleInfo.bundleName.c_str());
            continue;
        };
        
        for (const auto &storeMeta : StoreMetas) {
            if (!storeMeta.isEncrypt) continue;
            auto key = storeMeta.GetSecretKey();
            SecretKeyMetaData secretKeyMeta;
            if (!MetaDataManager::GetInstance().LoadMeta(key, secretKeyMeta, true)) {
                ZLOGE("Secret key meta load failed, bundleName: %{public}s, Db: "
                    "%{public}s, instanceId: %{public}d",
                    storeMeta.bundleName.c_str(), storeMeta.storeId.c_str(),
                    storeMeta.instanceId);
                continue;
            };
            std::vector<uint8_t> password;
            if (!CryptoManager::GetInstance().Decrypt(secretKeyMeta.sKey, password)) {
                ZLOGE("Secret key decrypt failed, bundleName: %{public}s, Db: "
                    "%{public}s, instanceId: %{public}d",
                    storeMeta.bundleName.c_str(), storeMeta.storeId.c_str(),
                    storeMeta.instanceId);
                continue;
            };
            auto reEncryptedKey = CryptoManager::GetInstance().BackupKeyEncrypt(password);
            
            SecretKeyBackupData::BackupItem item;

            item.time = secretKeyMeta.time;
            item.bundleName = bundleInfo.bundleName;
            item.dbName = storeMeta.storeId;
            item.instanceId = storeMeta.instanceId;
            item.sKey = DistributedData::Base64::Encode(reEncryptedKey);
            item.storeType = secretKeyMeta.storeType;
            item.user = userId;
            backupInfos.infos.push_back(item);
        }
    }
    content = Serializable::Marshall(backupInfos);
    return true;
}

ErrCode KvStoreDataService::OnRestore(MessageParcel& data, MessageParcel& reply)
{
    int32_t result;
    UniqueFd fd(data.ReadFileDescriptor());
    SecretKeyBackupData backupData;
    bool res = ParseBackupSecretKey(fd, backupData);
    if (!res || backupData.infos.size() == 0) {
        ZLOGE("Parse backup data failed or backup infos is empty! res:%{public}d, size:%{public}zu",
            res, backupData.infos.size());
        result = -1;
    } else {
        std::string cloneInfo = data.ReadString();
        CloneBackupInfo backupInfo;
        backupInfo.Unmarshal(DistributedData::Serializable::ToJson(cloneInfo));
        cloneInfo.assign(cloneInfo.size(), 0);
        result = RestoreSecretKey(backupData, backupInfo);
    }

    close(fd.Release());
    std::string replyCode = KvStoreDataService::SetBackupReplyCode(0);
    if (!reply.WriteString(replyCode)) {
        ZLOGE("Write reply failed");
        return -1;
    }
    return result;
}

bool KvStoreDataService::ParseBackupSecretKey(UniqueFd &fd, SecretKeyBackupData &backupData)
{
    struct stat statBuf;
    if (fd.Get() < 0 || fstat(fd.Get(), &statBuf) < 0) {
        ZLOGE("Parse backup secret key failed, fd:%{public}d, errno:%{public}d", fd.Get(), errno);
        return false;
    }
    char buffer[statBuf.st_size + 1];
    if (read(fd.Get(), buffer, statBuf.st_size + 1) < 0) {
        ZLOGE("Read backup secret key failed. errno:%{public}d", errno);
        return false;
    }
    std::string secretKeyStr(buffer);
    DistributedData::Serializable::Unmarshall(secretKeyStr, backupData);
    return true;
}

int32_t KvStoreDataService::RestoreSecretKey(const SecretKeyBackupData &backupData, const CloneBackupInfo &backupInfo)
{
    if (backupData.infos.size() == 0 || backupInfo.userId.empty()) {
        ZLOGE("Restore secret key data invalid! info.size:%{public}zu, userId:%{public}s",
            backupData.infos.size(), backupInfo.userId.c_str());
        return -1;
    }
    if (!CryptoManager::GetInstance().ImportBackupKey(backupInfo.encryptionInfo.symkey,
        backupInfo.encryptionInfo.iv)) {
        return -1;
    };
    for (auto &item : backupData.infos) {
        if (!item.IsValid()) {
            ZLOGW("Secretkey backup info is invalid! bundleName:%{public}s, dbName:%{public}s, instanceId:%{public}d, "
                "storeType:%{public}d, time.size:%{public}zu, sKey:%{public}s, user:%{public}s",
                item.bundleName.c_str(), Anonymous::Change(item.dbName).c_str(), item.instanceId, item.storeType,
                item.time.size(), Anonymous::Change(item.sKey).c_str(), item.user.c_str());
            continue;
        }
        StoreMetaData metaData;
        metaData.bundleName = item.bundleName;
        metaData.storeId = item.dbName;
        metaData.user = item.user == "0" ? "0" : backupInfo.userId;
        metaData.instanceId = item.instanceId;
        auto sKey = DistributedData::Base64::Decode(item.sKey);
        std::vector<uint8_t> rawKey;
        CryptoManager::GetInstance().BackupKeyDecrypt(sKey, rawKey);

        SecretKeyMetaData secretKey;
        secretKey.storeType = item.storeType;
        secretKey.sKey = CryptoManager::GetInstance().Encrypt(rawKey);
        secretKey.time = { item.time.begin(), item.time.end() };
        MetaDataManager::GetInstance().SaveMeta(metaData.GetCloneSecretKey(), secretKey, true);
        sKey.assign(sKey.size(), 0);
        rawKey.assign(rawKey.size(), 0);
    }
    return 0;
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
        DumpHelper::GetInstance().AddErrorInfo(SERVER_UNAVAILABLE, "StartService: Service publish failed.");
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
        features_.ForEachCopies([&identifier, &param](const auto &, sptr<DistributedData::FeatureStubImpl> &value) {
            value->ResolveAutoLaunch(identifier, param);
            return false;
        });
        return false;
    };
    KvStoreDelegateManager::SetAutoLaunchRequestCallback(autoLaunch);
    ZLOGI("Start distributedata Success, Publish ret: %{public}d", static_cast<int>(ret));
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

void KvStoreDataService::OnStop()
{
    ZLOGI("begin.");
    Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), 1, 0, DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
}

KvStoreDataService::KvStoreClientDeathObserverImpl::KvStoreClientDeathObserverImpl(
    const AppId &appId, KvStoreDataService &service, sptr<IRemoteObject> observer)
    : appId_(appId), dataService_(service), observerProxy_(std::move(observer)),
      deathRecipient_(new KvStoreDeathRecipient(*this))
{
    ZLOGD("KvStoreClientDeathObserverImpl");
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
KvStoreDataService::KvStoreClientDeathObserverImpl::KvStoreClientDeathObserverImpl(KvStoreDataService &service)
    : dataService_(service)
{
    Reset();
}

KvStoreDataService::KvStoreClientDeathObserverImpl::KvStoreClientDeathObserverImpl(
    KvStoreDataService::KvStoreClientDeathObserverImpl &&impl)
    : uid_(impl.uid_), pid_(impl.pid_), token_(impl.token_), dataService_(impl.dataService_)
{
    appId_.appId = std::move(impl.appId_.appId);
    impl.Reset();
}

KvStoreDataService::KvStoreClientDeathObserverImpl &KvStoreDataService::KvStoreClientDeathObserverImpl::operator=(
    KvStoreDataService::KvStoreClientDeathObserverImpl &&impl)
{
    uid_ = impl.uid_;
    pid_ = impl.pid_;
    token_ = impl.token_;
    appId_.appId = std::move(impl.appId_.appId);
    impl.Reset();
    return *this;
}

KvStoreDataService::KvStoreClientDeathObserverImpl::~KvStoreClientDeathObserverImpl()
{
    ZLOGI("~KvStoreClientDeathObserverImpl");
    if (deathRecipient_ != nullptr && observerProxy_ != nullptr) {
        ZLOGI("remove death recipient");
        observerProxy_->RemoveDeathRecipient(deathRecipient_);
    }
    if (uid_ == INVALID_UID || pid_ == INVALID_PID || token_ == INVALID_TOKEN || !appId_.IsValid()) {
        return;
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

void KvStoreDataService::KvStoreClientDeathObserverImpl::Reset()
{
    uid_ = INVALID_UID;
    pid_ = INVALID_PID;
    token_ = INVALID_TOKEN;
    appId_.appId = "";
}

KvStoreDataService::KvStoreClientDeathObserverImpl::KvStoreDeathRecipient::KvStoreDeathRecipient(
    KvStoreClientDeathObserverImpl &kvStoreClientDeathObserverImpl)
    : kvStoreClientDeathObserverImpl_(kvStoreClientDeathObserverImpl)
{
    ZLOGI("KvStore Client Death Observer");
}

KvStoreDataService::KvStoreClientDeathObserverImpl::KvStoreDeathRecipient::~KvStoreDeathRecipient()
{
    ZLOGD("~KvStore Client Death Observer");
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
    XCollie xcollie(__FUNCTION__, HiviewDFX::XCOLLIE_FLAG_LOG | HiviewDFX::XCOLLIE_FLAG_RECOVERY, XCOLLIE_COUNT);
    ZLOGI("account event %{public}d changed process, begin.", eventInfo.status);
    NotifyAccountEvent(eventInfo);
    switch (eventInfo.status) {
        case AccountStatus::DEVICE_ACCOUNT_DELETE: {
            g_kvStoreAccountEventStatus = 1;
            // delete all kvstore meta belong to this user
            std::vector<StoreMetaData> metaData;
            MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({""}), metaData, true);
            for (const auto &meta : metaData) {
                if (meta.user != eventInfo.userId) {
                    continue;
                }
                ZLOGI("bundleName:%{public}s, user:%{public}s", meta.bundleName.c_str(), meta.user.c_str());
                MetaDataManager::GetInstance().DelMeta(meta.GetKey());
                MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true);
                MetaDataManager::GetInstance().DelMeta(meta.GetKeyLocal(), true);
                MetaDataManager::GetInstance().DelMeta(meta.GetSecretKey(), true);
                MetaDataManager::GetInstance().DelMeta(meta.GetStrategyKey());
                MetaDataManager::GetInstance().DelMeta(meta.GetBackupSecretKey(), true);
                MetaDataManager::GetInstance().DelMeta(meta.GetAutoLaunchKey(), true);
                MetaDataManager::GetInstance().DelMeta(meta.appId, true);
                MetaDataManager::GetInstance().DelMeta(meta.GetDebugInfoKey(), true);
                MetaDataManager::GetInstance().DelMeta(meta.GetCloneSecretKey(), true);
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
    switch (eventInfo.status) {
        case AccountStatus::DEVICE_ACCOUNT_SWITCHED:
        case AccountStatus::DEVICE_ACCOUNT_DELETE:
        case AccountStatus::DEVICE_ACCOUNT_STOPPED: {
            std::vector<int32_t> users;
            AccountDelegate::GetInstance()->QueryUsers(users);
            std::set<int32_t> userIds(users.begin(), users.end());
            userIds.insert(0);
            AutoCache::GetInstance().CloseStore([&userIds](const StoreMetaData &meta) {
                if (userIds.count(atoi(meta.user.c_str())) == 0) {
                    ZLOGW("Illegal use of database by user %{public}s, %{public}s:%{public}s", meta.user.c_str(),
                          meta.bundleName.c_str(), meta.GetStoreAlias().c_str());
                    return true;
                }
                return false;
            });
            break;
        }
        case AccountStatus::DEVICE_ACCOUNT_STOPPING:
            AutoCache::GetInstance().CloseStore([&eventInfo](const StoreMetaData &meta) {
                return meta.user == eventInfo.userId;
            });
            break;
        default:
            break;
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

    security_->InitLocalSecurity();
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

void KvStoreDataService::OnSessionReady(const AppDistributedKv::DeviceInfo &info)
{
    if (info.uuid.empty()) {
        return;
    }
    features_.ForEachCopies([info](const auto &key, sptr<DistributedData::FeatureStubImpl> &value) {
        value->OnSessionReady(info.uuid);
        return false;
    });
}

int32_t KvStoreDataService::OnUninstall(const std::string &bundleName, int32_t user, int32_t index)
{
    auto staticActs = FeatureSystem::GetInstance().GetStaticActs();
    staticActs.ForEachCopies([bundleName, user, index](const auto &, const std::shared_ptr<StaticActs>& acts) {
        acts->OnAppUninstall(bundleName, user, index);
        return false;
    });
    return SUCCESS;
}

int32_t KvStoreDataService::OnUpdate(const std::string &bundleName, int32_t user, int32_t index)
{
    auto staticActs = FeatureSystem::GetInstance().GetStaticActs();
    staticActs.ForEachCopies([bundleName, user, index](const auto &, const std::shared_ptr<StaticActs>& acts) {
        acts->OnAppUpdate(bundleName, user, index);
        return false;
    });
    return SUCCESS;
}

int32_t KvStoreDataService::OnInstall(const std::string &bundleName, int32_t user, int32_t index)
{
    auto staticActs = FeatureSystem::GetInstance().GetStaticActs();
    staticActs.ForEachCopies([bundleName, user, index](const auto &, const std::shared_ptr<StaticActs>& acts) {
        acts->OnAppInstall(bundleName, user, index);
        return false;
    });
    return SUCCESS;
}

int32_t KvStoreDataService::OnScreenUnlocked(int32_t user)
{
    features_.ForEachCopies([user](const auto &key, sptr<DistributedData::FeatureStubImpl> &value) {
        value->OnScreenUnlocked(user);
        return false;
    });
    return SUCCESS;
}

int32_t KvStoreDataService::ClearAppStorage(const std::string &bundleName, int32_t userId, int32_t appIndex,
    int32_t tokenId)
{
    auto callerToken = IPCSkeleton::GetCallingTokenID();
    NativeTokenInfo nativeTokenInfo;
    if (AccessTokenKit::GetNativeTokenInfo(callerToken, nativeTokenInfo) != RET_SUCCESS ||
        nativeTokenInfo.processName != FOUNDATION_PROCESS_NAME) {
        ZLOGE("passed wrong, tokenId: %{public}u, bundleName:%{public}s, user:%{public}d, appIndex:%{public}d",
            tokenId, bundleName.c_str(), userId, appIndex);
        return ERROR;
    }

    auto staticActs = FeatureSystem::GetInstance().GetStaticActs();
    staticActs.ForEachCopies(
        [bundleName, userId, appIndex, tokenId](const auto &, const std::shared_ptr<StaticActs> &acts) {
            acts->OnClearAppStorage(bundleName, userId, appIndex, tokenId);
            return false;
        });

    std::vector<StoreMetaData> metaData;
    std::string prefix = StoreMetaData::GetPrefix(
        { DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid, std::to_string(userId), "default", bundleName });
    if (!MetaDataManager::GetInstance().LoadMeta(prefix, metaData, true)) {
        ZLOGE("Clear data load meta failed, bundleName:%{public}s, user:%{public}d, appIndex:%{public}d",
            bundleName.c_str(), userId, appIndex);
        return ERROR;
    }

    for (auto &meta : metaData) {
        if (meta.instanceId == appIndex && !meta.appId.empty() && !meta.storeId.empty()) {
            ZLOGI("data cleared bundleName:%{public}s, stordId:%{public}s, appIndex:%{public}d", bundleName.c_str(),
                Anonymous::Change(meta.storeId).c_str(), appIndex);
            MetaDataManager::GetInstance().DelMeta(meta.GetKey());
            MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetKeyLocal(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetSecretKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetStrategyKey());
            MetaDataManager::GetInstance().DelMeta(meta.GetBackupSecretKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.appId, true);
            MetaDataManager::GetInstance().DelMeta(meta.GetDebugInfoKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetAutoLaunchKey(), true);
            PermitDelegate::GetInstance().DelCache(meta.GetKey());
        }
    }
    return SUCCESS;
}

void KvStoreDataService::RegisterStoreInfo()
{
    OHOS::DistributedData::DumpManager::Config storeInfoConfig;
    storeInfoConfig.fullCmd = "--store-info";
    storeInfoConfig.abbrCmd = "-s";
    storeInfoConfig.dumpName = "STORE_INFO";
    storeInfoConfig.countPrintf = PRINTF_COUNT_2;
    storeInfoConfig.infoName = " <StoreId>";
    storeInfoConfig.minParamsNum = 0;
    storeInfoConfig.maxParamsNum = MAXIMUM_PARAMETER_LIMIT; // Store contains no more than three parameters
    storeInfoConfig.parentNode = "BUNDLE_INFO";
    storeInfoConfig.dumpCaption = { "| Display all the store statistics", "| Display the store statistics  by "
                                                                          "StoreID" };
    DumpManager::GetInstance().AddConfig(storeInfoConfig.dumpName, storeInfoConfig);
}

void KvStoreDataService::DumpStoreInfo(int fd, std::map<std::string, std::vector<std::string>> &params)
{
    std::vector<StoreMetaData> metas;
    std::string localDeviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ localDeviceId }), metas, true)) {
        ZLOGE("get full meta failed");
        return;
    }
    FilterData(metas, params);
    PrintfInfo(fd, metas);
}

void KvStoreDataService::FilterData(std::vector<StoreMetaData> &metas,
    std::map<std::string, std::vector<std::string>> &filterInfo)
{
    for (auto iter = metas.begin(); iter != metas.end();) {
        if ((IsExist("USER_INFO", filterInfo, iter->user)) || (IsExist("BUNDLE_INFO", filterInfo, iter->bundleName)) ||
            (IsExist("STORE_INFO", filterInfo, iter->storeId))) {
            iter = metas.erase(iter);
        } else {
            iter++;
        }
    }
}

bool KvStoreDataService::IsExist(const std::string &infoName,
    std::map<std::string, std::vector<std::string>> &filterInfo, std::string &metaParam)
{
    auto info = filterInfo.find(infoName);
    if (info != filterInfo.end()) {
        if (!info->second.empty()) {
            auto it = find(info->second.begin(), info->second.end(), metaParam);
            if (it == info->second.end()) {
                return true;
            }
        }
    }
    return false;
}

void KvStoreDataService::PrintfInfo(int fd, const std::vector<StoreMetaData> &metas)
{
    std::string info;
    int indentation = 0;
    if (metas.size() > 1) {
        info.append("Stores: ").append(std::to_string(metas.size())).append("\n");
        indentation++;
    }
    for (auto &data : metas) {
        char version[VERSION_WIDTH];
        auto flag = sprintf_s(version, sizeof(version), "0x%08X", data.version);
        if (flag < 0) {
            ZLOGE("get version failed");
            return;
        }
        info.append(GetIndentation(indentation))
            .append("--------------------------------------------------------------------------\n")
            .append(GetIndentation(indentation)).append("StoreID           : ").append(data.storeId).append("\n")
            .append(GetIndentation(indentation)).append("UId               : ").append(data.user).append("\n")
            .append(GetIndentation(indentation)).append("BundleName        : ").append(data.bundleName).append("\n")
            .append(GetIndentation(indentation)).append("AppID             : ").append(data.appId).append("\n")
            .append(GetIndentation(indentation)).append("StorePath         : ").append(data.dataDir).append("\n")
            .append(GetIndentation(indentation)).append("StoreType         : ")
            .append(std::to_string(data.storeType)).append("\n")
            .append(GetIndentation(indentation)).append("encrypt           : ")
            .append(std::to_string(data.isEncrypt)).append("\n")
            .append(GetIndentation(indentation)).append("autoSync          : ")
            .append(std::to_string(data.isAutoSync)).append("\n")
            .append(GetIndentation(indentation)).append("schema            : ").append(data.schema).append("\n")
            .append(GetIndentation(indentation)).append("securityLevel     : ")
            .append(std::to_string(data.securityLevel)).append("\n")
            .append(GetIndentation(indentation)).append("area              : ")
            .append(std::to_string(data.area)).append("\n")
            .append(GetIndentation(indentation)).append("instanceID        : ")
            .append(std::to_string(data.instanceId)).append("\n")
            .append(GetIndentation(indentation)).append("version           : ").append(version).append("\n");
    }
    dprintf(fd, "--------------------------------------StoreInfo-------------------------------------\n%s\n",
        info.c_str());
}

std::string KvStoreDataService::GetIndentation(int size)
{
    std::string indentation;
    for (int i = 0; i < size; i++) {
        indentation.append(INDENTATION);
    }
    return indentation;
}

void KvStoreDataService::RegisterUserInfo()
{
    DumpManager::Config userInfoConfig;
    userInfoConfig.fullCmd = "--user-info";
    userInfoConfig.abbrCmd = "-u";
    userInfoConfig.dumpName = "USER_INFO";
    userInfoConfig.countPrintf = PRINTF_COUNT_2;
    userInfoConfig.infoName = " <UId>";
    userInfoConfig.minParamsNum = 0;
    userInfoConfig.maxParamsNum = MAXIMUM_PARAMETER_LIMIT; // User contains no more than three parameters
    userInfoConfig.childNode = "BUNDLE_INFO";
    userInfoConfig.dumpCaption = { "| Display all the user statistics", "| Display the user statistics by UserId" };
    DumpManager::GetInstance().AddConfig(userInfoConfig.dumpName, userInfoConfig);
}

void KvStoreDataService::BuildData(std::map<std::string, UserInfo> &datas, const std::vector<StoreMetaData> &metas)
{
    for (auto &meta : metas) {
        auto it = datas.find(meta.user);
        if (it == datas.end()) {
            UserInfo userInfo;
            userInfo.userId = meta.user;
            userInfo.bundles.insert(meta.bundleName);
            datas[meta.user] = userInfo;
        } else {
            std::set<std::string> bundles_ = datas[meta.user].bundles;
            auto iter = std::find(bundles_.begin(), bundles_.end(), meta.bundleName);
            if (iter == bundles_.end()) {
                datas[meta.user].bundles.insert(meta.bundleName);
            }
        }
    }
}

void KvStoreDataService::PrintfInfo(int fd, const std::map<std::string, UserInfo> &datas)
{
    std::string info;
    int indentation = 0;
    if (datas.size() > 1) {
        info.append("Users : ").append(std::to_string(datas.size())).append("\n");
        indentation++;
    }

    for (auto &data : datas) {
        info.append(GetIndentation(indentation))
            .append("------------------------------------------------------------------------------\n")
            .append("UID        : ")
            .append(data.second.userId)
            .append("\n");
        info.append(GetIndentation(indentation))
            .append("Bundles       : ")
            .append(std::to_string(data.second.bundles.size()))
            .append("\n");
        indentation++;
        info.append("------------------------------------------------------------------------------\n");
        for (auto &bundle : data.second.bundles) {
            info.append(GetIndentation(indentation)).append("BundleName    : ").append(bundle).append("\n");
        }
        indentation--;
    }
    dprintf(fd, "--------------------------------------UserInfo--------------------------------------\n%s\n",
        info.c_str());
}

void KvStoreDataService::DumpUserInfo(int fd, std::map<std::string, std::vector<std::string>> &params)
{
    std::vector<StoreMetaData> metas;
    std::string localDeviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ localDeviceId }), metas, true)) {
        ZLOGE("get full meta failed");
        return;
    }
    FilterData(metas, params);
    std::map<std::string, UserInfo> datas;
    BuildData(datas, metas);
    PrintfInfo(fd, datas);
}

void KvStoreDataService::RegisterBundleInfo()
{
    DumpManager::Config bundleInfoConfig;
    bundleInfoConfig.fullCmd = "--bundle-info";
    bundleInfoConfig.abbrCmd = "-b";
    bundleInfoConfig.dumpName = "BUNDLE_INFO";
    bundleInfoConfig.countPrintf = PRINTF_COUNT_2;
    bundleInfoConfig.infoName = " <BundleName>";
    bundleInfoConfig.minParamsNum = 0;
    bundleInfoConfig.maxParamsNum = MAXIMUM_PARAMETER_LIMIT; // Bundle contains no more than three parameters
    bundleInfoConfig.parentNode = "USER_INFO";
    bundleInfoConfig.childNode = "STORE_INFO";
    bundleInfoConfig.dumpCaption = { "| Display all the bundle statistics", "| Display the bundle statistics by "
                                                                            "BundleName" };
    DumpManager::GetInstance().AddConfig(bundleInfoConfig.dumpName, bundleInfoConfig);
}

void KvStoreDataService::BuildData(std::map<std::string, BundleInfo> &datas, const std::vector<StoreMetaData> &metas)
{
    for (auto &meta : metas) {
        auto it = datas.find(meta.bundleName);
        if (it == datas.end()) {
            BundleInfo bundleInfo;
            bundleInfo.bundleName = meta.bundleName;
            bundleInfo.appId = meta.appId;
            bundleInfo.type = meta.appType;
            bundleInfo.uid = meta.uid;
            bundleInfo.tokenId = meta.tokenId;
            bundleInfo.userId = meta.user;
            std::string storeId = meta.storeId;
            storeId.resize(FORMAT_BLANK_SIZE, FORMAT_BLANK_SPACE);
            bundleInfo.storeIDs.insert(storeId);
            datas[meta.bundleName] = bundleInfo;
        } else {
            std::set<std::string> storeIDs_ = datas[meta.bundleName].storeIDs;
            std::string storeId = meta.storeId;
            storeId.resize(FORMAT_BLANK_SIZE, FORMAT_BLANK_SPACE);
            auto iter = std::find(storeIDs_.begin(), storeIDs_.end(), storeId);
            if (iter == storeIDs_.end()) {
                datas[meta.bundleName].storeIDs.insert(storeId);
            }
        }
    }
}

void KvStoreDataService::PrintfInfo(int fd, const std::map<std::string, BundleInfo> &datas)
{
    std::string info;
    int indentation = 0;
    if (datas.size() > 1) {
        info.append("Bundles: ").append(std::to_string(datas.size())).append("\n");
        indentation++;
    }
    for (auto &data : datas) {
        info.append(GetIndentation(indentation))
            .append("--------------------------------------------------------------------------\n");
        info.append(GetIndentation(indentation)).append("BundleName     : ")
            .append(data.second.bundleName).append("\n");
        info.append(GetIndentation(indentation)).append("AppID          : ")
            .append(data.second.appId).append("\n");
        info.append(GetIndentation(indentation)).append("Type           : ")
            .append(data.second.type).append("\n");
        info.append(GetIndentation(indentation))
            .append("UId            : ")
            .append(std::to_string(data.second.uid))
            .append("\n");
        info.append(GetIndentation(indentation))
            .append("TokenID        : ")
            .append(std::to_string(data.second.tokenId))
            .append("\n");
        info.append(GetIndentation(indentation)).append("UserID         : ").append(data.second.userId).append("\n");
        info.append(GetIndentation(indentation))
            .append("Stores         : ")
            .append(std::to_string(data.second.storeIDs.size()))
            .append("\n");
        indentation++;
        info.append("----------------------------------------------------------------------\n");
        for (auto &store : data.second.storeIDs) {
            info.append(GetIndentation(indentation)).append("StoreID    : ").append(store).append("\n");
        }
        indentation--;
    }
    dprintf(fd, "-------------------------------------BundleInfo-------------------------------------\n%s\n",
        info.c_str());
}

void KvStoreDataService::DumpBundleInfo(int fd, std::map<std::string, std::vector<std::string>> &params)
{
    std::vector<StoreMetaData> metas;
    std::string localDeviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ localDeviceId }), metas, true)) {
        ZLOGE("get full meta failed");
        return;
    }
    FilterData(metas, params);
    std::map<std::string, BundleInfo> datas;
    BuildData(datas, metas);
    PrintfInfo(fd, datas);
}
} // namespace OHOS::DistributedKv