/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include <mutex>
#include <thread>

#include "device_manager_adapter.h"
#include "dfx_types.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "reporter.h"
#include "securec.h"
#include "session.h"
#include "softbus_adapter.h"
#include "softbus_bus_center.h"
#include "softbus_error_code.h"
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "SoftBusAdapter"

namespace OHOS {
namespace AppDistributedKv {
enum SoftBusAdapterErrorCode : int32_t {
    SESSION_ID_INVALID = 2,
    MY_SESSION_NAME_INVALID,
    PEER_SESSION_NAME_INVALID,
    PEER_DEVICE_ID_INVALID,
    SESSION_SIDE_INVALID,
};
constexpr int32_t SESSION_NAME_SIZE_MAX = 65;
constexpr int32_t DEVICE_ID_SIZE_MAX = 65;
static constexpr int32_t DEFAULT_MTU_SIZE = 4096;
using namespace std;
using namespace OHOS::DistributedDataDfx;
using namespace OHOS::DistributedKv;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
struct ConnDetailsInfo {
    char myName[SESSION_NAME_SIZE_MAX] = "";
    char peerName[SESSION_NAME_SIZE_MAX] = "";
    std::string peerDevUuid;
    int32_t side = -1;
};
class AppDataListenerWrap {
public:
    static void SetDataHandler(SoftBusAdapter *handler);
    static int OnConnectOpened(int connId, int result);
    static void OnConnectClosed(int connId);
    static void OnBytesReceived(int connId, const void *data, unsigned int dataLen);

public:
    // notify all listeners when received message
    static void NotifyDataListeners(const uint8_t *data, const int size, const std::string &deviceId,
        const PipeInfo &pipeInfo);

private:
    static int GetConnDetailsInfo(int connId, ConnDetailsInfo &connInfo);
    static SoftBusAdapter *softBusAdapter_;
};
SoftBusAdapter *AppDataListenerWrap::softBusAdapter_;
std::shared_ptr<SoftBusAdapter> SoftBusAdapter::instance_;

namespace {
void NotCareEvent(NodeBasicInfo *info)
{
    return;
}

void NotCareEvent(NodeBasicInfoType type, NodeBasicInfo *info)
{
    return;
}

void OnCareEvent(NodeStatusType type, NodeStatus *status)
{
    if (type != TYPE_DATABASE_STATUS || status == nullptr) {
        return;
    }
    auto uuid = DmAdapter::GetInstance().GetUuidByNetworkId(status->basicInfo.networkId);
    SoftBusAdapter::GetInstance()->OnBroadcast({ uuid }, status->dataBaseStatus);
}

INodeStateCb g_callback = {
    .events = EVENT_NODE_STATUS_CHANGED,
    .onNodeOnline = NotCareEvent,
    .onNodeOffline = NotCareEvent,
    .onNodeBasicInfoChanged = NotCareEvent,
    .onNodeStatusChanged = OnCareEvent,
};
} // namespace

SoftBusAdapter::SoftBusAdapter()
{
    ZLOGI("begin");
    AppDataListenerWrap::SetDataHandler(this);

    sessionListener_.OnSessionOpened = AppDataListenerWrap::OnConnectOpened;
    sessionListener_.OnSessionClosed = AppDataListenerWrap::OnConnectClosed;
    sessionListener_.OnBytesReceived = AppDataListenerWrap::OnBytesReceived;
    sessionListener_.OnMessageReceived = AppDataListenerWrap::OnBytesReceived;
}

SoftBusAdapter::~SoftBusAdapter()
{
    ZLOGI("begin");
    if (onBroadcast_) {
        UnregNodeDeviceStateCb(&g_callback);
    }
    connects_.clear();
}

std::shared_ptr<SoftBusAdapter> SoftBusAdapter::GetInstance()
{
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&] { instance_ = std::make_shared<SoftBusAdapter>(); });
    return instance_;
}

Status SoftBusAdapter::StartWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo)
{
    ZLOGD("begin");
    if (observer == nullptr) {
        return Status::INVALID_ARGUMENT;
    }

    auto ret = dataChangeListeners_.Insert(pipeInfo.pipeId, observer);
    if (!ret) {
        ZLOGW("Add listener error or repeated adding.");
        return Status::ERROR;
    }

    return Status::SUCCESS;
}

Status SoftBusAdapter::StopWatchDataChange(__attribute__((unused)) const AppDataChangeListener *observer,
    const PipeInfo &pipeInfo)
{
    ZLOGD("begin");
    if (dataChangeListeners_.Erase(pipeInfo.pipeId) != 0) {
        return Status::SUCCESS;
    }
    ZLOGW("stop data observer error, pipeInfo:%{public}s", pipeInfo.pipeId.c_str());
    return Status::ERROR;
}

