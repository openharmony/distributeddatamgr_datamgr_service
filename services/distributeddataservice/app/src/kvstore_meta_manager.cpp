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
#define LOG_TAG "KvStoreMetaManager"

#include "kvstore_meta_manager.h"
#include <chrono>
#include <condition_variable>
#include <directory_ex.h>
#include <file_ex.h>
#include <ipc_skeleton.h>
#include <thread>
#include <unistd.h>
#include "account_delegate.h"
#include "bootstrap.h"
#include "communication_provider.h"
#include "constant.h"
#include "crypto_manager.h"
#include "device_manager_adapter.h"
#include "directory_utils.h"
#include "dump_helper.h"
#include "executor_factory.h"
#include "kvstore_data_service.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "metadata/capability_meta_data.h"
#include "metadata/user_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "rdb_types.h"
#include "reporter.h"
#include "serializable/serializable.h"
#include "user_delegate.h"
#include "utils/crypto.h"

namespace OHOS {
namespace DistributedKv {
using json = nlohmann::json;
using Commu = AppDistributedKv::CommunicationProvider;
using DmAdapter = DistributedData::DeviceManagerAdapter;
using namespace std::chrono;
using namespace OHOS::DistributedData;

// APPID: distributeddata
// USERID: default
// STOREID: service_meta
// dataDir: /data/misc_de/0/mdds/Meta/${storeId}/sin_gen.db
std::condition_variable KvStoreMetaManager::cv_;
std::mutex KvStoreMetaManager::cvMutex_;
KvStoreMetaManager::MetaDeviceChangeListenerImpl KvStoreMetaManager::listener_;

KvStoreMetaManager::KvStoreMetaManager()
    : metaDelegate_(nullptr),
      metaDBDirectory_("/data/service/el1/public/database/distributeddata/meta"),
      label_(Bootstrap::GetInstance().GetProcessLabel()),
      kvStoreDelegateManager_(Bootstrap::GetInstance().GetProcessLabel(), "default")
{
    ZLOGI("begin.");
}

KvStoreMetaManager::~KvStoreMetaManager()
{
}

KvStoreMetaManager &KvStoreMetaManager::GetInstance()
{
    static KvStoreMetaManager instance;
    return instance;
}

void KvStoreMetaManager::SubscribeMeta(const std::string &keyPrefix, const ChangeObserver &observer)
{
    metaObserver_.handlerMap_[keyPrefix] = observer;
}

void KvStoreMetaManager::InitMetaListener()
{
    InitMetaData();
    auto status = AppDistributedKv::CommunicationProvider::GetInstance().StartWatchDeviceChange(
        &listener_, { "metaMgr" });
    if (status != AppDistributedKv::Status::SUCCESS) {
        ZLOGW("register failed.");
        return;
    }
    ZLOGI("register meta device change success.");
    SubscribeMetaKvStore();
}

void KvStoreMetaManager::InitMetaData()
{
    ZLOGI("start.");
    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        ZLOGI("get meta failed.");
        return;
    }
    auto uid = getuid();
    const std::string accountId = AccountDelegate::GetInstance()->GetCurrentAccountId();
    const std::string userId = AccountDelegate::GetInstance()->GetDeviceAccountIdByUID(uid);
    StoreMetaData data;
    data.appId = label_;
    data.appType = "default";
    data.bundleName = label_;
    data.dataDir = metaDBDirectory_;
    data.user = userId;
    data.deviceId = Commu::GetInstance().GetLocalDevice().uuid;
    data.isAutoSync = false;
    data.isBackup = false;
    data.isEncrypt = false;
    data.storeType = KvStoreType::SINGLE_VERSION;
    data.schema = "";
    data.storeId = Constant::SERVICE_META_DB_NAME;
    data.account = accountId;
    data.uid = static_cast<int32_t>(uid);
    data.version = META_STORE_VERSION;
    data.securityLevel = SecurityLevel::S1;
    data.area = EL1;
    data.tokenId = IPCSkeleton::GetCallingTokenID();
    if (!MetaDataManager::GetInstance().SaveMeta(data.GetKey(), data)) {
        ZLOGE("save meta fail");
    }
    ZLOGI("end.");
}

void KvStoreMetaManager::InitMetaParameter()
{
    ZLOGI("start.");
    std::thread th = std::thread([]() {
        constexpr int RETRY_MAX_TIMES = 100;
        int retryCount = 0;
        constexpr int RETRY_TIME_INTERVAL_MILLISECOND = 1 * 1000 * 1000; // retry after 1 second
        while (retryCount < RETRY_MAX_TIMES) {
            auto status = CryptoManager::GetInstance().CheckRootKey();
            if (status == CryptoManager::ErrCode::SUCCESS) {
                ZLOGI("root key exist.");
                break;
            }
            if (status == CryptoManager::ErrCode::NOT_EXIST &&
                CryptoManager::GetInstance().GenerateRootKey() == CryptoManager::ErrCode::SUCCESS) {
                ZLOGI("GenerateRootKey success.");
                break;
            }
            retryCount++;
            ZLOGE("GenerateRootKey failed.");
            usleep(RETRY_TIME_INTERVAL_MILLISECOND);
        }
    });
    th.detach();

    bool ret = ForceCreateDirectory(metaDBDirectory_);
    if (!ret) {
        DumpHelper::GetInstance().AddErrorInfo("InitMetaParameter: user create directories failed.");
        ZLOGE("create directories failed");
        return;
    }
    ForceCreateDirectory(metaDBDirectory_ + "/backup");
    DistributedDB::KvStoreConfig kvStoreConfig {metaDBDirectory_};
    kvStoreDelegateManager_.SetKvStoreConfig(kvStoreConfig);
}

KvStoreMetaManager::NbDelegate KvStoreMetaManager::GetMetaKvStore()
{
    if (metaDelegate_ != nullptr) {
        return metaDelegate_;
    }

    std::lock_guard<decltype(mutex_)> lock(mutex_);
    if (metaDelegate_ == nullptr) {
        metaDelegate_ = CreateMetaKvStore();
    }
    ConfigMetaDataManager();
    return metaDelegate_;
}

KvStoreMetaManager::NbDelegate KvStoreMetaManager::CreateMetaKvStore()
{
    DistributedDB::DBStatus dbStatusTmp = DistributedDB::DBStatus::NOT_SUPPORT;
    DistributedDB::KvStoreNbDelegate::Option option;
    option.createIfNecessary = true;
    option.isMemoryDb = false;
    option.createDirByStoreIdOnly = true;
    option.isEncryptedDb = false;
    option.isNeedRmCorruptedDb = true;
    DistributedDB::KvStoreNbDelegate *kvStoreNbDelegatePtr = nullptr;
    kvStoreDelegateManager_.GetKvStore(
        Constant::SERVICE_META_DB_NAME, option,
        [&kvStoreNbDelegatePtr, &dbStatusTmp](DistributedDB::DBStatus dbStatus,
                                              DistributedDB::KvStoreNbDelegate *kvStoreNbDelegate) {
            kvStoreNbDelegatePtr = kvStoreNbDelegate;
            dbStatusTmp = dbStatus;
        });

    if (dbStatusTmp != DistributedDB::DBStatus::OK) {
        ZLOGE("GetKvStore return error status: %{public}d", static_cast<int>(dbStatusTmp));
        return nullptr;
    }
    auto release = [this](DistributedDB::KvStoreNbDelegate *delegate) {
        ZLOGI("release meta data  kv store");
        if (delegate == nullptr) {
            return;
        }

        auto result = kvStoreDelegateManager_.CloseKvStore(delegate);
        if (result != DistributedDB::DBStatus::OK) {
            ZLOGE("CloseMetaKvStore return error status: %{public}d", static_cast<int>(result));
        }
    };
    return NbDelegate(kvStoreNbDelegatePtr, release);
}

void KvStoreMetaManager::ConfigMetaDataManager()
{
    std::initializer_list<std::string> backList = {label_, "_", Constant::SERVICE_META_DB_NAME};
    std::string fileName = Constant::Concatenate(backList);
    std::initializer_list<std::string> backFull = { metaDBDirectory_, "/backup/",
        DistributedData::Crypto::Sha256(fileName)};
    auto fullName = Constant::Concatenate(backFull);
    auto backup = [fullName](const auto &store) -> int32_t {
        DistributedDB::CipherPassword password;
        return store->Export(fullName, password);
    };
    auto syncer = [](const auto &store, int32_t status) {
        ZLOGI("Syncer status: %{public}d", status);
        std::vector<std::string> devs;
        auto devices = AppDistributedKv::CommunicationProvider::GetInstance().GetRemoteDevices();
        for (auto const &dev : devices) {
            devs.push_back(dev.uuid);
        }

        if (devs.empty()) {
            ZLOGW("no devices need sync meta data.");
            return;
        }

        status = store->Sync(devs, DistributedDB::SyncMode::SYNC_MODE_PUSH_PULL, [](auto &) {
            ZLOGD("meta data sync completed.");
        });

        if (status != DistributedDB::OK) {
            ZLOGW("meta data sync error %{public}d.", status);
        }
    };
    MetaDataManager::GetInstance().Initialize(metaDelegate_, backup, syncer);
}

std::vector<uint8_t> KvStoreMetaManager::GetMetaKey(const std::string &deviceAccountId,
                                                    const std::string &groupId, const std::string &bundleName,
                                                    const std::string &storeId, const std::string &key)
{
    std::string originKey;
    if (key.empty()) {
        originKey = DmAdapter::GetInstance().GetLocalDevice().uuid + Constant::KEY_SEPARATOR +
                    deviceAccountId + Constant::KEY_SEPARATOR +
                    groupId + Constant::KEY_SEPARATOR +
                    bundleName + Constant::KEY_SEPARATOR +
                    storeId;
        return KvStoreMetaRow::GetKeyFor(originKey);
    }

    originKey = deviceAccountId + Constant::KEY_SEPARATOR +
                groupId + Constant::KEY_SEPARATOR +
                bundleName + Constant::KEY_SEPARATOR +
                storeId + Constant::KEY_SEPARATOR +
                key;
    return SecretMetaRow::GetKeyFor(originKey);
}

Status KvStoreMetaManager::CheckUpdateServiceMeta(const std::vector<uint8_t> &metaKey, FLAG flag,
                                                  const std::vector<uint8_t> &val)
{
    ZLOGD("begin.");
    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        ZLOGE("GetMetaKvStore return nullptr.");
        return Status::DB_ERROR;
    }

