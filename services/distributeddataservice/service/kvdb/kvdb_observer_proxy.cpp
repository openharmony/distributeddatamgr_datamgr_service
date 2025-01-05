/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define LOG_TAG "KVDBObserverProxy"

#include "kvdb_observer_proxy.h"

#include <cinttypes>
#include <ipc_skeleton.h>
#include "kv_types_util.h"
#include "itypes_util.h"
#include "log_print.h"
#include "message_parcel.h"
namespace OHOS {
namespace DistributedKv {
using namespace std::chrono;

enum {
    CLOUD_ONCHANGE,
    ONCHANGE,
};

KVDBObserverProxy::KVDBObserverProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IKvStoreObserver>(impl)
{
}

int64_t GetBufferSize(const std::vector<Entry> &entries)
{
    int64_t bufferSize = 0;
    for (const auto &item : entries) {
        bufferSize += item.key.RawSize() + item.value.RawSize();
    }
    return bufferSize;
}

void KVDBObserverProxy::OnChange(const ChangeNotification &changeNotification)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(KVDBObserverProxy::GetDescriptor())) {
        ZLOGE("Write descriptor failed");
        return;
    }
    int64_t insertSize = ITypesUtil::GetTotalSize(changeNotification.GetInsertEntries());
    int64_t updateSize = ITypesUtil::GetTotalSize(changeNotification.GetUpdateEntries());
    int64_t deleteSize = ITypesUtil::GetTotalSize(changeNotification.GetDeleteEntries());
    int64_t totalSize = insertSize + updateSize + deleteSize + sizeof(uint32_t);
    if (insertSize < 0 || updateSize < 0 || deleteSize < 0 || !data.WriteInt32(totalSize)) {
        ZLOGE("Write ChangeNotification buffer size to parcel failed.");
        return;
    }
    ZLOGD("I(%" PRId64 ") U(%" PRId64 ") D(%" PRId64 ") T(%" PRId64 ")", insertSize, updateSize, deleteSize, totalSize);
    if (totalSize < SWITCH_RAW_DATA_SIZE) {
        if (!ITypesUtil::Marshal(data, changeNotification)) {
            ZLOGW("Write ChangeNotification to parcel failed.");
            return;
        }
    } else {
        if (!ITypesUtil::Marshal(data, changeNotification.GetDeviceId(), uint32_t(changeNotification.IsClear())) ||
            !ITypesUtil::MarshalToBuffer(changeNotification.GetInsertEntries(), insertSize, data) ||
            !ITypesUtil::MarshalToBuffer(changeNotification.GetUpdateEntries(), updateSize, data) ||
            !ITypesUtil::MarshalToBuffer(changeNotification.GetDeleteEntries(), deleteSize, data)) {
            ZLOGE("WriteChangeList to Parcel by buffer failed");
            return;
        }
    }

    MessageOption mo{ MessageOption::TF_WAIT_TIME };
    int error = Remote()->SendRequest(ONCHANGE, data, reply, mo);
    if (error != 0) {
        ZLOGE("SendRequest failed, error %d", error);
    }
}

void KVDBObserverProxy::OnChange(const DataOrigin &origin, Keys &&keys)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(KVDBObserverProxy::GetDescriptor())) {
        ZLOGE("Write descriptor failed");
        return;
    }
    if (!ITypesUtil::Marshal(data, origin.store, keys[OP_INSERT], keys[OP_UPDATE], keys[OP_DELETE])) {
        ZLOGE("WriteChangeInfo to Parcel failed.");
        return;
    }

    MessageOption mo{ MessageOption::TF_WAIT_TIME };
    int error = Remote()->SendRequest(CLOUD_ONCHANGE, data, reply, mo);
    if (error != 0) {
        ZLOGE("SendRequest failed, error %d", error);
    }
}
} // namespace DistributedKv
} // namespace OHOS