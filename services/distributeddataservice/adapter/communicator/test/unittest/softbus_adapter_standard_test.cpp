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
#define LOG_TAG "SoftbusAdapterStandardTest"

#include "app_device_change_listener.h"
#include <cstdint>
#include "gtest/gtest.h"
#include <iostream>
#include "log_print.h"
#include "softbus_adapter.h"
#include "types.h"
#include <unistd.h>
#include <vector>

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::AppDistributedKv;
using DeviceInfo = OHOS::AppDistributedKv::DeviceInfo;
class AppDataChangeListenerImpl : public AppDataChangeListener {
    struct ServerSocketInfo {
        std::string name;      /**< Peer socket name */
        std::string networkId; /**< Peer network ID */
        std::string pkgName;   /**< Peer package name */
    };

    void OnMessage(const OHOS::AppDistributedKv::DeviceInfo &info, const uint8_t *ptr, const int size,
        const struct PipeInfo &id) const override;
};

void AppDataChangeListenerImpl::OnMessage(const OHOS::AppDistributedKv::DeviceInfo &info,
    const uint8_t *ptr, const int size, const struct PipeInfo &id) const
{
    ZLOGI("data  %{public}s  %s", info.deviceName.c_str(), ptr);
}

class SoftbusAdapterStandardTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
protected:
    static constexpr uint32_t DEFAULT_MTU_SIZE = 4096 * 1024u;
    static constexpr uint32_t DEFAULT_TIMEOUT = 30 * 1000;
};

/**
* @tc.name: StartWatchDeviceChange
* @tc.desc: start watch data change
* @tc.type: FUNC
* @tc.require:
* @tc.author: nhj
 */