    DistributedDB::Key dbKey = metaKey;
    DistributedDB::Value dbValue = val;
    DistributedDB::DBStatus dbStatus;
    DistributedDB::CipherPassword dbPassword;
    std::initializer_list<std::string> backList = {label_, "_", Constant::SERVICE_META_DB_NAME};
    std::string backupName = Constant::Concatenate(backList);
    std::initializer_list<std::string> backFullList = {metaDBDirectory_, "/backup/",
        DistributedData::Crypto::Sha256(backupName)};
    auto backupFullName = Constant::Concatenate(backFullList);

    switch (flag) {
        case UPDATE:
            dbStatus = metaDelegate->Put(dbKey, dbValue);
            metaDelegate->Export(backupFullName, dbPassword);
            break;
        case DELETE:
            dbStatus = metaDelegate->Delete(dbKey);
            metaDelegate->Export(backupFullName, dbPassword);
            break;
        case CHECK_EXIST:
            dbStatus = metaDelegate->Get(dbKey, dbValue);
            break;
        case UPDATE_LOCAL:
            dbStatus = metaDelegate->PutLocal(dbKey, dbValue);
            metaDelegate->Export(backupFullName, dbPassword);
            break;
        case DELETE_LOCAL:
            dbStatus = metaDelegate->DeleteLocal(dbKey);
            metaDelegate->Export(backupFullName, dbPassword);
            break;
        case CHECK_EXIST_LOCAL:
            dbStatus = metaDelegate->GetLocal(dbKey, dbValue);
            break;
        default:
            break;
    }
    ZLOGI("Flag: %{public}d status: %{public}d", static_cast<int>(flag), static_cast<int>(dbStatus));
    SyncMeta();
    return (dbStatus != DistributedDB::DBStatus::OK) ? Status::DB_ERROR : Status::SUCCESS;
}

