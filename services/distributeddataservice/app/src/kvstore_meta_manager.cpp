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
#include "device_matrix.h"
#include "directory_manager.h"
#include "dump_helper.h"
#include "eventcenter/event_center.h"
#include "kvstore_data_service.h"
#include "log_print.h"
#include "matrix_event.h"
#include "metadata/meta_data_manager.h"
#include "rdb_types.h"
#include "serializable/serializable.h"
#include "utils/anonymous.h"
#include "utils/crypto.h"

namespace OHOS {
namespace DistributedKv {
using json = nlohmann::json;
using Commu = AppDistributedKv::CommunicationProvider;
using DmAdapter = DistributedData::DeviceManagerAdapter;
using namespace std::chrono;
using namespace OHOS::DistributedData;
using namespace DistributedDB;

// APPID: distributeddata
// USERID: default
// STOREID: service_meta
// dataDir: /data/misc_de/0/mdds/Meta/${storeId}/sin_gen.db
KvStoreMetaManager::MetaDeviceChangeListenerImpl KvStoreMetaManager::listener_;

KvStoreMetaManager::KvStoreMetaManager()
    : metaDelegate_(nullptr), metaDBDirectory_(DirectoryManager::GetInstance().GetMetaStorePath()),
      label_(Bootstrap::GetInstance().GetProcessLabel()),
      delegateManager_(Bootstrap::GetInstance().GetProcessLabel(), "default")
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
    auto status = Commu::GetInstance().StartWatchDeviceChange(&listener_, { "metaMgr" });
    if (status != AppDistributedKv::Status::SUCCESS) {
        ZLOGW("register failed.");
        return;
    }
    ZLOGI("register meta device change success.");
    SubscribeMetaKvStore();
    SyncMeta();
    InitBroadcast();
    InitDeviceOnline();
}

void KvStoreMetaManager::InitBroadcast()
{
    auto pipe = Bootstrap::GetInstance().GetProcessLabel() + "-" + "default";
    auto result = Commu::GetInstance().ListenBroadcastMsg({ pipe },
        [](const std::string &device, uint16_t mask) { DeviceMatrix::GetInstance().OnBroadcast(device, mask); });

    EventCenter::GetInstance().Subscribe(DeviceMatrix::MATRIX_BROADCAST, [pipe](const Event &event) {
        auto &matrixEvent = static_cast<const MatrixEvent &>(event);
        Commu::GetInstance().Broadcast({ pipe }, matrixEvent.GetMask());
    });

    ZLOGI("observer matrix broadcast %{public}d.", result);
}

void KvStoreMetaManager::InitDeviceOnline()
{
    ZLOGI("observer matrix online event.");
    EventCenter::GetInstance().Subscribe(DeviceMatrix::MATRIX_ONLINE, [this](const Event &event) {
        const MatrixEvent &matrixEvent = static_cast<const MatrixEvent &>(event);
        auto mask = matrixEvent.GetMask();
        auto deviceId = matrixEvent.GetDeviceId();
        auto store = GetMetaKvStore();
        if (((mask & DeviceMatrix::META_STORE_MASK) != 0) && store != nullptr) {
            auto onComplete = [deviceId, mask](const std::map<std::string, DBStatus> &) {
                auto event = std::make_unique<MatrixEvent>(DeviceMatrix::MATRIX_META_FINISHED, deviceId, mask);
                DeviceMatrix::GetInstance().OnExchanged(deviceId, DeviceMatrix::META_STORE_MASK);
                EventCenter::GetInstance().PostEvent(std::move(event));
            };
            auto status = store->Sync({ deviceId }, DistributedDB::SyncMode::SYNC_MODE_PUSH_PULL, onComplete);
            if (status == OK) {
                return;
            }
            ZLOGW("meta db sync error %d.", status);
        }

        auto finEvent = std::make_unique<MatrixEvent>(DeviceMatrix::MATRIX_META_FINISHED, deviceId, mask);
        EventCenter::GetInstance().PostEvent(std::move(finEvent));
    });
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
    DistributedDB::KvStoreConfig kvStoreConfig{ metaDBDirectory_ };
    delegateManager_.SetKvStoreConfig(kvStoreConfig);
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
    DistributedDB::KvStoreNbDelegate *delegate = nullptr;
    delegateManager_.GetKvStore(Bootstrap::GetInstance().GetMetaDBName(), option,
        [&delegate, &dbStatusTmp](DistributedDB::DBStatus dbStatus, DistributedDB::KvStoreNbDelegate *nbDelegate) {
            delegate = nbDelegate;
            dbStatusTmp = dbStatus;
        });

    if (dbStatusTmp != DistributedDB::DBStatus::OK) {
        ZLOGE("GetKvStore return error status: %{public}d", static_cast<int>(dbStatusTmp));
        return nullptr;
    }
    delegate->SetRemotePushFinishedNotify([](const RemotePushNotifyInfo &info) {
        DeviceMatrix::GetInstance().OnExchanged(info.deviceId, DeviceMatrix::META_STORE_MASK);
    });
    auto release = [this](DistributedDB::KvStoreNbDelegate *delegate) {
        ZLOGI("release meta data  kv store");
        if (delegate == nullptr) {
            return;
        }

        auto result = delegateManager_.CloseKvStore(delegate);
        if (result != DistributedDB::DBStatus::OK) {
            ZLOGE("CloseMetaKvStore return error status: %{public}d", static_cast<int>(result));
        }
    };
    return NbDelegate(delegate, release);
}

void KvStoreMetaManager::ConfigMetaDataManager()
{
    auto fullName = GetBackupPath();
    auto backup = [fullName](const auto &store) -> int32_t {
        DistributedDB::CipherPassword password;
        return store->Export(fullName, password);
    };
    auto syncer = [](const auto &store, int32_t status) {
        ZLOGI("Syncer status: %{public}d", status);
        DeviceMatrix::GetInstance().OnChanged(DeviceMatrix::META_STORE_MASK);
        std::vector<std::string> devs;
        auto devices = Commu::GetInstance().GetRemoteDevices();
        for (auto const &dev : devices) {
            devs.push_back(dev.uuid);
        }

        if (devs.empty()) {
            ZLOGW("no devices need sync meta data.");
            return;
        }

        status = store->Sync(devs, DistributedDB::SyncMode::SYNC_MODE_PUSH_PULL, [](auto &results) {
            ZLOGD("meta data sync completed.");
            for (auto &[uuid, status] : results) {
                if (status != DistributedDB::OK) {
                    continue;
                }
                DeviceMatrix::GetInstance().OnExchanged(uuid, DeviceMatrix::META_STORE_MASK);
            }
        });

        if (status != DistributedDB::OK) {
            ZLOGW("meta data sync error %{public}d.", status);
        }
    };
    MetaDataManager::GetInstance().Initialize(metaDelegate_, backup, syncer);
}

std::vector<uint8_t> KvStoreMetaManager::GetMetaKey(const std::string &deviceAccountId, const std::string &groupId,
    const std::string &bundleName, const std::string &storeId, const std::string &key)
{
    std::string originKey;
    if (key.empty()) {
        originKey = DmAdapter::GetInstance().GetLocalDevice().uuid + Constant::KEY_SEPARATOR + deviceAccountId +
                    Constant::KEY_SEPARATOR + groupId + Constant::KEY_SEPARATOR + bundleName +
                    Constant::KEY_SEPARATOR + storeId;
        return KvStoreMetaRow::GetKeyFor(originKey);
    }

    originKey = deviceAccountId + Constant::KEY_SEPARATOR + groupId + Constant::KEY_SEPARATOR + bundleName +
                Constant::KEY_SEPARATOR + storeId + Constant::KEY_SEPARATOR + key;
    return SecretMetaRow::GetKeyFor(originKey);
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
}

void KvStoreMetaManager::KvStoreMetaObserver::HandleChanges(CHANGE_FLAG flag,
    const std::list<DistributedDB::Entry> &entries)
{
    for (const auto &entry : entries) {
        std::string key(entry.key.begin(), entry.key.end());
        for (const auto &item : handlerMap_) {
            ZLOGI("flag:%{public}d, key:%{public}s", flag, Anonymous::Change(key).c_str());
            if (key.find(item.first) == 0) {
                item.second(entry.key, entry.value, flag);
            }
        }
    }
}

void KvStoreMetaManager::MetaDeviceChangeListenerImpl::OnDeviceChanged(const AppDistributedKv::DeviceInfo &info,
    const AppDistributedKv::DeviceChangeType &type) const
{
    EventCenter::Defer defer;
    switch (type) {
        case AppDistributedKv::DeviceChangeType::DEVICE_OFFLINE:
            DeviceMatrix::GetInstance().Offline(info.uuid);
            break;
        case AppDistributedKv::DeviceChangeType::DEVICE_ONLINE:
            DeviceMatrix::GetInstance().Online(info.uuid);
            break;
        default:
            ZLOGI("flag:%{public}d", type);
            break;
    }
}

AppDistributedKv::ChangeLevelType KvStoreMetaManager::MetaDeviceChangeListenerImpl::GetChangeLevelType() const
{
    return AppDistributedKv::ChangeLevelType::LOW;
}

std::string KvStoreMetaData::Marshal() const
{
    json jval = {
        { DEVICE_ID, deviceId },
        { USER_ID, userId },
        { APP_ID, appId },
        { STORE_ID, storeId },
        { BUNDLE_NAME, bundleName },
        { KVSTORE_TYPE, kvStoreType },
        { ENCRYPT, isEncrypt },
        { BACKUP, isBackup },
        { AUTO_SYNC, isAutoSync },
        { SCHEMA, schema },
        { DATA_DIR, dataDir }, // Reserved for kvstore data storage directory.
        { APP_TYPE, appType }, // Reserved for the APP type which used kvstore.
        { DEVICE_ACCOUNT_ID, deviceAccountId },
        { UID, uid },
        { VERSION, version },
        { SECURITY_LEVEL, securityLevel },
        { DIRTY_KEY, isDirty },
        { TOKEN_ID, tokenId },
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
    deviceAccountId =
        Serializable::GetVal<std::string>(jObject, DEVICE_ACCOUNT_ID, json::value_t::string, deviceAccountId);
    dataDir = Serializable::GetVal<std::string>(jObject, DATA_DIR, json::value_t::string, dataDir);
    appType = Serializable::GetVal<std::string>(jObject, APP_TYPE, json::value_t::string, appType);
    deviceId = Serializable::GetVal<std::string>(jObject, DEVICE_ID, json::value_t::string, deviceId);
    schema = Serializable::GetVal<std::string>(jObject, SCHEMA, json::value_t::string, schema);
    uid = Serializable::GetVal<int32_t>(jObject, UID, json::value_t::number_unsigned, uid);
    version = Serializable::GetVal<uint32_t>(jObject, VERSION, json::value_t::number_unsigned, version);
    securityLevel =
        Serializable::GetVal<uint32_t>(jObject, SECURITY_LEVEL, json::value_t::number_unsigned, securityLevel);
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
    json jval = { { TIME, timeValue }, { SKEY, secretKey }, { KVSTORE_TYPE, kvStoreType } };
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
    DistributedDB::DBStatus dbStatus = metaDelegate->GetEntries({ metaKey.begin(), metaKey.end() }, kvStoreMetaEntries);
    if (dbStatus != DistributedDB::DBStatus::OK) {
        ZLOGE("Get kvstore meta data entries from metaDB failed, dbStatus: %d.", static_cast<int>(dbStatus));
        return false;
    }

    for (auto const &kvStoreMeta : kvStoreMetaEntries) {
        std::string jsonStr(kvStoreMeta.value.begin(), kvStoreMeta.value.end());
        ZLOGD("kvStoreMetaData get json: %s", jsonStr.c_str());
        auto metaObj = Serializable::ToJson(jsonStr);
        MetaData metaData{ 0 };
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
            const std::vector<uint8_t> metaSecretKey =
                KvStoreMetaManager::GetInstance().GetMetaKey(metaData.kvStoreMetaData.deviceAccountId, "default",
                    metaData.kvStoreMetaData.bundleName, metaData.kvStoreMetaData.storeId, keyType);
            DistributedDB::Value secretValue;
            metaDelegate->GetLocal(metaSecretKey, secretValue);
            auto secretObj = Serializable::ToJson({ secretValue.begin(), secretValue.end() });
            if (secretObj.empty()) {
                ZLOGE("Failed to find SKEY in SecretKeyMetaData.");
                continue;
            }
            metaData.secretKeyMetaData.Unmarshal(secretObj);
            CryptoManager::GetInstance().Decrypt(metaData.secretKeyMetaData.secretKey, decryptKey);
        }
        entries.insert({ { kvStoreMeta.key.begin(), kvStoreMeta.key.end() }, { metaData } });
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

bool KvStoreMetaManager::GetKvStoreMetaDataByAppId(const std::string &appId, KvStoreMetaData &metaData)
{
    return GetKvStoreMetaByType(KvStoreMetaData::APP_ID, appId, metaData);
}

std::string KvStoreMetaManager::GetBackupPath() const
{
    return (DirectoryManager::GetInstance().GetMetaBackupPath() + "/" +
            Crypto::Sha256(label_ + "_" + Bootstrap::GetInstance().GetMetaDBName()));
}
} // namespace DistributedKv
} // namespace OHOS
