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

#include "softbus_adapter.h"

#include <mutex>
#include <thread>
#include "dfx_types.h"
#include "kv_store_delegate_manager.h"
#include "log_print.h"
#include "process_communicator_impl.h"
#include "reporter.h"
#include "session.h"
#include "softbus_bus_center.h"
#include "securec.h"
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
constexpr int32_t INVALID_SESSION_ID = -1;
constexpr int32_t SESSION_NAME_SIZE_MAX = 65;
constexpr int32_t DEVICE_ID_SIZE_MAX = 65;
constexpr int32_t ID_BUF_LEN = 65;
using namespace std;
using namespace OHOS::DistributedKv;

class AppDeviceListenerWrap {
public:
    explicit AppDeviceListenerWrap() {}
    ~AppDeviceListenerWrap() {}
    static void SetDeviceHandler(SoftBusAdapter *handler);
    static void OnDeviceOnline(NodeBasicInfo *info);
    static void OnDeviceOffline(NodeBasicInfo *info);
    static void OnDeviceInfoChanged(NodeBasicInfoType type, NodeBasicInfo *info);
private:
    static SoftBusAdapter *softBusAdapter_;
    static void NotifyAll(NodeBasicInfo *info, DeviceChangeType type);
};
SoftBusAdapter *AppDeviceListenerWrap::softBusAdapter_;

class AppDataListenerWrap {
public:
    static void SetDataHandler(SoftBusAdapter *handler);
    static int OnSessionOpened(int sessionId, int result);
    static void OnSessionClosed(int sessionId);
    static void OnMessageReceived(int sessionId, const void *data, unsigned int dataLen);
    static void OnBytesReceived(int sessionId, const void *data, unsigned int dataLen);
public:
    // notify all listeners when received message
    static void NotifyDataListeners(const uint8_t *ptr, const int size, const std::string &deviceId,
                             const PipeInfo &pipeInfo);
    static SoftBusAdapter *softBusAdapter_;
};
SoftBusAdapter *AppDataListenerWrap::softBusAdapter_;
std::shared_ptr<SoftBusAdapter> SoftBusAdapter::instance_;

void AppDeviceListenerWrap::OnDeviceInfoChanged(NodeBasicInfoType type, NodeBasicInfo *info)
{
    std::string uuid = softBusAdapter_->GetUuidByNodeId(std::string(info->networkId));
    ZLOGI("[InfoChange] type:%{public}d, id:%{public}s, name:%{public}s",
        type, SoftBusAdapter::ToBeAnonymous(uuid).c_str(), info->deviceName);
}

void AppDeviceListenerWrap::OnDeviceOffline(NodeBasicInfo *info)
{
    std::string uuid = softBusAdapter_->GetUuidByNodeId(std::string(info->networkId));
    ZLOGI("[Offline] id:%{public}s, name:%{public}s, typeId:%{public}d",
        SoftBusAdapter::ToBeAnonymous(uuid).c_str(), info->deviceName, info->deviceTypeId);
    NotifyAll(info, DeviceChangeType::DEVICE_OFFLINE);
}

void AppDeviceListenerWrap::OnDeviceOnline(NodeBasicInfo *info)
{
    std::string uuid = softBusAdapter_->GetUuidByNodeId(std::string(info->networkId));
    ZLOGI("[Online] id:%{public}s, name:%{public}s, typeId:%{public}d",
        SoftBusAdapter::ToBeAnonymous(uuid).c_str(), info->deviceName, info->deviceTypeId);
    NotifyAll(info, DeviceChangeType::DEVICE_ONLINE);
}

void AppDeviceListenerWrap::SetDeviceHandler(SoftBusAdapter *handler)
{
    ZLOGI("SetDeviceHandler.");
    softBusAdapter_ = handler;
}

