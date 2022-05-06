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

#ifndef MOCK_COMMUNICATOR_H
#define MOCK_COMMUNICATOR_H

#include <gmock/gmock.h>
#include "icommunicator.h"

namespace DistributedDB {
class MockCommunicator : public ICommunicator {
public:
    MOCK_CONST_METHOD1(GetLocalIdentity, int(std::string &));
    MOCK_CONST_METHOD2(GetRemoteCommunicatorVersion, int(const std::string &, uint16_t &));
    MOCK_METHOD3(SendMessage, int(const std::string &, const Message *, const SendConfig &));
    MOCK_METHOD4(SendMessage, int(const std::string &, const Message *, const SendConfig &, const OnSendEnd &));
    MOCK_CONST_METHOD0(GetCommunicatorMtuSize, uint32_t(void));
    MOCK_CONST_METHOD1(GetCommunicatorMtuSize, uint32_t(const std::string &));
    MOCK_CONST_METHOD0(GetTimeout, uint32_t(void));
    MOCK_CONST_METHOD1(GetTimeout, uint32_t(const std::string &));
    MOCK_METHOD2(RegOnConnectCallback, int(const OnConnectCallback &, const Finalizer &));
    MOCK_METHOD2(RegOnSendableCallback, int(const std::function<void(void)> &, const Finalizer &));
    MOCK_METHOD2(RegOnMessageCallback, int(const OnMessageCallback &, const Finalizer &));
    MOCK_METHOD0(Activate, void(void));
    MOCK_CONST_METHOD1(IsDeviceOnline, bool(const std::string &));
};
} // namespace DistributedDB
#endif  // #define MOCK_COMMUNICATOR_H

