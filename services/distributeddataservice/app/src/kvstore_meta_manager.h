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

#include "app_device_change_listener.h"
#include "executor_pool.h"
#include "kv_store_delegate.h"
#include "kv_store_delegate_manager.h"
#include "system_ability.h"
#include "types.h"

namespace OHOS {
namespace DistributedKv {
enum class CHANGE_FLAG {
    INSERT,
    UPDATE,
    DELETE
};

class KvStoreMetaManager {
public:
    static constexpr uint32_t META_STORE_VERSION = 0x03000001;
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
    size_t GetSyncDataSize(const std::string &deviceId);
    void BindExecutor(std::shared_ptr<ExecutorPool> executors);
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

    ExecutorPool::Task GetTask(uint32_t retry);

    class KvStoreMetaObserver : public DistributedDB::KvStoreObserver {
    public:
        virtual ~KvStoreMetaObserver();

        // Database change callback
        void OnChange(const DistributedDB::KvStoreChangedData &data) override;
        std::map<std::string, ChangeObserver> handlerMap_;
    private:
        void HandleChanges(CHANGE_FLAG flag, const std::list<DistributedDB::Entry> &list);
    };

    static constexpr int32_t RETRY_MAX_TIMES = 100;
    static constexpr int32_t RETRY_INTERVAL = 1;
    NbDelegate metaDelegate_;
    std::string metaDBDirectory_;
    const std::string label_;
    DistributedDB::KvStoreDelegateManager delegateManager_;
    static MetaDeviceChangeListenerImpl listener_;
    KvStoreMetaObserver metaObserver_;
    std::recursive_mutex mutex_;
    std::shared_ptr<ExecutorPool> executors_;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif // KVSTORE_META_MANAGER_H
