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
#include "communicator_proxy.h"
#include "db_constant.h"
#include "db_common.h"
#include "db_dump_helper.h"
#include "log_print.h"

namespace DistributedDB {
CommunicatorProxy::CommunicatorProxy() : mainComm_(nullptr)
{
}

CommunicatorProxy::~CommunicatorProxy()
{
    if (mainComm_ != nullptr) {
        RefObject::DecObjRef(mainComm_);
    }
    mainComm_ = nullptr;

    std::lock_guard lock(devCommMapLock_);
    for (const auto &iter : devCommMap_) {
        RefObject::DecObjRef(devCommMap_[iter.first].second);
    }
    devCommMap_.clear();
}

int CommunicatorProxy::RegOnMessageCallback(const OnMessageCallback &onMessage, const Finalizer &inOper)
{
    if (mainComm_ != nullptr) {
        (void) mainComm_->RegOnMessageCallback(onMessage, inOper);
    }

    std::lock_guard lock(devCommMapLock_);
    for (const auto &iter : devCommMap_) {
        (void) devCommMap_[iter.first].second->RegOnMessageCallback(onMessage, inOper);
    }
    return E_OK;
}

int CommunicatorProxy::RegOnConnectCallback(const OnConnectCallback &onConnect, const Finalizer &inOper)
{
    if (mainComm_ != nullptr) {
        (void) mainComm_->RegOnConnectCallback(onConnect, inOper);
    }

    std::lock_guard lock(devCommMapLock_);
    for (const auto &iter : devCommMap_) {
        (void) devCommMap_[iter.first].second->RegOnConnectCallback(onConnect, inOper);
    }

    return E_OK;
}

int CommunicatorProxy::RegOnSendableCallback(const std::function<void(void)> &onSendable, const Finalizer &inOper)
{
    if (mainComm_ != nullptr) {
        (void) mainComm_->RegOnSendableCallback(onSendable, inOper);
    }

    std::lock_guard lock(devCommMapLock_);
    for (const auto &iter : devCommMap_) {
        (void) devCommMap_[iter.first].second->RegOnSendableCallback(onSendable, inOper);
    }

    return E_OK;
}

void CommunicatorProxy::Activate()
{
    if (mainComm_ != nullptr) {
        mainComm_->Activate();
    }

    // use temp map to avoid active in lock
    std::map<std::string, ICommunicator *> tempMap;
    {
        std::lock_guard lock(devCommMapLock_);
        for (const auto &iter : devCommMap_) {
            tempMap[iter.first] = devCommMap_[iter.first].second;
            RefObject::IncObjRef(devCommMap_[iter.first].second);
        }
    }

    for (const auto &iter : tempMap) {
        tempMap[iter.first]->Activate();
        RefObject::DecObjRef(tempMap[iter.first]);
    }
}

uint32_t CommunicatorProxy::GetCommunicatorMtuSize() const
{
    if (mainComm_ == nullptr) {
        return DBConstant::MIN_MTU_SIZE;
    }
    return mainComm_->GetCommunicatorMtuSize();
}

uint32_t CommunicatorProxy::GetCommunicatorMtuSize(const std::string &target) const
{
    ICommunicator *targetCommunicator = nullptr;
    {
        std::lock_guard<std::mutex> lock(devCommMapLock_);
        if (devCommMap_.count(target) != 0) {
            targetCommunicator = devCommMap_.at(target).second;
            RefObject::IncObjRef(targetCommunicator);
        }
    }
    if (targetCommunicator != nullptr) {
        uint32_t mtuSize = targetCommunicator->GetCommunicatorMtuSize(target);
        RefObject::DecObjRef(targetCommunicator);
        return mtuSize;
    }

    if (mainComm_ != nullptr) {
        return mainComm_->GetCommunicatorMtuSize(target);
    }

    return DBConstant::MIN_MTU_SIZE;
}

uint32_t CommunicatorProxy::GetTimeout() const
{
    if (mainComm_ == nullptr) {
        return DBConstant::MIN_TIMEOUT;
    }
    return mainComm_->GetTimeout();
}

uint32_t CommunicatorProxy::GetTimeout(const std::string &target) const
{
    ICommunicator *targetCommunicator = nullptr;
    {
        std::lock_guard<std::mutex> lock(devCommMapLock_);
        if (devCommMap_.count(target) != 0) {
            targetCommunicator = devCommMap_.at(target).second;
            RefObject::IncObjRef(targetCommunicator);
        }
    }
    if (targetCommunicator != nullptr) {
        uint32_t timeout = targetCommunicator->GetTimeout(target);
        RefObject::DecObjRef(targetCommunicator);
        return timeout;
    }

    if (mainComm_ != nullptr) {
        return mainComm_->GetTimeout(target);
    }

    return DBConstant::MIN_TIMEOUT;
}

bool CommunicatorProxy::IsDeviceOnline(const std::string &device) const
{
    return mainComm_->IsDeviceOnline(device);
}

int CommunicatorProxy::GetLocalIdentity(std::string &outTarget) const
{
    return mainComm_->GetLocalIdentity(outTarget);
}

int CommunicatorProxy::GetRemoteCommunicatorVersion(const std::string &target, uint16_t &outVersion) const
{
    ICommunicator *targetCommunicator = nullptr;
    {
        std::lock_guard<std::mutex> lock(devCommMapLock_);
        if (devCommMap_.count(target) != 0) {
            targetCommunicator = devCommMap_.at(target).second;
            RefObject::IncObjRef(targetCommunicator);
        }
    }
    if (targetCommunicator != nullptr) {
        int errCode = targetCommunicator->GetRemoteCommunicatorVersion(target, outVersion);
        RefObject::DecObjRef(targetCommunicator);
        return errCode;
    }

    if (mainComm_ != nullptr) {
        return mainComm_->GetRemoteCommunicatorVersion(target, outVersion);
    }

    return -E_NOT_INIT;
}

int CommunicatorProxy::SendMessage(const std::string &dstTarget, const Message *inMsg, const SendConfig &config)
{
    return SendMessage(dstTarget, inMsg, config, nullptr);
}

int CommunicatorProxy::SendMessage(const std::string &dstTarget, const Message *inMsg, const SendConfig &config,
    const OnSendEnd &onEnd)
{
    ICommunicator *targetCommunicator = nullptr;
    {
        std::lock_guard<std::mutex> lock(devCommMapLock_);
        if (devCommMap_.count(dstTarget) != 0) {
            targetCommunicator = devCommMap_[dstTarget].second;
            RefObject::IncObjRef(targetCommunicator);
        }
    }
    if (targetCommunicator != nullptr) {
        LOGD("[CommProxy] use equal label to send data");
        int errCode = targetCommunicator->SendMessage(dstTarget, inMsg, config, onEnd);
        RefObject::DecObjRef(targetCommunicator);
        return errCode;
    }

    if (mainComm_ != nullptr) {
        return mainComm_->SendMessage(dstTarget, inMsg, config, onEnd);
    }

    return -E_NOT_INIT;
}

void CommunicatorProxy::SetMainCommunicator(ICommunicator *communicator)
{
    mainComm_ = communicator;
    RefObject::IncObjRef(mainComm_);
}

void CommunicatorProxy::SetEqualCommunicator(ICommunicator *communicator, const std::string &identifier,
    const std::vector<std::string> &targets)
{
    std::lock_guard<std::mutex> lock(devCommMapLock_);
    // Clear offline target
    for (auto dev = devCommMap_.begin(); dev != devCommMap_.end();) {
        if (identifier != dev->second.first) {
            dev++;
            continue;
        }
        auto iter = std::find_if(targets.begin(), targets.end(),
            [&dev](const std::string &target) {
                return target == dev->first;
            });
        if (iter == targets.end()) {
            RefObject::DecObjRef(devCommMap_[dev->first].second);
            dev = devCommMap_.erase(dev);
            continue;
        }
        dev++;
    }

    // Add new online target
    for (const auto &target : targets) {
        if (devCommMap_.count(target) != 0) {
            // change the identifier and dev relation
            RefObject::DecObjRef(devCommMap_[target].second);
        }
        RefObject::IncObjRef(communicator);
        devCommMap_[target] = {identifier, communicator};
    }
}

void CommunicatorProxy::Dump(int fd)
{
    std::lock_guard<std::mutex> lock(devCommMapLock_);
    for (const auto &[target, communicator] : devCommMap_) {
        std::string label = DBCommon::TransferStringToHex(communicator.first);
        DBDumpHelper::Dump(fd, "\t\ttarget = %s, label = %s\n", target.c_str(), label.c_str());
    }
}
} // namespace DistributedDB