Status KvStoreMetaManager::WriteSecretKeyToMeta(const std::vector<uint8_t> &metaKey, const std::vector<uint8_t> &key)
{
    ZLOGD("start");
    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        ZLOGE("GetMetaKvStore return nullptr.");
        return Status::DB_ERROR;
    }

    SecretKeyMetaData secretKey;
    secretKey.kvStoreType = KvStoreType::DEVICE_COLLABORATION;
    secretKey.timeValue = TransferTypeToByteArray<time_t>(system_clock::to_time_t(system_clock::now()));
    secretKey.secretKey = CryptoManager::GetInstance().Encrypt(key);
    if (secretKey.secretKey.empty()) {
        ZLOGE("encrypt work key error.");
        return Status::CRYPT_ERROR;
    }

    DistributedDB::DBStatus dbStatus = metaDelegate->PutLocal(metaKey, secretKey);
    if (dbStatus != DistributedDB::DBStatus::OK) {
        ZLOGE("end with %d", static_cast<int>(dbStatus));
        return Status::DB_ERROR;
    } else {
        ZLOGD("normal end");
        return Status::SUCCESS;
    }
}

Status KvStoreMetaManager::WriteSecretKeyToFile(const std::string &secretKeyFile, const std::vector<uint8_t> &key)
{
    ZLOGD("start");
    std::vector<uint8_t> secretKey = CryptoManager::GetInstance().Encrypt(key);
    if (secretKey.empty()) {
        ZLOGW("encrypt work key error.");
        return Status::CRYPT_ERROR;
    }
    std::string dbDir = secretKeyFile.substr(0, secretKeyFile.find_last_of('/'));
    if (!ForceCreateDirectory(dbDir)) {
        return Status::ERROR;
    }

    std::vector<uint8_t> secretKeyInByte =
        TransferTypeToByteArray<time_t>(system_clock::to_time_t(system_clock::now()));
    std::vector<char> secretKeyInChar;
    secretKeyInChar.insert(secretKeyInChar.end(), secretKeyInByte.begin(), secretKeyInByte.end());
    secretKeyInChar.insert(secretKeyInChar.end(), secretKey.begin(), secretKey.end());
    if (SaveBufferToFile(secretKeyFile, secretKeyInChar)) {
        ZLOGD("normal end");
        return Status::SUCCESS;
    }
    ZLOGW("failure end");
    return Status::ERROR;
}

