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
#ifndef DISTRIBUTEDDATAMGR_OBJECT_MANAGER_H
#define DISTRIBUTEDDATAMGR_OBJECT_MANAGER_H

#include <atomic>

#include "concurrent_map.h"
#include "device_manager_adapter.h"
#include "kv_store_delegate_manager.h"
#include "kvstore_sync_callback.h"
#include "object_callback.h"
#include "object_callback_proxy.h"
#include "object_common.h"
#include "object_data_listener.h"
#include "object_snapshot.h"
#include "object_types.h"
#include "types.h"
#include "value_proxy.h"

namespace OHOS {
namespace DistributedObject {
using SyncCallBack = std::function<void(const std::map<std::string, int32_t> &results)>;

class SequenceSyncManager {
public:
    enum Result {
        SUCCESS_USER_IN_USE,
        SUCCESS_USER_HAS_FINISHED,
        ERR_SID_NOT_EXIST
    };
    static SequenceSyncManager *GetInstance()
    {
        static SequenceSyncManager sequenceSyncManager;
        return &sequenceSyncManager;
    }

    uint64_t AddNotifier(const std::string &userId, SyncCallBack &callback);
    Result DeleteNotifier(uint64_t sequenceId, std::string &userId);
    Result Process(
        uint64_t sequenceId, const std::map<std::string, DistributedDB::DBStatus> &results, std::string &userId);

private:
    Result DeleteNotifierNoLock(uint64_t sequenceId, std::string &userId);
    std::mutex notifierLock_;
    std::map<std::string, std::vector<uint64_t>> userIdSeqIdRelations_;
    std::map<uint64_t, SyncCallBack> seqIdCallbackRelations_;
};

class ObjectStoreManager {
public:
    using DmAdaper = OHOS::DistributedData::DeviceManagerAdapter;
    using UriToSnapshot = std::shared_ptr<std::map<std::string, std::shared_ptr<Snapshot>>>;
    ObjectStoreManager();
    static ObjectStoreManager *GetInstance()
    {
        static ObjectStoreManager *manager = new ObjectStoreManager();
        return manager;
    }
    int32_t Save(const std::string &appId, const std::string &sessionId,
        const std::map<std::string, std::vector<uint8_t>> &data, const std::string &deviceId,
        sptr<IRemoteObject> callback);
    int32_t RevokeSave(
        const std::string &appId, const std::string &sessionId, sptr<IRemoteObject> callback);
    int32_t Retrieve(const std::string &bundleName, const std::string &sessionId,
        sptr<IRemoteObject> callback, uint32_t tokenId);
    void SetData(const std::string &dataDir, const std::string &userId);
    int32_t Clear();
    int32_t DeleteByAppId(const std::string &appId);
    void RegisterRemoteCallback(const std::string &bundleName, const std::string &sessionId,
                                pid_t pid, uint32_t tokenId,
                                sptr<IRemoteObject> callback);
    void UnregisterRemoteCallback(const std::string &bundleName, pid_t pid, uint32_t tokenId,
                                  const std::string &sessionId = "");
    void NotifyChange(std::map<std::string, std::vector<uint8_t>> &changedData);
    void CloseAfterMinute();
    int32_t Open();
    void SetThreadPool(std::shared_ptr<ExecutorPool> executors);
    UriToSnapshot GetSnapShots(const std::string &bundleName, const std::string &storeName);
    int32_t BindAsset(const uint32_t tokenId, const std::string& appId, const std::string& sessionId,
        ObjectStore::Asset& asset, ObjectStore::AssetBindInfo& bindInfo);
    int32_t OnAssetChanged(const uint32_t tokenId, const std::string& appId, const std::string& sessionId,
         const std::string& deviceId, const ObjectStore::Asset& asset);
    void DeleteSnapshot(const std::string &bundleName, const std::string &sessionId);
private:
    constexpr static const char *SEPERATOR = "_";
    constexpr static const char *TIME_REGEX = "_\\d{10}_p_";
    constexpr static int BUNDLE_NAME_INDEX = 0;
    constexpr static int SESSION_ID_INDEX = 1;
    constexpr static int SOURCE_DEVICE_UDID_INDEX = 2;
    constexpr static int TIME_INDEX = 4;
    constexpr static int PROPERTY_NAME_INDEX = 5;
    constexpr static const char *LOCAL_DEVICE = "local";
    constexpr static const char *USERID = "USERID";
    constexpr static int8_t MAX_OBJECT_SIZE_PER_APP = 16;
    constexpr static int8_t DECIMAL_BASE = 10;
    constexpr static int WAIT_TIME = 60;
    struct CallbackInfo {
        pid_t pid;
        std::map<std::string, sptr<ObjectChangeCallbackProxy>> observers_;
        bool operator<(const CallbackInfo &it_) const
        {
            if (pid < it_.pid) {
                return true;
            }
            return false;
        }
    };
    DistributedDB::KvStoreNbDelegate *OpenObjectKvStore();
    void FlushClosedStore();
    void Close();
    int32_t SetSyncStatus(bool status);
    int32_t SaveToStore(const std::string &appId, const std::string &sessionId, const std::string &toDeviceId,
                        const std::map<std::string, std::vector<uint8_t>> &data);
    int32_t SyncOnStore(const std::string &prefix, const std::vector<std::string> &deviceList, SyncCallBack &callback);
    int32_t RevokeSaveToStore(const std::string &prefix);
    int32_t RetrieveFromStore(
        const std::string &appId, const std::string &sessionId, std::map<std::string, std::vector<uint8_t>> &results);
    void SyncCompleted(const std::map<std::string, DistributedDB::DBStatus> &results, uint64_t sequenceId);
    std::vector<std::string> SplitEntryKey(const std::string &key);
    std::string GetPropertyName(const std::string &key);
    void ProcessOldEntry(const std::string &appId);
    void ProcessSyncCallback(const std::map<std::string, int32_t> &results, const std::string &appId,
        const std::string &sessionId, const std::string &deviceId);
    void SaveUserToMeta();
    std::string GetCurrentUser();
    void DoNotify(uint32_t tokenId, const CallbackInfo& value,
        const std::map<std::string, std::map<std::string, std::vector<uint8_t>>>& data);
    std::map<std::string, std::map<std::string, Assets>> GetAssetsFromStore(
        const std::map<std::string, std::vector<uint8_t>>& changedData);
    static bool isAssetKey(const std::string& key);
    static bool isAssetComplete(const std::map<std::string, std::vector<uint8_t>>& result,
        const std::string& assetPrefix);
    Assets GetAssetsFromDBRecords(const std::map<std::string, std::vector<uint8_t>>& result);
    inline std::string GetPropertyPrefix(const std::string &appId, const std::string &sessionId)
    {
        return appId + SEPERATOR + sessionId + SEPERATOR + DmAdaper::GetInstance().GetLocalDevice().udid + SEPERATOR;
    };
    inline std::string GetPropertyPrefix(
        const std::string &appId, const std::string &sessionId, const std::string &toDeviceId)
    {
        return appId + SEPERATOR + sessionId + SEPERATOR + DmAdaper::GetInstance().GetLocalDevice().udid
             + SEPERATOR + toDeviceId + SEPERATOR;
    };
    inline std::string GetPrefixWithoutDeviceId(const std::string &appId, const std::string &sessionId)
    {
        return appId + SEPERATOR + sessionId + SEPERATOR;
    };
    inline std::string GetMetaUserIdKey(const std::string &userId, const std::string &appId)
    {
        return std::string(USERID) + SEPERATOR + userId + SEPERATOR + appId + SEPERATOR
             + DmAdaper::GetInstance().GetLocalDevice().udid;
    };
    std::recursive_mutex kvStoreMutex_;
    std::mutex mutex_;
    DistributedDB::KvStoreDelegateManager *kvStoreDelegateManager_ = nullptr;
    DistributedDB::KvStoreNbDelegate *delegate_ = nullptr;
    ObjectDataListener *objectDataListener_ = nullptr;
    uint32_t syncCount_ = 0;
    std::string userId_;
    std::atomic<bool> isSyncing_ = false;
    ConcurrentMap<uint32_t /* tokenId */, CallbackInfo > callbacks_;
    static constexpr size_t TIME_TASK_NUM = 1;
    static constexpr int64_t INTERVAL = 1;
    std::shared_ptr<ExecutorPool> executors_;
    DistributedData::AssetBindInfo ConvertBindInfo(ObjectStore::AssetBindInfo& bindInfo);
    VBucket ConvertVBucket(ObjectStore::ValuesBucket &vBucket);
    ConcurrentMap<std::string, std::shared_ptr<Snapshot>> snapshots_; // key:bundleName_sessionId
    ConcurrentMap<std::string, UriToSnapshot> bindSnapshots_; // key:bundleName_storeName
};
} // namespace DistributedObject
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_OBJECT_MANAGER_H
