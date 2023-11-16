/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#define LOG_TAG "SoftBusClient"
#include "communicator_context.h"
#include "device_manager_adapter.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "softbus_error_code.h"
#include "softbus_client.h"

namespace OHOS::AppDistributedKv {
using namespace OHOS::DistributedKv;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using Context = DistributedData::CommunicatorContext;
SoftBusClient::SoftBusClient(const PipeInfo &pipeInfo, const DeviceId &deviceId,
    const std::function<int32_t(int32_t)> &getConnStatus)
    : pipe_(pipeInfo), device_(deviceId), getConnStatus_(getConnStatus)
{
    mtu_ = DEFAULT_MTU_SIZE;
}

SoftBusClient::~SoftBusClient()
{
    if (connId_ > 0) {
        CloseSession(connId_);
    }
}

bool SoftBusClient::operator==(int32_t connId) const
{
    return connId_ == connId;
}

bool SoftBusClient::operator==(const std::string &deviceId) const
{
    return device_.deviceId == deviceId;
}

void SoftBusClient::RestoreDefaultValue()
{
    connId_ = INVALID_CONNECT_ID;
    status_ = ConnectStatus::DISCONNECT;
    routeType_ = RouteType::INVALID_ROUTE_TYPE;
    strategy_ = Strategy::DEFAULT;
    mtu_ = DEFAULT_MTU_SIZE;
}

uint32_t SoftBusClient::GetMtuSize() const
{
    ZLOGD("get mtu size connId:%{public}d mtu:%{public}d", connId_, mtu_);
    return mtu_;
}

uint32_t SoftBusClient::GetTimeout() const
{
    uint32_t timeout = DEFAULT_TIMEOUT;
    switch (routeType_) {
        case RouteType::WIFI_STA:
            timeout = WIFI_TIMEOUT;
            break;
        case RouteType::WIFI_P2P:
            timeout = WIFI_TIMEOUT;
            break;
        case RouteType::BT_BR:
            timeout = BR_TIMEOUT;
            break;
        case RouteType::BT_BLE:
            timeout = BR_TIMEOUT;
            break;
        default:
            break;
    }
    return timeout;
}

Status SoftBusClient::Send(const DataInfo &dataInfo, uint32_t totalLength)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto result = OpenConnect(totalLength);
    if (result != Status::SUCCESS) {
        return result;
    }
    expireTime_ = std::chrono::steady_clock::now() + GetDelayTime(totalLength);
    ZLOGD("send data connId:%{public}d, data size:%{public}u, total length:%{public}u.",
        connId_, dataInfo.length, totalLength);
    int32_t ret = SendBytes(connId_, dataInfo.data, dataInfo.length);
    if (ret != SOFTBUS_OK) {
        ZLOGE("send data to connId%{public}d failed, ret:%{public}d.", connId_, ret);
        return Status::ERROR;
    }
    return Status::SUCCESS;
}

Status SoftBusClient::OpenConnect(uint32_t totalLength)
{
    strategy_ = CommunicationStrategy::GetInstance().GetStrategy(device_.deviceId);
    if (status_ == ConnectStatus::CONNECT_OK) {
        status_ = ConnectStatus::DISCONNECT;
        auto result = SwitchChannel(totalLength);
        if (result == Status::SUCCESS) {
            status_ = ConnectStatus::CONNECT_OK;
        }
        return result;
    }
    if (!sessionFlag_ && routeType_ == RouteType::INVALID_ROUTE_TYPE) {
        return CreateChannel(totalLength);
    }
    if (routeType_ != RouteType::INVALID_ROUTE_TYPE) {
        status_ = ConnectStatus::CONNECT_OK;
        return Status::SUCCESS;
    }
    return Status::RATE_LIMIT;
}

Status SoftBusClient::SwitchChannel(uint32_t totalLength)
{
    if (strategy_ == Strategy::BUTT) {
        return Status::NETWORK_ERROR;
    }

    if (strategy_ == Strategy::ON_LINE_SELECT_CHANNEL) {
        return Status::SUCCESS;
    }

    if (routeType_ == RouteType::WIFI_STA) {
        return Status::SUCCESS;
    }

    if (routeType_ == RouteType::BT_BLE || routeType_ == RouteType::BT_BR) {
        if (totalLength < P2P_SIZE_THRESHOLD) {
            return Status::SUCCESS;
        }

        ZLOGD("switch %{public}s,session:%{public}s,connId:%{public}d,routeType:%{public}d to wifi or p2p.",
            KvStoreUtils::ToBeAnonymous(device_.deviceId).c_str(), pipe_.pipeId.c_str(), connId_, routeType_);
        RestoreDefaultValue();
        return OpenSessionByAsync(GetSessionAttribute(true));
    }

    if (routeType_ == RouteType::WIFI_P2P) {
        if (totalLength > P2P_SIZE_THRESHOLD * SWITCH_DELAY_FACTOR) {
            return Status::SUCCESS;
        }

        ZLOGD("switch %{public}s,session:%{public}s,connId:%{public}d,routeType:%{public}d to ble or br.",
            KvStoreUtils::ToBeAnonymous(device_.deviceId).c_str(), pipe_.pipeId.c_str(), connId_, routeType_);
        RestoreDefaultValue();
        return OpenSessionByAsync(GetSessionAttribute(false));
    }

    return Status::NETWORK_ERROR;
}

