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
#include "dds_trace.h"
#include "dfx_types.h"
#include "kv_store_delegate_manager.h"
#include "log_print.h"
#include "process_communicator_impl.h"
#include "reporter.h"
#include "session.h"
#include "softbus_bus_center.h"
#include "softbus_def.h"
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
    // notifiy all listeners when received message
    static void NotifyDataListeners(const std::string &ptr, const int size, const std::string &deviceId,
                             const PipeInfo &pipeInfo);
    static SoftBusAdapter *softBusAdapter_;
};
SoftBusAdapter *AppDataListenerWrap::softBusAdapter_;
std::shared_ptr<SoftBusAdapter> SoftBusAdapter::instance_;

void AppDeviceListenerWrap::OnDeviceInfoChanged(NodeBasicInfoType type, NodeBasicInfo *info)
{
    ZLOGI("device change listener info change NodeBasicInfoType:%d, networkId:%s, deviceName:%s, deviceTypeId:%d.",
        type, info->networkId, info->deviceName, info->deviceTypeId);
}

void AppDeviceListenerWrap::OnDeviceOffline(NodeBasicInfo *info)
{
    ZLOGI("device change listener offline.");
    NotifyAll(info, DeviceChangeType::DEVICE_OFFLINE);
}

void AppDeviceListenerWrap::OnDeviceOnline(NodeBasicInfo *info)
{
    ZLOGI("device change listener online.");
    NotifyAll(info, DeviceChangeType::DEVICE_ONLINE);
}

void AppDeviceListenerWrap::SetDeviceHandler(SoftBusAdapter *handler)
{
    ZLOGI("SetDeviceHandler.");
    softBusAdapter_ = handler;
}

void AppDeviceListenerWrap::NotifyAll(NodeBasicInfo *info, DeviceChangeType type)
{
    DeviceInfo di = {std::string(info->networkId), std::string(info->deviceName), std::to_string(info->deviceTypeId)};
    softBusAdapter_->NotifyAll(di, type);
}

SoftBusAdapter::SoftBusAdapter()
{
    AppDeviceListenerWrap::SetDeviceHandler(this);
    AppDataListenerWrap::SetDataHandler(this);
}

SoftBusAdapter::~SoftBusAdapter()
{
    INodeStateCb iNodeStateCb;
    iNodeStateCb.events = EVENT_NODE_STATE_MASK;
    iNodeStateCb.onNodeOnline = AppDeviceListenerWrap::OnDeviceOnline;
    iNodeStateCb.onNodeOffline = AppDeviceListenerWrap::OnDeviceOffline;
    iNodeStateCb.onNodeBasicInfoChanged = AppDeviceListenerWrap::OnDeviceInfoChanged;
    int32_t errNo = UnregNodeDeviceStateCb(&iNodeStateCb);
    if (errNo != SOFTBUS_OK) {
        ZLOGE("UnregNodeDeviceStateCb fail %{public}d", errNo);
    }
}

