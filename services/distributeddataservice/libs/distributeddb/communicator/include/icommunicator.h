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

#ifndef ICOMMUNICATOR_H
#define ICOMMUNICATOR_H

#include <string>
#include <functional>
#include "message.h"
#include "ref_object.h"
#include "communicator_type_define.h"
#include "iprocess_communicator.h"
#include "db_properties.h"

namespace DistributedDB {
// inMsg is heap memory, its ownership transfers by calling OnMessageCallback
using OnMessageCallback = std::function<void(const std::string &srcTarget, Message *inMsg)>;
constexpr uint32_t SEND_TIME_OUT = 3000; // 3s

struct SendConfig {
    bool nonBlock = false;
    bool isNeedExtendHead = false;
    uint32_t timeout = SEND_TIME_OUT;
    ExtendInfo paramInfo;
};

inline void SetSendConfigParam(const DBProperties &dbProperty, const std::string &dstTarget, bool nonBlock,
    uint32_t timeout, SendConfig &sendConf)
{
    sendConf.nonBlock = nonBlock;
    sendConf.timeout = timeout;
    sendConf.isNeedExtendHead = dbProperty.GetBoolProp(DBProperties::SYNC_DUAL_TUPLE_MODE,
        false);
    sendConf.paramInfo.appId = dbProperty.GetStringProp(DBProperties::APP_ID, "");
    sendConf.paramInfo.userId = dbProperty.GetStringProp(DBProperties::USER_ID, "");
    sendConf.paramInfo.storeId = dbProperty.GetStringProp(DBProperties::STORE_ID, "");
    sendConf.paramInfo.dstTarget = dstTarget;
}

class ICommunicator : public virtual RefObject {
public:
    // Message heap memory
    // Return 0 as success. Return negative as error
    virtual int RegOnMessageCallback(const OnMessageCallback &onMessage, const Finalizer &inOper) = 0;
    virtual int RegOnConnectCallback(const OnConnectCallback &onConnect, const Finalizer &inOper) = 0;
    virtual int RegOnSendableCallback(const std::function<void(void)> &onSendable, const Finalizer &inOper) = 0;

    virtual void Activate() = 0;

    // return optimal allowed data size(Some header is taken into account and subtract)
    virtual uint32_t GetCommunicatorMtuSize() const = 0;
    virtual uint32_t GetCommunicatorMtuSize(const std::string &target) const = 0;

    // return timeout in range [5s, 60s]
    virtual uint32_t GetTimeout() const = 0;
    virtual uint32_t GetTimeout(const std::string &target) const = 0;

    virtual bool IsDeviceOnline(const std::string &device) const = 0;

    // Get local target name for identify self
    virtual int GetLocalIdentity(std::string &outTarget) const = 0;

    // Get the protocol version of remote target. Return -E_NOT_FOUND if no record.
    virtual int GetRemoteCommunicatorVersion(const std::string &target, uint16_t &outVersion) const = 0;

    // inMsg is heap memory, its ownership transfers by calling SendMessage
    // If send fail in SendMessage, nonBlock true will return, nonBlock false will block and retry
    // timeout is ignore if nonBlock true. OnSendEnd won't always be called such as when in finalize stage.
    // Return 0 as success. Return negative as error
    virtual int SendMessage(const std::string &dstTarget, const Message *inMsg, const SendConfig &config) = 0;
    virtual int SendMessage(const std::string &dstTarget, const Message *inMsg, const SendConfig &config,
        const OnSendEnd &onEnd) = 0; // HW Code Regulation do not allow to use default parameters on virtual function

    virtual ~ICommunicator() {};
};
} // namespace DistributedDB

#endif // ICOMMUNICATOR_H
