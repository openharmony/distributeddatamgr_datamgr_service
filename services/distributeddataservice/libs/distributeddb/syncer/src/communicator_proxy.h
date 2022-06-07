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

#ifndef COMMUNICATOR_PROXY_H
#define COMMUNICATOR_PROXY_H

#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include "icommunicator.h"
#include "message.h"

namespace DistributedDB {
class CommunicatorProxy : public ICommunicator {
public:
    CommunicatorProxy();
    ~CommunicatorProxy();

    int RegOnMessageCallback(const OnMessageCallback &onMessage, const Finalizer &inOper) override;
    int RegOnConnectCallback(const OnConnectCallback &onConnect, const Finalizer &inOper) override;
    int RegOnSendableCallback(const std::function<void(void)> &onSendable, const Finalizer &inOper) override;
    void Activate() override;
    uint32_t GetCommunicatorMtuSize() const override;
    uint32_t GetCommunicatorMtuSize(const std::string &target) const override;
    uint32_t GetTimeout() const override;
    uint32_t GetTimeout(const std::string &target) const override;
    bool IsDeviceOnline(const std::string &device) const override;
    int GetLocalIdentity(std::string &outTarget) const override;
    int GetRemoteCommunicatorVersion(const std::string &target, uint16_t &outVersion) const override;
    int SendMessage(const std::string &dstTarget, const Message *inMsg, const SendConfig &config) override;
    int SendMessage(const std::string &dstTarget, const Message *inMsg, const SendConfig &config,
        const OnSendEnd &onEnd) override;

    // Set an Main communicator for this database, used userid & appId & storeId
    void SetMainCommunicator(ICommunicator *communicator);

    // Set an equal communicator for this database, After this called, send msg to the target will use this communicator
    void SetEqualCommunicator(ICommunicator *communicator, const std::string &identifier,
        const std::vector<std::string> &targets);

    void Dump(int fd);

private:
    ICommunicator *mainComm_;
    mutable std::mutex devCommMapLock_;
    // key: device value: <identifier, ICommunicator *>
    std::map<std::string, std::pair<std::string, ICommunicator *>> devCommMap_;
};
} // namespace DistributedDB
#endif // COMMUNICATOR_PROXY_H