void SoftBusAdapter::Init() const
{
    std::thread th = std::thread([&]() {
        auto communicator = std::make_shared<ProcessCommunicatorImpl>();
        auto retcom = DistributedDB::KvStoreDelegateManager::SetProcessCommunicator(communicator);
        ZLOGI("set communicator ret:%{public}d.", static_cast<int>(retcom));

        INodeStateCb iNodeStateCb;
        iNodeStateCb.events = EVENT_NODE_STATE_MASK;
        iNodeStateCb.onNodeOnline = AppDeviceListenerWrap::OnDeviceOnline;
        iNodeStateCb.onNodeOffline = AppDeviceListenerWrap::OnDeviceOffline;
        iNodeStateCb.onNodeBasicInfoChanged = AppDeviceListenerWrap::OnDeviceInfoChanged;

        int i = 0;
        constexpr int RETRY_TIMES = 300;
        while (i++ < RETRY_TIMES) {
            int32_t errNo = RegNodeDeviceStateCb("com.huawei.hwddmp", &iNodeStateCb);
            if (errNo != SOFTBUS_OK) {
                ZLOGE("RegNodeDeviceStateCb fail %{public}d, time:%{public}d", errNo, i);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            return;
        }
        ZLOGE("AppDeviceHandler Init failed %{public}d times and exit now.", RETRY_TIMES);
    });
    th.detach();
}

Status SoftBusAdapter::StartWatchDeviceChange(const AppDeviceStatusChangeListener *observer,
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

Status SoftBusAdapter::StopWatchDeviceChange(const AppDeviceStatusChangeListener *observer,
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
        std::lock_guard<std::mutex> lock(deviceChangeMutex_);
        ZLOGD("notify id:%{public}s", ToBeAnonymous(deviceInfo.deviceId).c_str());
        ZLOGD("high");
        std::string uuid = GetUuidByNodeId(deviceInfo.deviceId);
        UpdateRelationship(deviceInfo.deviceId, type);
        for (const auto &device : listeners_) {
            if (device == nullptr) {
                continue;
            }
            if (device->GetChangeLevelType() == ChangeLevelType::HIGH) {
                DeviceInfo di = {uuid, deviceInfo.deviceName, deviceInfo.deviceType};
                device->OnDeviceChanged(di, type);
                break;
            }
        }
        ZLOGD("low");
        for (const auto &device : listeners_) {
            if (device == nullptr) {
                continue;
            }
            if (device->GetChangeLevelType() == ChangeLevelType::LOW) {
                DeviceInfo di = {uuid, deviceInfo.deviceName, deviceInfo.deviceType};
                device->OnDeviceChanged(di, DeviceChangeType::DEVICE_OFFLINE);
                device->OnDeviceChanged(di, type);
            }
        }
        ZLOGD("min");
        for (const auto &device : listeners_) {
            if (device == nullptr) {
                continue;
            }
            if (device->GetChangeLevelType() == ChangeLevelType::MIN) {
                DeviceInfo di = {uuid, deviceInfo.deviceName, deviceInfo.deviceType};
                device->OnDeviceChanged(di, type);
            }
        }
    });
    th.detach();
}

std::vector<DeviceInfo> SoftBusAdapter::GetDeviceList() const
{
    std::vector<DeviceInfo> dis;
    NodeBasicInfo *info = nullptr;
    int32_t infoNum = 0;
    dis.clear();

    int32_t ret = GetAllNodeDeviceInfo("com.huawei.hwddmp", &info, &infoNum);
    if (ret != SOFTBUS_OK) {
        ZLOGE("GetAllNodeDeviceInfo error");
        return dis;
    }
    ZLOGD("GetAllNodeDeviceInfo success infoNum=%{public}d", infoNum);

    for (int i = 0; i < infoNum; i++) {
        dis.push_back({std::string(info[i].networkId), std::string(info[i].deviceName),
            std::to_string(info[i].deviceTypeId)});
    }
    return dis;
}

DeviceInfo SoftBusAdapter::GetLocalDevice()
{
    if (!localInfo_.deviceId.empty()) {
        return localInfo_;
    }

    NodeBasicInfo info;
    int32_t ret = GetLocalNodeDeviceInfo("com.huawei.hwddmp", &info);
    if (ret != SOFTBUS_OK) {
        ZLOGE("GetLocalNodeDeviceInfo error");
        return DeviceInfo();
    }
    ZLOGD("Udid:%{private}s, name:%{private}s, type:%{private}d",
        ToBeAnonymous(std::string(info.networkId)).c_str(), info.deviceName, info.deviceTypeId);
    localInfo_ = {std::string(info.networkId), std::string(info.deviceName), std::to_string(info.deviceTypeId)};
    return localInfo_;
}

std::string SoftBusAdapter::GetUuidByNodeId(const std::string &nodeId) const
{
    uint8_t info = 0;
    int32_t ret = GetNodeKeyInfo("com.huawei.hwddmp", nodeId.c_str(),
        NodeDeivceInfoKey::NODE_KEY_UUID, &info, sizeof(info));
    if (ret != SOFTBUS_OK) {
        ZLOGW("GetNodeKeyInfo error");
        return "";
    }
    ZLOGD("getUuid:%{public}s", nodeId.c_str());
    return std::to_string(info);
}

