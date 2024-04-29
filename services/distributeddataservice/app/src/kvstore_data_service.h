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
#include "feature_stub_impl.h"
#include "ikvstore_data_service.h"
#include "ithread_pool.h"
#include "kvstore_device_listener.h"
#include "kvstore_meta_manager.h"
#include "kvstore_data_service_stub.h"
#include "metadata/store_meta_data.h"
#include "reporter.h"
#include "runtime_config.h"
#include "security/security.h"
#include "system_ability.h"
#include "executor_pool.h"
#include "types.h"

namespace OHOS::DistributedKv {
class KvStoreAccountObserver;
class KvStoreDataService : public SystemAbility, public KvStoreDataServiceStub {
    DECLARE_SYSTEM_ABILITY(KvStoreDataService);
    using Handler = std::function<void(int, std::map<std::string, std::vector<std::string>> &)>;

public:
    struct UserInfo {
        std::string userId;
        std::set<std::string> bundles;
    };
    struct BundleInfo {
        std::string bundleName;
        std::string appId;
        std::string type;
        int32_t uid;
        uint32_t tokenId;
        std::string userId;
        std::set<std::string> storeIDs;
    };
    using StoreMetaData = DistributedData::StoreMetaData;
    // record kvstore meta version for compatible, should update when modify kvstore meta structure.
    static constexpr uint32_t STORE_VERSION = 0x03000001;

    explicit KvStoreDataService(bool runOnCreate = false);
    explicit KvStoreDataService(int32_t systemAbilityId, bool runOnCreate = false);
    virtual ~KvStoreDataService();

    void RegisterHandler(const std::string &name, Handler &handler);
    void RegisterStoreInfo();
    bool IsExist(const std::string &infoName, std::map<std::string, std::vector<std::string>> &filterInfo,
        std::string &metaParam);
    void DumpStoreInfo(int fd, std::map<std::string, std::vector<std::string>> &params);
    void FilterData(std::vector<StoreMetaData> &metas, std::map<std::string, std::vector<std::string>> &filterInfo);
    void PrintfInfo(int fd, const std::vector<StoreMetaData> &metas);
    std::string GetIndentation(int size);

    void RegisterUserInfo();
    void BuildData(std::map<std::string, UserInfo> &datas, const std::vector<StoreMetaData> &metas);
    void PrintfInfo(int fd, const std::map<std::string, UserInfo> &datas);
    void DumpUserInfo(int fd, std::map<std::string, std::vector<std::string>> &params);

    void RegisterBundleInfo();
    void BuildData(std::map<std::string, BundleInfo> &datas, const std::vector<StoreMetaData> &metas);
    void PrintfInfo(int fd, const std::map<std::string, BundleInfo> &datas);
    void DumpBundleInfo(int fd, std::map<std::string, std::vector<std::string>> &params);

    Status RegisterClientDeathObserver(const AppId &appId, sptr<IRemoteObject> observer) override;

    sptr<IRemoteObject> GetFeatureInterface(const std::string &name) override;

    int32_t ClearAppStorage(const std::string &bundleName, int32_t userId, int32_t appIndex, int32_t tokenId) override;

    void OnDump() override;

    int Dump(int fd, const std::vector<std::u16string> &args) override;

    void OnStart() override;

    void OnStop() override;

    void OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;

    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string &deviceId) override;

    void AccountEventChanged(const AccountEventInfo &eventInfo);

    void SetCompatibleIdentify(const AppDistributedKv::DeviceInfo &info) const;

    void OnDeviceOnline(const AppDistributedKv::DeviceInfo &info);

    void OnDeviceOffline(const AppDistributedKv::DeviceInfo &info);

    void OnDeviceOnReady(const AppDistributedKv::DeviceInfo &info);

    void OnSessionReady(const AppDistributedKv::DeviceInfo &info);

    int32_t OnUninstall(const std::string &bundleName, int32_t user, int32_t index);

    int32_t OnUpdate(const std::string &bundleName, int32_t user, int32_t index);

    int32_t OnInstall(const std::string &bundleName, int32_t user, int32_t index);

private:
    void NotifyAccountEvent(const AccountEventInfo &eventInfo);
    class KvStoreClientDeathObserverImpl {
    public:
        KvStoreClientDeathObserverImpl(const AppId &appId, KvStoreDataService &service, sptr<IRemoteObject> observer);
        explicit KvStoreClientDeathObserverImpl(KvStoreDataService &service);
        explicit KvStoreClientDeathObserverImpl(KvStoreClientDeathObserverImpl &&impl);
        KvStoreClientDeathObserverImpl &operator=(KvStoreClientDeathObserverImpl &&impl);

        virtual ~KvStoreClientDeathObserverImpl();

        pid_t GetPid() const;

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
        void Reset();
        pid_t uid_;
        pid_t pid_;
        uint32_t token_;
        AppId appId_;
        KvStoreDataService &dataService_;
        sptr<IRemoteObject> observerProxy_;
        sptr<KvStoreDeathRecipient> deathRecipient_;
    };

    void Initialize();

    void LoadFeatures();

    void StartService();

    void InitSecurityAdapter(std::shared_ptr<ExecutorPool> executors);

    void OnStoreMetaChanged(const std::vector<uint8_t> &key, const std::vector<uint8_t> &value, CHANGE_FLAG flag);

    Status AppExit(pid_t uid, pid_t pid, uint32_t token, const AppId &appId);

    bool ResolveAutoLaunchParamByIdentifier(const std::string &identifier, DistributedDB::AutoLaunchParam &param);
    void ResolveAutoLaunchCompatible(const StoreMetaData &storeMeta, const std::string &identifier);
    static DistributedDB::SecurityOption ConvertSecurity(int securityLevel);
    static Status InitNbDbOption(const Options &options, const std::vector<uint8_t> &cipherKey,
                          DistributedDB::KvStoreNbDelegate::Option &dbOption);

    static constexpr int TEN_SEC = 10;

    ConcurrentMap<uint32_t, KvStoreClientDeathObserverImpl> clients_;
    std::shared_ptr<KvStoreAccountObserver> accountEventObserver_;

    std::shared_ptr<Security> security_;
    ConcurrentMap<std::string, sptr<DistributedData::FeatureStubImpl>> features_;
    std::shared_ptr<KvStoreDeviceListener> deviceInnerListener_;
    std::shared_ptr<ExecutorPool> executors_;
    static constexpr int VERSION_WIDTH = 11;
    static constexpr const char *INDENTATION = "    ";
    static constexpr int32_t FORMAT_BLANK_SIZE = 32;
    static constexpr char FORMAT_BLANK_SPACE = ' ';
    static constexpr int32_t PRINTF_COUNT_2 = 2;
    static constexpr int MAXIMUM_PARAMETER_LIMIT = 3;
    static constexpr pid_t INVALID_UID = -1;
    static constexpr pid_t INVALID_PID = -1;
    static constexpr uint32_t INVALID_TOKEN = 0;
};
}
#endif  // KVSTORE_DATASERVICE_H
