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
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "SoftBusAdapter"

#include "softbus_adapter.h"

#include "accesstoken_kit.h"
#include "account/account_delegate.h"
#include "communication/connect_manager.h"
#include "communicator_context.h"
#include "data_level.h"
#include "device_manager_adapter.h"
#include "log_print.h"
#include "metadata/appid_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "softbus_error_code.h"
#include "utils/anonymous.h"

namespace OHOS {
namespace AppDistributedKv {
using namespace std;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedData;
using namespace OHOS::Security::AccessToken;

static constexpr const char *DEFAULT_USER = "default";
static constexpr const char *SYSTEM_USER = "0";
static constexpr const char *PKG_NAME = "distributeddata-default";
static constexpr uint32_t DEFAULT_MTU_SIZE = 4096 * 1024u;
static constexpr uint32_t DEFAULT_TIMEOUT = 30 * 1000;
static constexpr uint32_t QOS_COUNT = 3;
static constexpr QosTV Qos[QOS_COUNT] = { { .qos = QOS_TYPE_MIN_BW, .value = 64 * 1024 },
    { .qos = QOS_TYPE_MAX_LATENCY, .value = 15000 }, { .qos = QOS_TYPE_MIN_LATENCY, .value = 1600 } };

class AppDataListenerWrap {
public:
    static void SetDataHandler(SoftBusAdapter *handler);

    static void OnClientShutdown(int32_t socket, ShutdownReason reason);
    static void OnClientBytesReceived(int32_t socket, const void *data, uint32_t dataLen);
    static void OnClientSocketChanged(int32_t socket, QoSEvent eventId, const QosTV *qos, uint32_t qosCount);

    static void OnServerBind(int32_t socket, PeerSocketInfo info);
    static void OnServerShutdown(int32_t socket, ShutdownReason reason);
    static void OnServerBytesReceived(int32_t socket, const void *data, uint32_t dataLen);
    static bool OnServerAccessCheck(int32_t socket, PeerSocketInfo info, SocketAccessInfo *peerInfo,
        SocketAccessInfo *localInfo);

private:
    // notify all listeners when received message
    static void NotifyDataListeners(const uint8_t *data, const int size, const std::string &deviceId,
        const PipeInfo &pipeInfo);
    static std::string GetPipeId(const std::string &name);
    static std::pair<bool, StoreMetaData> LoadMetaData(const std::string &bundleName, const std::string &storeId,
        int32_t userId);