Status KvStoreMetaManager::GetSecretKeyFromMeta(const std::vector<uint8_t> &metaSecretKey, std::vector<uint8_t> &key,
                                                bool &outdated)
{
    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        ZLOGE("GetMetaKvStore return nullptr.");
        return Status::DB_ERROR;
    }

    DistributedDB::Key dbKey = metaSecretKey;
    DistributedDB::Value dbValue;
    DistributedDB::DBStatus dbStatus = metaDelegate->GetLocal(dbKey, dbValue);
    if (dbStatus != DistributedDB::DBStatus::OK) {
        return Status::DB_ERROR;
    }
    std::string jsonStr(dbValue.begin(), dbValue.end());
    json jsonObj = json::parse(jsonStr, nullptr, false);
    if (jsonObj.is_discarded()) {
        ZLOGE("parse json error");
        return Status::ERROR;
    }
    SecretKeyMetaData sKeyValue(jsonObj);
    time_t createTime = TransferByteArrayToType<time_t>(sKeyValue.timeValue);
    if (!CryptoManager::GetInstance().Decrypt(sKeyValue.secretKey, key)) {
        return Status::ERROR;
    }
    system_clock::time_point createTimeChrono = system_clock::from_time_t(createTime);
    outdated = ((createTimeChrono + hours(HOURS_PER_YEAR)) < system_clock::now()); // secretKey valid for 1 year.
    return Status::SUCCESS;
}

Status KvStoreMetaManager::RecoverSecretKeyFromFile(const std::string &secretKeyFile,
                                                    const std::vector<uint8_t> &metaSecretKey,
                                                    std::vector<uint8_t> &key, bool &outdated)
{
    std::vector<char> fileBuffer;
    if (!LoadBufferFromFile(secretKeyFile, fileBuffer)) {
        return Status::ERROR;
    }
    if (fileBuffer.size() < sizeof(time_t) / sizeof(uint8_t) + KEY_SIZE) {
        return Status::ERROR;
    }
    std::vector<uint8_t> timeVec;
    auto iter = fileBuffer.begin();
    for (int i = 0; i < static_cast<int>(sizeof(time_t) / sizeof(uint8_t)); i++) {
        timeVec.push_back(*iter);
        iter++;
    }
    time_t createTime = TransferByteArrayToType<time_t>(timeVec);
    SecretKeyMetaData secretKey;
    secretKey.secretKey.insert(secretKey.secretKey.end(), iter, fileBuffer.end());
    if (!CryptoManager::GetInstance().Decrypt(secretKey.secretKey, key)) {
        return Status::ERROR;
    }
    system_clock::time_point createTimeChrono = system_clock::from_time_t(createTime);
    outdated = ((createTimeChrono + hours(HOURS_PER_YEAR)) < system_clock::now()); // secretKey valid for 1 year.

    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        ZLOGE("GetMetaKvStore return nullptr.");
        return Status::DB_ERROR;
    }

    secretKey.timeValue = TransferTypeToByteArray<time_t>(createTime);
    secretKey.kvStoreType = KvStoreType::DEVICE_COLLABORATION;

    DistributedDB::DBStatus dbStatus = metaDelegate->PutLocal(metaSecretKey, secretKey);
    if (dbStatus != DistributedDB::DBStatus::OK) {
        ZLOGE("put work key failed.");
        return Status::DB_ERROR;
    }
    return Status::SUCCESS;
}

std::vector<uint8_t> KvStoreMetaManager::GetSecretKeyFromFile(const std::string &secretKeyFile)
{
    std::vector<char> fileBuffer;
    if (!LoadBufferFromFile(secretKeyFile, fileBuffer)) {
        return {};
    }
    if (fileBuffer.size() < sizeof(time_t) / sizeof(uint8_t) + KEY_SIZE) {
        return {};
    }
    auto iter = fileBuffer.begin() + (sizeof(time_t) / sizeof(uint8_t));

    SecretKeyMetaData secretKey;
    secretKey.secretKey.insert(secretKey.secretKey.end(), iter, fileBuffer.end());
    std::vector<uint8_t> key;
    CryptoManager::GetInstance().Decrypt(secretKey.secretKey, key);
    return key;
}

// StrategyMetaData###deviceId###deviceAccountID###${groupId}###bundleName###storeId
void KvStoreMetaManager::GetStrategyMetaKey(const StrategyMeta &params, std::string &retVal)
{
    std::vector<std::string> keys = {STRATEGY_META_PREFIX, params.devId, params.devAccId, params.grpId,
                                     params.bundleName, params.storeId};
    ConcatWithSharps(keys, retVal);
}

void KvStoreMetaManager::ConcatWithSharps(const std::vector<std::string> &params, std::string &retVal)
{
    int32_t len = static_cast<int32_t>(params.size());
    for (int32_t i = 0; i < len; i++) {
        retVal.append(params.at(i));
        if (i != (len - 1)) {
            retVal.append(Constant::KEY_SEPARATOR);
        }
    }
}

