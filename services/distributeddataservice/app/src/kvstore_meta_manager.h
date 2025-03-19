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

#include <memory>
#include <mutex>

#include "app_device_change_listener.h"
#include "executor_pool.h"
#include "kv_store_delegate.h"
#include "kv_store_delegate_manager.h"
#include "safe_block_queue.h"
#include "system_ability.h"
#include "types.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"

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
    static constexpr uint16_t DEFAULT_MASK = 0x000F;
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
    void SubscribeMeta(const std::string &keyPrefix, const ChangeObserver &observer);
    void BindExecutor(std::shared_ptr<ExecutorPool> executors);
private:
    using NbDelegate = std::shared_ptr<DistributedDB::KvStoreNbDelegate>;
    using TaskId = ExecutorPool::TaskId;
    using Backup = DistributedData::MetaDataManager::Backup;
    using TaskQueue = std::shared_ptr<SafeBlockQueue<Backup>>;
    NbDelegate GetMetaKvStore();

    NbDelegate CreateMetaKvStore(bool isRestore = false);

    void SetCloudSyncer();

    std::function<void()> CloudSyncTask();

    KvStoreMetaManager();

    void InitMetaData();

    void UpdateMetaData();

    void SubscribeMetaKvStore();

    void NotifyAllAutoSyncDBInfo();

    void OnDataChange(CHANGE_FLAG flag, const std::list<DistributedDB::Entry>& changedData);

    void OnDeviceChange(const std::string& deviceId);

    void AddDbInfo(const DistributedData::StoreMetaData& metaData, std::vector<DistributedDB::DBInfo>& dbInfos,
        bool isDeleted = false);

    void GetDbInfosByDeviceId(const std::string& deviceId, std::vector<DistributedDB::DBInfo>& dbInfos);

    std::string GetBackupPath() const;

    ExecutorPool::Task GetTask(uint32_t retry);

    DistributedDB::KvStoreNbDelegate::Option InitDBOption();
    
    void UpdateMetaDeviceId(const std::string &currentUUID);

    bool IsMetaDeviceIdChanged(const std::string &currentUUID);
    
    void UpdateStoreMetaData(const std::string &currentUUID, bool isLocal = false);
    
    void UpdateLocalMetaData(const std::string &currentUUID, bool isLocal = false);
    
    void UpdateAutoLaunchMetaData(const std::string &currentUUID, bool isLocal = false);
    
    void UpdateStrategyMetaData(const std::string &currentUUID, bool isLocal = false);
    
    void UpdateMatrixMetaData(const std::string &currentUUID, bool isLocal = false);
    
    void UpdateSwitchesMetaData(const std::string &currentUUID, bool isLocal = false);
    
    void UpdateUserMetaData(const std::string &currentUUID, bool isLocal = false);
    
    void UpdateCapMetaData(const std::string &currentUUID, bool isLocal = false);

    static ExecutorPool::Task GetBackupTask(
        TaskQueue queue, std::shared_ptr<ExecutorPool> executors, const NbDelegate store);

    class KvStoreMetaObserver : public DistributedDB::KvStoreObserver {
    public:
        virtual ~KvStoreMetaObserver();

        // Database change callback
        void OnChange(const DistributedDB::KvStoreChangedData &data) override;
        std::map<std::string, ChangeObserver> handlerMap_;
    private:
        void HandleChanges(CHANGE_FLAG flag, const std::list<DistributedDB::Entry> &entries);
    };

    class DBInfoDeviceChangeListenerImpl : public AppDistributedKv::AppDeviceChangeListener {
        void OnDeviceChanged(const AppDistributedKv::DeviceInfo &info,
            const AppDistributedKv::DeviceChangeType &type) const override;

        AppDistributedKv::ChangeLevelType GetChangeLevelType() const override;
    };

    static constexpr int32_t RETRY_MAX_TIMES = 100;
    static constexpr int32_t RETRY_INTERVAL = 1;
    static constexpr uint8_t COMPRESS_RATE = 10;
    static constexpr int32_t DELAY_SYNC = 500;
    NbDelegate metaDelegate_;
    std::string metaDBDirectory_;
    const std::string label_;
    DistributedDB::KvStoreDelegateManager delegateManager_;
    static MetaDeviceChangeListenerImpl listener_;
    static DBInfoDeviceChangeListenerImpl dbInfoListener_;
    KvStoreMetaObserver metaObserver_;
    std::mutex mutex_;
    std::shared_ptr<ExecutorPool> executors_;
    TaskId delaySyncTaskId_ = ExecutorPool::INVALID_TASK_ID;
    static constexpr int32_t META_VERSION = 4;
    static constexpr int32_t MAX_TASK_COUNT = 1;
    std::string oldUUID_;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif // KVSTORE_META_MANAGER_H