/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTEDDATASERVICE_RDB_SERVICE_H
#define DISTRIBUTEDDATASERVICE_RDB_SERVICE_H

#include "rdb_service_stub.h"

#include <map>
#include <mutex>
#include <string>
#include "concurrent_map.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "rdb_notifier_proxy.h"
#include "rdb_syncer.h"
#include "store_observer.h"
#include "store/auto_cache.h"
#include "visibility.h"
namespace OHOS::DistributedRdb {
class API_EXPORT RdbServiceImpl : public RdbServiceStub {
public:
    using StoreMetaData = OHOS::DistributedData::StoreMetaData;
    using SecretKeyMetaData = DistributedData::SecretKeyMetaData;
    RdbServiceImpl();

    void OnClientDied(pid_t pid);

    /* IPC interface */
    std::string ObtainDistributedTableName(const std::string& device, const std::string& table) override;

    int32_t InitNotifier(const RdbSyncerParam &param, const sptr<IRemoteObject> notifier) override;

    int32_t SetDistributedTables(const RdbSyncerParam& param, const std::vector<std::string>& tables) override;

    int32_t RemoteQuery(const RdbSyncerParam& param, const std::string& device, const std::string& sql,
                        const std::vector<std::string>& selectionArgs, sptr<IRemoteObject>& resultSet) override;

    void OnDataChange(pid_t pid, const DistributedDB::StoreChangedData& data);

    void OnChange(uint32_t tokenId, const std::string &storeName);

    int32_t CreateRDBTable(const RdbSyncerParam &param) override;

    int32_t DestroyRDBTable(const RdbSyncerParam &param) override;

    int32_t ResolveAutoLaunch(const std::string &identifier, DistributedDB::AutoLaunchParam &param) override;

    int32_t OnInitialize() override;

    int32_t GetSchema(const RdbSyncerParam &param) override;

    int32_t OnBind(const BindInfo &bindInfo) override;

protected:
    int32_t DoSync(const RdbSyncerParam& param, const SyncOption& option,
                   const RdbPredicates& predicates, SyncResult& result) override;

    int32_t DoAsync(const RdbSyncerParam& param, uint32_t seqNum, const SyncOption& option,
                    const RdbPredicates& predicates) override;

    int32_t DoSubscribe(const RdbSyncerParam& param, const SubscribeOption &option) override;

    int32_t DoUnSubscribe(const RdbSyncerParam& param) override;

private:
    using Watchers = DistributedData::AutoCache::Watchers;
    struct SyncAgent {
        pid_t pid_ = 0;
        std::string bundleName_;
        std::map<std::string, SubscribeMode> mode_;
        std::map<std::string, Watchers> watchers_;
        void ReInit(pid_t pid, const std::string &bundleName);
    };

    Watchers GetWatchers(uint32_t tokenId, const std::string &storeName);

    std::string GenIdentifier(const RdbSyncerParam& param);

    bool CheckAccess(const std::string& bundleName, const std::string& storeName);

    void SyncerTimeout(std::shared_ptr<RdbSyncer> syncer);

    std::shared_ptr<RdbSyncer> GetRdbSyncer(const RdbSyncerParam& param);

    void OnAsyncComplete(pid_t pid, uint32_t seqNum, const SyncResult& result);

    int32_t CreateMetaData(const RdbSyncerParam &param, StoreMetaData &old);

    StoreMetaData GetStoreMetaData(const RdbSyncerParam &param);

    int32_t SetSecretKey(const RdbSyncerParam &param, const StoreMetaData &meta);

    class DeathRecipientImpl : public IRemoteObject::DeathRecipient {
    public:
        using DeathCallback = std::function<void()>;
        explicit DeathRecipientImpl(const DeathCallback& callback);
        ~DeathRecipientImpl() override;
        void OnRemoteDied(const wptr<IRemoteObject> &object) override;
    private:
        const DeathCallback callback_;
    };
    class Factory {
    public:
        Factory();
        ~Factory();
    private:
        std::shared_ptr<RdbServiceImpl> product_;
    };

    using StoreSyncersType = std::map<std::string, std::shared_ptr<RdbSyncer>>;
    int32_t syncerNum_ {};
    ConcurrentMap<pid_t, StoreSyncersType> syncers_;
    ConcurrentMap<pid_t, sptr<RdbNotifierProxy>> notifiers_;
    ConcurrentMap<std::string, pid_t> identifiers_;
    RdbStoreObserverImpl autoLaunchObserver_;

    static Factory factory_;

    static std::string TransferStringToHex(const std::string& origStr);

    static constexpr int32_t MAX_SYNCER_NUM = 50;
    static constexpr int32_t MAX_SYNCER_PER_PROCESS = 10;
    static constexpr int32_t SYNCER_TIMEOUT = 60 * 1000; // ms
    std::shared_ptr<ExecutorPool> executors_;
    ConcurrentMap<uint32_t, SyncAgent> syncAgents_;
};
} // namespace OHOS::DistributedRdb
#endif
