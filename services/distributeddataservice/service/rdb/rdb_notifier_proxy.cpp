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
#define LOG_TAG "RdbNotifierProxy"
#include "rdb_notifier_proxy.h"
#include "itypes_util.h"
#include "log_print.h"
#include "utils/anonymous.h"
namespace OHOS::DistributedRdb {
using NotifierIFCode = RelationalStore::IRdbNotifierInterfaceCode;

RdbNotifierProxy::RdbNotifierProxy(const sptr<IRemoteObject> &object) : IRemoteProxy<RdbNotifierProxyBroker>(object)
{
}

RdbNotifierProxy::~RdbNotifierProxy() noexcept
{
}

int32_t RdbNotifierProxy::OnComplete(uint32_t seqNum, Details &&result)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return RDB_ERROR;
    }
    if (!ITypesUtil::Marshal(data, seqNum, result)) {
        return RDB_ERROR;
    }

    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    if (Remote()->SendRequest(
        static_cast<uint32_t>(NotifierIFCode::RDB_NOTIFIER_CMD_SYNC_COMPLETE), data, reply, option) != 0) {
        ZLOGE("seqNum:%{public}u, send request failed", seqNum);
        return RDB_ERROR;
    }
    return RDB_OK;
}

int32_t RdbNotifierProxy::OnChange(const Origin &origin, const PrimaryFields &primaries, ChangeInfo &&changeInfo)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return RDB_ERROR;
    }
    if (!ITypesUtil::Marshal(data, origin, primaries, changeInfo)) {
        ZLOGE("write store name or devices failed");
        return RDB_ERROR;
    }

    MessageParcel reply;
    MessageOption option;
    if (Remote()->SendRequest(
        static_cast<uint32_t>(NotifierIFCode::RDB_NOTIFIER_CMD_DATA_CHANGE), data, reply, option) != 0) {
        ZLOGE("storeName:%{public}s, send request failed", DistributedData::Anonymous::Change(origin.store).c_str());
        return RDB_ERROR;
    }
    return RDB_OK;
}

int32_t RdbNotifierProxy::OnComplete(const std::string& storeName, Details&& result)
{
    MessageParcel data;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return RDB_ERROR;
    }
    if (!ITypesUtil::Marshal(data, storeName, result)) {
        return RDB_ERROR;
    }

    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    if (Remote()->SendRequest(
        static_cast<uint32_t>(NotifierIFCode::RDB_NOTIFIER_CMD_AUTO_SYNC_COMPLETE), data, reply, option) != 0) {
        ZLOGE("storeName:%{public}s, send request failed", DistributedData::Anonymous::Change(storeName).c_str());
        return RDB_ERROR;
    }
    return RDB_OK;
}
} // namespace OHOS::DistributedRdb
