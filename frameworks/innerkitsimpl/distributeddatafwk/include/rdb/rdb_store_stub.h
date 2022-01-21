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

#ifndef DISTRIBUTEDDATAFWK_IRDB_STORE_STUB_H
#define DISTRIBUTEDDATAFWK_IRDB_STORE_STUB_H

#include <iremote_stub.h>
#include "irdb_store.h"

namespace OHOS::DistributedKv {
class RdbStoreStub : public IRemoteStub<IRdbStore> {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;

private:
    static bool CheckInterfaceToken(MessageParcel& data);
    
    int OnRemoteSetDistributedTables(MessageParcel &data, MessageParcel &reply);
    
    using RequestHandle = int (RdbStoreStub::*)(MessageParcel &, MessageParcel &);
    static constexpr RequestHandle HANDLERS[RDB_STORE_CMD_MAX] = {
        [RDB_STORE_CMD_SET_DIST_TABLES] = &RdbStoreStub::OnRemoteSetDistributedTables,
    };
};
}
#endif
