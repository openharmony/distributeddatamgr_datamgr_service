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
#include <fcntl.h>
#include <fstream>
#include <ipc_skeleton.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
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
#include "crypto/crypto_manager.h"
#include "db_info_handle_impl.h"
#include "device_manager_adapter.h"
#include "device_matrix.h"
#include "dfx/reporter.h"
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
#include "network/network_delegate.h"
#include "metadata/store_meta_data.h"
#include "permission_validator.h"
#include "permit_delegate.h"
#include "process_communicator_impl.h"
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
#include "utils/constant.h"
#include "utils/crypto.h"

namespace OHOS::DistributedKv {
using namespace std::chrono;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedDataDfx;
using namespace OHOS::Security::AccessToken;
using KvStoreDelegateManager = DistributedDB::KvStoreDelegateManager;
using SecretKeyMeta = DistributedData::SecretKeyMetaData;
using DmAdapter = DistributedData::DeviceManagerAdapter;
constexpr const char* EXTENSION_BACKUP = "backup";
constexpr const char* EXTENSION_RESTORE = "restore";
constexpr const char* CLONE_KEY_ALIAS = "distributed_db_backup_key";

REGISTER_SYSTEM_ABILITY_BY_ID(KvStoreDataService, DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID, true);

constexpr char FOUNDATION_PROCESS_NAME[] = "foundation";
constexpr int MAX_DOWNLOAD_ASSETS_COUNT = 50;
constexpr int MAX_DOWNLOAD_TASK = 5;
constexpr int KEY_SIZE = 32;
constexpr int AES_256_NONCE_SIZE = 32;
constexpr int MAX_CLIENT_DEATH_OBSERVER_SIZE = 16;

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
    deviceInnerListener_ = std::make_shared<KvStoreDeviceListener>(*this);
    DmAdapter::GetInstance().StartWatchDeviceChange(deviceInnerListener_.get(), { "innerListener" });
    CommunicatorContext::GetInstance().RegSessionListener(deviceInnerListener_.get());
    auto translateCall = [](const std::string &oriDevId, const DistributedDB::StoreInfo &info) {
        StoreMetaMapping storeMetaMapping;
        AppIDMetaData appIdMeta;
        MetaDataManager::GetInstance().LoadMeta(info.appId, appIdMeta, true);
        storeMetaMapping.bundleName = appIdMeta.bundleName;
        storeMetaMapping.storeId = info.storeId;
        storeMetaMapping.user = info.userId;
        storeMetaMapping.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
        MetaDataManager::GetInstance().LoadMeta(storeMetaMapping.GetKey(), storeMetaMapping, true);
        std::string uuid;
        if (OHOS::Security::AccessToken::AccessTokenKit::GetTokenTypeFlag(storeMetaMapping.tokenId) ==
            OHOS::Security::AccessToken::TOKEN_HAP) {
            uuid = DmAdapter::GetInstance().CalcClientUuid(storeMetaMapping.appId, oriDevId);
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
    features_.Compute(name, [&feature, &isFirstCreate](const auto &key, auto &value) -> bool {
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
    auto staticActs = FeatureSystem::GetInstance().GetStaticActs();
    staticActs.ForEach([exec = executors_](const auto &name, std::shared_ptr<StaticActs> acts) {
        if (acts != nullptr) {
            acts->SetThreadPool(exec);
        }
        return false;
    });
}

/* RegisterClientDeathObserver */
Status KvStoreDataService::RegisterClientDeathObserver(const AppId &appId, sptr<IRemoteObject> observer,
    const std::string &featureName)
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
    clients_.Compute(info.tokenId,
        [&appId, &info, pid, this, obs = std::move(observer), featureName](const auto tokenId, auto &clients) {
            auto it = clients.find(pid);
            if (it == clients.end()) {
                auto res = clients.try_emplace(pid, appId, *this, std::move(obs), featureName);
                ZLOGI("bundle:%{public}s, feature:%{public}s, uid:%{public}d, pid:%{public}d, emplace:%{public}s.",
                    appId.appId.c_str(), featureName.c_str(), info.uid, pid, res.second ? "success" : "failed");
            } else {
                auto res = it->second.Insert(obs, featureName);
                ZLOGI("bundle:%{public}s, feature:%{public}s, uid:%{public}d, pid:%{public}d, inserted:%{public}s.",
                    appId.appId.c_str(), featureName.c_str(), info.uid, pid, res ? "success" : "failed");
            }
            return !clients.empty();
        });
    return Status::SUCCESS;
}

int32_t KvStoreDataService::Exit(const std::string &featureName)
{
    ZLOGI("FeatureExit:%{public}s", featureName.c_str());
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    auto pid = IPCSkeleton::GetCallingPid();
    std::string bundleName;
    KvStoreClientDeathObserverImpl impl(*this);
    bool removed = false;
    clients_.ComputeIfPresent(tokenId, [pid, &featureName, &bundleName, &impl, &removed](auto &, auto &value) {
        auto it = value.find(pid);
        if (it == value.end()) {
            return !value.empty();
        }
        bundleName = it->second.GetAppId();
        removed = it->second.Delete(featureName);
        if (it->second.Empty()) {
            impl = std::move(it->second);
            value.erase(it);
        }
        return !value.empty();
    });
    auto [exist, feature] = features_.Find(featureName);
    if (removed && exist && feature != nullptr) {
        feature->OnFeatureExit(IPCSkeleton::GetCallingUid(), pid, tokenId, bundleName);
    }
    return Status::SUCCESS;
}

std::pair<int32_t, std::string> KvStoreDataService::GetSelfBundleName()
{
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    HapTokenInfo tokenInfo;
    auto result = AccessTokenKit::GetHapTokenInfo(tokenId, tokenInfo);
    if (result == RET_SUCCESS) {
        return { Status::SUCCESS, tokenInfo.bundleName };
    }
    NativeTokenInfo nativeTokenInfo;
    if (AccessTokenKit::GetNativeTokenInfo(tokenId, nativeTokenInfo) == RET_SUCCESS) {
        return { Status::SUCCESS, nativeTokenInfo.processName };
    }
    return { Status::ERROR, "" };
}

Status KvStoreDataService::AppExit(pid_t uid, pid_t pid, uint32_t token, const AppId &appId)
{
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
    NetworkDelegate::GetInstance()->BindExecutor(executors_);
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
    AddSystemAbilityListener(COMM_NET_CONN_MANAGER_SYS_ABILITY_ID);
    AddSystemAbilityListener(SUBSYS_ACCOUNT_SYS_ABILITY_ID_BEGIN);
    AddSystemAbilityListener(CONCURRENT_TASK_SERVICE_ID);
    RegisterStoreInfo();
    Handler handlerStoreInfo = std::bind(&KvStoreDataService::DumpStoreInfo, this, std::placeholders::_1,
        std::placeholders::_2);
    DumpManager::GetInstance().AddHandler("STORE_INFO", uintptr_t(this), handlerStoreInfo);
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
    Bootstrap::GetInstance().LoadAutoSyncApps();
    Bootstrap::GetInstance().LoadSyncTrustedApp();
    Bootstrap::GetInstance().LoadDoubleSyncConfig();
}

void KvStoreDataService::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    ZLOGI("add system abilityid:%{public}d", systemAbilityId);
    (void)deviceId;
    if (systemAbilityId == COMMON_EVENT_SERVICE_ID) {
        Installer::GetInstance().Init(this, executors_);
        ScreenManager::GetInstance()->SubscribeScreenEvent();
    } else if (systemAbilityId == SUBSYS_ACCOUNT_SYS_ABILITY_ID_BEGIN) {
        AccountDelegate::GetInstance()->SubscribeAccountEvent();
    } else if (systemAbilityId == MEMORY_MANAGER_SA_ID) {
        Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), 1, 1,
                                                                DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
        // process set critical true
        Memory::MemMgrClient::GetInstance().SetCritical(getpid(), true, DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
    } else if (systemAbilityId == COMM_NET_CONN_MANAGER_SYS_ABILITY_ID) {
        NetworkDelegate::GetInstance()->RegOnNetworkChange();
    }
    return;
}

void KvStoreDataService::OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    ZLOGI("remove system abilityid:%{public}d", systemAbilityId);
    (void)deviceId;
    if (systemAbilityId == SUBSYS_ACCOUNT_SYS_ABILITY_ID_BEGIN) {
        AccountDelegate::GetInstance()->UnsubscribeAccountEvent();
    }
    if (systemAbilityId != COMMON_EVENT_SERVICE_ID) {
        return;
    }
    ScreenManager::GetInstance()->UnsubscribeScreenEvent();
    Installer::GetInstance().UnsubscribeEvent();
}

int32_t KvStoreDataService::OnExtension(const std::string &extension, MessageParcel &data, MessageParcel &reply)
{
    ZLOGI("extension is %{public}s, callingPid:%{public}d.", extension.c_str(), IPCSkeleton::GetCallingPid());
    if (extension == EXTENSION_BACKUP) {
        return KvStoreDataService::OnBackup(data, reply);
    } else if (extension == EXTENSION_RESTORE) {
        return KvStoreDataService::OnRestore(data, reply);
    }
    return 0;
}

std::string KvStoreDataService::GetBackupReplyCode(int replyCode, const std::string &info)
{
    CloneReplyCode reply;
    CloneReplyResult result;
    result.errorCode = std::to_string(replyCode);
    result.errorInfo = info;
    reply.resultInfo.push_back(std::move(result));
    return Serializable::Marshall(reply);
}

std::vector<uint8_t> ConvertDecStrToVec(const std::string &inData)
{
    std::vector<uint8_t> outData;
    auto splitedToken = Constant::Split(inData, ",");
    outData.reserve(splitedToken.size());
    for (auto &token : splitedToken) {
        outData.push_back(static_cast<uint8_t>(atoi(token.c_str())));
    }
    return outData;
}

bool KvStoreDataService::ImportCloneKey(const std::string &keyStr)
{
    auto key = ConvertDecStrToVec(keyStr);
    if (key.size() != KEY_SIZE) {
        ZLOGE("ImportKey failed, key size not correct, key size:%{public}zu", key.size());
        key.assign(key.size(), 0);
        return false;
    }

    auto cloneKeyAlias = std::vector<uint8_t>(CLONE_KEY_ALIAS, CLONE_KEY_ALIAS + strlen(CLONE_KEY_ALIAS));
    if (!CryptoManager::GetInstance().ImportKey(key, cloneKeyAlias)) {
        key.assign(key.size(), 0);
        return false;
    }
    key.assign(key.size(), 0);
    return true;
}

void KvStoreDataService::DeleteCloneKey()
{
    auto cloneKeyAlias = std::vector<uint8_t>(CLONE_KEY_ALIAS, CLONE_KEY_ALIAS + strlen(CLONE_KEY_ALIAS));
    CryptoManager::GetInstance().DeleteKey(cloneKeyAlias);
}

bool KvStoreDataService::WriteBackupInfo(const std::string &content, const std::string &backupPath)
{
    FILE *fp = fopen(backupPath.c_str(), "w");

    if (!fp) {
        ZLOGE("Secret key backup file fopen failed, path: %{public}s, errno: %{public}d",
            Anonymous::Change(backupPath).c_str(), errno);
        return false;
    }
    size_t ret = fwrite(content.c_str(), 1, content.length(), fp);
    if (ret != content.length()) {
        ZLOGE("Secret key backup file fwrite failed, path: %{public}s, errno: %{public}d",
            Anonymous::Change(backupPath).c_str(), errno);
        (void)fclose(fp);
        return false;
    }

    (void)fflush(fp);
    (void)fsync(fileno(fp));
    (void)fclose(fp);
    return true;
}

int32_t KvStoreDataService::OnBackup(MessageParcel &data, MessageParcel &reply)
{
    CloneBackupInfo backupInfo;
    if (!backupInfo.Unmarshal(data.ReadString()) ||
        backupInfo.bundleInfos.empty() || backupInfo.userId.empty()) {
        std::fill(backupInfo.encryptionInfo.symkey.begin(), backupInfo.encryptionInfo.symkey.end(), '\0');
        return -1;
    }

    if (!ImportCloneKey(backupInfo.encryptionInfo.symkey)) {
        std::fill(backupInfo.encryptionInfo.symkey.begin(), backupInfo.encryptionInfo.symkey.end(), '\0');
        return -1;
    }
    std::fill(backupInfo.encryptionInfo.symkey.begin(), backupInfo.encryptionInfo.symkey.end(), '\0');

    auto iv = ConvertDecStrToVec(backupInfo.encryptionInfo.iv);
    if (iv.size() != AES_256_NONCE_SIZE) {
        ZLOGE("Iv size not correct, iv size:%{public}zu", iv.size());
        iv.assign(iv.size(), 0);
        return -1;
    }

    auto content = GetSecretKeyBackup(backupInfo.bundleInfos, backupInfo.userId, iv);
    DeleteCloneKey();

    std::string backupPath = DirectoryManager::GetInstance().GetClonePath(backupInfo.userId);
    if (backupPath.empty()) {
        ZLOGE("GetClonePath failed, userId:%{public}s errno: %{public}d", backupInfo.userId.c_str(), errno);
        return -1;
    }
    if (!WriteBackupInfo(content, backupPath)) {
        return -1;
    }

    UniqueFd fd(-1);
    fd = UniqueFd(open(backupPath.c_str(), O_RDONLY));
    std::string replyCode = GetBackupReplyCode(0);
    if (!reply.WriteFileDescriptor(fd) || !reply.WriteString(replyCode)) {
        close(fd.Release());
        ZLOGE("OnBackup fail: reply wirte fail, fd:%{public}d", fd.Get());
        return -1;
    }

    close(fd.Release());
    return 0;
}

std::vector<uint8_t> KvStoreDataService::ReEncryptKey(const std::string &key, SecretKeyMetaData &secretKeyMeta,
    const StoreMetaData &metaData, const std::vector<uint8_t> &iv)
{
    if (!MetaDataManager::GetInstance().LoadMeta(key, secretKeyMeta, true)) {
        return {};
    };
    CryptoManager::CryptoParams decryptParams = { .area = secretKeyMeta.area, .userId = metaData.user,
        .nonce = secretKeyMeta.nonce };
    auto password = CryptoManager::GetInstance().Decrypt(secretKeyMeta.sKey, decryptParams);
    if (password.empty()) {
        return {};
    }
    auto cloneKeyAlias = std::vector<uint8_t>(CLONE_KEY_ALIAS, CLONE_KEY_ALIAS + strlen(CLONE_KEY_ALIAS));
    CryptoManager::CryptoParams encryptParams = { .keyAlias = cloneKeyAlias, .nonce = iv };
    auto reEncryptedKey = CryptoManager::GetInstance().Encrypt(password, encryptParams);
    password.assign(password.size(), 0);
    if (reEncryptedKey.size() == 0) {
        return {};
    };
    return reEncryptedKey;
}

std::string KvStoreDataService::GetSecretKeyBackup(const std::vector<DistributedData::CloneBundleInfo> &bundleInfos,
    const std::string &userId, const std::vector<uint8_t> &iv)
{
    SecretKeyBackupData backupInfos;
    std::string deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    for (const auto& bundleInfo : bundleInfos) {
        std::string metaPrefix = StoreMetaData::GetKey({ deviceId, userId, "default", bundleInfo.bundleName });
        std::vector<StoreMetaData> StoreMetas;
        if (!MetaDataManager::GetInstance().LoadMeta(metaPrefix, StoreMetas, true)) {
            continue;
        };
        for (const auto &storeMeta : StoreMetas) {
            if (!storeMeta.isEncrypt) {
                continue;
            };
            auto key = storeMeta.GetSecretKey();
            SecretKeyMetaData secretKeyMeta;
            auto reEncryptedKey = ReEncryptKey(key, secretKeyMeta, storeMeta, iv);
            if (reEncryptedKey.size() == 0) {
                continue;
            };
            SecretKeyBackupData::BackupItem item;
            item.time = secretKeyMeta.time;
            item.bundleName = bundleInfo.bundleName;
            item.dbName = storeMeta.storeId;
            item.instanceId = storeMeta.instanceId;
            item.sKey = DistributedData::Base64::Encode(reEncryptedKey);
            item.storeType = secretKeyMeta.storeType;
            item.user = userId;
            item.area = storeMeta.area;
            backupInfos.infos.push_back(std::move(item));
        }
    }
    return Serializable::Marshall(backupInfos);
}

int32_t KvStoreDataService::OnRestore(MessageParcel &data, MessageParcel &reply)
{
    SecretKeyBackupData backupData;
    if (!ParseSecretKeyFile(data, backupData) || backupData.infos.size() == 0) {
        return ReplyForRestore(reply, -1);
    }
    CloneBackupInfo backupInfo;
    bool ret = backupInfo.Unmarshal(data.ReadString());
    if (!ret || backupInfo.userId.empty()) {
        std::fill(backupInfo.encryptionInfo.symkey.begin(), backupInfo.encryptionInfo.symkey.end(), '\0');
        return ReplyForRestore(reply, -1);
    }
    
    auto iv = ConvertDecStrToVec(backupInfo.encryptionInfo.iv);
    if (iv.size() != AES_256_NONCE_SIZE) {
        ZLOGE("Iv size not correct, iv size:%{public}zu", iv.size());
        std::fill(backupInfo.encryptionInfo.symkey.begin(), backupInfo.encryptionInfo.symkey.end(), '\0');
        return ReplyForRestore(reply, -1);
    }

    if (!ImportCloneKey(backupInfo.encryptionInfo.symkey)) {
        std::fill(backupInfo.encryptionInfo.symkey.begin(), backupInfo.encryptionInfo.symkey.end(), '\0');
        DeleteCloneKey();
        return ReplyForRestore(reply, -1);
    }
    std::fill(backupInfo.encryptionInfo.symkey.begin(), backupInfo.encryptionInfo.symkey.end(), '\0');

    for (const auto &item : backupData.infos) {
        if (!item.IsValid() || !RestoreSecretKey(item, backupInfo.userId, iv)) {
            continue;
        }
    }
    DeleteCloneKey();
    return ReplyForRestore(reply, 0);
}

int32_t KvStoreDataService::ReplyForRestore(MessageParcel &reply, int32_t result)
{
    std::string replyCode = GetBackupReplyCode(result);
    if (!reply.WriteString(replyCode)) {
        return -1;
    }
    return result;
}

bool KvStoreDataService::ParseSecretKeyFile(MessageParcel &data, SecretKeyBackupData &backupData)
{
    UniqueFd fd(data.ReadFileDescriptor());
    struct stat statBuf;
    if (fd.Get() < 0 || fstat(fd.Get(), &statBuf) < 0) {
        ZLOGE("Parse backup secret key failed, fd:%{public}d, errno:%{public}d", fd.Get(), errno);
        close(fd.Release());
        return false;
    }
    char buffer[statBuf.st_size + 1];
    if (read(fd.Get(), buffer, statBuf.st_size + 1) < 0) {
        ZLOGE("Read backup secret key failed. errno:%{public}d", errno);
        close(fd.Release());
        return false;
    }
    close(fd.Release());
    std::string secretKeyStr(buffer);
    DistributedData::Serializable::Unmarshall(secretKeyStr, backupData);
    return true;
}

bool KvStoreDataService::RestoreSecretKey(const SecretKeyBackupData::BackupItem &item, const std::string &userId,
    const std::vector<uint8_t> &iv)
{
    auto sKey = DistributedData::Base64::Decode(item.sKey);
    auto cloneKeyAlias = std::vector<uint8_t>(CLONE_KEY_ALIAS, CLONE_KEY_ALIAS + strlen(CLONE_KEY_ALIAS));
    CryptoManager::CryptoParams decryptParams = { .keyAlias = cloneKeyAlias, .nonce = iv };
    auto rawKey = CryptoManager::GetInstance().Decrypt(sKey, decryptParams);
    if (rawKey.empty()) {
        ZLOGE("Decrypt failed, bundleName:%{public}s, storeName:%{public}s, storeType:%{public}d",
            item.bundleName.c_str(), Anonymous::Change(item.dbName).c_str(), item.storeType);
        sKey.assign(sKey.size(), 0);
        rawKey.assign(rawKey.size(), 0);
        return false;
    }

    StoreMetaData metaData;
    metaData.bundleName = item.bundleName;
    metaData.storeId = item.dbName;
    metaData.user = item.user == "0" ? "0" : userId;
    metaData.instanceId = item.instanceId;

    SecretKeyMetaData secretKey;
    CryptoManager::CryptoParams encryptParams = { .area = item.area, .userId = metaData.user };
    secretKey.sKey = CryptoManager::GetInstance().Encrypt(rawKey, encryptParams);
    if (secretKey.sKey.empty()) {
        ZLOGE("Encrypt failed, bundleName:%{public}s, storeName:%{public}s, storeType:%{public}d",
            item.bundleName.c_str(), Anonymous::Change(item.dbName).c_str(), item.storeType);
        sKey.assign(sKey.size(), 0);
        rawKey.assign(rawKey.size(), 0);
    }
    secretKey.storeType = item.storeType;
    secretKey.nonce = encryptParams.nonce;
    secretKey.area = item.area;
    secretKey.time = { item.time.begin(), item.time.end() };

    sKey.assign(sKey.size(), 0);
    rawKey.assign(rawKey.size(), 0);
    return MetaDataManager::GetInstance().SaveMeta(metaData.GetCloneSecretKey(), secretKey, true);
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
    // process set critical false
    Memory::MemMgrClient::GetInstance().SetCritical(getpid(), false, DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
    Memory::MemMgrClient::GetInstance().NotifyProcessStatus(getpid(), 1, 0, DISTRIBUTED_KV_DATA_SERVICE_ABILITY_ID);
}

KvStoreDataService::KvStoreClientDeathObserverImpl::KvStoreClientDeathObserverImpl(const AppId &appId,
    KvStoreDataService &service, sptr<IRemoteObject> observer, const std::string &featureName)
    : appId_(appId), dataService_(service), deathRecipient_(new KvStoreDeathRecipient(*this))
{
    ZLOGD("KvStoreClientDeathObserverImpl");
    uid_ = IPCSkeleton::GetCallingUid();
    pid_ = IPCSkeleton::GetCallingPid();
    token_ = IPCSkeleton::GetCallingTokenID();
    if (observer != nullptr) {
        observer->AddDeathRecipient(deathRecipient_);
        observerProxy_.insert_or_assign(featureName, std::move(observer));
    } else {
        ZLOGW("observer is nullptr");
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
    if (deathRecipient_ != nullptr && !observerProxy_.empty()) {
        for (auto &[key, value] : observerProxy_) {
            if (value != nullptr) {
                value->RemoveDeathRecipient(deathRecipient_);
            }
        }
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

bool KvStoreDataService::KvStoreClientDeathObserverImpl::Insert(sptr<IRemoteObject> observer,
    const std::string &featureName)
{
    if (observer != nullptr && observerProxy_.size() < MAX_CLIENT_DEATH_OBSERVER_SIZE &&
        observerProxy_.insert({featureName, observer}).second) {
        observer->AddDeathRecipient(deathRecipient_);
        return true;
    }
    return false;
}

bool KvStoreDataService::KvStoreClientDeathObserverImpl::Delete(const std::string &featureName)
{
    auto it = observerProxy_.find(featureName);
    if (it != observerProxy_.end() && it->second != nullptr) {
        it->second->RemoveDeathRecipient(deathRecipient_);
    }
    return observerProxy_.erase(featureName) != 0;
}

std::string KvStoreDataService::KvStoreClientDeathObserverImpl::GetAppId()
{
    return appId_;
}

bool KvStoreDataService::KvStoreClientDeathObserverImpl::Empty()
{
    return observerProxy_.empty();
}

KvStoreDataService::KvStoreClientDeathObserverImpl::KvStoreDeathRecipient::KvStoreDeathRecipient(
    KvStoreClientDeathObserverImpl &kvStoreClientDeathObserverImpl)
    : kvStoreClientDeathObserverImpl_(kvStoreClientDeathObserverImpl)
{
    ZLOGD("KvStore Client Death Observer");
}

KvStoreDataService::KvStoreClientDeathObserverImpl::KvStoreDeathRecipient::~KvStoreDeathRecipient()
{
    ZLOGD("~KvStore Client Death Observer");
}

void KvStoreDataService::KvStoreClientDeathObserverImpl::KvStoreDeathRecipient::OnRemoteDied(
    const wptr<IRemoteObject> &remote)
{
    (void) remote;
    if (!clientDead_.exchange(true)) {
        kvStoreClientDeathObserverImpl_.NotifyClientDie();
    }
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
            MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({}), metaData, true);
            for (const auto &meta : metaData) {
                if (meta.user != eventInfo.userId) {
                    continue;
                }
                ZLOGI("StoreMetaData bundleName:%{public}s, user:%{public}s", meta.bundleName.c_str(),
                    meta.user.c_str());
                MetaDataManager::GetInstance().DelMeta(meta.GetKeyWithoutPath());
                MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true);
                MetaDataManager::GetInstance().DelMeta(meta.GetKeyLocal(), true);
                MetaDataManager::GetInstance().DelMeta(meta.GetSecretKey(), true);
                MetaDataManager::GetInstance().DelMeta(meta.GetStrategyKey());
                MetaDataManager::GetInstance().DelMeta(meta.GetBackupSecretKey(), true);
                MetaDataManager::GetInstance().DelMeta(meta.GetAutoLaunchKey(), true);
                MetaDataManager::GetInstance().DelMeta(meta.appId, true);
                MetaDataManager::GetInstance().DelMeta(meta.GetDebugInfoKey(), true);
                MetaDataManager::GetInstance().DelMeta(meta.GetDfxInfoKey(), true);
                MetaDataManager::GetInstance().DelMeta(meta.GetCloneSecretKey(), true);
                MetaDataManager::GetInstance().DelMeta(StoreMetaMapping(meta).GetKey(), true);
                PermitDelegate::GetInstance().DelCache(meta.GetKeyWithoutPath());
            }
            CheckerManager::GetInstance().ClearCache();
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

int32_t KvStoreDataService::OnUninstall(const AppDistributedKv::BundleEventInfo &bundleEventInfo)
{
    CheckerManager::GetInstance().DeleteCache(
        bundleEventInfo.bundleName, bundleEventInfo.userId, bundleEventInfo.appIndex);
    auto staticActs = FeatureSystem::GetInstance().GetStaticActs();
    staticActs.ForEachCopies([bundleEventInfo](const auto &, const std::shared_ptr<StaticActs> &acts) {
        acts->OnAppUninstall(bundleEventInfo.bundleName, bundleEventInfo.userId,
            bundleEventInfo.appIndex, bundleEventInfo.tokenId);
        return false;
    });
    return SUCCESS;
}

int32_t KvStoreDataService::OnUpdate(const AppDistributedKv::BundleEventInfo &bundleEventInfo)
{
    auto bundleName = bundleEventInfo.bundleName;
    auto user = bundleEventInfo.userId;
    auto index = bundleEventInfo.appIndex;
    CheckerManager::GetInstance().DeleteCache(bundleName, user, index);
    auto staticActs = FeatureSystem::GetInstance().GetStaticActs();
    staticActs.ForEachCopies([bundleName, user, index](const auto &, const std::shared_ptr<StaticActs> &acts) {
        acts->OnAppUpdate(bundleName, user, index);
        return false;
    });
    return SUCCESS;
}

int32_t KvStoreDataService::OnInstall(const AppDistributedKv::BundleEventInfo &bundleEventInfo)
{
    auto bundleName = bundleEventInfo.bundleName;
    auto user = bundleEventInfo.userId;
    auto index = bundleEventInfo.appIndex;
    CheckerManager::GetInstance().DeleteCache(bundleName, user, index);
    auto staticActs = FeatureSystem::GetInstance().GetStaticActs();
    staticActs.ForEachCopies([bundleName, user, index](const auto &, const std::shared_ptr<StaticActs> &acts) {
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
    CheckerManager::GetInstance().DeleteCache(bundleName, userId, appIndex);
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
            ZLOGI("StoreMetaData data cleared bundleName:%{public}s, stordId:%{public}s, appIndex:%{public}d",
                bundleName.c_str(), Anonymous::Change(meta.storeId).c_str(), appIndex);
            MetaDataManager::GetInstance().DelMeta(meta.GetKeyWithoutPath());
            MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetKeyLocal(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetSecretKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetStrategyKey());
            MetaDataManager::GetInstance().DelMeta(meta.GetBackupSecretKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.appId, true);
            MetaDataManager::GetInstance().DelMeta(meta.GetDebugInfoKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetDfxInfoKey(), true);
            MetaDataManager::GetInstance().DelMeta(meta.GetAutoLaunchKey(), true);
            MetaDataManager::GetInstance().DelMeta(StoreMetaMapping(meta).GetKey(), true);
            PermitDelegate::GetInstance().DelCache(meta.GetKeyWithoutPath());
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
    int user = 0;
    auto ret = AccountDelegate::GetInstance()->QueryForegroundUserId(user);
    if (!ret) {
        ZLOGE("get foreground userid failed");
        return;
    }
    if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ localDeviceId, std::to_string(user) }),
        metas, true)) {
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
    int user = 0;
    auto ret = AccountDelegate::GetInstance()->QueryForegroundUserId(user);
    if (!ret) {
        ZLOGE("get foreground userid failed");
        return;
    }
    if (!MetaDataManager::GetInstance().LoadMeta(StoreMetaData::GetPrefix({ localDeviceId, std::to_string(user) }),
        metas, true)) {
        ZLOGE("get full meta failed");
        return;
    }
    FilterData(metas, params);
    std::map<std::string, BundleInfo> datas;
    BuildData(datas, metas);
    PrintfInfo(fd, datas);
}
} // namespace OHOS::DistributedKv
