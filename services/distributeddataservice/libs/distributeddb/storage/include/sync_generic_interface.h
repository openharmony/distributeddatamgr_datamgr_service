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
#ifndef SYNC_GENERIC_INTERFACE_H
#define SYNC_GENERIC_INTERFACE_H

#include "isync_interface.h"
#include "single_ver_kv_entry.h"
#include "query_object.h"

namespace DistributedDB {
class SyncGenericInterface : public ISyncInterface {
public:
    // Constructor/Destructor.
    SyncGenericInterface() = default;
    ~SyncGenericInterface() override = default;

    virtual int GetSyncData(Timestamp begin, Timestamp end, std::vector<DataItem> &dataItems,
        ContinueToken &continueStmtToken, const DataSizeSpecInfo &dataSizeInfo) const
    {
        LOGE("GetSyncData not support!");
        return -E_NOT_SUPPORT;
    }

    // Get the data which would be synced to other devices according the timestamp.
    // if the data size is over than the blockSize, It would alloc one token and assign to continueStmtToken,
    // it should be released when the read operation terminate.
    virtual int GetSyncData(Timestamp begin, Timestamp end, std::vector<SingleVerKvEntry *> &entries,
        ContinueToken &continueStmtToken, const DataSizeSpecInfo &dataSizeInfo) const
    {
        LOGE("GetSyncData not support!");
        return -E_NOT_SUPPORT;
    }

    // Get the data which would be synced with query condition
    virtual int GetSyncData(QueryObject &query, const SyncTimeRange &timeRange,
        const DataSizeSpecInfo &dataSizeInfo, ContinueToken &continueStmtToken,
        std::vector<SingleVerKvEntry *> &entries) const
    {
        return -E_NOT_SUPPORT;
    }

    virtual int GetSyncDataNext(std::vector<DataItem> &dataItems, ContinueToken &continueStmtToken,
        const DataSizeSpecInfo &dataSizeInfo) const
    {
        return -E_NOT_SUPPORT;
    }

    virtual int GetSyncDataNext(std::vector<SingleVerKvEntry *> &entries, ContinueToken &continueStmtToken,
        const DataSizeSpecInfo &dataSizeInfo) const
    {
        return -E_NOT_SUPPORT;
    }

    virtual int GetCompressionOption(bool &needCompressOnSync, uint8_t &compressionRate) const
    {
        return -E_NOT_SUPPORT;
    }

    // Release the continue token of getting data.
    virtual void ReleaseContinueToken(ContinueToken &continueStmtToken) const
    {
    }

    virtual int RemoveDeviceData(const std::string &deviceName, bool isNeedNotify)
    {
        return -E_NOT_SUPPORT;
    }

    virtual bool IsReadable() const
    {
        return true;
    }

    virtual int GetSecurityOption(SecurityOption &option) const
    {
        return -E_NOT_SUPPORT;
    }

    virtual void NotifyRemotePushFinished(const std::string &targetId) const
    {
    }

    // Get the timestamp when database created or imported
    virtual int GetDatabaseCreateTimestamp(Timestamp &outTime) const
    {
        return -E_NOT_SUPPORT;
    }

    virtual int PutSyncDataWithQuery(const QueryObject &query, const std::vector<SingleVerKvEntry *> &entries,
        const std::string &deviceName)
    {
        return -E_NOT_SUPPORT;
    }

    virtual int CheckAndInitQueryCondition(QueryObject &query) const
    {
        return -E_NOT_SUPPORT;
    }

    virtual int InterceptData(std::vector<SingleVerKvEntry *> &entries, const std::string &sourceID,
        const std::string &targetID) const
    {
        return -E_NOT_SUPPORT;
    }

    virtual int AddSubscribe(const std::string &subscribeId, const QueryObject &query, bool needCacheSubscribe)
    {
        return -E_NOT_SUPPORT;
    }

    virtual int RemoveSubscribe(const std::string &subscribeId)
    {
        return -E_NOT_SUPPORT;
    }

    virtual int RemoveSubscribe(const std::vector<std::string> &subscribeIds)
    {
        return -E_NOT_SUPPORT;
    }

    virtual int GetCompressionAlgo(std::set<CompressAlgorithm> &algorithmSet) const
    {
        return -E_NOT_SUPPORT;
    }

    virtual bool CheckCompatible(const std::string &schema, uint8_t type) const
    {
        return false;
    }

    std::vector<uint8_t> GetDualTupleIdentifier() const override
    {
        return {};
    }
};
}
#endif // SYNC_GENERIC_INTERFACE_H
