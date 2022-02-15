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

#ifndef DISTRIBUTED_RDB_SERVICE_H
#define DISTRIBUTED_RDB_SERVICE_H

#include <memory>
#include <string>

#include "rdb_types.h"

namespace OHOS::DistributedRdb {
class RdbService {
public:
    virtual std::string ObtainDistributedTableName(const std::string& device, const std::string& table) = 0;

    virtual int32_t SetDistributedTables(const RdbSyncerParam& param,
                                         const std::vector<std::string>& tables) = 0;
    
    virtual int32_t Sync(const RdbSyncerParam& param, const SyncOption& option,
                         const RdbPredicates& predicates, const SyncCallback& callback) = 0;

    virtual int32_t Subscribe(const RdbSyncerParam& param, const SubscribeOption& option,
                              RdbStoreObserver *observer) = 0;

    virtual int32_t UnSubscribe(const RdbSyncerParam& param, const SubscribeOption& option,
                                RdbStoreObserver *observer) = 0;
};
}
#endif
