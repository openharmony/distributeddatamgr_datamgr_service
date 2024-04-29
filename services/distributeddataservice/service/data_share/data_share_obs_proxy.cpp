/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#define LOG_TAG "ObserverProxy"
#include "data_share_obs_proxy.h"

#include "itypes_util.h"
#include "log_print.h"

namespace OHOS {
namespace DataShare {
static constexpr int REQUEST_CODE = 0;
void RdbObserverProxy::OnChangeFromRdb(RdbChangeNode &changeNode)
{
    MessageParcel parcel;
    if (!parcel.WriteInterfaceToken(RdbObserverProxy::GetDescriptor())) {
        return;
    }

    if (!ITypesUtil::Marshal(parcel, changeNode)) {
        ZLOGE("failed to WriteParcelable changeNode ");
        return;
    }

    MessageParcel reply;
    MessageOption option;
    int32_t result = Remote()->SendRequest(REQUEST_CODE, parcel, reply, option);
    if (result != ERR_NONE) {
        ZLOGE("SendRequest error, result=%{public}d", result);
        return;
    }
    ZLOGI("SendRequest ok, retval is %{public}d", reply.ReadInt32());
}

void PublishedDataObserverProxy::OnChangeFromPublishedData(PublishedDataChangeNode &changeNode)
{
    MessageParcel parcel;
    if (!parcel.WriteInterfaceToken(PublishedDataObserverProxy::GetDescriptor())) {
        return;
    }

    if (!ITypesUtil::Marshal(parcel, changeNode)) {
        ZLOGE("failed to WriteParcelable changeNode ");
        return;
    }

    MessageParcel reply;
    MessageOption option;
    int32_t result = Remote()->SendRequest(REQUEST_CODE, parcel, reply, option);
    if (result != ERR_NONE) {
        ZLOGE("SendRequest error, result=%{public}d", result);
        return;
    }
    ZLOGI("SendRequest ok, retval is %{public}d", reply.ReadInt32());
}
} // namespace DataShare
} // namespace OHOS