std::string SoftBusAdapter::GetUdidByNodeId(const std::string &nodeId) const
{
    uint8_t info = 0;
    int32_t ret = GetNodeKeyInfo("com.huawei.hwddmp", nodeId.c_str(),
        NodeDeivceInfoKey::NODE_KEY_UDID, &info, sizeof(info));
    if (ret != SOFTBUS_OK) {
        ZLOGW("GetNodeKeyInfo error");
        return "";
    }
    ZLOGD("getUdid:%{public}s", nodeId.c_str());
    return std::to_string(info);
}

DeviceInfo SoftBusAdapter::GetLocalBasicInfo() const
{
    ZLOGD("SoftBusAdapter::GetLocalBasicInfo begin");
    NodeBasicInfo info;
    int32_t ret = GetLocalNodeDeviceInfo("com.huawei.hwddmp", &info);
    if (ret != SOFTBUS_OK) {
        ZLOGE("GetLocalNodeDeviceInfo error");
        return DeviceInfo();
    }
    ZLOGD("Udid:%{private}s, name:%{private}s, type:%{private}d",
        ToBeAnonymous(std::string(info.networkId)).c_str(), info.deviceName, info.deviceTypeId);
    DeviceInfo localInfo = {std::string(info.networkId), std::string(info.deviceName),
        std::to_string(info.deviceTypeId)};
    return localInfo;
}

std::vector<DeviceInfo> SoftBusAdapter::GetRemoteNodesBasicInfo() const
{
    std::vector<DeviceInfo> ret;
    return ret;
}

void SoftBusAdapter::UpdateRelationship(const std::string &networkid, const DeviceChangeType &type)
{
    auto uuid = GetUuidByNodeId(networkid);
    auto udid = GetUdidByNodeId(networkid);
    switch (type) {
        case DeviceChangeType::DEVICE_OFFLINE: {
            auto size = this->networkId2UuidUdid_.erase(networkid);
            if (size == 0) {
                ZLOGW("not found id:%{public}s.", networkid.c_str());
            }
            break;
        }
        case DeviceChangeType::DEVICE_ONLINE: {
            std::pair<std::string, std::tuple<std::string, std::string>> value = {networkid, {uuid, udid}};
            auto res = this->networkId2UuidUdid_.insert(std::move(value));
            if (!res.second) {
                ZLOGW("insert failed.");
            }
            break;
        }
        default: {
            ZLOGW("unknown type.");
            break;
        }
    }
}

std::string SoftBusAdapter::ToUUID(const std::string& id) const
{
    auto res = networkId2UuidUdid_.find(id);
    if (res != networkId2UuidUdid_.end()) { // id is networkid
        return std::get<0>(res->second);
    }

    for (auto const &e : networkId2UuidUdid_) {
        auto tup = e.second;
        if (id == (std::get<0>(tup))) { // id is uuid
            return id;
        }
        if (id == (std::get<1>(tup))) { // id is udid
            return std::get<0>(tup);
        }
    }
    ZLOGW("unknown id.");
    return "";
}

std::string SoftBusAdapter::ToNodeID(const std::string& id, const std::string &nodeId) const
{
    for (auto const &e : networkId2UuidUdid_) {
        auto tup = e.second;
        if (nodeId == (std::get<0>(tup))) { // id is uuid
            return e.first;
        }
        if (id == (std::get<1>(tup))) { // id is udid
            return e.first;
        }
    }
    return "";
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
    int sessionId = OpenSession(pipeInfo.pipeId.c_str(), pipeInfo.pipeId.c_str(),
        deviceId.deviceId.c_str(), pipeInfo.userId.c_str(), &attr);
    if (sessionId == INVALID_SESSION_ID) {
        ZLOGW("create session %{public}s, %{public}d failed, session is null.", pipeInfo.pipeId.c_str(), info.msgType);
        return Status::CREATE_SESSION_ERROR;
    }

    ZLOGD("SendBytes start,size is %{public}d, session type is %{public}d.", size, attr.dataType);
    int32_t ret = SendBytes(sessionId, (void*)ptr, size);
    if (ret != SOFTBUS_OK) {
        ZLOGW("send data %{public}s failed, session is null.", pipeInfo.pipeId.c_str());
        return Status::ERROR;
    }
    return Status::SUCCESS;
}

