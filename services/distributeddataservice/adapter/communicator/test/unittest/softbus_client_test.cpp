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

#include "softbus_client.h"

#include <gtest/gtest.h>

#include "app_device_change_listener.h"
#include "communicator_context.h"
#include "executor_pool.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::AppDistributedKv;
using namespace OHOS::DistributedData;

static constexpr size_t THREAD_MIN = 0;
static constexpr size_t THREAD_MAX = 1;
static constexpr const char* PIP_ID = "SoftBusClientTest";
static constexpr const char* UUID = "uuid";
static constexpr const char* NETWORK_ID = "network_id";
static constexpr const char* INVALID_NETWORK_ID = "invalid_network_id";

class DeviceChangeListenerTest : public AppDeviceChangeListener {
public:
    void OnDeviceChanged(const DeviceInfo &info, const DeviceChangeType &type) const override {}
    void OnSessionReady(const DeviceInfo &info, int32_t errCode) const override;
    void ResetReadyFlag();

    mutable int32_t bindResult_;
    mutable bool isReady_ = false;
};

void DeviceChangeListenerTest::OnSessionReady(const DeviceInfo &info, int32_t errCode) const
{
    (void)info;
    bindResult_ = errCode;
    isReady_ = true;
}

void DeviceChangeListenerTest::ResetReadyFlag()
{
    isReady_ = false;
}

class SoftBusClientTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown() {}

    static PipeInfo pipeInfo_;
    static DeviceId deviceId_;
    static SessionAccessInfo accessInfo_;
    static DeviceChangeListenerTest deviceListener_;
    static std::shared_ptr<ExecutorPool> executors_;
};

PipeInfo SoftBusClientTest::pipeInfo_;
DeviceId SoftBusClientTest::deviceId_;
SessionAccessInfo SoftBusClientTest::accessInfo_;
DeviceChangeListenerTest SoftBusClientTest::deviceListener_;
std::shared_ptr<ExecutorPool> SoftBusClientTest::executors_;

void SoftBusClientTest::SetUpTestCase(void)
{
    pipeInfo_.pipeId = PIP_ID;
    deviceId_.deviceId = UUID;
    CommunicatorContext::GetInstance().RegSessionListener(&deviceListener_);
    executors_ = std::make_shared<ExecutorPool>(THREAD_MAX, THREAD_MIN);
}

void SoftBusClientTest::TearDownTestCase(void)
{
    CommunicatorContext::GetInstance().UnRegSessionListener(&deviceListener_);
}

void SoftBusClientTest::SetUp()
{
    CommunicatorContext::GetInstance().SetThreadPool(executors_);
}

/**
* @tc.name: OperatorTest001
* @tc.desc: operator test
* @tc.type: FUNC
*/
HWTEST_F(SoftBusClientTest, OperatorTest001, TestSize.Level1)
{
    auto connect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, NETWORK_ID, SoftBusClient::QOS_HML,
        accessInfo_);
    ASSERT_TRUE(*connect == UUID);
    ASSERT_TRUE(*connect == DEFAULT_SOCKET);
    ASSERT_EQ(connect->GetSocket(), DEFAULT_SOCKET);
}

/**
* @tc.name: NetworkIdTest001
* @tc.desc: network id test
* @tc.type: FUNC
*/
HWTEST_F(SoftBusClientTest, NetworkIdTest001, TestSize.Level1)
{
    auto connect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, INVALID_NETWORK_ID, SoftBusClient::QOS_HML,
        accessInfo_);
    auto networkId = connect->GetNetworkId();
    ASSERT_EQ(networkId, INVALID_NETWORK_ID);
    connect->UpdateNetworkId(NETWORK_ID);
    networkId = connect->GetNetworkId();
    ASSERT_EQ(networkId, NETWORK_ID);
}

/**
* @tc.name: OpenConnectTest001
* @tc.desc: open connect fail and socketId is invalid
* @tc.type: FUNC
*/
HWTEST_F(SoftBusClientTest, OpenConnectTest001, TestSize.Level1)
{
    ConfigSocketId(INVALID_SOCKET);
    accessInfo_.isOHType = true;
    auto connect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, NETWORK_ID, SoftBusClient::QOS_HML,
        accessInfo_);
    auto status = connect->OpenConnect(nullptr);
    ASSERT_EQ(status, Status::NETWORK_ERROR);
}

