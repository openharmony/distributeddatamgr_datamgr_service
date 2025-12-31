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
#include "utils/anonymous.h"
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
    int64_t totalSize = insertSize + updateSize + deleteSize + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint64_t)
        + sizeof(uint64_t);
    if (insertSize < 0 || updateSize < 0 || deleteSize < 0 || !ITypesUtil::Marshal(data, totalSize)) {
        ZLOGE("Write entry size to parcel fail, totalSize:%{public}" PRId64, totalSize);
        return;
    }
    if (totalSize < SWITCH_RAW_DATA_SIZE) {
        if (!ITypesUtil::Marshal(data, changeNotification)) {
            ZLOGE("Write ChangeNotification to parcel fail, totalSize:%{public}" PRId64, totalSize);
            return;
        }
    } else {
        if (!ITypesUtil::Marshal(data, changeNotification.GetDeviceId(), uint32_t(changeNotification.IsClear()),
            static_cast<uint64_t>(changeNotification.GetInsertEntries().size()),
            static_cast<uint64_t>(changeNotification.GetUpdateEntries().size()),
            static_cast<uint64_t>(changeNotification.GetDeleteEntries().size()))) {
            ZLOGE("Write deviceId to parcel fail, deviceId:%{public}s",
                DistributedData::Anonymous::Change(changeNotification.GetDeviceId()).c_str());
            return;
        }
        std::vector<Entry> totalEntries;
        totalEntries.reserve(changeNotification.GetInsertEntries().size() +
            changeNotification.GetUpdateEntries().size() + changeNotification.GetDeleteEntries().size());
        totalEntries.insert(totalEntries.end(), changeNotification.GetInsertEntries().begin(),
            changeNotification.GetInsertEntries().end());
        totalEntries.insert(totalEntries.end(), changeNotification.GetUpdateEntries().begin(),
            changeNotification.GetUpdateEntries().end());
        totalEntries.insert(totalEntries.end(), changeNotification.GetDeleteEntries().begin(),
            changeNotification.GetDeleteEntries().end());
        if (!ITypesUtil::MarshalToBuffer(totalEntries, totalSize, data)) {
            ZLOGE("Write entries to parcel buffer fail, totalSize:%{public}" PRId64, totalSize);
            return;
        }
    }
    MessageOption option(MessageOption::TF_ASYNC);
    int error = Remote()->SendRequest(ONCHANGE, data, reply, option);
    if (error != 0) {
        ZLOGE("send request fail, error:%{public}d", error);
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