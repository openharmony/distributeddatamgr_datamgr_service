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
#define LOG_TAG "NetworkAdapter"
#include "network_adapter.h"

#include "device_manager_adapter.h"
#include "log_print.h"
#include "net_conn_callback_stub.h"
#include "net_conn_client.h"
#include "net_handle.h"
namespace OHOS::DistributedData {
using namespace OHOS::NetManagerStandard;
static NetworkAdapter::NetworkType Convert(NetManagerStandard::NetBearType bearType)
{
    switch (bearType) {
        case NetManagerStandard::BEARER_WIFI:
            return NetworkAdapter::WIFI;
        case NetManagerStandard::BEARER_CELLULAR:
            return NetworkAdapter::CELLULAR;
        case NetManagerStandard::BEARER_ETHERNET:
            return NetworkAdapter::ETHERNET;
        default:
            return NetworkAdapter::OTHER;
    }
}

class NetConnCallbackObserver : public NetConnCallbackStub {
public:
    explicit NetConnCallbackObserver(NetworkAdapter &netAdapter) : netAdapter_(netAdapter) {}
    ~NetConnCallbackObserver() override = default;
    int32_t NetAvailable(sptr<NetHandle> &netHandle) override;
    int32_t NetCapabilitiesChange(sptr<NetHandle> &netHandle, const sptr<NetAllCapabilities> &netAllCap) override;
    int32_t NetConnectionPropertiesChange(sptr<NetHandle> &netHandle, const sptr<NetLinkInfo> &info) override;
    int32_t NetLost(sptr<NetHandle> &netHandle) override;
    int32_t NetUnavailable() override;
    int32_t NetBlockStatusChange(sptr<NetHandle> &netHandle, bool blocked) override;

private:
    NetworkAdapter &netAdapter_;
};

int32_t NetConnCallbackObserver::NetAvailable(sptr<NetManagerStandard::NetHandle> &netHandle)
{
    ZLOGI("OnNetworkAvailable");
    return 0;
}

int32_t NetConnCallbackObserver::NetUnavailable()
{
    ZLOGI("OnNetworkUnavailable");
    netAdapter_.SetNet(NetworkAdapter::NONE);
    return 0;
}

int32_t NetConnCallbackObserver::NetCapabilitiesChange(sptr<NetHandle> &netHandle,
    const sptr<NetAllCapabilities> &netAllCap)
{
    ZLOGI("OnNetCapabilitiesChange");
    if (netHandle == nullptr || netAllCap == nullptr) {
        return 0;
    }
    if (netAllCap->netCaps_.count(NetManagerStandard::NET_CAPABILITY_VALIDATED) && !netAllCap->bearerTypes_.empty()) {
        netAdapter_.SetNet(Convert(*netAllCap->bearerTypes_.begin()));
    } else {
        netAdapter_.SetNet(NetworkAdapter::NONE);
    }
    return 0;
}

int32_t NetConnCallbackObserver::NetConnectionPropertiesChange(sptr<NetHandle> &netHandle,
    const sptr<NetLinkInfo> &info)
{
    ZLOGI("OnNetConnectionPropertiesChange");
    return 0;
}

int32_t NetConnCallbackObserver::NetLost(sptr<NetHandle> &netHandle)
{
    ZLOGI("OnNetLost");
    netAdapter_.SetNet(NetworkAdapter::NONE);
    return 0;
}

int32_t NetConnCallbackObserver::NetBlockStatusChange(sptr<NetHandle> &netHandle, bool blocked)
{
    ZLOGI("OnNetBlockStatusChange");
    return 0;
}

NetworkAdapter::NetworkAdapter()
    : cloudDmInfo({ "cloudDeviceId", "cloudDeviceName", 0, "cloudNetworkId", 0 })
{
}

NetworkAdapter::~NetworkAdapter()
{
}

 NetworkAdapter &NetworkAdapter::GetInstance()
{
    static NetworkAdapter adapter;
    return adapter;
}

void NetworkAdapter::RegOnNetworkChange()
{
    static std::atomic_bool flag = false;
    if (flag.exchange(true)) {
        ZLOGW("run only one");
        return;
    }
    sptr<NetConnCallbackObserver> observer = new (std::nothrow) NetConnCallbackObserver(*this);
    if (observer == nullptr) {
        ZLOGE("new operator error.observer is nullptr");
        flag.store(false);
        return;
    }
    auto nRet = NetConnClient::GetInstance().RegisterNetConnCallback(observer);
    if (nRet != NETMANAGER_SUCCESS) {
        ZLOGE("RegisterNetConnCallback failed, ret = %{public}d", nRet);
        flag.store(false);
    }
}

bool NetworkAdapter::IsNetworkAvailable()
{
    if (defaultNetwork_ != NONE || expireTime_ > GetTimeStamp()) {
        return defaultNetwork_ != NONE;
    }
    return RefreshNet() != NONE;
}

NetworkAdapter::NetworkType NetworkAdapter::SetNet(NetworkType netWorkType)
{
    auto oldNet = defaultNetwork_;
    bool ready = oldNet == NONE && netWorkType != NONE && (GetTimeStamp() - netLostTime_) > NET_LOST_DURATION;
    bool offline = oldNet != NONE && netWorkType == NONE;
    if (offline) {
        netLostTime_ = GetTimeStamp();
    }
    defaultNetwork_ = netWorkType;
    expireTime_ = GetTimeStamp() + EFFECTIVE_DURATION;
    if (ready) {
        DeviceManagerAdapter::GetInstance().OnReady(cloudDmInfo);
    }
    if (offline) {
        DeviceManagerAdapter::GetInstance().Offline(cloudDmInfo);
    }
    return netWorkType;
}

NetworkAdapter::NetworkType NetworkAdapter::GetNetworkType(bool retrieve)
{
    if (!retrieve) {
        return defaultNetwork_;
    }
    return RefreshNet();
}

NetworkAdapter::NetworkType NetworkAdapter::RefreshNet()
{
    NetHandle handle;
    auto status = NetConnClient::GetInstance().GetDefaultNet(handle);
    if (status != 0 || handle.GetNetId() == 0) {
        return SetNet(NONE);
    }
    NetAllCapabilities capabilities;
    status = NetConnClient::GetInstance().GetNetCapabilities(handle, capabilities);
    if (status != 0 || !capabilities.netCaps_.count(NetManagerStandard::NET_CAPABILITY_VALIDATED) ||
        capabilities.bearerTypes_.empty()) {
        return SetNet(NONE);
    }
    return SetNet(Convert(*capabilities.bearerTypes_.begin()));
}
}
