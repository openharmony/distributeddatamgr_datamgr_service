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

#include "communication_provider.h"
#include "iobject_callback.h"
#include "kvstore_sync_callback.h"
#include "types.h"
#include "kv_store_delegate_manager.h"
#include "object_common.h"

namespace OHOS {
namespace DistributedObject {
using SyncCallBack = std::function<void(const std::map<std::string, int32_t> &results)>;
enum Result {
    SUCCESS_USER_IN_USE,
    SUCCESS_USER_HAS_FINISHED,
    ERR_SID_NOT_EXIST
};
enum Status : int32_t {
    SUCCESS = 0,
    FAILED
};
class SequenceSyncManager {
public:
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
    ObjectStoreManager();
    static ObjectStoreManager *GetInstance()
    {
        static ObjectStoreManager *manager = new ObjectStoreManager();
        return manager;
    }
    int32_t Save(const std::string &appId, const std::string &sessionId,
        const std::map<std::string, std::vector<uint8_t>> &data, const std::vector<std::string> &deviceList,
        sptr<IObjectSaveCallback> &callback);
    int32_t RevokeSave(
        const std::string &appId, const std::string &sessionId, sptr<IObjectRevokeSaveCallback> &callback);
    int32_t Retrieve(const std::string &appId, const std::string &sessionId, sptr<IObjectRetrieveCallback> callback);
    void SetData(const std::string &dataDir, const std::string &userId);
    int32_t Clear();
private:
    constexpr static const char *SEPERATOR = "_";
    constexpr static const char *PROPERTY_PREFIX = "p_";
    constexpr static const char *LOCAL_DEVICE = "local";
    DistributedDB::KvStoreNbDelegate *OpenObjectKvStore();
    void FlushClosedStore();
    int32_t Open();
    int32_t Close();
    int32_t SetSyncStatus(bool status);
    int32_t SaveToStore(const std::string &appId, const std::string &sessionId,
        const std::map<std::string, std::vector<uint8_t>> &data);
    int32_t SyncOnStore(const std::string &prefix, const std::vector<std::string> &deviceList, SyncCallBack &callback);
    int32_t RevokeSaveToStore(const std::string &appId, const std::string &sessionId);
    int32_t RetrieveFromStore(
        const std::string &appId, const std::string &sessionId, std::map<std::string, std::vector<uint8_t>> &results);
    void SyncCompleted(const std::map<std::string, DistributedDB::DBStatus> &results, uint64_t sequenceId);
    inline std::string GetPropertyPrefix(const std::string &appId, const std::string &sessionId)
    {
        return appId + SEPERATOR + sessionId + SEPERATOR
               + AppDistributedKv::CommunicationProvider::GetInstance().GetLocalDevice().udid + PROPERTY_PREFIX;
    };
    inline std::string GetDeletePrefix(const std::string &appId, const std::string &sessionId)
    {
        return appId + SEPERATOR + sessionId + SEPERATOR;
    };
    inline std::string BytesToStr(const std::vector<uint8_t> &src)
    {
        return std::string(src.begin(), src.end());
    };
    std::mutex kvStoreMutex_;
    DistributedDB::KvStoreDelegateManager *kvStoreDelegateManager_ = nullptr;
    DistributedDB::KvStoreNbDelegate *delegate_ = nullptr;
    uint32_t syncCount_ = 0;
    std::string userId_;
    std::atomic<bool> isSyncing_ = false;
};
} // namespace DistributedObject
} // namespace OHOS
#endif // DISTRIBUTEDDATAMGR_OBJECT_MANAGER_H
