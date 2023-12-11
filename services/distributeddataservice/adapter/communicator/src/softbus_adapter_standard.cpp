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

#include "communicator_context.h"
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
using Context = DistributedData::CommunicatorContext;
enum SoftBusAdapterErrorCode : int32_t {
    SESSION_ID_INVALID = 2,
    MY_SESSION_NAME_INVALID,
    PEER_SESSION_NAME_INVALID,
    PEER_DEVICE_ID_INVALID,
    SESSION_SIDE_INVALID,
    ROUTE_TYPE_INVALID,
};
constexpr int32_t SESSION_NAME_SIZE_MAX = 65;
constexpr int32_t DEVICE_ID_SIZE_MAX = 65;
constexpr uint32_t DEFAULT_MTU_SIZE = 4096u;
constexpr uint32_t DEFAULT_TIMEOUT = 15 * 1000;
using namespace std;
using namespace OHOS::DistributedDataDfx;
using namespace OHOS::DistributedKv;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Strategy = CommunicationStrategy::Strategy;
struct ConnDetailsInfo {
    char myName[SESSION_NAME_SIZE_MAX] = "";
    char peerName[SESSION_NAME_SIZE_MAX] = "";
    std::string peerDevUuid;
    int32_t side = -1;
    int32_t routeType = -1;
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

    auto status = DmAdapter::GetInstance().StartWatchDeviceChange(this, {"softBusAdapter"});
    if (status != Status::SUCCESS) {
        ZLOGW("register device change failed, status:%d", static_cast<int>(status));
    }
}

