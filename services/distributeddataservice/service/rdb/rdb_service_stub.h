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

#ifndef DISTRIBUTED_RDB_SERVICE_STUB_H
#define DISTRIBUTED_RDB_SERVICE_STUB_H

#include <iremote_stub.h>
#include "irdb_service.h"
#include "rdb_notifier.h"
#include "feature/feature_system.h"

namespace OHOS::DistributedRdb {
class RdbServiceStub : public RdbService, public DistributedData::FeatureSystem::Feature {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply) override;

    int32_t Sync(const RdbSyncerParam& param, const SyncOption& option,
                 const RdbPredicates& predicates, const SyncCallback& callback) override
    {
        return 0;
    }

    int32_t Subscribe(const RdbSyncerParam& param, const SubscribeOption& option,
                      RdbStoreObserver *observer) override
    {
        return 0;
    }

    int32_t UnSubscribe(const RdbSyncerParam& param, const SubscribeOption& option,
                        RdbStoreObserver *observer) override
    {
        return 0;
    }

private:
    static bool CheckInterfaceToken(MessageParcel& data);

    int32_t OnRemoteObtainDistributedTableName(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteInitNotifier(MessageParcel&data, MessageParcel& reply);

    int32_t OnRemoteSetDistributedTables(MessageParcel &data, MessageParcel &reply);

    int32_t OnRemoteDoSync(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteDoAsync(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteDoSubscribe(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteDoUnSubscribe(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteDoRemoteQuery(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteDoCreateTable(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteDoDestroyTable(MessageParcel& data, MessageParcel& reply);

    using RequestHandle = int (RdbServiceStub::*)(MessageParcel &, MessageParcel &);
    static constexpr RequestHandle HANDLERS[RDB_SERVICE_CMD_MAX] = {
        [RDB_SERVICE_CMD_OBTAIN_TABLE] = &RdbServiceStub::OnRemoteObtainDistributedTableName,
        [RDB_SERVICE_CMD_INIT_NOTIFIER] = &RdbServiceStub::OnRemoteInitNotifier,
        [RDB_SERVICE_CMD_SET_DIST_TABLE] = &RdbServiceStub::OnRemoteSetDistributedTables,
        [RDB_SERVICE_CMD_SYNC] = &RdbServiceStub::OnRemoteDoSync,
        [RDB_SERVICE_CMD_ASYNC] = &RdbServiceStub::OnRemoteDoAsync,
        [RDB_SERVICE_CMD_SUBSCRIBE] = &RdbServiceStub::OnRemoteDoSubscribe,
        [RDB_SERVICE_CMD_UNSUBSCRIBE] = &RdbServiceStub::OnRemoteDoUnSubscribe,
        [RDB_SERVICE_CMD_REMOTE_QUERY] = &RdbServiceStub::OnRemoteDoRemoteQuery,
        [RDB_SERVICE_CREATE_RDB_TABLE] = &RdbServiceStub::OnRemoteDoCreateTable,
        [RDB_SERVICE_DESTROY_RDB_TABLE] = &RdbServiceStub::OnRemoteDoDestroyTable
    };
};
} // namespace OHOS::DistributedRdb
#endif