void AppDeviceListenerWrap::NotifyAll(NodeBasicInfo *info, DeviceChangeType type)
{
    std::string uuid = softBusAdapter_->GetUuidByNodeId(std::string(info->networkId));
    std::string udid = softBusAdapter_->GetUdidByNodeId(std::string(info->networkId));
    DeviceInfo di = { uuid, udid, info->networkId, info->deviceName, info->deviceTypeId };
    softBusAdapter_->NotifyAll(di, type);
}

SoftBusAdapter::SoftBusAdapter()
{
    ZLOGI("begin");
    AppDeviceListenerWrap::SetDeviceHandler(this);
    AppDataListenerWrap::SetDataHandler(this);

    nodeStateCb_.events = EVENT_NODE_STATE_MASK;
    nodeStateCb_.onNodeOnline = AppDeviceListenerWrap::OnDeviceOnline;
    nodeStateCb_.onNodeOffline = AppDeviceListenerWrap::OnDeviceOffline;
    nodeStateCb_.onNodeBasicInfoChanged = AppDeviceListenerWrap::OnDeviceInfoChanged;

    sessionListener_.OnSessionOpened = AppDataListenerWrap::OnSessionOpened;
    sessionListener_.OnSessionClosed = AppDataListenerWrap::OnSessionClosed;
    sessionListener_.OnBytesReceived = AppDataListenerWrap::OnBytesReceived;
    sessionListener_.OnMessageReceived = AppDataListenerWrap::OnMessageReceived;
}

SoftBusAdapter::~SoftBusAdapter()
{
    ZLOGI("begin");
    int32_t errNo = UnregNodeDeviceStateCb(&nodeStateCb_);
    if (errNo != SOFTBUS_OK) {
        ZLOGE("UnregNodeDeviceStateCb fail %{public}d", errNo);
    }
}

