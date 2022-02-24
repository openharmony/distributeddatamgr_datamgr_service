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

#ifndef DISTRIBUTEDDATAFWK_IRDB_SERVICE_H
#define DISTRIBUTEDDATAFWK_IRDB_SERVICE_H

#include <string>

#include <iremote_broker.h>
#include "rdb_service.h"
#include "rdb_types.h"

namespace OHOS::DistributedRdb {
class IRdbService : public RdbService, public IRemoteBroker {
public:
    enum {
        RDB_SERVICE_CMD_OBTAIN_TABLE,
        RDB_SERVICE_CMD_INIT_NOTIFIER,
        RDB_SERVICE_CMD_SET_DIST_TABLE,
        RDB_SERVICE_CMD_SYNC,
        RDB_SERVICE_CMD_ASYNC,
        RDB_SERVICE_CMD_SUBSCRIBE,
        RDB_SERVICE_CMD_UNSUBSCRIBE,
        RDB_SERVICE_CMD_MAX
    };
    
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedRdb.IRdbService");

    virtual int32_t InitNotifier(const RdbSyncerParam& param, const sptr<IRemoteObject> notifier) = 0;

protected:
    virtual int32_t DoSync(const RdbSyncerParam& param, const SyncOption& option,
                           const RdbPredicates& predicates, SyncResult& result) = 0;
    
    virtual int32_t DoAsync(const RdbSyncerParam& param, uint32_t seqNum,
                            const SyncOption& option, const RdbPredicates& predicates) = 0;

    virtual int32_t DoSubscribe(const RdbSyncerParam& param) = 0;

    virtual int32_t DoUnSubscribe(const RdbSyncerParam& param) = 0;
};
}
#endif
