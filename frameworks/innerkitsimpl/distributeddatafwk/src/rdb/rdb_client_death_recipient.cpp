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

#define LOG_TAG "RdbClientDeathRecipientProxy"

#include "rdb_client_death_recipient.h"
#include "log_print.h"

namespace OHOS::DistributedKv {
RdbClientDeathRecipientProxy::RdbClientDeathRecipientProxy(const sptr<IRemoteObject> &object)
    : IRemoteProxy<IRdbClientDeathRecipient>(object)
{
    ZLOGI("construct");
}

RdbClientDeathRecipientProxy::~RdbClientDeathRecipientProxy()
{
    ZLOGI("deconstruct");
}

RdbClientDeathRecipientStub::RdbClientDeathRecipientStub()
{
    ZLOGI("construct");
}

RdbClientDeathRecipientStub::~RdbClientDeathRecipientStub()
{
    ZLOGI("deconstruct");
}

int RdbClientDeathRecipientStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                                                 MessageOption &option)
{
    return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
}
}