Status SoftBusClient::CreateChannel(uint32_t totalLength)
{
    if (strategy_ == Strategy::BUTT) {
        return Status::NETWORK_ERROR;
    }
    if (strategy_ == Strategy::ON_LINE_SELECT_CHANNEL) {
        return OpenSessionByAsync(GetSessionAttribute(true));
    }
    if (totalLength < P2P_SIZE_THRESHOLD) {
        return OpenSessionByAsync(GetSessionAttribute(false));
    }
    return OpenSessionByAsync(GetSessionAttribute(true));
}

Status SoftBusClient::OpenSessionByAsync(SessionAttribute attr)
{
    auto openSessionTask = [attr, client = shared_from_this()]() {
        if (client == nullptr) {
            ZLOGE("OpenSessionByAsync client is nullptr.");
            return;
        }
        (void)client->Open(attr);
        client->sessionFlag_ = false;
    };
    Context::GetInstance().GetThreadPool()->Execute(openSessionTask);
    sessionFlag_ = true;
    return Status::RATE_LIMIT;
}

Status SoftBusClient::Open(SessionAttribute attr)
{
    int id = OpenSession(pipe_.pipeId.c_str(), pipe_.pipeId.c_str(),
                         DmAdapter::GetInstance().ToNetworkID(device_.deviceId).c_str(), "GROUP_ID", &attr);
    ZLOGI("open %{public}s,session:%{public}s,connId:%{public}d,linkNum:%{public}d,strategy:%{public}d",
          KvStoreUtils::ToBeAnonymous(device_.deviceId).c_str(), pipe_.pipeId.c_str(), id, attr.linkTypeNum, strategy_);
    if (id < 0) {
        ZLOGW("Open %{public}s, type:%{public}d failed, connId:%{public}d",
              pipe_.pipeId.c_str(), attr.dataType, id);
        return Status::NETWORK_ERROR;
    }

    connId_ = id;
    int state = getConnStatus_(connId_);
    ZLOGI("waited for notification, state:%{public}d connId:%{public}d", state, id);
    if (state != SOFTBUS_OK) {
        ZLOGE("open callback result error");
        return Status::NETWORK_ERROR;
    }

    int32_t routeType = RouteType::INVALID_ROUTE_TYPE;
    auto ret = GetSessionOption(connId_, SESSION_OPTION_LINK_TYPE, &routeType, sizeof(routeType));
    if (ret != SOFTBUS_OK) {
        ZLOGE("get routeType failed, session:%{public}s, connId:%{public}d", pipe_.pipeId.c_str(), connId_);
        return Status::NETWORK_ERROR;
    }
    routeType_ = routeType;

    UpdateMtuSize();
    ZLOGI("open %{public}s, session:%{public}s success, connId:%{public}d, routeType:%{public}d",
        KvStoreUtils::ToBeAnonymous(device_.deviceId).c_str(), pipe_.pipeId.c_str(), connId_, routeType_);
    return Status::SUCCESS;
}

SessionAttribute SoftBusClient::GetSessionAttribute(bool isP2p)
{
    SessionAttribute attr;
    attr.dataType = TYPE_BYTES;
    // If the dataType is BYTES, the default strategy is wifi_5G > wifi_2.4G > BR, without P2P;
    if (!isP2p) {
        return attr;
    }

    int index = 0;
    attr.linkType[index++] = LINK_TYPE_WIFI_WLAN_5G;
    attr.linkType[index++] = LINK_TYPE_WIFI_WLAN_2G;
    attr.linkType[index++] = LINK_TYPE_WIFI_P2P;
    attr.linkType[index++] = LINK_TYPE_BR;
    attr.linkTypeNum = index;
    return attr;
}

void SoftBusClient::UpdateMtuSize()
{
    uint32_t mtu = 0;
    auto result = GetSessionOption(connId_, SESSION_OPTION_MAX_SENDBYTES_SIZE, &mtu, sizeof(mtu));
    if (result != SOFTBUS_OK) {
        return;
    }
    mtu_ = mtu;
}

void SoftBusClient::AfterStrategyUpdate(Strategy strategy)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (strategy != strategy_ && connId_ > 0) {
        ZLOGI("close connId:%{public}d,strategy current:%{public}d, new:%{public}d", connId_, strategy_, strategy);
        CloseSession(connId_);
        RestoreDefaultValue();
    }
}

SoftBusClient::Time SoftBusClient::GetExpireTime() const
{
    return expireTime_;
}

int32_t SoftBusClient::GetConnId() const
{
    return connId_;
}

int32_t SoftBusClient::GetRoutType() const
{
    return routeType_;
}

void SoftBusClient::UpdateExpireTime()
{
    expireTime_ = std::chrono::steady_clock::now() + GetDelayTime(0);
}

SoftBusClient::Duration SoftBusClient::GetDelayTime(uint32_t dataLength)
{
    if (routeType_ == RouteType::WIFI_P2P) {
        return P2P_CLOSE_DELAY + std::chrono::microseconds(dataLength >> P2P_TRANSFER_PER_MICROSECOND);
    }
    return SESSION_CLOSE_DELAY;
}
}