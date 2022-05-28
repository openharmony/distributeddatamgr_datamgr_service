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

#ifndef SINGLE_KV_STORE_H
#define SINGLE_KV_STORE_H

#include <map>
#include "kvstore.h"
#include "kvstore_observer.h"
#include "kvstore_result_set.h"
#include "kvstore_sync_callback.h"
#include "types.h"
#include "data_query.h"

namespace OHOS {
namespace DistributedKv {
// This is a public interface. Implementation of this class is in AppKvStoreImpl.
// This class provides put, delete, search, sync and subscribe functions of a key-value store.
class API_EXPORT SingleKvStore : public virtual KvStore {
public:
    API_EXPORT SingleKvStore() = default;

    API_EXPORT virtual ~SingleKvStore() {}

    // Get value from AppKvStore by its key.
    // Parameters:
    //     key: key of this entry.
    //     value: value will be returned in this parameter.
    // Return:
    //     Status of this get operation.
    virtual Status Get(const Key &key, Value &value) = 0;

    // Get all entries in this store which key start with prefixKey.
    // Parameters:
    //     perfixkey: the prefix to be searched.
    //     entries: entries will be returned in this parameter.
    // Return:
    //     Status of this GetEntries operation.
    virtual Status GetEntries(const Key &prefix, std::vector<Entry> &entries) const = 0;

    // Get all entries in this store by query.
    // Parameters:
    //     query: the query object.
    //     entries: entries will be returned in this parameter.
    // Return:
    //     Status of this GetEntries operation.
    virtual Status GetEntries(const DataQuery &query, std::vector<Entry> &entries) const = 0;

    // Get ResultSet in this store which key start with prefixKey.
    // Parameters:
    //     perfixkey: the prefix to be searched.
    //     resultSet: resultSet will be returned in this parameter.
    // Return:
    //     Status of this GetResultSet operation.
    virtual Status GetResultSet(const Key &prefix, std::shared_ptr<KvStoreResultSet> &resultSet) const = 0;

    // Get ResultSet in this store by Query.
    // Parameters:
    //     query: the query object.
    //     resultSet: resultSet will be returned in this parameter.
    // Return:
    //     Status of this GetResultSet operation.
    virtual Status GetResultSet(const DataQuery &query, std::shared_ptr<KvStoreResultSet> &resultSet) const = 0;

    // Close the ResultSet returned by GetResultSet.
    // Parameters:
    //     resultSet: resultSet will be returned in this parameter.
    // Return:
    //     Status of this CloseResultSet operation.
    virtual Status CloseResultSet(std::shared_ptr<KvStoreResultSet> &resultSet) = 0;

    // Get the number of result by query.
    // Parameters:
    //     query: the query object.
    //     result: result will be returned in this parameter.
    // Return:
    //     Status of this CloseResultSet operation.
    virtual Status GetCount(const DataQuery &query, int &count) const = 0;

    // Remove the device data synced from remote.
    // Parameters:
    //     device: device id.
    // Return:
    //     Status of this remove operation.
    virtual Status RemoveDeviceData(const std::string &device) = 0;

    virtual Status GetSecurityLevel(SecurityLevel &securityLevel) const = 0;

    // Sync store with other devices. This is an asynchronous method,
    // sync will fail if there is a syncing operation in progress.
    // Parameters:
    //     deviceIds: device list to sync.
    //     mode: mode can be set to SyncMode::PUSH, SyncMode::PULL and SyncMode::PUTH_PULL. PUSH_PULL will firstly
    //           push all not-local store to listed devices, then pull these stores back.
    //     allowedDelayMs: allowed delay milli-second to sync.
    // Return:
    //     Status of this Sync operation.
    virtual Status Sync(const std::vector<std::string> &devices, SyncMode mode, uint32_t allowedDelayMs) = 0;

    /*
     *  Sync store with other devices only syncing the data which is satisfied with the condition.
     *  This is an asynchronous method, sync will fail if there is a syncing operation in progress.
     * Parameters:
     *     deviceIds: device list to sync, this is network id from soft bus.
     *     query: the query condition.
     *     mode: mode can be set to SyncMode::PUSH, SyncMode::PULL and SyncMode::PUSH_PULL. PUSH_PULL will firstly
     *           push all not-local store to listed devices, then pull these stores back.
     * Return:
     *     Status of this Sync operation.
     */
    virtual Status Sync(const std::vector<std::string> &devices, SyncMode mode, const DataQuery &query,
        std::shared_ptr<KvStoreSyncCallback> syncCallback) = 0;

    API_EXPORT inline Status Sync(const std::vector<std::string> &devices, SyncMode mode)
    {
        return Sync(devices, mode, 0);
    }

    API_EXPORT inline Status Sync(const std::vector<std::string> &devices, SyncMode mode, const DataQuery &query)
    {
        return Sync(devices, mode, query, nullptr);
    }

    // register message for sync operation.
    // Parameters:
    //     callback: callback to register.
    // Return:
    //     Status of this register operation.
    virtual Status RegisterSyncCallback(std::shared_ptr<KvStoreSyncCallback> callback) = 0;

    // un-register message for sync operation.
    // Parameters:
    //     callback: callback to register.
    // Return:
    //     Status of this register operation.
    virtual Status UnRegisterSyncCallback() = 0;

    // set synchronization parameters of this store.
    // Parameters:
    //     syncParam: sync policy parameter.
    // Return:
    //     Status of this operation.
    virtual Status SetSyncParam(const KvSyncParam &syncParam) = 0;

    // get synchronization parameters of this store.
    // Parameters:
    //     syncParam: sync policy parameter.
    // Return:
    //     Status of this operation.
    virtual Status GetSyncParam(KvSyncParam &syncParam) = 0;

    virtual Status SetCapabilityEnabled(bool enabled) const = 0;

    virtual Status SetCapabilityRange(const std::vector<std::string> &localLabels,
                                      const std::vector<std::string> &remoteLabels) const = 0;

    /*
     * Subscribe store with other devices consistently Synchronize the data which is satisfied with the condition.
     * Parameters:
     *     deviceIds: device list to sync, this is network id from soft bus.
     *     query: the query condition.
     * Return:
     *     Status of this Subscribe operation.
     */
    virtual Status SubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query) = 0;

    /*
     * UnSubscribe store with other devices which is satisfied with the condition.
     * Parameters:
     *     deviceIds: device list to sync, this is network id from soft bus.
     *     query: the query condition.
     * Return:
     *     Status of this UnSubscribe operation.
     */
    virtual Status UnsubscribeWithQuery(const std::vector<std::string> &devices, const DataQuery &query) = 0;
};
}  // namespace DistributedKv
}  // namespace OHOS
#endif  // SINGLE_KV_STORE_H