HWTEST_F(SoftbusAdapterStandardTest, StartWatchDeviceChange, TestSize.Level0)
{
    auto status = SoftBusAdapter::GetInstance()->StartWatchDataChange(nullptr, {});
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: StartWatchDeviceChange
* @tc.desc: start watch data change
* @tc.type: FUNC
* @tc.require:
* @tc.author: nhj
 */
HWTEST_F(SoftbusAdapterStandardTest, StartWatchDeviceChange01, TestSize.Level0)
{
    PipeInfo appId;
    appId.pipeId = "appId";
    appId.userId = "groupId";
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    auto status = SoftBusAdapter::GetInstance()->StartWatchDataChange(dataListener, appId);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: StartWatchDeviceChange
* @tc.desc: start watch data change
* @tc.type: FUNC
* @tc.require:
* @tc.author: nhj
 */
HWTEST_F(SoftbusAdapterStandardTest, StartWatchDeviceChange02, TestSize.Level0)
{
    PipeInfo appId;
    appId.pipeId = "";
    appId.userId = "groupId";
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    auto status = SoftBusAdapter::GetInstance()->StartWatchDataChange(dataListener, appId);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: StartWatchDeviceChange03
* @tc.desc:the observer is nullptr
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(SoftbusAdapterStandardTest, StartWatchDeviceChange03, TestSize.Level1)
{
    ZLOGI("begin.");
    PipeInfo appId;
    appId.pipeId = "appId06";
    appId.userId = "groupId06";
    auto flag = SoftBusAdapter::GetInstance();
    ASSERT_NE(flag, nullptr);
    SoftBusAdapter::GetInstance()->StartWatchDataChange(nullptr, appId);
    auto status = SoftBusAdapter::GetInstance()->StartWatchDataChange(nullptr, appId);
    EXPECT_EQ(Status::INVALID_ARGUMENT, status);
}

/**
* @tc.name: StopWatchDataChange
* @tc.desc: stop watch data change
* @tc.type: FUNC
* @tc.require:
* @tc.author: nhj
 */
HWTEST_F(SoftbusAdapterStandardTest, StopWatchDataChange, TestSize.Level0)
{
    PipeInfo appId;
    appId.pipeId = "appId";
    appId.userId = "groupId";
    auto flag = SoftBusAdapter::GetInstance();
    ASSERT_NE(flag, nullptr);
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    auto status = SoftBusAdapter::GetInstance()->StopWatchDataChange(dataListener, appId);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: StopWatchDataChange
* @tc.desc: stop watch data change
* @tc.type: FUNC
* @tc.require:
* @tc.author: nhj
 */
HWTEST_F(SoftbusAdapterStandardTest, StopWatchDataChange01, TestSize.Level0)
{
    PipeInfo appId;
    appId.pipeId = "";
    appId.userId = "groupId";
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    auto status = SoftBusAdapter::GetInstance()->StopWatchDataChange(dataListener, appId);
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: SendData
* @tc.desc: parse sent data
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(SoftbusAdapterStandardTest, SendData, TestSize.Level1)
{
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    PipeInfo id;
    id.pipeId = "appId";
    id.userId = "groupId";
    auto secRegister = SoftBusAdapter::GetInstance()->StartWatchDataChange(dataListener, id);
    EXPECT_EQ(Status::SUCCESS, secRegister);
    std::string content = "Helloworlds";
    const uint8_t *t = reinterpret_cast<const uint8_t*>(content.c_str());
    DeviceId di = {"DeviceId"};
    DataInfo data = { const_cast<uint8_t *>(t), static_cast<uint32_t>(content.length())};
    auto status = SoftBusAdapter::GetInstance()->SendData(id, di, data, 11, { MessageType::DEFAULT });
    EXPECT_NE(status.first, Status::SUCCESS);
    SoftBusAdapter::GetInstance()->StopWatchDataChange(dataListener, id);
    delete dataListener;
}

/**
* @tc.name: SendData01
* @tc.desc: parse sent data
* @tc.type: FUNC
* @tc.author:
*/
HWTEST_F(SoftbusAdapterStandardTest, SendData01, TestSize.Level1)
{
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    PipeInfo pipe01;
    pipe01.pipeId = "appId";
    pipe01.userId = "groupId";
    auto flag = SoftBusAdapter::GetInstance();
    ASSERT_NE(flag, nullptr);
    auto secRegister = SoftBusAdapter::GetInstance()->StartWatchDataChange(dataListener, pipe01);
    EXPECT_EQ(Status::SUCCESS, secRegister);
    std::string content = "";
    const uint8_t *t = reinterpret_cast<const uint8_t*>(content.c_str());
    DeviceId di = {"DeviceId"};
    DataInfo data = { const_cast<uint8_t *>(t), static_cast<uint32_t>(content.length())};
    auto status = SoftBusAdapter::GetInstance()->SendData(pipe01, di, data, 10, { MessageType::FILE });
    EXPECT_NE(status.first, Status::ERROR);
    EXPECT_EQ(status.first, Status::RATE_LIMIT);
    SoftBusAdapter::GetInstance()->StopWatchDataChange(dataListener, pipe01);
    delete dataListener;
}

/**
* @tc.name: GetMtuSize
* @tc.desc: get size
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(SoftbusAdapterStandardTest, GetMtuSize, TestSize.Level1)
{
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    PipeInfo id;
    id.pipeId = "appId";
    id.userId = "groupId";
    SoftBusAdapter::GetInstance()->StartWatchDataChange(dataListener, id);
    DeviceId di = {"DeviceId"};
    auto size = SoftBusAdapter::GetInstance()->GetMtuSize(di);
    EXPECT_EQ(size, DEFAULT_MTU_SIZE);
    SoftBusAdapter::GetInstance()->GetCloseSessionTask();
    SoftBusAdapter::GetInstance()->StopWatchDataChange(dataListener, id);
    delete dataListener;
}

/**
* @tc.name: GetTimeout
* @tc.desc: get timeout
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(SoftbusAdapterStandardTest, GetTimeout, TestSize.Level1)
{
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    PipeInfo id;
    id.pipeId = "appId01";
    id.userId = "groupId01";
    SoftBusAdapter::GetInstance()->StartWatchDataChange(dataListener, id);
    DeviceId di = {"DeviceId"};
    auto time = SoftBusAdapter::GetInstance()->GetTimeout(di);
    EXPECT_EQ(time, DEFAULT_TIMEOUT);
    SoftBusAdapter::GetInstance()->StopWatchDataChange(dataListener, id);
    delete dataListener;
}

/**
* @tc.name: IsSameStartedOnPeer
* @tc.desc: get size
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(SoftbusAdapterStandardTest, IsSameStartedOnPeer, TestSize.Level1)
{
    PipeInfo id;
    id.pipeId = "appId01";
    id.userId = "groupId01";
    DeviceId di = {"DeviceId"};
    SoftBusAdapter::GetInstance()->SetMessageTransFlag(id, true);
    auto status = SoftBusAdapter::GetInstance()->IsSameStartedOnPeer(id, di);
    EXPECT_EQ(status, true);
}

/**
* @tc.name: ReuseConnect
* @tc.desc: reuse connect
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(SoftbusAdapterStandardTest, ReuseConnect, TestSize.Level1)
{
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    PipeInfo pipe;
    pipe.pipeId = "appId";
    pipe.userId = "groupId";
    auto flag = SoftBusAdapter::GetInstance();
    ASSERT_NE(flag, nullptr);
    SoftBusAdapter::GetInstance()->StartWatchDataChange(dataListener, pipe);
    DeviceId device = {"DeviceId"};
    auto reuse = SoftBusAdapter::GetInstance()->ReuseConnect(pipe, device);
    EXPECT_EQ(reuse, Status::NOT_SUPPORT);
    SoftBusAdapter::GetInstance()->StopWatchDataChange(dataListener, pipe);
    delete dataListener;
}

/**
* @tc.name: GetConnect
* @tc.desc: get connect
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(SoftbusAdapterStandardTest, GetConnect, TestSize.Level1)
{
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    PipeInfo pipe;
    pipe.pipeId = "appId01";
    pipe.userId = "groupId01";
    auto flag = SoftBusAdapter::GetInstance();
    ASSERT_NE(flag, nullptr);
    SoftBusAdapter::GetInstance()->StartWatchDataChange(dataListener, pipe);
    DeviceId device = {"DeviceId01"};
    std::shared_ptr<SoftBusClient> conn = nullptr;
    auto reuse = SoftBusAdapter::GetInstance()->GetConnect(pipe, device, 1);
    EXPECT_NE(reuse, nullptr);
    SoftBusAdapter::GetInstance()->StopWatchDataChange(dataListener, pipe);
    delete dataListener;
}

/**
* @tc.name: Broadcast
* @tc.desc: broadcast
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(SoftbusAdapterStandardTest, Broadcast, TestSize.Level1)
{
    PipeInfo id;
    id.pipeId = "appId";
    id.userId = "groupId";
    LevelInfo level;
    level.dynamic = 1;
    level.statics = 1;
    level.switches = 1;
    level.switchesLen = 1;
    auto flag = SoftBusAdapter::GetInstance();
    ASSERT_NE(flag, nullptr);
    SoftBusAdapter::GetInstance()->SetMessageTransFlag(id, true);
    auto status = SoftBusAdapter::GetInstance()->Broadcast(id, level);
    EXPECT_EQ(status, Status::ERROR);
}

/**
* @tc.name: OpenConnect
* @tc.desc: open connect
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(SoftbusAdapterStandardTest, OpenConnect, TestSize.Level1)
{
    DeviceId device = {"DeviceId"};
    std::shared_ptr<SoftBusClient> conn = nullptr;
    auto flag = SoftBusAdapter::GetInstance();
    ASSERT_NE(flag, nullptr);
    auto status = SoftBusAdapter::GetInstance()->OpenConnect(conn, device);
    EXPECT_NE(status.first, Status::SUCCESS);
    EXPECT_EQ(status.first, Status::RATE_LIMIT);
    EXPECT_EQ(status.second, 0);
}

/**
* @tc.name: CloseSession
* @tc.desc: close session
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(SoftbusAdapterStandardTest, CloseSession, TestSize.Level1)
{
    std::string networkId = "networkId";
    auto flag = SoftBusAdapter::GetInstance();
    ASSERT_NE(flag, nullptr);
    auto status = SoftBusAdapter::GetInstance()->CloseSession(networkId);
    EXPECT_EQ(status, false);
}

/**
* @tc.name: CloseSession01
* @tc.desc: close session
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(SoftbusAdapterStandardTest, CloseSession01, TestSize.Level1)
{
    std::string networkId = "";
    auto flag = SoftBusAdapter::GetInstance();
    ASSERT_NE(flag, nullptr);
    auto status = SoftBusAdapter::GetInstance()->CloseSession(networkId);
    EXPECT_EQ(status, false);
}

/**
* @tc.name: GetPeerSocketInfo
* @tc.desc: get socket info
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(SoftbusAdapterStandardTest, GetPeerSocketInfo, TestSize.Level1)
{
    AppDistributedKv::SoftBusAdapter::ServerSocketInfo info;
    info.name = "kv";
    info.networkId= "192.168.1.1";
    info.pkgName = "test";
    auto flag = SoftBusAdapter::GetInstance();
    ASSERT_NE(flag, nullptr);
    auto status = SoftBusAdapter::GetInstance()->GetPeerSocketInfo(-1, info);
    EXPECT_EQ(status, false);
}

/**
* @tc.name: GetPeerSocketInfo
* @tc.desc: get socket info
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(SoftbusAdapterStandardTest, GetPeerSocketInfo01, TestSize.Level1)
{
    AppDistributedKv::SoftBusAdapter::ServerSocketInfo info;
    info.name = "service";
    info.networkId= "192.168.1.1";
    info.pkgName = "test";
    auto flag = SoftBusAdapter::GetInstance();
    ASSERT_NE(flag, nullptr);
    auto status = SoftBusAdapter::GetInstance()->GetPeerSocketInfo(1, info);
    EXPECT_EQ(status, false);
}
} // namespace OHOS::Test