Status SoftBusAdapter::SendData(const PipeInfo &pipeInfo, const DeviceId &deviceId, const uint8_t *data, int size,
    const MessageInfo &info)
{
    std::shared_ptr<SoftBusClient> conn;
    {
        lock_guard<mutex> lock(connMutex_);
        std::string key = pipeInfo.pipeId + deviceId.deviceId;
        if (connects_.find(key) == connects_.end()) {
            connects_.emplace(key, std::make_shared<SoftBusClient>(pipeInfo, deviceId));
        }
        conn = connects_[key];
    }

    if (conn) {
        return conn->Send(data, size);
    }

    return Status::ERROR;
}

uint32_t SoftBusAdapter::GetMtuSize(const DeviceId &deviceId)
{
    lock_guard<mutex> lock(connMutex_);
    for (const auto& conn : connects_) {
        if (*conn.second == deviceId) {
            return conn.second->GetMtuSize();
        }
    }

    return DEFAULT_MTU_SIZE;
}

void SoftBusAdapter::OnSessionOpen(int32_t connId, int32_t status)
{
    lock_guard<mutex> lock(connMutex_);
    for (const auto& conn : connects_) {
        if (*conn.second == connId) {
            conn.second->OnConnected(status);
            break;
        }
    }
}

std::string SoftBusAdapter::OnSessionClose(int32_t connId)
{
    lock_guard<mutex> lock(connMutex_);
    std::string name = "";
    for (const auto& conn : connects_) {
        if (*conn.second == connId) {
            name = conn.first;
            connects_.erase(conn.first);
            break;
        }
    }
    return name;
}

bool SoftBusAdapter::IsSameStartedOnPeer(const struct PipeInfo &pipeInfo,
    __attribute__((unused)) const struct DeviceId &peer)
{
    ZLOGI("pipeInfo:%{public}s deviceId:%{public}s", pipeInfo.pipeId.c_str(),
          KvStoreUtils::ToBeAnonymous(peer.deviceId).c_str());
    return true;
}

void SoftBusAdapter::SetMessageTransFlag(const PipeInfo &pipeInfo, bool flag)
{
    ZLOGI("pipeInfo: %s flag: %d", pipeInfo.pipeId.c_str(), flag);
    flag_ = flag;
}

int SoftBusAdapter::CreateSessionServerAdapter(const std::string &sessionName)
{
    ZLOGD("begin");
    return CreateSessionServer("ohos.distributeddata", sessionName.c_str(), &sessionListener_);
}

int SoftBusAdapter::RemoveSessionServerAdapter(const std::string &sessionName) const
{
    ZLOGD("begin");
    return RemoveSessionServer("ohos.distributeddata", sessionName.c_str());
}

void SoftBusAdapter::NotifyDataListeners(const uint8_t *data, int size, const std::string &deviceId,
    const PipeInfo &pipeInfo)
{
    ZLOGD("begin");
    auto ret = dataChangeListeners_.ComputeIfPresent(pipeInfo.pipeId,
        [&data, &size, &deviceId, &pipeInfo](const auto &key, const AppDataChangeListener *&value) {
            ZLOGD("ready to notify, pipeName:%{public}s, deviceId:%{public}s.", pipeInfo.pipeId.c_str(),
                  KvStoreUtils::ToBeAnonymous(deviceId).c_str());
            DeviceInfo deviceInfo = DmAdapter::GetInstance().GetDeviceInfo(deviceId);
            value->OnMessage(deviceInfo, data, size, pipeInfo);
            TrafficStat ts{ pipeInfo.pipeId, deviceId, 0, size };
            Reporter::GetInstance()->TrafficStatistic()->Report(ts);
            return true;
        });
    if (!ret) {
        ZLOGW("no listener %{public}s.", pipeInfo.pipeId.c_str());
    }
}

int32_t SoftBusAdapter::Broadcast(const PipeInfo &pipeInfo, uint16_t mask)
{
    return SetNodeDataChangeFlag(pipeInfo.pipeId.c_str(), DmAdapter::GetInstance().GetLocalDevice().networkId.c_str(),
        mask);
}

void SoftBusAdapter::OnBroadcast(const DeviceId &device, uint16_t mask)
{
    ZLOGI("device:%{public}s mask:0x%{public}x", KvStoreUtils::ToBeAnonymous(device.deviceId).c_str(), mask);
    if (!onBroadcast_) {
        ZLOGW("no listener device:%{public}s mask:0x%{public}x",
              KvStoreUtils::ToBeAnonymous(device.deviceId).c_str(), mask);
        return;
    }
    onBroadcast_(device.deviceId, mask);
}

