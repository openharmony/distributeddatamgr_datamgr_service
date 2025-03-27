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
#define LOG_TAG "NetworkDelegateNormalImpl"

#include "network_delegate_normal_impl.h"

#include "device_manager_adapter.h"
#include "log_print.h"
#include "net_conn_callback_stub.h"
#include "net_conn_client.h"
#include "net_handle.h"
namespace OHOS::DistributedData {
using namespace OHOS::NetManagerStandard;
__attribute__((used)) static bool g_isInit = NetworkDelegateNormalImpl::Init();
static NetworkDelegateNormalImpl::NetworkType Convert(NetManagerStandard::NetBearType bearType)
{
    switch (bearType) {
        case NetManagerStandard::BEARER_WIFI:
            return NetworkDelegate::WIFI;
        case NetManagerStandard::BEARER_CELLULAR:
            return NetworkDelegate::CELLULAR;
        case NetManagerStandard::BEARER_ETHERNET:
            return NetworkDelegate::ETHERNET;
        default:
            return NetworkDelegate::OTHER;
    }
}

class NetConnCallbackObserver : public NetConnCallbackStub {
public:
    explicit NetConnCallbackObserver(NetworkDelegateNormalImpl &delegate) : delegate_(delegate)
    {
    }
    ~NetConnCallbackObserver() override = default;
    int32_t NetAvailable(sptr<NetHandle> &netHandle) override;
    int32_t NetCapabilitiesChange(sptr<NetHandle> &netHandle, const sptr<NetAllCapabilities> &netAllCap) override;
    int32_t NetConnectionPropertiesChange(sptr<NetHandle> &netHandle, const sptr<NetLinkInfo> &info) override;
    int32_t NetLost(sptr<NetHandle> &netHandle) override;
    int32_t NetUnavailable() override;
    int32_t NetBlockStatusChange(sptr<NetHandle> &netHandle, bool blocked) override;

private:
    NetworkDelegateNormalImpl &delegate_;
};

int32_t NetConnCallbackObserver::NetAvailable(sptr<NetManagerStandard::NetHandle> &netHandle)
{
    ZLOGI("OnNetworkAvailable");
    return 0;
}

int32_t NetConnCallbackObserver::NetUnavailable()
{
    ZLOGI("OnNetworkUnavailable");
    delegate_.SetNet(NetworkDelegateNormalImpl::NONE);
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
        delegate_.SetNet(Convert(*netAllCap->bearerTypes_.begin()));
    } else {
        delegate_.SetNet(NetworkDelegateNormalImpl::NONE);
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
    delegate_.SetNet(NetworkDelegateNormalImpl::NONE);
    return 0;
}

int32_t NetConnCallbackObserver::NetBlockStatusChange(sptr<NetHandle> &netHandle, bool blocked)
{
    ZLOGI("OnNetBlockStatusChange");
    return 0;
}

NetworkDelegateNormalImpl::NetworkDelegateNormalImpl()
    : cloudDmInfo_({ "cloudDeviceId", "cloudDeviceName", 0, "cloudNetworkId", 0 })
{
}

NetworkDelegateNormalImpl::~NetworkDelegateNormalImpl()
{
}

void NetworkDelegateNormalImpl::RegOnNetworkChange()
{
    if (executors_ != nullptr) {
        executors_->Execute(GetTask(0));
    }
}

void NetworkDelegateNormalImpl::BindExecutor(std::shared_ptr<ExecutorPool> executors)
{
    executors_ = executors;
}

ExecutorPool::Task NetworkDelegateNormalImpl::GetTask(uint32_t retry)
{
    return [this, retry] {
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
        if (nRet == NETMANAGER_SUCCESS) {
            return;
        }
        ZLOGE("RegisterNetConnCallback failed, ret = %{public}d", nRet);
        flag.store(false);
        if (retry + 1 > MAX_RETRY_TIME) {
            ZLOGE("fail to register subscriber!");
            return;
        }
        executors_->Schedule(std::chrono::seconds(RETRY_WAIT_TIME_S), GetTask(retry + 1));
    };
}

bool NetworkDelegateNormalImpl::IsNetworkAvailable()
{
    if (defaultNetwork_ != NONE || expireTime_ > GetTimeStamp()) {
        return defaultNetwork_ != NONE;
    }
    return RefreshNet() != NONE;
}

NetworkDelegateNormalImpl::NetworkType NetworkDelegateNormalImpl::SetNet(NetworkType netWorkType)
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
        DeviceManagerAdapter::GetInstance().OnReady(cloudDmInfo_);
    }
    if (offline) {
        DeviceManagerAdapter::GetInstance().Offline(cloudDmInfo_);
    }
    return netWorkType;
}

NetworkDelegate::NetworkType NetworkDelegateNormalImpl::GetNetworkType(bool retrieve)
{
    if (!retrieve) {
        return defaultNetwork_;
    }
    return RefreshNet();
}

NetworkDelegateNormalImpl::NetworkType NetworkDelegateNormalImpl::RefreshNet()
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

bool NetworkDelegateNormalImpl::Init()
{
    static NetworkDelegateNormalImpl delegate;
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&]() { NetworkDelegate::RegisterNetworkInstance(&delegate); });
    return true;
}
} // namespace OHOS::DistributedData