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

#ifndef SINGLE_VER_SUBSCRIBE_MANAGER_H
#define SINGLE_VER_SUBSCRIBE_MANAGER_H

#include <map>
#include <shared_mutex>
#include "query_sync_object.h"

namespace DistributedDB {
enum class SubscribeStatus {
    NOT_ACTIVE = 0,
    ACTIVE = 1,
};

using SubscribeMap = std::map<std::string, std::map<std::string, SubscribeStatus>>;
using SubscribedTotalMap = std::map<std::string, std::pair<QuerySyncObject, int>>;

class SubscribeManager {
public:
    SubscribeManager() = default;
    ~SubscribeManager() {};

    DISABLE_COPY_ASSIGN_MOVE(SubscribeManager);

    // clear remoteSubscribeMap_[device] list when remote db is closed or dev offline.
    void ClearRemoteSubscribeQuery(const std::string &device);

    // clear localSubscribeMap_[device] list when device is offline.
    void ClearLocalSubscribeQuery(const std::string &device);

    void ClearAllRemoteQuery();

    // add query when receive subscribe command
    int ReserveRemoteSubscribeQuery(const std::string &device, const QuerySyncObject &query);

    // active query to ACTIVE when send ack ok
    int ActiveRemoteSubscribeQuery(const std::string &device, const QuerySyncObject &query);

    // reserve query when user call SubscribeRemoteQuery, status set to NOT_ACTIVE
    int ReserveLocalSubscribeQuery(const std::string &device, const QuerySyncObject &query);

    // active query to ACTIVE when receive ack ok
    int ActiveLocalSubscribeQuery(const std::string &device, const QuerySyncObject &query);

    // delete local subscribe query when recv wrong errCode, only not_active status allowed to del
    void DeleteLocalSubscribeQuery(const std::string &device, const QuerySyncObject &query);

    // delete remote subscribe query when send msg failed, only not_active status allowed to del
    void DeleteRemoteSubscribeQuery(const std::string &device, const QuerySyncObject &query);

    // put subscribe queries into unfinished map when remote db online
    void PutLocalUnFiniedSubQueries(const std::string &device, std::vector<QuerySyncObject> &subscribeQueries);

    // get all device unFinished subscribe queries which triggered by auto subscribe and need retry subscribe
    void GetAllUnFinishSubQueries(std::map<std::string, std::vector<QuerySyncObject>> &allSyncQueries) const;

    // remove query when receive unsubscribe command
    void RemoveRemoteSubscribeQuery(const std::string &device, const QuerySyncObject &query);

    // remove query when user call UnSubscribeRemoteQuery
    void RemoveLocalSubscribeQuery(const std::string &device, const QuerySyncObject &query);

    // get device active subscribeQueries from localSubscribeMap_
    void GetLocalSubscribeQueries(const std::string &device, std::vector<QuerySyncObject> &subscribeQueries) const;

    // get device remote queryId from remoteSubscribedMap_ while data change
    void GetRemoteSubscribeQueryIds(const std::string &device, std::vector<std::string> &subscribeQueryIds) const;
    // get device remote subscribeQueries from remoteSubscribedMap_ while data change
    void GetRemoteSubscribeQueries(const std::string &device, std::vector<QuerySyncObject> &subscribeQueries) const;

    bool IsRemoteContainSubscribe(const std::string &device, const QuerySyncObject &query) const;

    int LocalSubscribeLimitCheck(const std::vector<std::string> &devices, QuerySyncObject &query) const;
private:
    void ClearSubscribeQuery(const std::string &device, SubscribeMap &subscribeMap,
        SubscribedTotalMap &subscribedTotalMap);

    int ReserveSubscribeQuery(const std::string &device, const QuerySyncObject &query, SubscribeMap &subscribeMap,
        SubscribedTotalMap &subscribedTotalMap);

    int ActiveSubscribeQuery(const std::string &device, const std::string &queryId, SubscribeMap &subscribeMap,
        SubscribedTotalMap &subscribedTotalMap);

    void DeleteSubscribeQuery(const std::string &device, const std::string &queryId, SubscribeMap &subscribeMap,
        SubscribedTotalMap &subscribedTotalMap);

    void RemoveSubscribeQuery(const std::string &device, const std::string &queryId, SubscribeMap &subscribeMap,
        SubscribedTotalMap &subscribedTotalMap);

    void GetSubscribeQueries(const std::string &device, const SubscribeMap &subscribeMap,
        const SubscribedTotalMap &subscribedTotalMap, std::vector<QuerySyncObject> &subscribeQueries) const;

    mutable std::shared_mutex localSubscribeMapLock_;
    // subscribe sponsor, key: device, value: pair<queryId, status> map
    // status 0: active, 1: not active
    SubscribeMap localSubscribeMap_;

    // used retry subscribe in db open scene, key: device value: set<queryId>
    std::map<std::string, std::set<std::string>> unFinishedLocalAutoSubMap_;

    // subscribe sponsor total query info, key:queryId, value:<QuerySyncObject, user_num>
    // while use_num is 0, delete item from the map
    SubscribedTotalMap localSubscribeTotalMap_;

    mutable std::shared_mutex remoteSubscribedMapLock_;
    // subscribed, key: device, value: pair<queryId, status> map
    // status 0: active, 1: not active
    SubscribeMap remoteSubscribedMap_;

    // subscribed total query info, key:queryId, value:<QuerySyncObject, user_num>
    // while use_num is 0, delete item from the map
    SubscribedTotalMap remoteSubscribedTotalMap_;
};
} // namespace DistributedDB

#endif // SINGLE_VER_SUBSCRIBE_MANAGER_H