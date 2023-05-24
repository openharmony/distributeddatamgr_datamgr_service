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

#define LOG_TAG "IObjectCallbackProxy"
#include "object_callback_proxy.h"

#include "ipc_skeleton.h"
#include "itypes_util.h"
#include "log_print.h"

namespace OHOS {
namespace DistributedObject {
ObjectSaveCallbackProxy::ObjectSaveCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<ObjectSaveCallbackProxyBroker>(impl)
{
}

ObjectRevokeSaveCallbackProxy::ObjectRevokeSaveCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<ObjectRevokeSaveCallbackProxyBroker>(impl)
{
}

ObjectRetrieveCallbackProxy::ObjectRetrieveCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<ObjectRetrieveCallbackProxyBroker>(impl)
{
}

ObjectChangeCallbackProxy::ObjectChangeCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<ObjectChangeCallbackProxyBroker>(impl)
{
}

void ObjectSaveCallbackProxy::Completed(const std::map<std::string, int32_t> &results)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return;
    }
    if (!ITypesUtil::Marshal(data, results)) {
        ZLOGE("Marshalling failed");
        return;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int error = Remote()->SendRequest(COMPLETED, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest failed, error %d", error);
    }
}

void ObjectRevokeSaveCallbackProxy::Completed(int32_t status)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return;
    }
    if (!ITypesUtil::Marshal(data, status)) {
        ZLOGE("write descriptor failed");
        return;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int error = Remote()->SendRequest(COMPLETED, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest failed, error %d", error);
    }
}

void ObjectRetrieveCallbackProxy::Completed(const std::map<std::string, std::vector<uint8_t>> &results)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return;
    }
    if (!ITypesUtil::Marshal(data, results)) {
        ZLOGE("write descriptor failed");
        return;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int error = Remote()->SendRequest(COMPLETED, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest failed, error %d", error);
    }
}

void ObjectChangeCallbackProxy::Completed(const std::map<std::string, std::vector<uint8_t>> &results)
{
    MessageParcel data;
    MessageParcel reply;
    if (!data.WriteInterfaceToken(GetDescriptor())) {
        ZLOGE("write descriptor failed");
        return;
    }
    if (!ITypesUtil::Marshal(data, results)) {
        ZLOGE("write descriptor failed");
        return;
    }
    MessageOption mo { MessageOption::TF_SYNC };
    int error = Remote()->SendRequest(COMPLETED, data, reply, mo);
    if (error != 0) {
        ZLOGW("SendRequest failed, error %d", error);
    }
}
}  // namespace DistributedObject
}  // namespace OHOS