bool SoftBusAdapter::IsSameStartedOnPeer(const struct PipeInfo &pipeInfo,
                                         __attribute__((unused))const struct DeviceId &peer)
{
    ZLOGI("pipeInfo:%{public}s", pipeInfo.pipeId.c_str());
    {
        lock_guard<mutex> lock(busSessionMutex_);
        if (busSessionMap_.find(pipeInfo.pipeId + peer.deviceId) != busSessionMap_.end()) {
            ZLOGI("Found session in map. Return true.");
            return true;
        }
    }
    SessionAttribute attr;
    attr.dataType = TYPE_BYTES;
    int sessionId = OpenSession(pipeInfo.pipeId.c_str(), pipeInfo.pipeId.c_str(), peer.deviceId.c_str(),
        pipeInfo.userId.c_str(), &attr);
    if (sessionId == INVALID_SESSION_ID) {
        ZLOGE("OpenSession return null, pipeInfo:%{public}s. Return false.", pipeInfo.pipeId.c_str());
        return false;
    }
    ZLOGI("session started, pipeInfo:%{public}s. Return true.", pipeInfo.pipeId.c_str());
    return true;
}

void SoftBusAdapter::SetMessageTransFlag(const PipeInfo &pipeInfo, bool flag)
{
    ZLOGI("SetMessageTransFlag pipeInfo: %s flag: %d", pipeInfo.pipeId.c_str(), static_cast<bool>(flag));
    flag_ = flag;
}

int SoftBusAdapter::CreateSessionServerAdapter(const std::string &sessionName) const
{
    ISessionListener sessionListener;
    sessionListener.OnSessionOpened = AppDataListenerWrap::OnSessionOpened;
    sessionListener.OnSessionClosed = AppDataListenerWrap::OnSessionClosed;
    sessionListener.OnBytesReceived = AppDataListenerWrap::OnBytesReceived;
    sessionListener.OnMessageReceived = AppDataListenerWrap::OnMessageReceived;
    int ret = CreateSessionServer("com.huawei.hwddmp", sessionName.c_str(), &sessionListener);
    if (ret != SOFTBUS_OK) {
        ZLOGW("Start pipeInfo:%{public}s, failed ret:%{public}d.", sessionName.c_str(), ret);
        return ret;
    }
    return SOFTBUS_OK;
}

int SoftBusAdapter::RemoveSessionServerAdapter(const std::string &sessionName) const
{
    return RemoveSessionServer("com.huawei.hwddmp", sessionName.c_str());
}

void SoftBusAdapter::InsertSession(const std::string sessionName)
{
    lock_guard<mutex> lock(busSessionMutex_);
    busSessionMap_.insert({sessionName, true});
}

void SoftBusAdapter::DeleteSession(const std::string sessionName)
{
    lock_guard<mutex> lock(busSessionMutex_);
    busSessionMap_.erase(sessionName);
}

void SoftBusAdapter::NotifyDataListeners(const std::string &ptr, const int size, const std::string &deviceId,
    const PipeInfo &pipeInfo)
{
    lock_guard<mutex> lock(dataChangeMutex_);
    auto it = dataChangeListeners_.find(pipeInfo.pipeId);
    if (it != dataChangeListeners_.end()) {
        ZLOGD("ready to notify, pipeName:%{public}s.", pipeInfo.pipeId.c_str());
        it->second->OnMessage({deviceId, "", ""}, (uint8_t *)ptr.c_str(), size, pipeInfo);
        TrafficStat ts { pipeInfo.pipeId, deviceId, 0, size };
        Reporter::GetInstance()->TrafficStatistic()->Report(ts);
        return;
    }
    ZLOGW("no listener %{public}s.", pipeInfo.pipeId.c_str());
}

