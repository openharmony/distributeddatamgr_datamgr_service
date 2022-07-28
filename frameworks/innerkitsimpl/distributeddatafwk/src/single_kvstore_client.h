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

#ifndef DISTRIBUTEDDATAMGR2_SINGLE_KVSTORE_CLIENT_H
#define DISTRIBUTEDDATAMGR2_SINGLE_KVSTORE_CLIENT_H

#include <atomic>
#include "data_query.h"
#include "ikvstore_single.h"
#include "single_kvstore.h"
#include "kvstore_sync_callback_client.h"
#include "sync_observer.h"

namespace OHOS::DistributedKv {
class SingleKvStoreClient : public SingleKvStore {
public:
    explicit SingleKvStoreClient(sptr<ISingleKvStore> kvStoreProxy, const std::string &storeId);

    ~SingleKvStoreClient();

    StoreId GetStoreId() const override;

    Status GetEntries(const Key &prefix, std::vector<Entry> &entries) const override;

    Status GetEntries(const DataQuery &query, std::vector<Entry> &entries) const override;

    Status GetResultSet(const Key &prefix, std::shared_ptr<KvStoreResultSet> &resultSet) const override;

    Status GetResultSet(const DataQuery &query, std::shared_ptr<KvStoreResultSet> &resultSet) const override;

    Status CloseResultSet(std::shared_ptr<KvStoreResultSet> &resultSet) override;

    Status GetCount(const DataQuery &query, int &count) const override;

    Status Sync(const std::vector<std::string> &devices, SyncMode mode, uint32_t delay) override;

    Status RemoveDeviceData(const std::string &device) override;

    Status Delete(const Key &key) override;

    Status Put(const Key &key, const Value &value) override;

    Status Get(const Key &key, Value &value) override;

    Status SubscribeKvStore(SubscribeType subscribeType, std::shared_ptr<KvStoreObserver> observer) override;

    Status UnSubscribeKvStore(SubscribeType subscribeType, std::shared_ptr<KvStoreObserver> observer) override;

    Status RegisterSyncCallback(std::shared_ptr<KvStoreSyncCallback> callback) override;

    Status RegisterCallback();

    Status UnRegisterSyncCallback() override;

    Status PutBatch(const std::vector<Entry> &entries) override;

    Status DeleteBatch(const std::vector<Key> &keys) override;

    Status StartTransaction() override;

    Status Commit() override;

    Status Rollback() override;

    Status SetSyncParam(const KvSyncParam &syncParam) override;

    Status GetSyncParam(KvSyncParam &syncParam) override;
    Status SetCapabilityEnabled(bool enabled) const override;
    Status SetCapabilityRange(const std::vector<std::string> &localLabels,
                              const std::vector<std::string> &remoteLabels) const override;

    Status GetSecurityLevel(SecurityLevel &secLevel) const override;
    Status Sync(const std::vector<std::string> &devices, SyncMode mode, const DataQuery &query,
                std::shared_ptr<KvStoreSyncCallback> syncCallback) override;

    Status SubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query) override;
    Status UnsubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query) override;

    Status Backup(std::string &file, std::string &baseDir) override;
    Status Restore(std::string &file, std::string &baseDir) override;
    Status DeleteBackup(std::vector<std::string> &files, std::string &baseDir,
        std::map<std::string, DistributedKv::Status> &status) override;

protected:
    Status Control(KvControlCmd cmd, const KvParam &inputParam, KvParam &outputParam);

private:
    sptr<ISingleKvStore> kvStoreProxy_;
    std::map<KvStoreObserver *, sptr<IKvStoreObserver>> registeredObservers_;
    std::mutex observerMapMutex_;
    std::string storeId_;
    sptr<KvStoreSyncCallbackClient> syncCallbackClient_;
    std::shared_ptr<SyncObserver> syncObserver_;
    bool isRegisterSyncCallback_ = false;
    std::mutex registerCallbackMutex_;
};
} // namespace OHOS::DistributedKv
#endif // DISTRIBUTEDDATAMGR2_SINGLE_KVSTORE_CLIENT_H
