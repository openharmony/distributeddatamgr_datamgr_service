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

#define LOG_TAG "processCommunication"

#include "communicator_context.h"
#include "device_manager_adapter.h"
#include "log_print.h"
#include "process_communicator_impl.h"
#include "utils/anonymous.h"
namespace OHOS {
namespace AppDistributedKv {
using namespace DistributedDB;
using namespace OHOS::DistributedData;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
ProcessCommunicatorImpl::ProcessCommunicatorImpl()
{
}

ProcessCommunicatorImpl::~ProcessCommunicatorImpl()
{
    ZLOGE("destructor.");
}

ProcessCommunicatorImpl *ProcessCommunicatorImpl::GetInstance()
{
    static ProcessCommunicatorImpl *processCommunicatorImpl = new ProcessCommunicatorImpl();
    return processCommunicatorImpl;
}

void ProcessCommunicatorImpl::SetRouteHeadHandlerCreator(RouteHeadHandlerCreator handlerCreator)
{
    return;
}

DBStatus ProcessCommunicatorImpl::Start(const std::string &processLabel)
{
    return DBStatus::OK;
}

DBStatus ProcessCommunicatorImpl::Stop()
{
    return DBStatus::OK;
}

DBStatus ProcessCommunicatorImpl::RegOnDeviceChange(const OnDeviceChange &callback)
{
    return DBStatus::OK;
}

DBStatus ProcessCommunicatorImpl::RegOnDataReceive(const OnDataReceive &callback)
{
    return DBStatus::OK;
}

void ProcessCommunicatorImpl::RegOnSendAble(const OnSendAble &sendAbleCallback)
{
    return;
}

DBStatus ProcessCommunicatorImpl::SendData(const DeviceInfos &dstDevInfo, const uint8_t *data, uint32_t length)
{
    return DBStatus::DB_ERROR;
}

DBStatus ProcessCommunicatorImpl::SendData(const DeviceInfos &dstDevInfo, const uint8_t *data, uint32_t length,
    uint32_t totalLength)
{
    return DBStatus::DB_ERROR;
}

uint32_t ProcessCommunicatorImpl::GetMtuSize()
{
    return MTU_SIZE;
}

uint32_t ProcessCommunicatorImpl::GetMtuSize(const DeviceInfos &devInfo)
{
    return 0;
}

uint32_t ProcessCommunicatorImpl::GetTimeout(const DeviceInfos &devInfo)
{
    return 0;
}

DeviceInfos ProcessCommunicatorImpl::GetLocalDeviceInfos()
{
    return {};
}

std::vector<DeviceInfos> ProcessCommunicatorImpl::GetRemoteOnlineDeviceInfosList()
{
    return {};
}

bool ProcessCommunicatorImpl::IsSameProcessLabelStartedOnPeerDevice(const DeviceInfos &peerDevInfo)
{
    return true;
}

void ProcessCommunicatorImpl::OnMessage(const DeviceInfo &info, const uint8_t *ptr, const int size,
                                        __attribute__((unused)) const PipeInfo &pipeInfo) const
{
    return;
}

void ProcessCommunicatorImpl::OnDeviceChanged(const DeviceInfo &info, const DeviceChangeType &type) const
{
    return;
}

void ProcessCommunicatorImpl::OnSessionReady(const DeviceInfo &info, int32_t errCode) const
{
    return;
}

std::shared_ptr<ExtendHeaderHandle> ProcessCommunicatorImpl::GetExtendHeaderHandle(const ExtendInfo &info)
{
    return {};
}

DBStatus ProcessCommunicatorImpl::GetDataHeadInfo(DataHeadInfo dataHeadInfo, uint32_t &headLength)
{
    return DBStatus::OK;
}

DBStatus ProcessCommunicatorImpl::GetDataUserInfo(DataUserInfo dataUserInfo, std::vector<UserInfo> &userInfos)
{
    return DBStatus::OK;
}

Status ProcessCommunicatorImpl::ReuseConnect(const DeviceId &deviceId, const ExtraDataInfo &extraInfo)
{
    return Status::ERROR;
}
} // namespace AppDistributedKv
} // namespace OHOS