void AppDataListenerWrap::SetDataHandler(SoftBusAdapter *handler)
{
    ZLOGI("SetDeviceHandler.");
    softBusAdapter_ = handler;
}

int AppDataListenerWrap::OnSessionOpened(int sessionId, int result)
{
    char mySessionName[SESSION_NAME_SIZE_MAX] = "";
    char peerSessionName[SESSION_NAME_SIZE_MAX] = "";
    char peerDevId[DEVICE_ID_SIZE_MAX] = "";

    if (result != SOFTBUS_OK) {
        ZLOGW("open session %{public}d failed, result id is %{public}d.", sessionId, result);
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
    ZLOGW("%{public}s, %{public}s", mySessionName, peerSessionName);

    if (strlen(peerSessionName) < 1) {
        softBusAdapter_->InsertSession(std::string(mySessionName) + std::string(peerDevId));
    } else {
        softBusAdapter_->InsertSession(std::string(peerSessionName) + std::string(peerDevId));
    }
    return 0;
}

void AppDataListenerWrap::OnSessionClosed(int sessionId)
{
    char mySessionName[SESSION_NAME_SIZE_MAX] = "";
    char peerSessionName[SESSION_NAME_SIZE_MAX] = "";
    char peerDevId[DEVICE_ID_SIZE_MAX] = "";

    int ret = GetMySessionName(sessionId, mySessionName, sizeof(mySessionName));
    if (ret != SOFTBUS_OK) {
        ZLOGW("get my session name failed, session id is %{public}d.", sessionId);
        return;
    }
    ret = GetPeerSessionName(sessionId, peerSessionName, sizeof(peerSessionName));
    if (ret != SOFTBUS_OK) {
        ZLOGW("get my peer session name failed, session id is %{public}d.", sessionId);
        return;
    }
    ret = GetPeerDeviceId(sessionId, peerDevId, sizeof(peerDevId));
    if (ret != SOFTBUS_OK) {
        ZLOGW("get my peer device id failed, session id is %{public}d.", sessionId);
        return;
    }
    ZLOGW("%{public}s, %{public}s", mySessionName, peerSessionName);

    if (strlen(peerSessionName) < 1) {
        softBusAdapter_->DeleteSession(std::string(mySessionName) + std::string(peerDevId));
    } else {
        softBusAdapter_->DeleteSession(std::string(peerSessionName) + std::string(peerDevId));
    }
}

void AppDataListenerWrap::OnMessageReceived(int sessionId, const void *data, unsigned int dataLen)
{
    if (sessionId == INVALID_SESSION_ID) {
        return;
    }

    char peerSessionName[SESSION_NAME_SIZE_MAX] = "";
    char peerDevId[DEVICE_ID_SIZE_MAX] = "";
    std::string dataStr = *static_cast<const std::string*>(data);
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
    NotifyDataListeners(dataStr, dataLen, std::string(peerDevId), {std::string(peerSessionName), ""});
}

void AppDataListenerWrap::OnBytesReceived(int sessionId, const void *data, unsigned int dataLen)
{
    if (sessionId == INVALID_SESSION_ID) {
        return;
    }

    char peerSessionName[SESSION_NAME_SIZE_MAX] = "";
    char peerDevId[DEVICE_ID_SIZE_MAX] = "";
    std::string dataStr = *static_cast<const std::string*>(data);
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
    NotifyDataListeners(dataStr, dataLen, std::string(peerDevId), {std::string(peerSessionName), ""});
}

void AppDataListenerWrap::NotifyDataListeners(const std::string &ptr, const int size, const string &deviceId,
    const PipeInfo &pipeInfo)
{
    return softBusAdapter_->NotifyDataListeners(ptr, size, deviceId, pipeInfo);
}
}  // namespace AppDistributedKv
}  // namespace OHOS