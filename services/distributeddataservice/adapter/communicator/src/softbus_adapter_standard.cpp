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

#include "communication_strategy.h"
#include "device_manager_adapter.h"
#include "dfx_types.h"
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
constexpr int32_t HEAD_SIZE = 3;
constexpr int32_t END_SIZE = 3;
constexpr int32_t MIN_SIZE = HEAD_SIZE + END_SIZE + 3;
constexpr const char *REPLACE_CHAIN = "***";
constexpr const char *DEFAULT_ANONYMOUS = "******";
constexpr int32_t SOFTBUS_OK = 0;
constexpr int32_t SOFTBUS_ERR = 1;
enum SoftBusAdapterErrorCode : int32_t {
    SESSION_ID_INVALID = 2,
    MY_SESSION_NAME_INVALID,
    PEER_SESSION_NAME_INVALID,
    PEER_DEVICE_ID_INVALID,
    SESSION_SIDE_INVALID,
};
constexpr int32_t SESSION_NAME_SIZE_MAX = 65;
constexpr int32_t DEVICE_ID_SIZE_MAX = 65;
constexpr uint32_t WAIT_MAX_TIME = 10;
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

    scheduler_.Every(std::chrono::seconds(1), CloseIdleConnect());
}

SoftBusAdapter::~SoftBusAdapter()
{
    ZLOGI("begin");
    if (onBroadcast_) {
        UnregNodeDeviceStateCb(&g_callback);
    }
    schedulerRunning_ = false;
    connects_.Clear();
}

std::string SoftBusAdapter::ToBeAnonymous(const std::string &name)
{
    if (name.length() <= HEAD_SIZE) {
        return DEFAULT_ANONYMOUS;
    }

    if (name.length() < MIN_SIZE) {
        return (name.substr(0, HEAD_SIZE) + REPLACE_CHAIN);
    }

    return (name.substr(0, HEAD_SIZE) + REPLACE_CHAIN + name.substr(name.length() - END_SIZE, END_SIZE));
}

std::shared_ptr<SoftBusAdapter> SoftBusAdapter::GetInstance()
{
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&] { instance_ = std::make_shared<SoftBusAdapter>(); });
    return instance_;
}

