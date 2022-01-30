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

#ifndef DISTRIBUTED_RDB_DEVICE_SYNCER_H
#define DISTRIBUTED_RDB_DEVICE_SYNCER_H

#include "rdb_syncer_impl.h"
#include "rdb_syncer_stub.h"
#include "rdb_types.h"
#include "rdb_syncer_factory.h"

namespace DistributedDB {
class RelationalStoreManager;
class RelationalStoreDelegate;
}

namespace OHOS::DistributedRdb {
class RdbDeviceSyncer : public RdbSyncerImpl {
public:
    explicit RdbDeviceSyncer(const RdbSyncerParam& param, pid_t uid);
    
    ~RdbDeviceSyncer() override;
    
    /* IPC interface */
    int SetDistributedTables(const std::vector<std::string>& tables) override;
    
    /* RdbStore interface */
    int Init() override;
    
private:
    int CreateMetaData();
    
    DistributedDB::RelationalStoreDelegate* GetDelegate();
    
    std::mutex mutex_;
    bool isInit_;
    DistributedDB::RelationalStoreManager* manager_;
    DistributedDB::RelationalStoreDelegate* delegate_;
    
    static inline RdbSyncerRegistration<RdbDeviceSyncer, RDB_DEVICE_COLLABORATION> registration_;
};
}
#endif
