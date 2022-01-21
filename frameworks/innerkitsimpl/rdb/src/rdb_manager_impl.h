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

#ifndef DISTRIBUTED_RDB_MANAGER_IMPL_H
#define DISTRIBUTED_RDB_MANAGER_IMPL_H

#include <map>
#include <memory>
#include <mutex>

#include "refbase.h"
#include "iremote_object.h"
#include "concurrent_map.h"
#include "rdb_types.h"
#include "rdb_syncer.h"

namespace OHOS::DistributedKv {
class IKvStoreDataService;
}

namespace OHOS::DistributedRdb {
class RdbService;
class RdbManagerImpl {
public:
    static RdbManagerImpl &GetInstance();
    
    std::shared_ptr<RdbSyncer> GetRdbSyncer(const RdbSyncerParam& param);
    
    int RegisterRdbServiceDeathObserver(const std::string &storeName, const std::function<void()>& callback);
    
    int UnRegisterRdbServiceDeathObserver(const std::string &storeName);
    
    void OnRemoteDied();

private:
    RdbManagerImpl();
    
    ~RdbManagerImpl();
    
    std::shared_ptr<RdbService> GetRdbService();
    
    void ResetServiceHandle();
    
    void NotifyServiceDeath();
    
    void RegisterClientDeathRecipient(const std::string &bundleName);
    
    std::mutex mutex_;
    sptr<OHOS::DistributedKv::IKvStoreDataService> distributedDataMgr_;
    std::shared_ptr<RdbService> rdbService_;
    sptr<IRemoteObject> clientDeathObject_;
    
    ConcurrentMap<std::string, std::function<void()>> serviceDeathObservers_;
};
}
#endif //DISTRIBUTED_RDB_MANAGER_IMPL_H