/**
* @tc.name: OpenConnectTest002
* @tc.desc: open connect fail and socket is valid but set access info fail
* @tc.type: FUNC
*/
HWTEST_F(SoftBusClientTest, OpenConnectTest002, TestSize.Level1)
{
    ConfigSocketId(INVALID_SET_SOCKET);
    accessInfo_.isOHType = true;
    auto connect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, NETWORK_ID, SoftBusClient::QOS_HML,
        accessInfo_);
    auto status = connect->OpenConnect(nullptr);
    ASSERT_EQ(status, Status::NETWORK_ERROR);
}

/**
* @tc.name: OpenConnectTest003
* @tc.desc: open connect fail and executor pool is nullptr
* @tc.type: FUNC
*/
HWTEST_F(SoftBusClientTest, OpenConnectTest003, TestSize.Level1)
{
    ConfigSocketId(INVALID_BIND_SOCKET);
    CommunicatorContext::GetInstance().SetThreadPool(nullptr);
    accessInfo_.isOHType = true;
    auto connect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, NETWORK_ID, SoftBusClient::QOS_HML,
        accessInfo_);
    auto status = connect->OpenConnect(nullptr);
    ASSERT_EQ(status, Status::NETWORK_ERROR);
}

/**
* @tc.name: OpenConnectTest004
* @tc.desc: open connect fail and socket is valid but bind fail
* @tc.type: FUNC
*/
HWTEST_F(SoftBusClientTest, OpenConnectTest004, TestSize.Level1)
{
    ConfigSocketId(INVALID_BIND_SOCKET);

    accessInfo_.isOHType = false;
    auto connect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, NETWORK_ID, SoftBusClient::QOS_BR,
        accessInfo_);
    auto status = connect->OpenConnect(nullptr);
    ASSERT_EQ(status, Status::RATE_LIMIT);
    while (!(deviceListener_.isReady_)) {
        sleep(1);
    }
    deviceListener_.ResetReadyFlag();
    auto result = deviceListener_.bindResult_;
    ASSERT_NE(result, 0);

    accessInfo_.isOHType = true;
    auto ohConnect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, NETWORK_ID, SoftBusClient::QOS_HML,
        accessInfo_);
    status = ohConnect->OpenConnect(nullptr);
    ASSERT_EQ(status, Status::RATE_LIMIT);
    while (!(deviceListener_.isReady_)) {
        sleep(1);
    }
    deviceListener_.ResetReadyFlag();
    result = deviceListener_.bindResult_;
    ASSERT_NE(result, 0);
}

/**
* @tc.name: OpenConnectTest005
* @tc.desc: open connect fail and bind success but get mtu fail
* @tc.type: FUNC
*/
HWTEST_F(SoftBusClientTest, OpenConnectTest005, TestSize.Level1)
{
    ConfigSocketId(INVALID_MTU_SOCKET);
    accessInfo_.isOHType = true;
    auto connect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, NETWORK_ID, SoftBusClient::QOS_HML,
        accessInfo_);
    auto status = connect->OpenConnect(nullptr);
    ASSERT_EQ(status, Status::RATE_LIMIT);
    while (!(deviceListener_.isReady_)) {
        sleep(1);
    }
    deviceListener_.ResetReadyFlag();
    auto result = deviceListener_.bindResult_;
    ASSERT_NE(result, 0);
}

/**
* @tc.name: OpenConnectTest006
* @tc.desc: open connect success
* @tc.type: FUNC
*/
HWTEST_F(SoftBusClientTest, OpenConnectTest006, TestSize.Level1)
{
    ConfigSocketId(VALID_SOCKET);
    accessInfo_.isOHType = true;
    auto connect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, NETWORK_ID, SoftBusClient::QOS_HML,
        accessInfo_);
    auto status = connect->OpenConnect(nullptr);
    ASSERT_EQ(status, Status::RATE_LIMIT);
    while (!(deviceListener_.isReady_)) {
        sleep(1);
    }
    deviceListener_.ResetReadyFlag();
    auto result = deviceListener_.bindResult_;
    ASSERT_EQ(result, 0);
    status = connect->CheckStatus();
    ASSERT_EQ(status, Status::SUCCESS);

    status = connect->OpenConnect(nullptr);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: ReuseConnectTest001