Status KvStoreMetaManager::SaveStrategyMetaEnable(const std::string &key, bool enable)
{
    ZLOGD("begin");
    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        return Status::ERROR;
    }
    auto dbkey = std::vector<uint8_t>(key.begin(), key.end());
    std::vector<uint8_t> values;
    auto dbStatus = metaDelegate->Get(dbkey, values);
    if (dbStatus == DistributedDB::DBStatus::NOT_FOUND) {
        json j;
        j[CAPABILITY_ENABLED] = enable;
        std::string json = j.dump();
        if (metaDelegate->Put(dbkey, std::vector<uint8_t>(json.begin(), json.end())) != DistributedDB::OK) {
            ZLOGE("save failed.");
            return Status::DB_ERROR;
        }
        ZLOGD("save end");
    } else if (dbStatus == DistributedDB::DBStatus::OK) {
        std::string jsonStr(values.begin(), values.end());
        auto jsonObj = json::parse(jsonStr, nullptr, false);
        if (jsonObj.is_discarded()) {
            ZLOGE("invalid json.");
            return Status::ERROR;
        }
        jsonObj[CAPABILITY_ENABLED] = enable;
        std::string json = jsonObj.dump();
        if (metaDelegate->Put(dbkey, std::vector<uint8_t>(json.begin(), json.end())) != DistributedDB::OK) {
            ZLOGE("save failed.");
            return Status::DB_ERROR;
        }
        ZLOGD("update end");
    } else {
        ZLOGE("failed.");
        return Status::DB_ERROR;
    }
    SyncMeta();
    return Status::SUCCESS;
}

Status KvStoreMetaManager::SaveStrategyMetaLabels(const std::string &key,
                                                  const std::vector<std::string> &localLabels,
                                                  const std::vector<std::string> &remoteSupportLabels)
{
    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        return Status::ERROR;
    }
    auto dbkey = std::vector<uint8_t>(key.begin(), key.end());
    std::vector<uint8_t> values;
    auto dbStatus = metaDelegate->Get(dbkey, values);
    if (dbStatus == DistributedDB::DBStatus::NOT_FOUND) {
        json j;
        j[CAPABILITY_RANGE][LOCAL_LABEL] = localLabels;
        j[CAPABILITY_RANGE][REMOTE_LABEL] = remoteSupportLabels;
        std::string metaJson = j.dump();
        if (metaDelegate->Put(dbkey, std::vector<uint8_t>(metaJson.begin(), metaJson.end())) != DistributedDB::OK) {
            ZLOGE("save failed.");
            return Status::DB_ERROR;
        }
    } else if (dbStatus == DistributedDB::DBStatus::OK) {
        std::string jsonStr(values.begin(), values.end());
        auto j = json::parse(jsonStr, nullptr, false);
        if (j.is_discarded()) {
            return Status::ERROR;
        }
        j[CAPABILITY_RANGE][LOCAL_LABEL] = localLabels;
        j[CAPABILITY_RANGE][REMOTE_LABEL] = remoteSupportLabels;
        std::string metaJson = j.dump();
        if (metaDelegate->Put(dbkey, std::vector<uint8_t>(metaJson.begin(), metaJson.end())) != DistributedDB::OK) {
            ZLOGE("save failed.");
            return Status::DB_ERROR;
        }
    } else {
        ZLOGE("failed.");
        return Status::DB_ERROR;
    }
    SyncMeta();
    return Status::SUCCESS;
}

void KvStoreMetaManager::SyncMeta()
{
    std::vector<std::string> devs;
    auto deviceList = AppDistributedKv::CommunicationProvider::GetInstance().GetRemoteDevices();
    for (auto const &dev : deviceList) {
        devs.push_back(dev.uuid);
    }

    if (devs.empty()) {
        ZLOGW("meta db sync fail, devices is empty.");
        return;
    }

    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        ZLOGW("meta db sync failed.");
        return;
    }
    auto onComplete = [this](const std::map<std::string, DistributedDB::DBStatus> &) {
        ZLOGD("meta db sync complete.");
        cv_.notify_all();
        ZLOGD("meta db sync complete end.");
    };
    auto dbStatus = metaDelegate->Sync(devs, DistributedDB::SyncMode::SYNC_MODE_PUSH_PULL, onComplete);
    if (dbStatus != DistributedDB::OK) {
        ZLOGW("meta db sync error %d.", dbStatus);
    }
}

void KvStoreMetaManager::SubscribeMetaKvStore()
{
    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        ZLOGW("register meta observer failed.");
        return;
    }

    int mode = DistributedDB::OBSERVER_CHANGES_NATIVE | DistributedDB::OBSERVER_CHANGES_FOREIGN;
    auto dbStatus = metaDelegate->RegisterObserver(DistributedDB::Key(), mode, &metaObserver_);
    if (dbStatus != DistributedDB::DBStatus::OK) {
        ZLOGW("register meta observer failed :%{public}d.", dbStatus);
    }
}

