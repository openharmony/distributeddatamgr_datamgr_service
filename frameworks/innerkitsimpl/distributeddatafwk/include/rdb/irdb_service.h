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
#include "irdb_store.h"
#include "rdb_parcel.h"
#include "rdb_client_death_recipient.h"

namespace OHOS::DistributedKv {
class IRdbService : public IRemoteBroker {
public:
    enum {
        RDB_SERVICE_CMD_GET_STORE,
        RDB_SERVICE_CMD_REGISTER_CLIENT_DEATH,
        RDB_SERVICE_CMD_MAX
    };
    
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedKv.IRdbService");
    
    virtual sptr<IRdbStore> GetRdbStore(const RdbStoreParam& param) = 0;
    
    virtual int RegisterClientDeathRecipient(const std::string& bundleName, sptr<IRemoteObject> object) = 0;
};
}
#endif
