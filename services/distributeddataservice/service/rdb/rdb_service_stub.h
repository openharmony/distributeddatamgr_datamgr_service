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

#include "iremote_stub.h"
#include "rdb_service.h"
#include "rdb_notifier.h"
#include "feature/feature_system.h"

namespace OHOS::DistributedRdb {
using RdbServiceCode = OHOS::DistributedRdb::RelationalStore::RdbServiceInterfaceCode;

class RdbServiceStub : public RdbService, public DistributedData::FeatureSystem::Feature {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"OHOS.DistributedRdb.IRdbService");
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply) override;

private:
    static bool CheckInterfaceToken(MessageParcel& data);

    int32_t OnRemoteObtainDistributedTableName(MessageParcel& data, MessageParcel& reply);

    int32_t OnDelete(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteInitNotifier(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteSetDistributedTables(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteDoSync(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteDoAsync(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteDoSubscribe(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteDoUnSubscribe(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteRegisterDetailProgressObserver(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteUnregisterDetailProgressObserver(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteDoRemoteQuery(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteNotifyDataChange(MessageParcel& data, MessageParcel& reply);

    int32_t OnRemoteQuerySharingResource(MessageParcel& data, MessageParcel& reply);

    int32_t OnBeforeOpen(MessageParcel& data, MessageParcel& reply);

    int32_t OnAfterOpen(MessageParcel& data, MessageParcel& reply);

    int32_t OnDisable(MessageParcel& data, MessageParcel& reply);

    int32_t OnEnable(MessageParcel& data, MessageParcel& reply);

    using RequestHandle = int (RdbServiceStub::*)(MessageParcel &, MessageParcel &);
    static constexpr RequestHandle HANDLERS[static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_MAX)] = {
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_OBTAIN_TABLE)] =
            &RdbServiceStub::OnRemoteObtainDistributedTableName,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_INIT_NOTIFIER)] = &RdbServiceStub::OnRemoteInitNotifier,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_SET_DIST_TABLE)] =
            &RdbServiceStub::OnRemoteSetDistributedTables,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_SYNC)] = &RdbServiceStub::OnRemoteDoSync,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_ASYNC)] = &RdbServiceStub::OnRemoteDoAsync,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_SUBSCRIBE)] = &RdbServiceStub::OnRemoteDoSubscribe,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_UNSUBSCRIBE)] = &RdbServiceStub::OnRemoteDoUnSubscribe,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_REMOTE_QUERY)] = &RdbServiceStub::OnRemoteDoRemoteQuery,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_DELETE)] = &RdbServiceStub::OnDelete,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_REGISTER_AUTOSYNC_PROGRESS_OBSERVER)] =
            &RdbServiceStub::OnRemoteRegisterDetailProgressObserver,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_UNREGISTER_AUTOSYNC_PROGRESS_OBSERVER)] =
            &RdbServiceStub::OnRemoteUnregisterDetailProgressObserver,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_NOTIFY_DATA_CHANGE)] =
            &RdbServiceStub::OnRemoteNotifyDataChange,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_QUERY_SHARING_RESOURCE)] =
            &RdbServiceStub::OnRemoteQuerySharingResource,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_DISABLE)] = &RdbServiceStub::OnDisable,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_ENABLE)] = &RdbServiceStub::OnEnable,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_BEFORE_OPEN)] = &RdbServiceStub::OnBeforeOpen,
        [static_cast<uint32_t>(RdbServiceCode::RDB_SERVICE_CMD_AFTER_OPEN)] = &RdbServiceStub::OnAfterOpen
    };
};
} // namespace OHOS::DistributedRdb
#endif