SoftBusAdapter::~SoftBusAdapter()
{
    ZLOGI("begin");
    if (onBroadcast_) {
        UnregNodeDeviceStateCb(&g_callback);
    }
    connects_.Clear();
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

Status SoftBusAdapter::SendData(const PipeInfo &pipeInfo, const DeviceId &deviceId, const DataInfo &dataInfo,
    uint32_t length, const MessageInfo &info)
{
    std::shared_ptr<SoftBusClient> conn;
    connects_.Compute(deviceId.deviceId,
        [this, &pipeInfo, &deviceId, &conn, length](const auto& key,
            std::vector<std::shared_ptr<SoftBusClient>> &connects) -> bool {
                for (auto &connect : connects) {
                    if (connect->Support(length)) {
                        conn = connect;
                        return true;
                    }
                }
                auto connect = std::make_shared<SoftBusClient>(pipeInfo, deviceId, [this](int32_t connId) {
                    return GetSessionStatus(connId);
                });
                connects.emplace_back(connect);
                conn = connect;
                return true;
            });
    if (conn == nullptr) {
        return Status::ERROR;
    }
    auto status = conn->Send(dataInfo, length);
    if ((status != Status::NETWORK_ERROR) && (status != Status::RATE_LIMIT)) {
        Time now = std::chrono::steady_clock::now();
        auto expireTime = conn->GetExpireTime() > now ? conn->GetExpireTime() : now;
        lock_guard<decltype(taskMutex_)> lock(taskMutex_);
        if (taskId_ != ExecutorPool::INVALID_TASK_ID && expireTime < next_) {
            taskId_ = Context::GetInstance().GetThreadPool()->Reset(taskId_, expireTime - now);
            next_ = expireTime;
            if (taskId_ == ExecutorPool::INVALID_TASK_ID) {
                return status;
            }
        }
        if (taskId_ == ExecutorPool::INVALID_TASK_ID) {
            taskId_ = Context::GetInstance().GetThreadPool()->Schedule(expireTime - now, GetCloseSessionTask());
            next_ = expireTime;
        }
    }
    return status;
}

SoftBusAdapter::Task SoftBusAdapter::GetCloseSessionTask()
{
    return [this]() mutable {
        Time now = std::chrono::steady_clock::now();
        std::vector<std::shared_ptr<SoftBusClient>> connToClose;
        connects_.ForEach([&now, &connToClose](const auto &key, auto &connects) -> bool {
            std::vector<std::shared_ptr<SoftBusClient>> holdConnects;
            for (auto conn : connects) {
                if (conn == nullptr) {
                    continue;
                }
                auto expireTime = conn->GetExpireTime();
                if (expireTime <= now) {
                    ZLOGD("[timeout] close session connId:%{public}d", conn->GetConnId());
                    connToClose.emplace_back(conn);
                } else {
                    holdConnects.emplace_back(conn);
                }
            }
            connects = std::move(holdConnects);
            return false;
        });
        connects_.EraseIf([](const auto &key, const auto &conn) -> bool { return conn.empty(); });

        Time next = INVALID_NEXT;
        lock_guard<decltype(taskMutex_)> lg(taskMutex_);
        connects_.ForEach([&next](const auto &key,  auto &connects) -> bool {
            for (auto conn : connects) {
                if (conn == nullptr) {
                    continue;
                }
                auto expireTime = conn->GetExpireTime();
                if (expireTime < next) {
                    next = expireTime;
                }
            }
            return false;
        });
        if (next == INVALID_NEXT) {
            taskId_ = ExecutorPool::INVALID_TASK_ID;
            return;
        }
        taskId_ = Context::GetInstance().GetThreadPool()->Schedule(
            next > now ? next - now : ExecutorPool::INVALID_DELAY, GetCloseSessionTask());
        next_ = next;
    };
}

uint32_t SoftBusAdapter::GetMtuSize(const DeviceId &deviceId)
{
    uint32_t mtuSize = DEFAULT_MTU_SIZE;
    connects_.ComputeIfPresent(deviceId.deviceId, [&mtuSize](auto, auto &connects) {
        uint32_t mtu = 0;
        for (auto conn : connects) {
            if (conn == nullptr) {
                continue;
            }
            if (mtu < conn->GetMtuSize()) {
                mtu = conn->GetMtuSize();
            }
        }
        if (mtu != 0) {
            mtuSize = mtu;
        }
        return true;
    });
    return mtuSize;
}

uint32_t SoftBusAdapter::GetTimeout(const DeviceId &deviceId)
{
    uint32_t interval = DEFAULT_TIMEOUT;
    connects_.ComputeIfPresent(deviceId.deviceId, [&interval](auto, auto &connects) {
            uint32_t time = 0;
            for (auto conn : connects) {
                if (conn == nullptr) {
                    continue;
                }
                if (time < conn->GetTimeout()) {
                    time = conn->GetTimeout();
                }
            }
            if (time != 0) {
                interval = time;
            }
            return true;
    });
    return interval;
}

std::string SoftBusAdapter::DelConnect(int32_t connId)
{
    std::string name;
    connects_.ForEach([connId, &name](const auto &deviceId, auto &connects) -> bool {
        for (auto iter = connects.begin(); iter != connects.end();) {
            if (*iter != nullptr && **iter == connId) {
                name += deviceId;
                name += " ";
                iter = connects.erase(iter);
            } else {
                iter++;
            }
        }
        return false;
    });
    return name;
}

void SoftBusAdapter::DelSessionStatus(int32_t connId)
{
    lock_guard<mutex> lock(statusMutex_);
    auto it = sessionsStatus_.find(connId);
    if (it != sessionsStatus_.end()) {
        it->second->Clear(SOFTBUS_ERR);
        sessionsStatus_.erase(it);
    }
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
    if (status != SOFTBUS_OK) {
        return;
    }
    connects_.ForEach([connId](const auto &key, auto &connects) -> bool {
        for (auto conn : connects) {
            if (conn != nullptr && *conn == connId) {
                conn->UpdateExpireTime();
            }
        }
        return false;
    });
}

std::string SoftBusAdapter::OnSessionClose(int32_t connId)
{
    DelSessionStatus(connId);
    return DelConnect(connId);
}

std::shared_ptr<BlockData<int32_t>> SoftBusAdapter::GetSemaphore(int32_t connId)
{
    lock_guard<mutex> lock(statusMutex_);
    if (sessionsStatus_.find(connId) == sessionsStatus_.end()) {
        sessionsStatus_.emplace(connId, std::make_shared<BlockData<int32_t>>(WAIT_MAX_TIME, SOFTBUS_ERR));
    }
    return sessionsStatus_[connId];
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

    int32_t routeType = RouteType::INVALID_ROUTE_TYPE;
    ret = GetSessionOption(connId, SESSION_OPTION_LINK_TYPE, &routeType, sizeof(routeType));
    if (ret != SOFTBUS_OK) {
        return ROUTE_TYPE_INVALID;
    }
    connInfo.routeType = routeType;

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
          "peer devId:%{public}s, side:%{public}d, routeType:%{public}d", connId, connInfo.myName, connInfo.peerName,
          KvStoreUtils::ToBeAnonymous(connInfo.peerDevUuid).c_str(), connInfo.side, connInfo.routeType);
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

void SoftBusAdapter::OnDeviceChanged(const AppDistributedKv::DeviceInfo &info,
    const AppDistributedKv::DeviceChangeType &type) const
{
    if (info.uuid == DmAdapter::CLOUD_DEVICE_UUID) {
        return;
    }

    switch (type) {
        case AppDistributedKv::DeviceChangeType::DEVICE_ONLINE:
            CommunicationStrategy::GetInstance().SetStrategy(info.uuid, Strategy::ON_LINE_SELECT_CHANNEL);
            break;
        case AppDistributedKv::DeviceChangeType::DEVICE_OFFLINE:
            CommunicationStrategy::GetInstance().RemoveStrategy(info.uuid);
            break;
        case AppDistributedKv::DeviceChangeType::DEVICE_ONREADY:
            CommunicationStrategy::GetInstance().SetStrategy(info.uuid, Strategy::DEFAULT);
            break;
        default:
            break;
    }
}
} // namespace AppDistributedKv
} // namespace OHOS