std::function<void()> SoftBusAdapter::CloseIdleConnect()
{
    return [this]() {
        if (!this->schedulerRunning_) {
            return;
        }

        this->connects_.EraseIf([this](auto &key, ConnectInfo &info) {
            if (info.idleCount >= CONNECT_IDLE_CLOSE_COUNT) {
                this->OnSessionClose(info.connId);
                CloseSession(info.connId);
                ZLOGE("[CloseIdleConnect] connId:%{public}d,", info.connId);
                return true;
            }
            info.idleCount++;
            return false;
        });
    };
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

Status SoftBusAdapter::OpenConnect(const PipeInfo &pipeInfo, const DeviceId &deviceId,
                                   const std::vector<LinkType> &linkTypes, int32_t &connId)
{
    SessionAttribute attr = { 0 };
    InitSessionAttribute(linkTypes, attr);
    int id = OpenSession(pipeInfo.pipeId.c_str(), pipeInfo.pipeId.c_str(),
                         DmAdapter::GetInstance().ToNetworkID(deviceId.deviceId).c_str(), "GROUP_ID", &attr);
    ZLOGI("[OpenConnect] to %{public}s,session:%{public}s,connId:%{public}d,linkNum:%{public}d,typeSize:%{public}zu",
          ToBeAnonymous(deviceId.deviceId).c_str(), pipeInfo.pipeId.c_str(), id, attr.linkTypeNum, linkTypes.size());
    if (id < 0) {
        ZLOGW("OpenConnect %{public}s, type:%{public}d failed, connId:%{public}d",
              pipeInfo.pipeId.c_str(), attr.dataType, id);
        return Status::NETWORK_ERROR;
    }
    int state = GetSessionStatus(id);
    ZLOGI("waited for notification, state:%{public}d connId:%{public}d", state, id);
    if (state != SOFTBUS_OK) {
        ZLOGE("OpenSession callback result error");
        return Status::NETWORK_ERROR;
    }

    connId = id;
    return Status::SUCCESS;
}

void SoftBusAdapter::InitSessionAttribute(const std::vector<LinkType> &linkTypes, SessionAttribute &attr)
{
    attr.dataType = TYPE_BYTES;
    int index = 0;
    for (auto const &element : linkTypes) {
        attr.linkType[index++] = element;
        if (index >= LINK_TYPE_MAX) {
            break;
        }
    }
    attr.linkTypeNum = index;
}

Status SoftBusAdapter::GetConnect(const PipeInfo &pipeInfo, const DeviceId &deviceId, int32_t dataSize, int32_t &connId)
{
    std::vector<LinkType> linkTypes;
    bool isReconnect = CommunicationStrategy::GetInstance().GetStrategy(deviceId.deviceId, dataSize, linkTypes);
    Status status = Status::ERROR;
    auto result = connects_.ComputeIfPresent(pipeInfo.pipeId + deviceId.deviceId,
        [&isReconnect, &status, &connId](const std::string& key, ConnectInfo & value) {
            connId = value.connId;
            value.idleCount = 0;
            if (value.hasReconnect || !isReconnect) {
                status = Status::SUCCESS;
            }
            return true;
    });

    if (status == Status::SUCCESS) {
        return Status::SUCCESS;
    }

    if (result) {
        OnSessionClose(connId);
        CloseSession(connId);
        connId = INVALID_CONNECT_ID;
    }

    status = OpenConnect(pipeInfo, deviceId, linkTypes, connId);
    if (status == Status::SUCCESS) {
        ConnectInfo conn = {connId, 0, isReconnect, deviceId.deviceId, GetConnectMtuSize(connId)};
        connects_.InsertOrAssign(pipeInfo.pipeId + deviceId.deviceId, conn);
        return Status::SUCCESS;
    }

    connId = INVALID_CONNECT_ID;
    return status;
}

std::shared_ptr<std::recursive_mutex> SoftBusAdapter::GetMutex(const PipeInfo &pipeInfo, const DeviceId &deviceId)
{
    lock_guard<mutex> lock(mutex_);
    std::string key = pipeInfo.pipeId + deviceId.deviceId;
    if (mutexes_.find(key) == mutexes_.end()) {
        mutexes_.emplace(key, std::make_shared<std::recursive_mutex>());
    }

    return mutexes_[key];
}

Status SoftBusAdapter::SendData(const PipeInfo &pipeInfo, const DeviceId &deviceId, const uint8_t *data, int size,
    const MessageInfo &info)
{
    std::shared_ptr<std::recursive_mutex> connMutex = GetMutex(pipeInfo, deviceId);
    lock_guard<std::recursive_mutex> lock(*connMutex);
    int connId = INVALID_CONNECT_ID;
    auto state = GetConnect(pipeInfo, deviceId, size, connId);
    if (state != Status::SUCCESS) {
        ZLOGW("get connect %{public}s, type:%{public}d failed, status:%{public}d", pipeInfo.pipeId.c_str(),
            info.msgType, state);
        return state;
    }

    ZLOGD("[SendData] to %{public}s, session:%{public}s, send len:%{public}d, connId:%{public}d",
        ToBeAnonymous(deviceId.deviceId).c_str(), pipeInfo.pipeId.c_str(), size, connId);
    int32_t ret = SendBytes(connId, data, size);
    if (ret != SOFTBUS_OK) {
        ZLOGE("[SendBytes] to %{public}d failed, ret:%{public}d.", connId, ret);
        return Status::ERROR;
    }
    return Status::SUCCESS;
}

int32_t SoftBusAdapter::GetSessionStatus(int32_t connId)
{
    auto semaphore = GetSemaphore(connId);
    return semaphore->GetValue();
}

void SoftBusAdapter::OnSessionOpen(int32_t connId, int32_t status)
{
    auto semaphore = GetSemaphore(connId);
    semaphore->SetValue(status);
}

void SoftBusAdapter::OnSessionClose(int32_t connId)
{
    lock_guard<mutex> lock(statusMutex_);
    auto it = sessionsStatus_.find(connId);
    if (it != sessionsStatus_.end()) {
        it->second->Clear(SOFTBUS_ERR);
        sessionsStatus_.erase(it);
    }
    ZLOGD("[OnSessionClose] connId:%{public}d, size:%{public}zu", connId, sessionsStatus_.size());
}

std::shared_ptr<BlockData<int32_t>> SoftBusAdapter::GetSemaphore(int32_t connId)
{
    lock_guard<mutex> lock(statusMutex_);
    if (sessionsStatus_.find(connId) == sessionsStatus_.end()) {
        sessionsStatus_.emplace(connId, std::make_shared<BlockData<int32_t>>(WAIT_MAX_TIME, SOFTBUS_ERR));
    }
    return sessionsStatus_[connId];
}

uint32_t SoftBusAdapter::GetMtuSize(const DeviceId &deviceId)
{
    uint32_t mtu = DEFAULT_MTU_SIZE;
    connects_.ForEach([&deviceId, &mtu](const std::string &key, ConnectInfo &value) {
        if (value.deviceId == deviceId.deviceId) {
            mtu = value.mtu;
            return true;
        }
        return false;
    });
    return mtu;
}

uint32_t SoftBusAdapter::GetConnectMtuSize(int32_t connId)
{
    uint32_t mtu = 0;
    auto result = GetSessionOption(connId, SESSION_OPTION_MAX_SENDBYTES_SIZE, &mtu, sizeof(mtu));
    if (result != SOFTBUS_OK) {
        return DEFAULT_MTU_SIZE;
    }
    return mtu;
}
bool SoftBusAdapter::IsSameStartedOnPeer(const struct PipeInfo &pipeInfo,
    __attribute__((unused)) const struct DeviceId &peer)
{
    ZLOGI("pipeInfo:%{public}s deviceId:%{public}s", pipeInfo.pipeId.c_str(), ToBeAnonymous(peer.deviceId).c_str());

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

std::string SoftBusAdapter::DeleteSession(int32_t connId)
{
    std::string name = "";
    connects_.EraseIf([&connId, &name](auto &key, ConnectInfo &info) {
        if (connId == info.connId) {
            name = key;
            return true;
        }
        return false;
    });

    return name;
}

void SoftBusAdapter::NotifyDataListeners(const uint8_t *data, int size, const std::string &deviceId,
    const PipeInfo &pipeInfo)
{
    ZLOGD("begin");
    auto ret = dataChangeListeners_.ComputeIfPresent(pipeInfo.pipeId,
        [&data, &size, &deviceId, &pipeInfo](const auto &key, const AppDataChangeListener *&value) {
            ZLOGD("ready to notify, pipeName:%{public}s, deviceId:%{public}s.", pipeInfo.pipeId.c_str(),
                ToBeAnonymous(deviceId).c_str());
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
    ZLOGI("device:%{public}s mask:0x%{public}x", ToBeAnonymous(device.deviceId).c_str(), mask);
    if (!onBroadcast_) {
        ZLOGW("no listener device:%{public}s mask:0x%{public}x", ToBeAnonymous(device.deviceId).c_str(), mask);
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
        softBusAdapter_->OnSessionClose(connId);
        return result;
    }

    ConnDetailsInfo connInfo;
    int ret = GetConnDetailsInfo(connId, connInfo);
    if (ret != SOFTBUS_OK) {
        ZLOGE("[SessionOpened] session id:%{public}d get info fail error: %{public}d", connId, ret);
        return ret;
    }

    ZLOGD("[OnConnectOpened] conn id:%{public}d, my name:%{public}s, peer name:%{public}s, "
          "peer devId:%{public}s, side:%{public}d",
          connId, connInfo.myName, connInfo.peerName, SoftBusAdapter::ToBeAnonymous(connInfo.peerDevUuid).c_str(),
          connInfo.side);
    return 0;
}

void AppDataListenerWrap::OnConnectClosed(int connId)
{
    // when the local close the session, this callback function will not be triggered;
    // when the current function is called, soft bus has released the session resource, only connId is valid;
    softBusAdapter_->OnSessionClose(connId);
    std::string name = softBusAdapter_->DeleteSession(connId);
    ZLOGI("[SessionClosed] connId:%{public}d, name:%{public}s", connId, SoftBusAdapter::ToBeAnonymous(name).c_str());
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
          SoftBusAdapter::ToBeAnonymous(connInfo.peerDevUuid).c_str(), connInfo.side, dataLen);
    NotifyDataListeners(reinterpret_cast<const uint8_t *>(data), dataLen, connInfo.peerDevUuid,
        {std::string(connInfo.peerName), ""});
}

void AppDataListenerWrap::NotifyDataListeners(const uint8_t *data, const int size, const std::string &deviceId,
    const PipeInfo &pipeInfo)
{
    softBusAdapter_->NotifyDataListeners(data, size, deviceId, pipeInfo);
}
} // namespace AppDistributedKv
} // namespace OHOS