Status KvStoreMetaManager::GetStategyMeta(const std::string &key,
                                          std::map<std::string, std::vector<std::string>> &strategies)
{
    ZLOGD("get meta key:%s.", key.c_str());
    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        ZLOGW("get delegate error.");
        return Status::ERROR;
    }

    DistributedDB::Value values;
    auto dbStatus = metaDelegate->Get(DistributedDB::Key(key.begin(), key.end()), values);
    if (dbStatus != DistributedDB::DBStatus::OK) {
        ZLOGW("get meta error %d.", dbStatus);
        return Status::DB_ERROR;
    }

    std::string jsonStr(values.begin(), values.end());
    auto jsonObj = json::parse(jsonStr, nullptr, false);
    if (jsonObj.is_discarded()) {
        jsonObj = json::parse(jsonStr.substr(1), nullptr, false); // 1 drop for a
        if (jsonObj.is_discarded()) {
            ZLOGW("get meta parse error.");
            return Status::ERROR;
        }
    }

    auto range = jsonObj.find(CAPABILITY_RANGE);
    if (range == jsonObj.end()) {
        ZLOGW("get meta parse no range.");
        return Status::ERROR;
    }

    auto local = range->find(LOCAL_LABEL);
    if (local != range->end()) {
        json obj = *local;
        if (obj.is_array()) {
            std::vector <std::string> v;
            obj.get_to(v);
            strategies.insert({LOCAL_LABEL, v});
        }
    }
    auto remote = range->find(REMOTE_LABEL);
    if (remote != range->end()) {
        json obj = *remote;
        if (obj.is_array()) {
            std::vector <std::string> v;
            obj.get_to(v);
            strategies.insert({REMOTE_LABEL, v});
        }
    }
    return Status::SUCCESS;
}

KvStoreMetaManager::KvStoreMetaObserver::~KvStoreMetaObserver()
{
    ZLOGW("meta observer destruct.");
}

void KvStoreMetaManager::KvStoreMetaObserver::OnChange(const DistributedDB::KvStoreChangedData &data)
{
    ZLOGD("on data change.");
    HandleChanges(CHANGE_FLAG::INSERT, data.GetEntriesInserted());
    HandleChanges(CHANGE_FLAG::UPDATE, data.GetEntriesUpdated());
    HandleChanges(CHANGE_FLAG::DELETE, data.GetEntriesDeleted());
    KvStoreMetaManager::GetInstance().SyncMeta();
}

void KvStoreMetaManager::KvStoreMetaObserver::HandleChanges(
    CHANGE_FLAG flag, const std::list<DistributedDB::Entry> &entries)
{
    for (const auto &entry : entries) {
        std::string key(entry.key.begin(), entry.key.end());
        for (const auto &item : handlerMap_) {
            ZLOGI("flag:%{public}d, key:%{public}s", flag, key.c_str());
            if (key.find(item.first) == 0) {
                item.second(entry.key, entry.value, flag);
            }
        }
    }
}

void KvStoreMetaManager::MetaDeviceChangeListenerImpl::OnDeviceChanged(
    const AppDistributedKv::DeviceInfo &info, const AppDistributedKv::DeviceChangeType &type) const
{
    if (type == AppDistributedKv::DeviceChangeType::DEVICE_OFFLINE) {
        ZLOGD("offline ignore.");
        return;
    }

    ZLOGD("begin to sync.");
    KvStoreMetaManager::GetInstance().SyncMeta();
    ZLOGD("end.");
}

AppDistributedKv::ChangeLevelType KvStoreMetaManager::MetaDeviceChangeListenerImpl::GetChangeLevelType() const
{
    return AppDistributedKv::ChangeLevelType::LOW;
}

Status KvStoreMetaManager::QueryKvStoreMetaDataByDeviceIdAndAppId(const std::string &devId, const std::string &appId,
                                                                  KvStoreMetaData &val)
{
    ZLOGD("query meta start.");
    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        ZLOGW("get delegate error.");
        return Status::ERROR;
    }
    std::string dbPrefixKey;
    std::string prefix = KvStoreMetaRow::KEY_PREFIX;
    ConcatWithSharps({prefix, devId}, dbPrefixKey);
    std::vector<DistributedDB::Entry> values;
    auto status = metaDelegate->GetEntries(DistributedDB::Key(dbPrefixKey.begin(), dbPrefixKey.end()), values);
    if (status != DistributedDB::DBStatus::OK) {
        status = metaDelegate->GetEntries(DistributedDB::Key(prefix.begin(), prefix.end()), values);
        if (status != DistributedDB::DBStatus::OK) {
            ZLOGW("query db failed key:%s, ret:%d.", dbPrefixKey.c_str(), static_cast<int>(status));
            return Status::ERROR;
        }
    }

    for (auto const &entry : values) {
        std::string str(entry.value.begin(), entry.value.end());
        json j = Serializable::ToJson(str);
        val.Unmarshal(j);
        if (val.appId == appId) {
            ZLOGD("query meta success.");
            return Status::SUCCESS;
        }
    }

    ZLOGW("find meta failed id: %{public}s", appId.c_str());
    return Status::ERROR;
}

Status KvStoreMetaManager::GetKvStoreMeta(const std::vector<uint8_t> &metaKey, KvStoreMetaData &metaData)
{
    ZLOGD("begin.");
    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        ZLOGE("GetMetaKvStore return nullptr.");
        return Status::DB_ERROR;
    }
    DistributedDB::Value dbValue;
    DistributedDB::DBStatus dbStatus = metaDelegate->Get(metaKey, dbValue);
    ZLOGI("status: %{public}d", static_cast<int>(dbStatus));
    if (dbStatus == DistributedDB::DBStatus::NOT_FOUND) {
        ZLOGI("key not found.");
        return Status::KEY_NOT_FOUND;
    }
    if (dbStatus != DistributedDB::DBStatus::OK) {
        ZLOGE("GetKvStoreMeta failed.");
        return Status::DB_ERROR;
    }

    std::string jsonStr(dbValue.begin(), dbValue.end());
    metaData.Unmarshal(Serializable::ToJson(jsonStr));
    return Status::SUCCESS;
}