int32_t SoftBusAdapter::ListenBroadcastMsg(const PipeInfo &pipeInfo,
    std::function<void(const std::string &, uint16_t)> listener)
{
    if (onBroadcast_) {
        return SOFTBUS_ALREADY_EXISTED;
    }
    onBroadcast_ = std::move(listener);
    return RegNodeDeviceStateCb(pipeInfo.pipeId.c_str(), &g_callback);
}

void AppDataListenerWrap::SetDataHandler(SoftBusAdapter *handler)
{
    ZLOGI("begin");
    softBusAdapter_ = handler;
}

int AppDataListenerWrap::GetConnDetailsInfo(int connId, ConnDetailsInfo &connInfo)
{
    if (connId < 0) {
        return SESSION_ID_INVALID;
    }

    int ret = GetMySessionName(connId, connInfo.myName, sizeof(connInfo.myName));
    if (ret != SOFTBUS_OK) {
        return MY_SESSION_NAME_INVALID;
    }

    ret = GetPeerSessionName(connId, connInfo.peerName, sizeof(connInfo.peerName));
    if (ret != SOFTBUS_OK) {
        return PEER_SESSION_NAME_INVALID;
    }

    char peerDevId[DEVICE_ID_SIZE_MAX] = "";
    ret = GetPeerDeviceId(connId, peerDevId, sizeof(peerDevId));
    if (ret != SOFTBUS_OK) {
        return PEER_DEVICE_ID_INVALID;
    }
    connInfo.peerDevUuid = DmAdapter::GetInstance().GetUuidByNetworkId(std::string(peerDevId));

    connInfo.side = GetSessionSide(connId);
    if (connInfo.side < 0) {
        return SESSION_SIDE_INVALID;
    }

    return SOFTBUS_OK;
}

int AppDataListenerWrap::OnConnectOpened(int connId, int result)
{
    ZLOGI("[SessionOpen] connId:%{public}d, result:%{public}d", connId, result);
    softBusAdapter_->OnSessionOpen(connId, result);
    if (result != SOFTBUS_OK) {
        ZLOGW("session %{public}d open failed, result:%{public}d.", connId, result);
        return result;
    }

    ConnDetailsInfo connInfo;
    int ret = GetConnDetailsInfo(connId, connInfo);
    if (ret != SOFTBUS_OK) {
        ZLOGE("[SessionOpened] session id:%{public}d get info fail error: %{public}d", connId, ret);
        return ret;
    }

    ZLOGD("[OnConnectOpened] conn id:%{public}d, my name:%{public}s, peer name:%{public}s, "
          "peer devId:%{public}s, side:%{public}d", connId, connInfo.myName, connInfo.peerName,
          KvStoreUtils::ToBeAnonymous(connInfo.peerDevUuid).c_str(), connInfo.side);
    return 0;
}

void AppDataListenerWrap::OnConnectClosed(int connId)
{
    // when the local close the session, this callback function will not be triggered;
    // when the current function is called, soft bus has released the session resource, only connId is valid;
    std::string name = softBusAdapter_->OnSessionClose(connId);
    ZLOGI("[SessionClosed] connId:%{public}d, name:%{public}s", connId, KvStoreUtils::ToBeAnonymous(name).c_str());
}

void AppDataListenerWrap::OnBytesReceived(int connId, const void *data, unsigned int dataLen)
{
    ConnDetailsInfo connInfo;
    int ret = GetConnDetailsInfo(connId, connInfo);
    if (ret != SOFTBUS_OK) {
        ZLOGE("[OnBytesReceived] session id:%{public}d get info fail error: %{public}d", connId, ret);
        return;
    }

    ZLOGD("[OnBytesReceived] conn id:%{public}d, peer name:%{public}s, "
          "peer devId:%{public}s, side:%{public}d, data len:%{public}u", connId, connInfo.peerName,
          KvStoreUtils::ToBeAnonymous(connInfo.peerDevUuid).c_str(), connInfo.side, dataLen);

    NotifyDataListeners(reinterpret_cast<const uint8_t *>(data), dataLen, connInfo.peerDevUuid,
        { std::string(connInfo.peerName), "" });
}

void AppDataListenerWrap::NotifyDataListeners(const uint8_t *data, const int size, const std::string &deviceId,
    const PipeInfo &pipeInfo)
{
    softBusAdapter_->NotifyDataListeners(data, size, deviceId, pipeInfo);
}
} // namespace AppDistributedKv
} // namespace OHOS
