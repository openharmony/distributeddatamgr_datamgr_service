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

#include <cstdint>
#include <gtest/gtest.h>
#include <iostream>
#include <unistd.h>
#include <vector>
#include "softbus_client.h"
#include "types.h"


namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::AppDistributedKv;
using PipeInfo = OHOS::AppDistributedKv::PipeInfo;
constexpr int32_t SOFTBUS_OK = 0;

class SoftbusClientTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() {}
    void TearDown() {}

    static  std::shared_ptr<SoftBusClient> client;
};

std::shared_ptr<SoftBusClient> SoftbusClientTest::client =  nullptr;

void SoftbusClientTest::SetUpTestCase(void)
{
    PipeInfo pipeInfo;
    pipeInfo.pipeId = "pipeId";
    pipeInfo.userId = "userId";
    DeviceId id = {"DeviceId"};
    client = std::make_shared<SoftBusClient>(pipeInfo, id, "");
}

void SoftbusClientTest::TearDownTestCase(void)
{
    client = nullptr;
}

/**
* @tc.name: SendData
* @tc.desc: SendData test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(SoftbusClientTest, SendData, TestSize.Level0)
{
    ASSERT_NE(client, nullptr);
    std::string content = "Helloworlds";
    const uint8_t *data = reinterpret_cast<const uint8_t*>(content.c_str());
    DataInfo info = { const_cast<uint8_t *>(data), static_cast<uint32_t>(content.length())};
    ISocketListener *listener = nullptr;
    auto status = client->SendData(info, listener);
    EXPECT_EQ(status, Status::ERROR);
    client->bindState_ = 0;
    status = client->SendData(info, listener);
    EXPECT_EQ(status, Status::ERROR);
}

/**
* @tc.name: OpenConnect
* @tc.desc: OpenConnect test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(SoftbusClientTest, OpenConnect, TestSize.Level0)
{
    ASSERT_NE(client, nullptr);
    ISocketListener *listener = nullptr;
    client->bindState_ = 0;
    auto status = client->OpenConnect(listener);
    EXPECT_EQ(status, Status::SUCCESS);
    client->bindState_ = 1;
    client->isOpening_.store(true);
    status = client->OpenConnect(listener);
    EXPECT_EQ(status, Status::RATE_LIMIT);
    client->isOpening_.store(false);
    status = client->OpenConnect(listener);
    EXPECT_EQ(status, Status::NETWORK_ERROR);
}

/**
* @tc.name: CheckStatus
* @tc.desc: CheckStatus test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(SoftbusClientTest, CheckStatus, TestSize.Level0)
{
    ASSERT_NE(client, nullptr);
    client->bindState_ = 0;
    auto status = client->CheckStatus();
    EXPECT_EQ(status, Status::SUCCESS);
    client->bindState_ = -1;
    client->isOpening_.store(true);
    status = client->CheckStatus();
    EXPECT_EQ(status, Status::RATE_LIMIT);
    client->isOpening_.store(false);
    status = client->CheckStatus();
    EXPECT_EQ(status, Status::ERROR);
}

/**
* @tc.name: Open
* @tc.desc: Open test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(SoftbusClientTest, Open, TestSize.Level0)
{
    ASSERT_NE(client, nullptr);
    ISocketListener *listener = nullptr;
    auto status = client->Open(1, AppDistributedKv::SoftBusClient::QOS_BR, listener, false);
    EXPECT_NE(status, SOFTBUS_OK);
}

/**
* @tc.name: UpdateExpireTime
* @tc.desc: UpdateExpireTime test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(SoftbusClientTest, UpdateExpireTime, TestSize.Level0)
{
    ASSERT_NE(client, nullptr);
    client->type_ = AppDistributedKv::SoftBusClient::QOS_BR;
    auto expireTime = client->CalcExpireTime();
    EXPECT_NO_FATAL_FAILURE(client->UpdateExpireTime(true));
    EXPECT_LT(expireTime, client->expireTime_);
    EXPECT_NO_FATAL_FAILURE(client->UpdateExpireTime(false));
    EXPECT_LT(expireTime, client->expireTime_);
}

/**
* @tc.name: UpdateBindInfo
* @tc.desc: UpdateBindInfo test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(SoftbusClientTest, UpdateBindInfo, TestSize.Level0)
{
    ASSERT_NE(client, nullptr);
    EXPECT_NO_FATAL_FAILURE(client->UpdateBindInfo(1, 1, AppDistributedKv::SoftBusClient::QOS_BR, false));
    EXPECT_NO_FATAL_FAILURE(client->UpdateBindInfo(1, 1, AppDistributedKv::SoftBusClient::QOS_BR, true));
}

/**
* @tc.name: ReuseConnect
* @tc.desc: ReuseConnect test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(SoftbusClientTest, ReuseConnect, TestSize.Level0)
{
    ASSERT_NE(client, nullptr);
    ISocketListener *listener = nullptr;
    client->bindState_ = 0;
    auto status = client->ReuseConnect(listener);
    EXPECT_EQ(status, Status::SUCCESS);
    client->bindState_ = -1;
    client->isOpening_.store(true);
    status = client->ReuseConnect(listener);
    EXPECT_EQ(status, Status::NETWORK_ERROR);
}

/**
* @tc.name: UpdateNetworkId
* @tc.desc: UpdateNetworkId test
* @tc.type: FUNC
 */
HWTEST_F(SoftbusClientTest, UpdateNetworkId, TestSize.Level1)
{
    ASSERT_NE(client, nullptr);
    const std::string newNetworkId = "newId";
    client->UpdateNetworkId(newNetworkId);
    EXPECT_EQ(client->GetNetworkId(), newNetworkId);
}
} // namespace OHOS::Test