std::string KvStoreMetaData::Marshal() const
{
    json jval = {
        {DEVICE_ID, deviceId},
        {USER_ID, userId},
        {APP_ID, appId},
        {STORE_ID, storeId},
        {BUNDLE_NAME, bundleName},
        {KVSTORE_TYPE, kvStoreType},
        {ENCRYPT, isEncrypt},
        {BACKUP, isBackup},
        {AUTO_SYNC, isAutoSync},
        {SCHEMA, schema},
        {DATA_DIR, dataDir}, // Reserved for kvstore data storage directory.
        {APP_TYPE, appType}, // Reserved for the APP type which used kvstore.
        {DEVICE_ACCOUNT_ID, deviceAccountId},
        {UID, uid},
        {VERSION, version},
        {SECURITY_LEVEL, securityLevel},
        {DIRTY_KEY, isDirty},
        {TOKEN_ID, tokenId},
    };
    return jval.dump();
}

json Serializable::ToJson(const std::string &jsonStr)
{
    json jsonObj = json::parse(jsonStr, nullptr, false);
    if (jsonObj.is_discarded()) {
        // if the string size is less than 1, means the string is invalid.
        if (jsonStr.empty()) {
            ZLOGE("empty jsonStr, error.");
            return {};
        }
        jsonObj = json::parse(jsonStr.substr(1), nullptr, false); // drop first char to adapt A's value;
        if (jsonObj.is_discarded()) {
            ZLOGE("parse jsonStr, error.");
            return {};
        }
    }
    return jsonObj;
}

void KvStoreMetaData::Unmarshal(const nlohmann::json &jObject)
{
    kvStoreType = Serializable::GetVal<KvStoreType>(jObject, KVSTORE_TYPE, json::value_t::number_unsigned, kvStoreType);
    isBackup = Serializable::GetVal<bool>(jObject, BACKUP, json::value_t::boolean, isBackup);
    isEncrypt = Serializable::GetVal<bool>(jObject, ENCRYPT, json::value_t::boolean, isEncrypt);
    isAutoSync = Serializable::GetVal<bool>(jObject, AUTO_SYNC, json::value_t::boolean, isAutoSync);
    appId = Serializable::GetVal<std::string>(jObject, APP_ID, json::value_t::string, appId);
    userId = Serializable::GetVal<std::string>(jObject, USER_ID, json::value_t::string, userId);
    storeId = Serializable::GetVal<std::string>(jObject, STORE_ID, json::value_t::string, storeId);
    bundleName = Serializable::GetVal<std::string>(jObject, BUNDLE_NAME, json::value_t::string, bundleName);
    deviceAccountId = Serializable::GetVal<std::string>(jObject, DEVICE_ACCOUNT_ID, json::value_t::string,
                                                        deviceAccountId);
    dataDir = Serializable::GetVal<std::string>(jObject, DATA_DIR, json::value_t::string, dataDir);
    appType = Serializable::GetVal<std::string>(jObject, APP_TYPE, json::value_t::string, appType);
    deviceId = Serializable::GetVal<std::string>(jObject, DEVICE_ID, json::value_t::string, deviceId);
    schema = Serializable::GetVal<std::string>(jObject, SCHEMA, json::value_t::string, schema);
    uid = Serializable::GetVal<int32_t>(jObject, UID, json::value_t::number_unsigned, uid);
    version = Serializable::GetVal<uint32_t>(jObject, VERSION, json::value_t::number_unsigned, version);
    securityLevel = Serializable::GetVal<uint32_t>(jObject, SECURITY_LEVEL, json::value_t::number_unsigned,
                                                   securityLevel);
    isDirty = Serializable::GetVal<uint32_t>(jObject, DIRTY_KEY, json::value_t::boolean, isDirty);
    tokenId = Serializable::GetVal<uint32_t>(jObject, TOKEN_ID, json::value_t::number_unsigned, tokenId);
}

template<typename T>
T Serializable::GetVal(const json &j, const std::string &name, json::value_t type, const T &val)
{
    auto it = j.find(name);
    if (it != j.end() && it->type() == type) {
        return *it;
    }
    ZLOGW("not found name:%s.", name.c_str());
    return val;
}

std::vector<uint8_t> SecretKeyMetaData::Marshal() const
{
    json jval = {
        {TIME, timeValue},
        {SKEY, secretKey},
        {KVSTORE_TYPE, kvStoreType}
    };
    auto value = jval.dump();
    return std::vector<uint8_t>(value.begin(), value.end());
}

