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

#ifndef KVSTORE_DATASERVICE_H
#define KVSTORE_DATASERVICE_H

#include <map>
#include <mutex>
#include <set>

#include "account_delegate.h"
#include "constant.h"
#include "device_change_listener_impl.h"
#include "ikvstore_data_service.h"
#include "kvstore_device_listener.h"
#include "kvstore_user_manager.h"
#include "metadata/corrupted_meta_data.h"
#include "metadata/store_meta_data.h"
#include "reporter.h"
#include "security/security.h"
#include "single_kvstore_impl.h"
#include "system_ability.h"
#include "types.h"
#include "dump_helper.h"
#include "feature_stub_impl.h"

namespace OHOS::DistributedKv {
class KvStoreAccountObserver;
class KvStoreDataService : public SystemAbility, public KvStoreDataServiceStub {
    DECLARE_SYSTEM_ABILITY(KvStoreDataService);

public:
    using CorruptedMetaData = DistributedData::CorruptedMetaData;
    using StoreMetaData = DistributedData::StoreMetaData;
    // record kvstore meta version for compatible, should update when modify kvstore meta structure.
    static constexpr uint32_t STORE_VERSION = 0x03000001;

    explicit KvStoreDataService(bool runOnCreate = false);
    explicit KvStoreDataService(int32_t systemAbilityId, bool runOnCreate = false);
    virtual ~KvStoreDataService();

    Status GetSingleKvStore(const Options &options, const AppId &appId, const StoreId &storeId,
                      std::function<void(sptr<ISingleKvStore>)> callback) override;

    void GetAllKvStoreId(const AppId &appId, std::function<void(Status, std::vector<StoreId> &)> callback) override;

    Status CloseKvStore(const AppId &appId, const StoreId &storeId) override;

    Status CloseAllKvStore(const AppId &appId) override;

    Status DeleteKvStore(const AppId &appId, const StoreId &storeId) override;

    Status DeleteAllKvStore(const AppId &appId) override;

    Status RegisterClientDeathObserver(const AppId &appId, sptr<IRemoteObject> observer) override;

    Status GetLocalDevice(DeviceInfo &device) override;
    Status GetRemoteDevices(std::vector<DeviceInfo> &deviceInfoList, DeviceFilterStrategy strategy) override;
    Status StartWatchDeviceChange(sptr<IDeviceStatusChangeListener> observer, DeviceFilterStrategy strategy) override;
    Status StopWatchDeviceChange(sptr<IDeviceStatusChangeListener> observer) override;
    sptr<IRemoteObject> GetFeatureInterface(const std::string &name) override;

    void OnDump() override;

    int Dump(int fd, const std::vector<std::u16string> &args) override;

    void OnStart() override;

    void OnStop() override;

    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;

    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;

    Status DeleteKvStoreOnly(const StoreMetaData &metaData);

    void AccountEventChanged(const AccountEventInfo &eventInfo);

    void SetCompatibleIdentify(const AppDistributedKv::DeviceInfo &info) const;

    void OnDeviceOnline(const AppDistributedKv::DeviceInfo &info);

    bool CheckBackupFileExist(const std::string &userId, const std::string &bundleName,
                              const std::string &storeId, int pathType);
    int32_t OnUninstall(const std::string &bundleName, int32_t user, int32_t index, uint32_t tokenId);

    Status DeleteKvStore(StoreMetaData &metaData);

    struct SecretKeyPara {
        std::vector<uint8_t> metaKey;
        std::vector<uint8_t> secretKey;
        std::vector<uint8_t> metaSecretKey;
        std::string secretKeyFile;
        Status alreadyCreated = Status::SUCCESS;
        bool outdated = false;
    };

private:
    void NotifyAccountEvent(const AccountEventInfo &eventInfo);
    class KvStoreClientDeathObserverImpl {
    public:
        KvStoreClientDeathObserverImpl(const AppId &appId, KvStoreDataService &service, sptr<IRemoteObject> observer);

        virtual ~KvStoreClientDeathObserverImpl();

    private:
        class KvStoreDeathRecipient : public IRemoteObject::DeathRecipient {
        public:
            explicit KvStoreDeathRecipient(KvStoreClientDeathObserverImpl &kvStoreClientDeathObserverImpl);
            virtual ~KvStoreDeathRecipient();
            void OnRemoteDied(const wptr<IRemoteObject> &remote) override;

