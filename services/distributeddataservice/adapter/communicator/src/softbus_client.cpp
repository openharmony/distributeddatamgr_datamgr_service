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
#include "device_manager_adapter.h"
#include "kvstore_utils.h"
#include "log_print.h"
#include "softbus_error_code.h"
#include "softbus_client.h"

namespace OHOS::AppDistributedKv {
using namespace OHOS::DistributedKv;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
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

bool SoftBusClient::operator==(int32_t connId)
{
    return connId_ == connId;
}

bool SoftBusClient::operator==(const std::string &deviceId)
{
    return device_.deviceId == deviceId;
}

void SoftBusClient::RestoreDefaultValue()
{
    connId_ = INVALID_CONNECT_ID;
    status_ = ConnectStatus::DISCONNECT;
    strategy_ = Strategy::DEFAULT;
    mtu_ = DEFAULT_MTU_SIZE;
}

uint32_t SoftBusClient::GetMtuSize() const
{
    ZLOGD("get mtu size connId:%{public}d mtu:%{public}d", connId_, mtu_);
    return mtu_;
}

Status SoftBusClient::Send(const uint8_t *data, int size)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto result = OpenConnect();
    if (result != Status::SUCCESS) {
        return result;
    }

    ZLOGD("send data connId:%{public}d, data size:%{public}d.", connId_, size);
    int32_t ret = SendBytes(connId_, data, size);
    if (ret != SOFTBUS_OK) {
        ZLOGE("send data to connId%{public}d failed, ret:%{public}d.", connId_, ret);
        return Status::ERROR;
    }

    return Status::SUCCESS;
}

Status SoftBusClient::OpenConnect()
{
    if (status_ == ConnectStatus::CONNECT_OK) {
        return Status::SUCCESS;
    }

    auto result = Open();
    if (result != Status::SUCCESS) {
        return result;
    }
    status_ = ConnectStatus::CONNECT_OK;
    UpdateMtuSize();
    return Status::SUCCESS;
}

Status SoftBusClient::Open()
{
    Strategy strategy = CommunicationStrategy::GetInstance().GetStrategy(device_.deviceId);
    SessionAttribute attr = { 0 };
    InitSessionAttribute(strategy, attr);
    int id = OpenSession(pipe_.pipeId.c_str(), pipe_.pipeId.c_str(),
                         DmAdapter::GetInstance().ToNetworkID(device_.deviceId).c_str(), "GROUP_ID", &attr);
    ZLOGI("open %{public}s,session:%{public}s,connId:%{public}d,linkNum:%{public}d,strategy:%{public}d",
          KvStoreUtils::ToBeAnonymous(device_.deviceId).c_str(), pipe_.pipeId.c_str(), id, attr.linkTypeNum, strategy);
    if (id < 0) {
        ZLOGW("Open %{public}s, type:%{public}d failed, connId:%{public}d",
              pipe_.pipeId.c_str(), attr.dataType, id);
        return Status::NETWORK_ERROR;
    }

    connId_ = id;
    strategy_ = strategy;
    int state = getConnStatus_(connId_);
    ZLOGI("waited for notification, state:%{public}d connId:%{public}d", state, id);
    if (state != SOFTBUS_OK) {
        ZLOGE("open callback result error");
        return Status::NETWORK_ERROR;
    }
    return Status::SUCCESS;
}

void SoftBusClient::InitSessionAttribute(Strategy strategy, SessionAttribute &attr)
{
    attr.dataType = TYPE_BYTES;
    // If the dataType is BYTES, the default strategy is wifi_5G > wifi_2.4G > BR, without P2P;
    if (strategy == Strategy::DEFAULT) {
        return;
    }

    int index = 0;
    attr.linkType[index++] = LINK_TYPE_WIFI_WLAN_5G;
    attr.linkType[index++] = LINK_TYPE_WIFI_WLAN_2G;
    attr.linkType[index++] = LINK_TYPE_WIFI_P2P;
    attr.linkType[index++] = LINK_TYPE_BR;
    attr.linkTypeNum = index;
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
}