void SoftBusAdapter::Init()
{
    ZLOGI("begin");
    std::thread th = std::thread([this]() {
        auto communicator = std::make_shared<ProcessCommunicatorImpl>();
        auto retcom = DistributedDB::KvStoreDelegateManager::SetProcessCommunicator(communicator);
        ZLOGI("set communicator ret:%{public}d.", static_cast<int>(retcom));
        int i = 0;
        constexpr int RETRY_TIMES = 300;
        while (i++ < RETRY_TIMES) {
            int32_t errNo = RegNodeDeviceStateCb("ohos.distributeddata", &nodeStateCb_);
            if (errNo != SOFTBUS_OK) {
                ZLOGE("RegNodeDeviceStateCb fail %{public}d, time:%{public}d", errNo, i);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            ZLOGI("RegNodeDeviceStateCb success");
            return;
        }
        ZLOGE("Init failed %{public}d times and exit now.", RETRY_TIMES);
    });
    th.detach();
}

Status SoftBusAdapter::StartWatchDeviceChange(const AppDeviceChangeListener *observer,
    __attribute__((unused)) const PipeInfo &pipeInfo)
{
    ZLOGI("begin");
    if (observer == nullptr) {
        ZLOGW("observer is null.");
        return Status::ERROR;
    }
    std::lock_guard<std::mutex> lock(deviceChangeMutex_);
    auto result = listeners_.insert(observer);
    if (!result.second) {
        ZLOGW("Add listener error.");
        return Status::ERROR;
    }
    ZLOGI("end");
    return Status::SUCCESS;
}

Status SoftBusAdapter::StopWatchDeviceChange(const AppDeviceChangeListener *observer,
    __attribute__((unused)) const PipeInfo &pipeInfo)
{
    ZLOGI("begin");
    if (observer == nullptr) {
        ZLOGW("observer is null.");
        return Status::ERROR;
    }
    std::lock_guard<std::mutex> lock(deviceChangeMutex_);
    auto result = listeners_.erase(observer);
    if (result <= 0) {
        return Status::ERROR;
    }
    ZLOGI("end");
    return Status::SUCCESS;
}

void SoftBusAdapter::NotifyAll(const DeviceInfo &deviceInfo, const DeviceChangeType &type)
{
    std::thread th = std::thread([this, deviceInfo, type]() {
        std::vector<const AppDeviceChangeListener *> listeners;
        {
            std::lock_guard<std::mutex> lock(deviceChangeMutex_);
            for (const auto &listener : listeners_) {
                listeners.push_back(listener);
            }
        }
        ZLOGD("high");
        ZLOGD("[Notify] to DB from: %{public}s, type:%{public}d", ToBeAnonymous(deviceInfo.uuid).c_str(), type);
        UpdateRelationship(deviceInfo, type);
        for (const auto &device : listeners) {
            if (device == nullptr) {
                continue;
            }
            if (device->GetChangeLevelType() == ChangeLevelType::HIGH) {
                device->OnDeviceChanged(deviceInfo, type);
                break;
            }
        }
        ZLOGD("low");
        for (const auto &device : listeners) {
            if (device == nullptr) {
                continue;
            }
            if (device->GetChangeLevelType() == ChangeLevelType::LOW) {
                device->OnDeviceChanged(deviceInfo, DeviceChangeType::DEVICE_OFFLINE);
                device->OnDeviceChanged(deviceInfo, type);
            }
        }
        ZLOGD("min");
        for (const auto &device : listeners) {
            if (device == nullptr) {
                continue;
            }
            if (device->GetChangeLevelType() == ChangeLevelType::MIN) {
                device->OnDeviceChanged(deviceInfo, type);
            }
        }
    });
    th.detach();
}

std::vector<DeviceInfo> SoftBusAdapter::GetRemoteDevices() const
{
    std::vector<DeviceInfo> dis;
    NodeBasicInfo *info = nullptr;
    int32_t infoNum = 0;
    dis.clear();

    int32_t ret = GetAllNodeDeviceInfo("ohos.distributeddata", &info, &infoNum);
    if (ret != SOFTBUS_OK) {
        ZLOGE("GetAllNodeDeviceInfo error");
        return dis;
    }
    ZLOGD("GetAllNodeDeviceInfo success infoNum=%{public}d", infoNum);

    for (int i = 0; i < infoNum; i++) {
        std::string uuid = GetUuidByNodeId(std::string(info[i].networkId));
        std::string udid = GetUdidByNodeId(std::string(info[i].networkId));
        DeviceInfo deviceInfo = { uuid, udid, info[i].networkId, info[i].deviceName, info[i].deviceTypeId };
        dis.push_back(deviceInfo);
    }
    if (info != nullptr) {
        FreeNodeInfo(info);
    }
    return dis;
}

DeviceInfo SoftBusAdapter::GetDeviceInfo(const std::string &id)
{
    DeviceInfo cacheInfo = GetDeviceCacheInfo(id);
    if (cacheInfo.networkId.empty()) {
        ZLOGE("Get DeviceInfo failed.");
    }
    return cacheInfo;
}

DeviceInfo SoftBusAdapter::GetLocalDevice()
{
    if (!localInfo_.uuid.empty()) {
        return localInfo_;
    }

    NodeBasicInfo info;
    int32_t ret = GetLocalNodeDeviceInfo("ohos.distributeddata", &info);
    if (ret != SOFTBUS_OK) {
        ZLOGE("GetLocalNodeDeviceInfo error");
        return {};
    }
    std::string uuid = GetUuidByNodeId(std::string(info.networkId));
    std::string udid = GetUdidByNodeId(std::string(info.networkId));
    ZLOGD("[LocalDevice] id:%{private}s, name:%{private}s, type:%{private}d",
        ToBeAnonymous(uuid).c_str(), info.deviceName, info.deviceTypeId);
    localInfo_ = { uuid, udid, info.networkId, info.deviceName, info.deviceTypeId };
    return localInfo_;
}

std::string SoftBusAdapter::GetUuidByNodeId(const std::string &nodeId) const
{
    char uuid[ID_BUF_LEN] = {0};
    int32_t ret = GetNodeKeyInfo("ohos.distributeddata", nodeId.c_str(),
        NodeDeviceInfoKey::NODE_KEY_UUID, reinterpret_cast<uint8_t *>(uuid), ID_BUF_LEN);
    if (ret != SOFTBUS_OK) {
        ZLOGW("GetNodeKeyInfo error, nodeId:%{public}s", ToBeAnonymous(nodeId).c_str());
        return "";
    }
    return std::string(uuid);
}

std::string SoftBusAdapter::GetUdidByNodeId(const std::string &nodeId) const
{
    char udid[ID_BUF_LEN] = {0};
    int32_t ret = GetNodeKeyInfo("ohos.distributeddata", nodeId.c_str(),
        NodeDeviceInfoKey::NODE_KEY_UDID, reinterpret_cast<uint8_t *>(udid), ID_BUF_LEN);
    if (ret != SOFTBUS_OK) {
        ZLOGW("GetNodeKeyInfo error, nodeId:%{public}s", ToBeAnonymous(nodeId).c_str());
        return "";
    }
    return std::string(udid);
}

DeviceInfo SoftBusAdapter::GetLocalBasicInfo() const
{
    ZLOGD("begin");
    NodeBasicInfo info;
    int32_t ret = GetLocalNodeDeviceInfo("ohos.distributeddata", &info);
    if (ret != SOFTBUS_OK) {
        ZLOGE("GetLocalNodeDeviceInfo error");
        return {};
    }
    ZLOGD("[LocalBasicInfo] networkId:%{private}s, name:%{private}s, type:%{private}d",
        ToBeAnonymous(std::string(info.networkId)).c_str(), info.deviceName, info.deviceTypeId);
    std::string uuid = GetUuidByNodeId(std::string(info.networkId));
    std::string udid = GetUdidByNodeId(std::string(info.networkId));
    DeviceInfo localInfo = { uuid, udid, info.networkId, info.deviceName, info.deviceTypeId };
    return localInfo;
}

void SoftBusAdapter::UpdateRelationship(const DeviceInfo &deviceInfo, const DeviceChangeType &type)
{
    switch (type) {
        case DeviceChangeType::DEVICE_OFFLINE: {
            deviceInfos_.Erase(deviceInfo.networkId);
            deviceInfos_.Erase(deviceInfo.uuid);
            deviceInfos_.Erase(deviceInfo.udid);
            break;
        }
        case DeviceChangeType::DEVICE_ONLINE: {
            deviceInfos_.InsertOrAssign(deviceInfo.networkId, deviceInfo);
            deviceInfos_.InsertOrAssign(deviceInfo.uuid, deviceInfo);
            deviceInfos_.InsertOrAssign(deviceInfo.udid, deviceInfo);
            break;
        }
        default: {
            ZLOGW("unknown type.");
            break;
        }
    }
}

DeviceInfo SoftBusAdapter::GetDeviceInfoFromCache(const std::string &id) const
{
    auto deviceInfo = deviceInfos_.Find(id);
    if (deviceInfo.first) {
        return deviceInfo.second;
    }
    ZLOGI("did not get deviceInfo from cache. ");
    return {};
}

void SoftBusAdapter::UpdateDeviceCacheInfo() const
{
    ZLOGW("get the network id from devices.");
    NodeBasicInfo *info = nullptr;
    int32_t infoNum = 0;
    int32_t ret = GetAllNodeDeviceInfo("ohos.distributeddata", &info, &infoNum);  // to find from softbus
    if (ret != SOFTBUS_OK) {
        ZLOGE("GetAllNodeDeviceInfo error");
        return;
    }
    ZLOGD("GetAllNodeDeviceInfo success infoNum=%{public}d", infoNum);
    for (int i = 0; i < infoNum; i++) {
        auto networkId = info[i].networkId;
        auto uuid = GetUuidByNodeId(networkId);
        auto udid = GetUdidByNodeId(networkId);
        DeviceInfo deviceInfo = { uuid, udid, networkId, info[i].deviceName, info[i].deviceTypeId };
        deviceInfos_.InsertOrAssign(uuid, deviceInfo);
        deviceInfos_.InsertOrAssign(udid, deviceInfo);
        deviceInfos_.InsertOrAssign(networkId, deviceInfo);
    }
    if (info != nullptr) {
        FreeNodeInfo(info);
    }
}

DeviceInfo SoftBusAdapter::GetDeviceCacheInfo(const std::string &id) const
{
    DeviceInfo cacheInfo = GetDeviceInfoFromCache(id);
    if (!cacheInfo.networkId.empty()) {
        return cacheInfo;
    }
    UpdateDeviceCacheInfo();
    cacheInfo = GetDeviceInfoFromCache(id);
    return cacheInfo;
}

std::string SoftBusAdapter::ToUUID(const std::string &id) const
{
    DeviceInfo cacheInfo = GetDeviceCacheInfo(id);
    if (cacheInfo.uuid.empty()) {
        ZLOGW("unknown id.");
        return "";
    }
    return cacheInfo.uuid;
}

std::string SoftBusAdapter::ToNodeID(const std::string &nodeId, const std::string &defaultId) const
{
    DeviceInfo cacheInfo = GetDeviceCacheInfo(nodeId);
    if (cacheInfo.networkId.empty()) {
        ZLOGW("unknown id.");
        return defaultId;
    }
    return cacheInfo.networkId;
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
    std::call_once(onceFlag, [&] {instance_ = std::make_shared<SoftBusAdapter>(); });
    return instance_;
}

Status SoftBusAdapter::StartWatchDataChange(const AppDataChangeListener *observer, const PipeInfo &pipeInfo)
{
    ZLOGD("begin");
    if (observer == nullptr) {
        return Status::INVALID_ARGUMENT;
    }
    lock_guard<mutex> lock(dataChangeMutex_);
    auto it = dataChangeListeners_.find(pipeInfo.pipeId);
    if (it != dataChangeListeners_.end()) {
        ZLOGW("Add listener error or repeated adding.");
        return Status::ERROR;
    }
    ZLOGD("current appid %{public}s", pipeInfo.pipeId.c_str());
    dataChangeListeners_.insert({pipeInfo.pipeId, observer});
    return Status::SUCCESS;
}

Status SoftBusAdapter::StopWatchDataChange(__attribute__((unused))const AppDataChangeListener *observer,
                                           const PipeInfo &pipeInfo)
{
    ZLOGD("begin");
    lock_guard<mutex> lock(dataChangeMutex_);
    if (dataChangeListeners_.erase(pipeInfo.pipeId)) {
        return Status::SUCCESS;
    }
    ZLOGW("stop data observer error, pipeInfo:%{public}s", pipeInfo.pipeId.c_str());
    return Status::ERROR;
}

Status SoftBusAdapter::SendData(const PipeInfo &pipeInfo, const DeviceId &deviceId, const uint8_t *ptr, int size,
                                const MessageInfo &info)
{
    SessionAttribute attr;
    attr.dataType = TYPE_BYTES;
    ZLOGD("[SendData] to %{public}s ,session:%{public}s, size:%{public}d", ToBeAnonymous(deviceId.deviceId).c_str(),
        pipeInfo.pipeId.c_str(), size);
    int sessionId = OpenSession(pipeInfo.pipeId.c_str(), pipeInfo.pipeId.c_str(),
        ToNodeID(deviceId.deviceId, "").c_str(), "GROUP_ID", &attr);
    if (sessionId < 0) {
        ZLOGW("OpenSession %{public}s, type:%{public}d failed, sessionId:%{public}d",
            pipeInfo.pipeId.c_str(), info.msgType, sessionId);
        return Status::NETWORK_ERROR;
    }
    int state = GetSessionStatus(sessionId);
    ZLOGD("waited for notification, state:%{public}d", state);
    if (state != SOFTBUS_OK) {
        ZLOGE("OpenSession callback result error");
        return Status::NETWORK_ERROR;
    }
    ZLOGD("[SendBytes] start, size is %{public}d, session type is %{public}d.", size, attr.dataType);
    int32_t ret = SendBytes(sessionId, (void*)ptr, size);
    if (ret != SOFTBUS_OK) {
        ZLOGE("[SendBytes] to %{public}d failed, ret:%{public}d.", sessionId, ret);
        return Status::ERROR;
    }
    return Status::SUCCESS;
}

int32_t SoftBusAdapter::GetSessionStatus(int32_t sessionId)
{
    auto semaphore = GetSemaphore(sessionId);
    return semaphore->GetValue();
}

void SoftBusAdapter::OnSessionOpen(int32_t sessionId, int32_t status)
{
    auto semaphore = GetSemaphore(sessionId);
    semaphore->SetValue(status);
}

void SoftBusAdapter::OnSessionClose(int32_t sessionId)
{
    lock_guard<mutex> lock(statusMutex_);
    auto it = sessionsStatus_.find(sessionId);
    if (it != sessionsStatus_.end()) {
        it->second->Clear();
        sessionsStatus_.erase(it);
    }
}

std::shared_ptr<BlockData<int32_t>> SoftBusAdapter::GetSemaphore(int32_t sessionId)
{
    lock_guard<mutex> lock(statusMutex_);
    if (sessionsStatus_.find(sessionId) == sessionsStatus_.end()) {
        sessionsStatus_.emplace(sessionId, std::make_shared<BlockData<int32_t>>());
    }
    return sessionsStatus_[sessionId];
}

bool SoftBusAdapter::IsSameStartedOnPeer(const struct PipeInfo &pipeInfo,
                                         __attribute__((unused))const struct DeviceId &peer)
{
    ZLOGI("pipeInfo:%{public}s peer.deviceId:%{public}s", pipeInfo.pipeId.c_str(),
        ToBeAnonymous(peer.deviceId).c_str());
    {
        lock_guard<mutex> lock(busSessionMutex_);
        if (busSessionMap_.find(pipeInfo.pipeId + peer.deviceId) != busSessionMap_.end()) {
            ZLOGI("Found session in map. Return true.");
            return true;
        }
    }
    SessionAttribute attr;
    attr.dataType = TYPE_BYTES;
    int sessionId = OpenSession(pipeInfo.pipeId.c_str(), pipeInfo.pipeId.c_str(), ToNodeID(peer.deviceId, "").c_str(),
        "GROUP_ID", &attr);
    ZLOGI("[IsSameStartedOnPeer] sessionId=%{public}d", sessionId);
    if (sessionId == INVALID_SESSION_ID) {
        ZLOGE("OpenSession return null, pipeInfo:%{public}s. Return false.", pipeInfo.pipeId.c_str());
        return false;
    }
    ZLOGI("session started, pipeInfo:%{public}s. sessionId:%{public}d Return true. ",
        pipeInfo.pipeId.c_str(), sessionId);
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

void SoftBusAdapter::InsertSession(const std::string &sessionName)
{
    lock_guard <mutex> lock(busSessionMutex_);
    busSessionMap_.insert({sessionName, true});
}

void SoftBusAdapter::DeleteSession(const std::string &sessionName)
{
    lock_guard<mutex> lock(busSessionMutex_);
    busSessionMap_.erase(sessionName);
}

void SoftBusAdapter::NotifyDataListeners(const uint8_t *ptr, int size, const std::string &deviceId,
                                         const PipeInfo &pipeInfo)
{
    ZLOGD("begin");
    lock_guard<mutex> lock(dataChangeMutex_);
    auto it = dataChangeListeners_.find(pipeInfo.pipeId);
    if (it != dataChangeListeners_.end()) {
        ZLOGD("ready to notify, pipeName:%{public}s, deviceId:%{public}s.",
            pipeInfo.pipeId.c_str(), ToBeAnonymous(deviceId).c_str());
        DeviceInfo deviceInfo = GetDeviceInfo(deviceId);
        it->second->OnMessage(deviceInfo, ptr, size, pipeInfo);
        TrafficStat ts { pipeInfo.pipeId, deviceId, 0, size };
        Reporter::GetInstance()->TrafficStatistic()->Report(ts);
        return;
    }
    ZLOGW("no listener %{public}s.", pipeInfo.pipeId.c_str());
}

void AppDataListenerWrap::SetDataHandler(SoftBusAdapter *handler)
{
    ZLOGI("begin");
    softBusAdapter_ = handler;
}

int AppDataListenerWrap::OnSessionOpened(int sessionId, int result)
{
    ZLOGI("[SessionOpen] sessionId:%{public}d, result:%{public}d", sessionId, result);
    char mySessionName[SESSION_NAME_SIZE_MAX] = "";
    char peerSessionName[SESSION_NAME_SIZE_MAX] = "";
    char peerDevId[DEVICE_ID_SIZE_MAX] = "";

    softBusAdapter_->OnSessionOpen(sessionId, result);
    if (result != SOFTBUS_OK) {
        ZLOGW("session %{public}d open failed, result:%{public}d.", sessionId, result);
        return result;
    }
    int ret = GetMySessionName(sessionId, mySessionName, sizeof(mySessionName));
    if (ret != SOFTBUS_OK) {
        ZLOGW("get my session name failed, session id is %{public}d.", sessionId);
        return SOFTBUS_ERR;
    }
    ret = GetPeerSessionName(sessionId, peerSessionName, sizeof(peerSessionName));
    if (ret != SOFTBUS_OK) {
        ZLOGW("get my peer session name failed, session id is %{public}d.", sessionId);
        return SOFTBUS_ERR;
    }
    ret = GetPeerDeviceId(sessionId, peerDevId, sizeof(peerDevId));
    if (ret != SOFTBUS_OK) {
        ZLOGW("get my peer device id failed, session id is %{public}d.", sessionId);
        return SOFTBUS_ERR;
    }
    std::string peerUuid = softBusAdapter_->GetUuidByNodeId(std::string(peerDevId));
    ZLOGD("[SessionOpen] mySessionName:%{public}s, peerSessionName:%{public}s, peerDevId:%{public}s",
        mySessionName, peerSessionName, SoftBusAdapter::ToBeAnonymous(peerUuid).c_str());

    if (strlen(peerSessionName) < 1) {
        softBusAdapter_->InsertSession(std::string(mySessionName) + peerUuid);
    } else {
        softBusAdapter_->InsertSession(std::string(peerSessionName) + peerUuid);
    }
    return 0;
}

void AppDataListenerWrap::OnSessionClosed(int sessionId)
{
    ZLOGI("[SessionClosed] sessionId:%{public}d", sessionId);
    char mySessionName[SESSION_NAME_SIZE_MAX] = "";
    char peerSessionName[SESSION_NAME_SIZE_MAX] = "";
    char peerDevId[DEVICE_ID_SIZE_MAX] = "";

    softBusAdapter_->OnSessionClose(sessionId);
    int ret = GetMySessionName(sessionId, mySessionName, sizeof(mySessionName));
    if (ret != SOFTBUS_OK) {
        ZLOGW("get my session name failed, session id is %{public}d.", sessionId);
        return;
    }
    ret = GetPeerSessionName(sessionId, peerSessionName, sizeof(peerSessionName));
    if (ret != SOFTBUS_OK) {
        ZLOGW("get my peer session name failed.");
        return;
    }
    ret = GetPeerDeviceId(sessionId, peerDevId, sizeof(peerDevId));
    if (ret != SOFTBUS_OK) {
        ZLOGW("get my peer device id failed, session id is %{public}d.", sessionId);
        return;
    }
    std::string peerUuid = softBusAdapter_->GetUuidByNodeId(std::string(peerDevId));
    ZLOGD("[SessionClosed] mySessionName:%{public}s, peerSessionName:%{public}s, peerDevId:%{public}s",
        mySessionName, peerSessionName, SoftBusAdapter::ToBeAnonymous(peerUuid).c_str());

    if (strlen(peerSessionName) < 1) {
        softBusAdapter_->DeleteSession(std::string(mySessionName) + peerUuid);
    } else {
        softBusAdapter_->DeleteSession(std::string(peerSessionName) + peerUuid);
    }
}

void AppDataListenerWrap::OnMessageReceived(int sessionId, const void *data, unsigned int dataLen)
{
    ZLOGI("begin");
    if (sessionId == INVALID_SESSION_ID) {
        return;
    }
    char peerSessionName[SESSION_NAME_SIZE_MAX] = "";
    char peerDevId[DEVICE_ID_SIZE_MAX] = "";
    int ret = GetPeerSessionName(sessionId, peerSessionName, sizeof(peerSessionName));
    if (ret != SOFTBUS_OK) {
        ZLOGW("get my peer session name failed, session id is %{public}d.", sessionId);
        return;
    }
    ret = GetPeerDeviceId(sessionId, peerDevId, sizeof(peerDevId));
    if (ret != SOFTBUS_OK) {
        ZLOGW("get my peer device id failed, session id is %{public}d.", sessionId);
        return;
    }
    std::string peerUuid = softBusAdapter_->GetUuidByNodeId(std::string(peerDevId));
    ZLOGD("[MessageReceived] peerSessionName:%{public}s, peerDevId:%{public}s", peerSessionName,
        SoftBusAdapter::ToBeAnonymous(peerUuid).c_str());
    NotifyDataListeners(reinterpret_cast<const uint8_t *>(data), dataLen, peerUuid, {std::string(peerSessionName), ""});
}

void AppDataListenerWrap::OnBytesReceived(int sessionId, const void *data, unsigned int dataLen)
{
    ZLOGI("begin");
    if (sessionId == INVALID_SESSION_ID) {
        return;
    }
    char peerSessionName[SESSION_NAME_SIZE_MAX] = "";
    char peerDevId[DEVICE_ID_SIZE_MAX] = "";
    int ret = GetPeerSessionName(sessionId, peerSessionName, sizeof(peerSessionName));
    if (ret != SOFTBUS_OK) {
        ZLOGW("get my peer session name failed, session id is %{public}d.", sessionId);
        return;
    }
    ret = GetPeerDeviceId(sessionId, peerDevId, sizeof(peerDevId));
    if (ret != SOFTBUS_OK) {
        ZLOGW("get my peer device id failed, session id is %{public}d.", sessionId);
        return;
    }
    std::string peerUuid = softBusAdapter_->GetUuidByNodeId(std::string(peerDevId));
    ZLOGD("[BytesReceived] peerSessionName:%{public}s, peerDevId:%{public}s", peerSessionName,
        SoftBusAdapter::ToBeAnonymous(peerUuid).c_str());
    NotifyDataListeners(reinterpret_cast<const uint8_t *>(data), dataLen, peerUuid, {std::string(peerSessionName), ""});
}

void AppDataListenerWrap::NotifyDataListeners(const uint8_t *ptr, const int size, const std::string &deviceId,
    const PipeInfo &pipeInfo)
{
    return softBusAdapter_->NotifyDataListeners(ptr, size, deviceId, pipeInfo);
}
}  // namespace AppDistributedKv
}  // namespace OHOS