    static SoftBusAdapter *softBusAdapter_;
};

SoftBusAdapter *AppDataListenerWrap::softBusAdapter_;
std::shared_ptr<SoftBusAdapter> SoftBusAdapter::instance_;

namespace {
void OnDataLevelChanged(const char* networkId, const DataLevel dataLevel)
{
    if (networkId == nullptr) {
        return;
    }
    LevelInfo level = {
        .dynamic = dataLevel.dynamicLevel,
        .statics = dataLevel.staticLevel,
        .switches = dataLevel.switchLevel,
        .switchesLen = dataLevel.switchLength,
    };
    auto uuid = DeviceManagerAdapter::GetInstance().GetUuidByNetworkId(networkId);
    SoftBusAdapter::GetInstance()->OnBroadcast({ uuid }, std::move(level));
}

IDataLevelCb g_callback = {
    .onDataLevelChanged = OnDataLevelChanged,
};
} // namespace

SoftBusAdapter::SoftBusAdapter()
{
    ZLOGI("begin");
    AppDataListenerWrap::SetDataHandler(this);

    clientListener_.OnShutdown = AppDataListenerWrap::OnClientShutdown;
    clientListener_.OnBytes = AppDataListenerWrap::OnClientBytesReceived;
    clientListener_.OnMessage = AppDataListenerWrap::OnClientBytesReceived;
    clientListener_.OnQos = AppDataListenerWrap::OnClientSocketChanged;

    serverListener_.OnBind = AppDataListenerWrap::OnServerBind;
    serverListener_.OnShutdown = AppDataListenerWrap::OnServerShutdown;
    serverListener_.OnBytes = AppDataListenerWrap::OnServerBytesReceived;
    serverListener_.OnMessage = AppDataListenerWrap::OnServerBytesReceived;
    serverListener_.OnNegotiate2 = AppDataListenerWrap::OnServerAccessCheck;

    CommunicatorContext::GetInstance().SetSessionListener([this](const std::string &deviceId) {
        StartCloseSessionTask(deviceId);
    });

    ConnectManager::GetInstance()->RegisterCloseSessionTask([this](const std::string &networkId) {
        return CloseSession(networkId);
    });
    ConnectManager::GetInstance()->RegisterSessionCloseListener("CommunicatorContext",
        [](const std::string &networkId) {
            auto uuid = DeviceManagerAdapter::GetInstance().GetUuidByNetworkId(networkId);
            CommunicatorContext::GetInstance().NotifySessionClose(uuid);
        });
    ConnectManager::GetInstance()->OnStart();
}

SoftBusAdapter::~SoftBusAdapter()
{
    ZLOGI("begin");
    if (onBroadcast_) {
        UnregDataLevelChangeCb(PKG_NAME);
    }
    connects_.Clear();
    ConnectManager::GetInstance()->OnDestory();
}

std::shared_ptr<SoftBusAdapter> SoftBusAdapter::GetInstance()
{
    static std::once_flag onceFlag;
    std::call_once(onceFlag, [&] {
        instance_ = std::make_shared<SoftBusAdapter>();
    });
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

void SoftBusAdapter::GetExpireTime(std::shared_ptr<SoftBusClient> &conn)
{
    Time now = std::chrono::steady_clock::now();
    auto expireTime = conn->GetExpireTime() > now ? conn->GetExpireTime() : now;
    lock_guard<decltype(taskMutex_)> lock(taskMutex_);
    if (taskId_ != ExecutorPool::INVALID_TASK_ID && expireTime < next_) {
        taskId_ = CommunicatorContext::GetInstance().GetThreadPool()->Reset(taskId_, expireTime - now);
        next_ = expireTime;
    }
}

std::pair<Status, int32_t> SoftBusAdapter::SendData(const PipeInfo &pipeInfo, const DeviceId &deviceId,
    const DataInfo &dataInfo, uint32_t length, const MessageInfo &info)
{
    std::shared_ptr<SoftBusClient> conn = GetConnect(pipeInfo, deviceId);
    if (conn == nullptr) {
        return std::make_pair(Status::ERROR, 0);
    }
    auto status = conn->CheckStatus();
    if (status == Status::RATE_LIMIT) {
        return std::make_pair(Status::RATE_LIMIT, 0);
    }
    if (status != Status::SUCCESS) {
        return OpenConnect(conn, deviceId, dataInfo.extraInfo);
    }
    status = conn->SendData(dataInfo);
    if ((status != Status::NETWORK_ERROR) && (status != Status::RATE_LIMIT)) {
        GetExpireTime(conn);
    }
    auto errCode = conn->GetSoftBusError();
    return std::make_pair(status, errCode);
}

bool SoftBusAdapter::ConfigSessionAccessInfo(const ExtraDataInfo &extraInfo, SessionAccessInfo &sessionAccessInfo)
{
    if (extraInfo.userId.empty() || (extraInfo.bundleName.empty() && extraInfo.appId.empty()) ||
        extraInfo.storeId.empty()) {
        ZLOGE("extraInfo is empty, userId:%{public}s, bundleName:%{public}s, appId:%{public}s, storeId:%{public}s",
            extraInfo.userId.c_str(), extraInfo.bundleName.c_str(), extraInfo.appId.c_str(),
            Anonymous::Change(extraInfo.storeId).c_str());
        return false;
    }
    AppIDMetaData appIdMeta;
    if (extraInfo.bundleName.empty() && !MetaDataManager::GetInstance().LoadMeta(extraInfo.appId, appIdMeta, true)) {
        ZLOGE("load appIdMeta fail, appId:%{public}s", extraInfo.appId.c_str());
        return false;
    }
    StoreMetaData metaData;
    metaData.deviceId = DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.user = extraInfo.userId == DEFAULT_USER ? SYSTEM_USER : extraInfo.userId;
    metaData.bundleName = extraInfo.bundleName.empty() ? appIdMeta.bundleName : extraInfo.bundleName;
    metaData.storeId = extraInfo.storeId;
    if ((extraInfo.tokenId == 0) && !MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData)) {
        ZLOGE("get meta data fail, metaKey:%{public}s", Anonymous::Change(metaData.GetKeyWithoutPath()).c_str());
        return false;
    }
    sessionAccessInfo.bundleName = metaData.bundleName;
    sessionAccessInfo.storeId = metaData.storeId;
    sessionAccessInfo.tokenId = (extraInfo.tokenId != 0) ? extraInfo.tokenId : metaData.tokenId;
    int foregroundUserId = 0;
    auto ret = AccountDelegate::GetInstance()->QueryForegroundUserId(foregroundUserId);
    if (!ret) {
        return false;
    }
    sessionAccessInfo.userId = foregroundUserId;
    auto accountId = AccountDelegate::GetInstance()->GetCurrentAccountId();
    if (accountId.empty()) {
        return false;
    }
    sessionAccessInfo.accountId = accountId;
    return true;
}

std::shared_ptr<SoftBusClient> SoftBusAdapter::GetConnect(const PipeInfo &pipeInfo, const DeviceId &deviceId)
{
    std::string networkId = DeviceManagerAdapter::GetInstance().ToNetworkID(deviceId.deviceId);
    if (networkId.empty()) {
        ZLOGE("networkId is empty, deviceId:%{public}s", Anonymous::Change(deviceId.deviceId).c_str());
        return nullptr;
    }
    std::shared_ptr<SoftBusClient> conn;
    uint32_t qosType = DeviceManagerAdapter::GetInstance().IsOHOSType(deviceId.deviceId) ? SoftBusClient::QOS_HML :
        SoftBusClient::QOS_BR;
    connects_.Compute(deviceId.deviceId,
        [&pipeInfo, &deviceId, &conn, qosType, &networkId](const auto &key,
            std::vector<std::shared_ptr<SoftBusClient>> &connects) -> bool {
            for (auto &connect : connects) {
                if (connect == nullptr) {
                    continue;
                }
                if (connect->GetQoSType() == qosType) {
                    conn = connect;
                    return true;
                }
            }
            auto connect = std::make_shared<SoftBusClient>(pipeInfo, deviceId, networkId, qosType);
            connects.emplace_back(connect);
            conn = connect;
            return true;
        });
    return conn;
}

std::pair<Status, int32_t> SoftBusAdapter::OpenConnect(const std::shared_ptr<SoftBusClient> &conn,
    const DeviceId &deviceId, const ExtraDataInfo &extraInfo)
{
    auto networkId = DeviceManagerAdapter::GetInstance().ToNetworkID(deviceId.deviceId);
    if (conn != nullptr) {
        auto oldNetworkId = conn->GetNetworkId();
        if (networkId != oldNetworkId) {
            ZLOGI("peer networkId changed, %{public}s->%{public}s", Anonymous::Change(oldNetworkId).c_str(),
                Anonymous::Change(networkId).c_str());
            conn->UpdateNetworkId(networkId);
        }
    }
    auto isOHType = DeviceManagerAdapter::GetInstance().IsOHOSType(deviceId.deviceId);
    SessionAccessInfo sessionAccessInfo = { .isOHType = isOHType };
    if (isOHType && !ConfigSessionAccessInfo(extraInfo, sessionAccessInfo)) {
        ZLOGE("peer device is not oh device or config accessInfo fail, isOHType:%{public}d, deviceId:%{public}s",
            isOHType, Anonymous::Change(deviceId.deviceId).c_str());
        return std::make_pair(Status::ERROR, 0);
    }
    auto applyTask = [deviceId](int32_t errcode) {
        CommunicatorContext::GetInstance().NotifySessionReady(deviceId.deviceId, errcode);
    };
    auto connectTask = [this, connect = std::weak_ptr<SoftBusClient>(conn), &sessionAccessInfo]() {
        auto conn = connect.lock();
        if (conn != nullptr) {
            conn->OpenConnect(&clientListener_, sessionAccessInfo);
        }
    };
    ConnectManager::GetInstance()->ApplyConnect(networkId, applyTask, connectTask);
    return std::make_pair(Status::RATE_LIMIT, 0);
}

void SoftBusAdapter::StartCloseSessionTask(const std::string &deviceId)
{
    std::shared_ptr<SoftBusClient> conn;
    bool isOHOSType = DeviceManagerAdapter::GetInstance().IsOHOSType(deviceId);
    uint32_t qosType = isOHOSType ? SoftBusClient::QOS_HML : SoftBusClient::QOS_BR;
    auto connects = connects_.Find(deviceId);
    if (!connects.first) {
        return;
    }
    for (auto &connect : connects.second) {
        if (connect->GetQoSType() == qosType) {
            conn = connect;
            break;
        }
    }
    if (conn == nullptr) {
        return;
    }
    Time now = std::chrono::steady_clock::now();
    auto expireTime = conn->GetExpireTime() > now ? conn->GetExpireTime() : now;
    lock_guard<decltype(taskMutex_)> lock(taskMutex_);
    if (taskId_ == ExecutorPool::INVALID_TASK_ID) {
        ZLOGI("Start close session, deviceId:%{public}s", Anonymous::Change(deviceId).c_str());
        taskId_ = CommunicatorContext::GetInstance().GetThreadPool()->Schedule(expireTime - now, GetCloseSessionTask());
        next_ = expireTime;
    }
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
                    ZLOGI("[timeout] close session socket:%{public}d", conn->GetSocket());
                    connToClose.emplace_back(conn);
                } else {
                    holdConnects.emplace_back(conn);
                }
            }
            connects = std::move(holdConnects);
            return false;
        });
        connects_.EraseIf([](const auto &key, const auto &conn) -> bool {
            if (conn.empty()) {
                ConnectManager::GetInstance()->OnSessionClose(DeviceManagerAdapter::GetInstance().ToNetworkID(key));
            }
            return conn.empty();
        });
        Time next = INVALID_NEXT;
        lock_guard<decltype(taskMutex_)> lg(taskMutex_);
        connects_.ForEach([&next](const auto &key, auto &connects) -> bool {
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
        taskId_ = CommunicatorContext::GetInstance().GetThreadPool()->Schedule(
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
            if (mtu < conn->GetMtuBuffer()) {
                mtu = conn->GetMtuBuffer();
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
    return DEFAULT_TIMEOUT;
}

std::string SoftBusAdapter::DelConnect(int32_t socket, bool isForce)
{
    std::string name;
    std::set<std::string> closedConnect;
    connects_.EraseIf([socket, isForce, &name, &closedConnect](const auto &deviceId, auto &connects) -> bool {
        if (!isForce && DeviceManagerAdapter::GetInstance().IsOHOSType(deviceId)) {
            return false;
        }
        std::string networkId;
        for (auto iter = connects.begin(); iter != connects.end();) {
            if (*iter != nullptr && **iter == socket) {
                name += deviceId;
                name += " ";
                networkId = (*iter)->GetNetworkId();
                iter = connects.erase(iter);
            } else {
                iter++;
            }
        }
        if (connects.empty() && !networkId.empty()) {
            closedConnect.insert(std::move(networkId));
            return true;
        }
        return false;
    });
    for (const auto &networkId : closedConnect) {
        ConnectManager::GetInstance()->OnSessionClose(networkId);
    }
    return name;
}

std::string SoftBusAdapter::OnClientShutdown(int32_t socket, bool isForce)
{
    return DelConnect(socket, isForce);
}

bool SoftBusAdapter::IsSameStartedOnPeer(const struct PipeInfo &pipeInfo,
    __attribute__((unused)) const struct DeviceId &peer)
{
    ZLOGI("pipeInfo:%{public}s deviceId:%{public}s", pipeInfo.pipeId.c_str(),
        Anonymous::Change(peer.deviceId).c_str());
    return true;
}

void SoftBusAdapter::SetMessageTransFlag(const PipeInfo &pipeInfo, bool flag)
{
    ZLOGI("pipeInfo:%{public}s, flag:%{public}d", pipeInfo.pipeId.c_str(), flag);
    flag_ = flag;
}

int SoftBusAdapter::CreateSessionServerAdapter(const std::string &sessionName)
{
    ZLOGD("begin");
    SocketInfo socketInfo;
    std::string sessionServerName = sessionName;
    socketInfo.name = const_cast<char *>(sessionServerName.c_str());
    std::string pkgName = "ohos.distributeddata";
    socketInfo.pkgName = pkgName.data();
    socket_ = Socket(socketInfo);
    return Listen(socket_, Qos, QOS_COUNT, &serverListener_);
}

int SoftBusAdapter::RemoveSessionServerAdapter(const std::string &sessionName) const
{
    ZLOGD("begin");
    Shutdown(socket_);
    return 0;
}

void SoftBusAdapter::NotifyDataListeners(const uint8_t *data, int size, const std::string &deviceId,
    const PipeInfo &pipeInfo)
{
    ZLOGD("begin");
    auto ret = dataChangeListeners_.ComputeIfPresent(pipeInfo.pipeId,
        [&data, &size, &deviceId, &pipeInfo](const auto &key, const AppDataChangeListener *&value) {
            ZLOGD("ready to notify, pipeName:%{public}s, deviceId:%{public}s.", pipeInfo.pipeId.c_str(),
                Anonymous::Change(deviceId).c_str());
            DeviceInfo deviceInfo = DeviceManagerAdapter::GetInstance().GetDeviceInfo(deviceId);
            value->OnMessage(deviceInfo, data, size, pipeInfo);
            return true;
        });
    if (!ret) {
        ZLOGW("no listener %{public}s.", pipeInfo.pipeId.c_str());
    }
}

Status SoftBusAdapter::Broadcast(const PipeInfo &pipeInfo, const LevelInfo &levelInfo)
{
    DataLevel level = {
        .dynamicLevel = levelInfo.dynamic,
        .staticLevel = levelInfo.statics,
        .switchLevel = levelInfo.switches,
        .switchLength = levelInfo.switchesLen,
    };
    auto status = SetDataLevel(&level);
    if (status == SOFTBUS_FUNC_NOT_SUPPORT) {
        return Status::NOT_SUPPORT_BROADCAST;
    }
    return status != SOFTBUS_OK ? Status::ERROR : Status::SUCCESS;
}

void SoftBusAdapter::OnBroadcast(const DeviceId &device, const LevelInfo &levelInfo)
{
    ZLOGI("device:%{public}s", Anonymous::Change(device.deviceId).c_str());
    if (!onBroadcast_) {
        ZLOGW("no listener device:%{public}s", Anonymous::Change(device.deviceId).c_str());
        return;
    }
    onBroadcast_(device.deviceId, levelInfo);
}

int32_t SoftBusAdapter::ListenBroadcastMsg(const PipeInfo &pipeInfo,
    std::function<void(const std::string &, const LevelInfo &)> listener)
{
    if (onBroadcast_) {
        return SOFTBUS_ALREADY_EXISTED;
    }
    onBroadcast_ = std::move(listener);
    return RegDataLevelChangeCb(pipeInfo.pipeId.c_str(), &g_callback);
}

void AppDataListenerWrap::SetDataHandler(SoftBusAdapter *handler)
{
    ZLOGI("begin");
    softBusAdapter_ = handler;
}

void AppDataListenerWrap::OnClientShutdown(int32_t socket, ShutdownReason reason)
{
    // when the local close the session, this callback function will not be triggered;
    // when the current function is called, soft bus has released the session resource, only connId is valid;
    std::string name = softBusAdapter_->OnClientShutdown(socket);
    ZLOGI("[shutdown] socket:%{public}d, name:%{public}s", socket, Anonymous::Change(name).c_str());
}

void AppDataListenerWrap::OnClientBytesReceived(int32_t socket, const void *data, uint32_t dataLen) {}

void AppDataListenerWrap::OnClientSocketChanged(int32_t socket, QoSEvent eventId, const QosTV *qos, uint32_t qosCount)
{
    if (eventId == QoSEvent::QOS_SATISFIED && qos != nullptr && qos[0].qos == QOS_TYPE_MIN_BW && qosCount == 1) {
        auto name = softBusAdapter_->OnClientShutdown(socket, false);
        ZLOGI("[SocketChanged] socket:%{public}d, name:%{public}s", socket, Anonymous::Change(name).c_str());
    }
}

void AppDataListenerWrap::OnServerBind(int32_t socket, PeerSocketInfo info)
{
    softBusAdapter_->OnBind(socket, info);
    std::string peerDevUuid = DeviceManagerAdapter::GetInstance().GetUuidByNetworkId(std::string(info.networkId));

    ZLOGI("[OnServerBind] socket:%{public}d, peer name:%{public}s, peer devId:%{public}s", socket, info.name,
        Anonymous::Change(peerDevUuid).c_str());
}

void AppDataListenerWrap::OnServerShutdown(int32_t socket, ShutdownReason reason)
{
    softBusAdapter_->OnServerShutdown(socket);
    ZLOGI("Shut down reason:%{public}d socket id:%{public}d", reason, socket);
}

void AppDataListenerWrap::OnServerBytesReceived(int32_t socket, const void *data, uint32_t dataLen)
{
    SoftBusAdapter::ServerSocketInfo info;
    if (!softBusAdapter_->GetPeerSocketInfo(socket, info)) {
        ZLOGE("Get peer socket info failed, socket id %{public}d", socket);
        return;
    };
    std::string peerDevUuid = DeviceManagerAdapter::GetInstance().GetUuidByNetworkId(std::string(info.networkId));
    ZLOGD("[OnBytesReceived] socket:%{public}d, peer name:%{public}s, peer devId:%{public}s, data len:%{public}u",
        socket, info.name.c_str(), Anonymous::Change(peerDevUuid).c_str(), dataLen);

    std::string pipeId = GetPipeId(info.name);
    if (pipeId.empty()) {
        ZLOGE("pipId is invalid");
        return;
    }

    NotifyDataListeners(reinterpret_cast<const uint8_t *>(data), dataLen, peerDevUuid, { pipeId, "" });
}

std::pair<bool, StoreMetaData> AppDataListenerWrap::LoadMetaData(const std::string &bundleName,
    const std::string &storeId, int32_t userId)
{
    StoreMetaData metaData;
    metaData.deviceId = DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    metaData.bundleName = bundleName;
    metaData.storeId = storeId;
    metaData.user = std::to_string(userId);
    auto ret = MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData);
    if (!ret) {
        metaData.user = SYSTEM_USER;
        ret = MetaDataManager::GetInstance().LoadMeta(metaData.GetKeyWithoutPath(), metaData);
    }
    return { ret, metaData };
}

bool AppDataListenerWrap::OnServerAccessCheck(int32_t socket, PeerSocketInfo info, SocketAccessInfo *peerInfo,
    SocketAccessInfo *localInfo)
{
    ZLOGI("receive bind request, socket:%{public}d", socket);
    if (peerInfo == nullptr || localInfo == nullptr) {
        ZLOGE("peerInfo or localInfo is nullptr, peerInfo:%{public}d, localInfo:%{public}d", peerInfo == nullptr,
            localInfo == nullptr);
        return false;
    }
    SoftBusClient::AccessExtraInfo peerExtraInfo;
    if (!DistributedData::Serializable::Unmarshall(peerInfo->extraAccessInfo, peerExtraInfo)) {
        ZLOGE("Unmarshall failed, peer extraAccessInfo:%{public}s", peerInfo->extraAccessInfo);
        return false;
    }
    int foregroundUserId = 0;
    if (!AccountDelegate::GetInstance()->QueryForegroundUserId(foregroundUserId)) {
        return false;
    }
    localInfo->userId = foregroundUserId;
    auto resultPair = LoadMetaData(peerExtraInfo.bundleName, peerExtraInfo.storeId, foregroundUserId);
    if (!resultPair.first) {
        ZLOGE("local device has no store, bundleName:%{public}s", peerExtraInfo.bundleName.c_str());
        return false;
    }
    localInfo->localTokenId = resultPair.second.tokenId;
    auto type = AccessTokenKit::GetTokenTypeFlag(resultPair.second.tokenId);
    if (type == TOKEN_NATIVE || type == TOKEN_SHELL) {
        ZLOGW("this is system service sync, bundleName:%{public}s", peerExtraInfo.bundleName.c_str());
        return true;
    }
    AclParams aclParams;
    aclParams.accCaller.bundleName = peerExtraInfo.bundleName;
    aclParams.accCaller.accountId = AccountDelegate::GetInstance()->GetCurrentAccountId();
    aclParams.accCaller.userId = foregroundUserId;
    aclParams.accCaller.networkId = DeviceManagerAdapter::GetInstance().GetLocalDevice().networkId;
    aclParams.accCallee.accountId = peerExtraInfo.accountId;
    aclParams.accCallee.userId = peerInfo->userId;
    aclParams.accCallee.networkId = std::string(info.networkId);
    bool accessFlag = false;
    if (resultPair.second.authType == AuthType::IDENTICAL_ACCOUNT) {
        accessFlag = DeviceManagerAdapter::GetInstance().IsSameAccount(aclParams.accCaller, aclParams.accCallee);
    } else {
        accessFlag = DeviceManagerAdapter::GetInstance().CheckAccessControl(aclParams.accCaller, aclParams.accCallee);
    }
    return accessFlag;
}

std::string AppDataListenerWrap::GetPipeId(const std::string &name)
{
    auto pos = name.find('_');
    if (pos != std::string::npos) {
        return name.substr(0, pos);
    }
    return name;
}

void AppDataListenerWrap::NotifyDataListeners(const uint8_t *data, const int size, const std::string &deviceId,
    const PipeInfo &pipeInfo)
{
    softBusAdapter_->NotifyDataListeners(data, size, deviceId, pipeInfo);
}

bool SoftBusAdapter::GetPeerSocketInfo(int32_t socket, ServerSocketInfo &info)
{
    auto it = peerSocketInfos_.Find(socket);
    if (it.first) {
        info = it.second;
        return true;
    }
    return false;
}

void SoftBusAdapter::OnBind(int32_t socket, PeerSocketInfo info)
{
    ServerSocketInfo socketInfo;
    socketInfo.name = std::string(info.name);
    socketInfo.networkId = std::string(info.networkId);
    socketInfo.pkgName = std::string(info.pkgName);
    peerSocketInfos_.Insert(socket, socketInfo);
}

void SoftBusAdapter::OnServerShutdown(int32_t socket)
{
    peerSocketInfos_.Erase(socket);
}

bool SoftBusAdapter::CloseSession(const std::string &networkId)
{
    auto uuid = DeviceManagerAdapter::GetInstance().GetUuidByNetworkId(networkId);
    auto ret = connects_.Erase(uuid);
    if (ret != 0) {
        ConnectManager::GetInstance()->OnSessionClose(networkId);
    }
    return ret != 0;
}

Status SoftBusAdapter::ReuseConnect(const PipeInfo &pipeInfo, const DeviceId &deviceId, const ExtraDataInfo &extraInfo)
{
    bool isOHOSType = DeviceManagerAdapter::GetInstance().IsOHOSType(deviceId.deviceId);
    if (!isOHOSType) {
        return Status::NOT_SUPPORT;
    }
    std::shared_ptr<SoftBusClient> conn = GetConnect(pipeInfo, deviceId);
    if (conn == nullptr) {
        return Status::ERROR;
    }
    SessionAccessInfo sessionAccessInfo;
    if (!ConfigSessionAccessInfo(extraInfo, sessionAccessInfo)) {
        ZLOGE("config accessInfo fail, deviceId:%{public}s", Anonymous::Change(deviceId.deviceId).c_str());
        return Status::ERROR;
    }
    auto status = conn->ReuseConnect(&clientListener_, sessionAccessInfo);
    if (status != Status::SUCCESS) {
        return status;
    }
    uint32_t qosType = SoftBusClient::QOS_HML;
    // Avoid being cleared by scheduled tasks
    connects_.Compute(deviceId.deviceId, [&conn, qosType](const auto &key,
        std::vector<std::shared_ptr<SoftBusClient>> &connects) -> bool {
        for (auto &connect : connects) {
            if (connect == nullptr) {
                continue;
            }
            if (connect->GetQoSType() == qosType) {
                return true;
            }
        }
        connects.emplace_back(conn);
        return true;
    });
    StartCloseSessionTask(deviceId.deviceId);
    return Status::SUCCESS;
}
} // namespace AppDistributedKv
} // namespace OHOS