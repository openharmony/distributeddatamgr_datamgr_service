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
#ifndef KVSTORE_META_MANAGER_H
#define KVSTORE_META_MANAGER_H

#include <mutex>
#include <nlohmann/json.hpp>

#include "app_device_change_listener.h"
#include "kv_store_delegate.h"
#include "kv_store_delegate_manager.h"
#include "kv_store_task.h"
#include "system_ability.h"
#include "types.h"

namespace OHOS {
namespace DistributedKv {
enum FLAG {
    UPDATE,
    DELETE,
    CHECK_EXIST,
    UPDATE_LOCAL,
    DELETE_LOCAL,
    CHECK_EXIST_LOCAL,
};

enum class CHANGE_FLAG {
    INSERT,
    UPDATE,
    DELETE
};

struct Serializable {
    using json = nlohmann::json;
    template<typename T>
    static T GetVal(const json &j, const std::string &name, json::value_t type, const T &def);
    static json ToJson(const std::string &jsonStr);
};

struct SecretKeyMetaData {
    static constexpr const char *SKEY = "skey";
    std::vector<uint8_t> timeValue {};
    std::vector<uint8_t> secretKey {};
    KvStoreType kvStoreType = KvStoreType::INVALID_TYPE;
    SecretKeyMetaData() {}
    ~SecretKeyMetaData()
    {
        secretKey.assign(secretKey.size(), 0);
    }
    explicit SecretKeyMetaData(const nlohmann::json &jObject)
    {
        Unmarshal(jObject);
    }

    std::vector<uint8_t> Marshal() const;
    void Unmarshal(const nlohmann::json &jObject);
    operator std::vector<uint8_t>() const
    {
        return Marshal();
    }
private:
    static constexpr const char *TIME = "time";
    static constexpr const char *KVSTORE_TYPE = "kvStoreType";
};

struct KvStoreMetaData {
    static constexpr const char *APP_ID = "appId";
    static constexpr const char *BUNDLE_NAME = "bundleName";
    using json = nlohmann::json;
    std::string appId = "";
    std::string appType = "";
    std::string bundleName = "";
    std::string dataDir = "";
    std::string deviceAccountId = "";
    std::string deviceId = "";
    bool isAutoSync = false;
    bool isBackup = false;
    bool isEncrypt = false;
    KvStoreType kvStoreType = KvStoreType::DEVICE_COLLABORATION;
    std::string schema = "";
    std::string storeId = "";
    std::uint32_t tokenId = 0;
    std::string userId = "";
    std::int32_t uid = -1;
    std::uint32_t version = 0;
    int securityLevel = 0;
    bool isDirty = false;
    std::string Marshal() const;
    void Unmarshal(const json &jObject);

    static inline std::string GetAppId(const json &jObject)
    {
        return Serializable::GetVal<std::string>(jObject, APP_ID, json::value_t::string, "");
    }

    static inline std::string GetStoreId(const json &jObject)
    {
        return Serializable::GetVal<std::string>(jObject, STORE_ID, json::value_t::string, "");
    }
private:
    static constexpr const char *KVSTORE_TYPE = "kvStoreType";
    static constexpr const char *DEVICE_ID = "deviceId";
    static constexpr const char *USER_ID = "userId";
    static constexpr const char *STORE_ID = "storeId";
    static constexpr const char *ENCRYPT = "isEncrypt";
    static constexpr const char *BACKUP = "isBackup";
    static constexpr const char *AUTO_SYNC = "isAutoSync";
    static constexpr const char *SCHEMA = "schema";
    static constexpr const char *DATA_DIR = "dataDir";
    static constexpr const char *APP_TYPE = "appType";
    static constexpr const char *DEVICE_ACCOUNT_ID = "deviceAccountID";
    static constexpr const char *UID = "UID";
    static constexpr const char *VERSION = "version";
    static constexpr const char *SECURITY_LEVEL = "securityLevel";
    static constexpr const char *DIRTY_KEY = "isDirty";
    static constexpr const char *TOKEN_ID = "tokenId";
};

struct MetaData {
    std::int32_t kvStoreType;
    KvStoreMetaData kvStoreMetaData;
    SecretKeyMetaData secretKeyMetaData;

    static inline KvStoreType GetKvStoreType(const nlohmann::json &jObject)
    {
        return Serializable::GetVal<KvStoreType>(jObject, KVSTORE_TYPE, nlohmann::json::value_t::number_unsigned,
                                                 KvStoreType::INVALID_TYPE);
    }
private:
    static constexpr const char *KVSTORE_TYPE = "kvStoreType";
};

class KvStoreMetaManager {
public:
    static constexpr uint32_t META_STORE_VERSION = 0x03000001;
    enum DatabaseType {
        KVDB,
        RDB,
    };
    using ChangeObserver = std::function<void(const std::vector<uint8_t> &, const std::vector<uint8_t> &, CHANGE_FLAG)>;

    class MetaDeviceChangeListenerImpl : public AppDistributedKv::AppDeviceChangeListener {
        void OnDeviceChanged(const AppDistributedKv::DeviceInfo &info,
                             const AppDistributedKv::DeviceChangeType &type) const override;

        AppDistributedKv::ChangeLevelType GetChangeLevelType() const override;
    };

    ~KvStoreMetaManager();

    static KvStoreMetaManager &GetInstance();

    void InitMetaParameter();
    void InitMetaListener();
    void InitBroadcast();
    void InitDeviceOnline();
    void SubscribeMeta(const std::string &keyPrefix, const ChangeObserver &observer);

    static std::vector<uint8_t> GetMetaKey(
        const std::string &deviceAccountId, const std::string &groupId, const std::string &bundleName,
        const std::string &storeId, const std::string &key = "");

    bool GetKvStoreMetaDataByAppId(const std::string &appId, KvStoreMetaData &metaData);

    bool GetFullMetaData(std::map<std::string, MetaData> &entries, enum DatabaseType type = KVDB);

private:
    using NbDelegate = std::shared_ptr<DistributedDB::KvStoreNbDelegate>;
    NbDelegate GetMetaKvStore();

    NbDelegate CreateMetaKvStore();

    void ConfigMetaDataManager();

    KvStoreMetaManager();

    void InitMetaData();

    void SubscribeMetaKvStore();

    void SyncMeta();

    std::string GetBackupPath() const;

    bool GetKvStoreMetaByType(const std::string &name, const std::string &val, KvStoreMetaData &metaData);

    class KvStoreMetaObserver : public DistributedDB::KvStoreObserver {
    public:
        virtual ~KvStoreMetaObserver();

        // Database change callback
        void OnChange(const DistributedDB::KvStoreChangedData &data) override;
        std::map<std::string, ChangeObserver> handlerMap_;
    private:
        void HandleChanges(CHANGE_FLAG flag, const std::list<DistributedDB::Entry> &list);
    };

    NbDelegate metaDelegate_;
    std::string metaDBDirectory_;
    const std::string label_;
    DistributedDB::KvStoreDelegateManager delegateManager_;
    static MetaDeviceChangeListenerImpl listener_;
    KvStoreMetaObserver metaObserver_;
    std::recursive_mutex mutex_;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif // KVSTORE_META_MANAGER_H
