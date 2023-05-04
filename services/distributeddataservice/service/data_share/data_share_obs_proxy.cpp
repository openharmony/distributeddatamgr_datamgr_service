//
// Created by niudongyao on 2023/3/17.
//
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
    if (result == ERR_NONE) {
        ZLOGI("SendRequest ok, retval is %{public}d", reply.ReadInt32());
    } else {
        ZLOGE("SendRequest error, result=%{public}d", result);
        return;
    }
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
    if (result == ERR_NONE) {
        ZLOGI("SendRequest ok, retval is %{public}d", reply.ReadInt32());
    } else {
        ZLOGE("SendRequest error, result=%{public}d", result);
        return;
    }
}
} // namespace DataShare
} // namespace OHOS