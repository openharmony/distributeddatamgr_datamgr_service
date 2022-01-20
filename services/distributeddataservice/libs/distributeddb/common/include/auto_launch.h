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

#ifndef AUTO_LAUNCH_H
#define AUTO_LAUNCH_H

#include <set>
#include <map>
#include <mutex>
#include "auto_launch_export.h"
#include "db_properties.h"
#include "ikvdb_connection.h"
#include "icommunicator_aggregator.h"
#include "kv_store_observer.h"
#include "kvdb_properties.h"
#include "types_export.h"
#include "relational_store_connection.h"
#include "relationaldb_properties.h"
#include "store_observer.h"

namespace DistributedDB {
enum class AutoLaunchItemState {
    UN_INITIAL = 0,
    IN_ENABLE,
    IN_LIFE_CYCLE_CALL_BACK, // in LifeCycleCallback
    IN_COMMUNICATOR_CALL_BACK, // in OnConnectCallback or CommunicatorLackCallback
    IDLE,
};

enum class DBType {
    DB_KV = 0,
    DB_RELATION,
    DB_INVALID,
};

struct AutoLaunchItem {
    std::shared_ptr<DBProperties> propertiesPtr;
    AutoLaunchNotifier notifier;
    KvStoreObserver *observer = nullptr;
    int conflictType = 0;
    KvStoreNbConflictNotifier conflictNotifier;
    void *conn = nullptr;
    KvDBObserverHandle *observerHandle = nullptr;
    bool isWriteOpenNotifiered = false;
    AutoLaunchItemState state = AutoLaunchItemState::UN_INITIAL;
    bool isDisable = false;
    bool inObserver = false;
    bool isAutoSync = true;
    DBType type = DBType::DB_INVALID;
    StoreObserver *relationObserver = nullptr;
};

class AutoLaunch {
public:
    AutoLaunch() = default;

    ~AutoLaunch();

    DISABLE_COPY_ASSIGN_MOVE(AutoLaunch);

    void SetCommunicatorAggregator(ICommunicatorAggregator *aggregator);

    int EnableKvStoreAutoLaunch(const KvDBProperties &properties, AutoLaunchNotifier notifier,
        const AutoLaunchOption &option);

    int DisableKvStoreAutoLaunch(const std::string &identifier);

    void GetAutoLaunchSyncDevices(const std::string &identifier, std::vector<std::string> &devices) const;

    void SetAutoLaunchRequestCallback(const AutoLaunchRequestCallback &callback, DBType type);

    static int GetAutoLaunchProperties(const AutoLaunchParam &param, const DBType &openType,
        std::shared_ptr<DBProperties> &propertiesPtr);

private:

    int EnableKvStoreAutoLaunchParmCheck(AutoLaunchItem &autoLaunchItem, const std::string &identifier);

    int GetKVConnectionInEnable(AutoLaunchItem &autoLaunchItem, const std::string &identifier);

    static int OpenOneConnection(AutoLaunchItem &autoLaunchItem);

    // we will return errCode, if errCode != E_OK
    static int CloseConnectionStrict(AutoLaunchItem &autoLaunchItem);

    // before ReleaseDatabaseConnection, if errCode != E_OK, we not return, we try close more
    static void TryCloseConnection(AutoLaunchItem &autoLaunchItem);

    int RegisterObserverAndLifeCycleCallback(AutoLaunchItem &autoLaunchItem, const std::string &identifier,
        bool isExt);

    int RegisterObserver(AutoLaunchItem &autoLaunchItem, const std::string &identifier, bool isExt);

    void ObserverFunc(const KvDBCommitNotifyData &notifyData, const std::string &identifier);

    void ConnectionLifeCycleCallbackTask(const std::string &identifier);

    void OnlineCallBackTask();

    void GetDoOpenMap(std::map<std::string, AutoLaunchItem> &doOpenMap);

    void GetConnInDoOpenMap(std::map<std::string, AutoLaunchItem> &doOpenMap);

    void UpdateGlobalMap(std::map<std::string, AutoLaunchItem> &doOpenMap);

    void ReceiveUnknownIdentifierCallBackTask(const std::string &identifier);

    static void CloseNotifier(const AutoLaunchItem &autoLaunchItem);

    void ConnectionLifeCycleCallback(const std::string &identifier);

    void OnlineCallBack(const std::string &device, bool isConnect);

    int ReceiveUnknownIdentifierCallBack(const LabelType &label);

    int AutoLaunchExt(const std::string &identifier);

    void AutoLaunchExtTask(const std::string identifier, AutoLaunchItem autoLaunchItem);

    void ExtObserverFunc(const KvDBCommitNotifyData &notifyData, const std::string &identifier);

    void ExtConnectionLifeCycleCallback(const std::string &identifier);

    void ExtConnectionLifeCycleCallbackTask(const std::string &identifier);

    static int SetConflictNotifier(AutoLaunchItem &autoLaunchItem);

    int ExtAutoLaunchRequestCallBack(const std::string &identifier, AutoLaunchParam &param, DBType &openType);

    static int GetAutoLaunchKVProperties(const AutoLaunchParam &param,
        const std::shared_ptr<KvDBProperties> &propertiesPtr);
        
    static int GetAutoLaunchRelationProperties(const AutoLaunchParam &param,
        const std::shared_ptr<RelationalDBProperties> &propertiesPtr);

    static int OpenKvConnection(AutoLaunchItem &autoLaunchItem);

    static int OpenRelationalConnection(AutoLaunchItem &autoLaunchItem);

    int RegisterLifeCycleCallback(AutoLaunchItem &autoLaunchItem, const std::string &identifier,
        bool isExt);

    static int PragmaAutoSync(AutoLaunchItem &autoLaunchItem);

    static void TryCloseKvConnection(AutoLaunchItem &autoLaunchItem);

    static void TryCloseRelationConnection(AutoLaunchItem &autoLaunchItem);

    mutable std::mutex dataLock_;
    mutable std::mutex communicatorLock_;
    std::set<std::string> onlineDevices_;
    std::map<std::string, AutoLaunchItem> autoLaunchItemMap_;
    ICommunicatorAggregator *communicatorAggregator_ = nullptr;
    std::condition_variable cv_;

    std::mutex extLock_;
    std::map<DBType, AutoLaunchRequestCallback> autoLaunchRequestCallbackMap_;
    std::map<std::string, AutoLaunchItem> extItemMap_;
};
} // namespace DistributedDB
#endif // AUTO_LAUNCH_H