* @tc.desc: reuse connect fail and socketId is invalid
* @tc.type: FUNC
*/
HWTEST_F(SoftBusClientTest, ReuseConnectTest001, TestSize.Level1)
{
    ConfigSocketId(INVALID_SOCKET);
    accessInfo_.isOHType = true;
    auto connect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, NETWORK_ID, SoftBusClient::QOS_REUSE,
        accessInfo_);
    auto status = connect->ReuseConnect(nullptr);
    ASSERT_EQ(status, Status::NETWORK_ERROR);
}

/**
* @tc.name: ReuseConnectTest002
* @tc.desc: reuse connect fail and socket is valid but set access info fail
* @tc.type: FUNC
*/
HWTEST_F(SoftBusClientTest, ReuseConnectTest002, TestSize.Level1)
{
    ConfigSocketId(INVALID_SET_SOCKET);
    accessInfo_.isOHType = true;
    auto connect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, NETWORK_ID, SoftBusClient::QOS_REUSE,
        accessInfo_);
    auto status = connect->ReuseConnect(nullptr);
    ASSERT_EQ(status, Status::NETWORK_ERROR);
}

/**
* @tc.name: ReuseConnectTest003
* @tc.desc: reuse connect fail and socket is valid but bind fail
* @tc.type: FUNC
*/
HWTEST_F(SoftBusClientTest, ReuseConnectTest003, TestSize.Level1)
{
    ConfigSocketId(INVALID_BIND_SOCKET);
    accessInfo_.isOHType = true;
    auto connect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, NETWORK_ID, SoftBusClient::QOS_REUSE,
        accessInfo_);
    auto status = connect->ReuseConnect(nullptr);
    ASSERT_EQ(status, Status::NETWORK_ERROR);
}

/**
* @tc.name: ReuseConnectTest004
* @tc.desc: reuse connect fail and bind success but get mtu fail
* @tc.type: FUNC
*/
HWTEST_F(SoftBusClientTest, ReuseConnectTest004, TestSize.Level1)
{
    ConfigSocketId(INVALID_MTU_SOCKET);
    accessInfo_.isOHType = true;
    auto connect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, NETWORK_ID, SoftBusClient::QOS_REUSE,
        accessInfo_);
    auto status = connect->ReuseConnect(nullptr);
    ASSERT_EQ(status, Status::NETWORK_ERROR);
}

/**
* @tc.name: ReuseConnectTest005
* @tc.desc: reuse connect success
* @tc.type: FUNC
*/
HWTEST_F(SoftBusClientTest, ReuseConnectTest005, TestSize.Level1)
{
    ConfigSocketId(VALID_SOCKET);
    accessInfo_.isOHType = true;
    auto connect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, NETWORK_ID, SoftBusClient::QOS_REUSE,
        accessInfo_);
    auto status = connect->ReuseConnect(nullptr);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: SendDataTest001
* @tc.desc: send data fail
* @tc.type: FUNC
*/
HWTEST_F(SoftBusClientTest, SendDataTest001, TestSize.Level1)
{
    auto connect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, NETWORK_ID, SoftBusClient::QOS_REUSE,
        accessInfo_);
    DataInfo dataInfo;
    auto status = connect->SendData(dataInfo);
    ASSERT_NE(status, Status::SUCCESS);

    ConfigSocketId(INVALID_SEND_SOCKET);
    status = connect->ReuseConnect(nullptr);
    ASSERT_EQ(status, Status::SUCCESS);
    status = connect->SendData(dataInfo);
    ASSERT_NE(status, Status::SUCCESS);
}

/**
* @tc.name: SendDataTest002
* @tc.desc: send data success
* @tc.type: FUNC
*/
HWTEST_F(SoftBusClientTest, SendDataTest002, TestSize.Level1)
{
    ConfigSocketId(VALID_SOCKET);
    auto connect = std::make_shared<SoftBusClient>(pipeInfo_, deviceId_, NETWORK_ID, SoftBusClient::QOS_REUSE,
        accessInfo_);
    DataInfo dataInfo;
    auto status = connect->ReuseConnect(nullptr);
    ASSERT_EQ(status, Status::SUCCESS);
    status = connect->SendData(dataInfo);
    ASSERT_EQ(status, Status::SUCCESS);
}
} // namespace OHOS::Test