void SecretKeyMetaData::Unmarshal(const nlohmann::json &jObject)
{
    timeValue = Serializable::GetVal<std::vector<uint8_t>>(jObject, TIME, json::value_t::array, timeValue);
    secretKey = Serializable::GetVal<std::vector<uint8_t>>(jObject, SKEY, json::value_t::array, secretKey);
    kvStoreType = Serializable::GetVal<KvStoreType>(jObject, KVSTORE_TYPE, json::value_t::number_unsigned, kvStoreType);
}

bool KvStoreMetaManager::GetFullMetaData(std::map<std::string, MetaData> &entries, enum DatabaseType type)
{
    ZLOGI("start");
    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        return false;
    }

    std::vector<DistributedDB::Entry> kvStoreMetaEntries;
    const std::string &metaKey = KvStoreMetaRow::KEY_PREFIX;
    DistributedDB::DBStatus dbStatus = metaDelegate->GetEntries({metaKey.begin(), metaKey.end()}, kvStoreMetaEntries);
    if (dbStatus != DistributedDB::DBStatus::OK) {
        ZLOGE("Get kvstore meta data entries from metaDB failed, dbStatus: %d.", static_cast<int>(dbStatus));
        return false;
    }

    for (auto const &kvStoreMeta : kvStoreMetaEntries) {
        std::string jsonStr(kvStoreMeta.value.begin(), kvStoreMeta.value.end());
        ZLOGD("kvStoreMetaData get json: %s", jsonStr.c_str());
        auto metaObj = Serializable::ToJson(jsonStr);
        MetaData metaData {0};
        metaData.kvStoreType = MetaData::GetKvStoreType(metaObj);
        if (!(type == KVDB && metaData.kvStoreType < KvStoreType::INVALID_TYPE) &&
             !(type == RDB && metaData.kvStoreType >= DistributedRdb::RdbDistributedType::RDB_DEVICE_COLLABORATION)) {
            continue;
        }

        metaData.kvStoreMetaData.Unmarshal(metaObj);
        std::vector<uint8_t> decryptKey;
        if (metaData.kvStoreMetaData.isEncrypt) {
            ZLOGE("isEncrypt.");
            const std::string keyType = ((metaData.kvStoreType == KvStoreType::SINGLE_VERSION) ? "SINGLE_KEY" : "KEY");
            const std::vector<uint8_t> metaSecretKey = KvStoreMetaManager::GetInstance().GetMetaKey(
                metaData.kvStoreMetaData.deviceAccountId, "default", metaData.kvStoreMetaData.bundleName,
                metaData.kvStoreMetaData.storeId, keyType);
            DistributedDB::Value secretValue;
            metaDelegate->GetLocal(metaSecretKey, secretValue);
            auto secretObj = Serializable::ToJson({secretValue.begin(), secretValue.end()});
            if (secretObj.empty()) {
                ZLOGE("Failed to find SKEY in SecretKeyMetaData.");
                continue;
            }
            metaData.secretKeyMetaData.Unmarshal(secretObj);
            CryptoManager::GetInstance().Decrypt(metaData.secretKeyMetaData.secretKey, decryptKey);
        }
        entries.insert({{kvStoreMeta.key.begin(), kvStoreMeta.key.end()}, {metaData}});
        std::fill(decryptKey.begin(), decryptKey.end(), 0);
    }

    return true;
}

bool KvStoreMetaManager::GetKvStoreMetaByType(const std::string &name, const std::string &val,
                                              KvStoreMetaData &metaData)
{
    auto metaDelegate = GetMetaKvStore();
    if (metaDelegate == nullptr) {
        return false;
    }

    DistributedDB::Key metaKeyPrefix = KvStoreMetaRow::GetKeyFor("");
    std::vector<DistributedDB::Entry> metaEntries;
    DistributedDB::DBStatus dbStatus = metaDelegate->GetEntries(metaKeyPrefix, metaEntries);
    if (dbStatus != DistributedDB::DBStatus::OK) {
        ZLOGE("Get meta entries from metaDB failed, dbStatus: %d.", static_cast<int>(dbStatus));
        return false;
    }

    for (auto const &metaEntry : metaEntries) {
        std::string jsonStr(metaEntry.value.begin(), metaEntry.value.end());
        ZLOGD("KvStore get json: %s", jsonStr.c_str());
        json jsonObj = json::parse(jsonStr, nullptr, false);
        if (jsonObj.is_discarded()) {
            ZLOGE("parse json error");
            continue;
        }

        std::string metaTypeVal;
        jsonObj[name].get_to(metaTypeVal);
        if (metaTypeVal == val) {
            metaData.Unmarshal(Serializable::ToJson(jsonStr));
        }
    }
    return true;
}

bool KvStoreMetaManager::GetKvStoreMetaDataByBundleName(const std::string &bundleName, KvStoreMetaData &metaData)
{
    return GetKvStoreMetaByType(KvStoreMetaData::BUNDLE_NAME, bundleName, metaData);
}

bool KvStoreMetaManager::GetKvStoreMetaDataByAppId(const std::string &appId, KvStoreMetaData &metaData)
{
    return GetKvStoreMetaByType(KvStoreMetaData::APP_ID, appId, metaData);
}
} // namespace DistributedKv
} // namespace OHOS