        private:
            KvStoreClientDeathObserverImpl &kvStoreClientDeathObserverImpl_;
        };
        void NotifyClientDie();
        pid_t uid_;
        pid_t pid_;
        uint32_t token_;
        AppId appId_;
        KvStoreDataService &dataService_;
        sptr<IRemoteObject> observerProxy_;
        sptr<KvStoreDeathRecipient> deathRecipient_;
    };

    void Initialize();

    void InitObjectStore();

    void StartService();

    void InitSecurityAdapter();

    template<class T>
    Status RecoverKvStore(const Options &options, const StoreMetaData &metaData,
        const std::vector<uint8_t> &secretKey, sptr<T> &kvStore);

    Status GetSecretKey(const Options &options, const StoreMetaData &metaData, SecretKeyPara &secretKeyParas);

    Status RecoverSecretKey(const Status &alreadyCreated, bool &outdated, const std::vector<uint8_t> &metaSecretKey,
        std::vector<uint8_t> &secretKey, const std::string &secretKeyFile);

    Status UpdateMetaData(const Options &options, const StoreMetaData &metaData);

    void OnStoreMetaChanged(const std::vector<uint8_t> &key, const std::vector<uint8_t> &value, CHANGE_FLAG flag);

    Status GetSingleKvStoreFailDo(Status status, const Options &options, const StoreMetaData &metaData,
        SecretKeyPara &secKeyParas, KvStoreUserManager &kvUserManager, sptr<SingleKvStoreImpl> &kvStore);

    Status AppExit(pid_t uid, pid_t pid, uint32_t token, const AppId &appId);

    bool ResolveAutoLaunchParamByIdentifier(const std::string &identifier, DistributedDB::AutoLaunchParam &param);
    static void ResolveAutoLaunchCompatible(const MetaData &meta, const std::string &identifier);

    bool CheckOptions(const Options &options, const std::vector<uint8_t> &metaKey) const;
    static Status FillStoreParam(
        const Options &options, const AppId &appId, const StoreId &storeId, StoreMetaData &metaData);

    void DumpAll(int fd);
    void DumpUserInfo(int fd);
    void DumpAppInfo(int fd, const std::string &appId);
    void DumpStoreInfo(int fd, const std::string &storeId);

    static constexpr int TEN_SEC = 10;

    std::mutex accountMutex_;
    std::map<std::string, KvStoreUserManager> deviceAccountMap_;
    std::mutex clientDeathObserverMutex_;
    std::map<uint32_t, KvStoreClientDeathObserverImpl> clientDeathObserverMap_;
    std::shared_ptr<KvStoreAccountObserver> accountEventObserver_;
    std::map<IRemoteObject *, sptr<IDeviceStatusChangeListener>> deviceListeners_;
    std::mutex deviceListenerMutex_;
    std::shared_ptr<DeviceChangeListenerImpl> deviceListener_;

    std::shared_ptr<Security> security_;
    ConcurrentMap<std::string, sptr<DistributedData::FeatureStubImpl>> features_;
    std::shared_ptr<KvStoreDeviceListener> deviceInnerListener_;
};

class DbMetaCallbackDelegateMgr : public DbMetaCallbackDelegate {
public:
    using Option = DistributedDB::KvStoreNbDelegate::Option;
    virtual ~DbMetaCallbackDelegateMgr() {}

    explicit DbMetaCallbackDelegateMgr(DistributedDB::KvStoreDelegateManager *delegate)
        : delegate_(delegate) {}
    bool GetKvStoreDiskSize(const std::string &storeId, uint64_t &size) override;
    void GetKvStoreKeys(std::vector<StoreInfo> &dbStats) override;
    bool IsDestruct()
    {
        return delegate_ == nullptr;
    }

private:
    void Split(const std::string &str, const std::string &delimiter, std::vector<std::string> &out)
    {
        size_t start;
        size_t end = 0;
        while ((start = str.find_first_not_of(delimiter, end)) != std::string::npos) {
            end = str.find(delimiter, start);
            if (end == std::string::npos) {
                end = str.size();
            }
            out.push_back(str.substr(start, end - start));
        }
    }

    DistributedDB::KvStoreDelegateManager *delegate_ {};
    static const inline int USER_ID = 0;
    static const inline int APP_ID = 1;
    static const inline int STORE_ID = 2;
    static const inline int VECTOR_SIZE = 2;
};
}
#endif  // KVSTORE_DATASERVICE_H
