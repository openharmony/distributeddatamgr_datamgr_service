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
#include "app_device_change_listener.h"
#include <cstdint>
#include "gtest/gtest.h"
#include <gmock/gmock.h>
#include <iostream>
#include "device_manager_adapter_mock.h"
#include "softbus_adapter.h"
#include "softbus_adapter_standard.cpp"
#include "softbus_error_code.h"
#include "types.h"
#include <unistd.h>
#include <vector>

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::AppDistributedKv;
using namespace OHOS::DistributedData;
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
{}

class SoftbusAdapterStandardTest : public testing::Test {
public:
    static inline std::shared_ptr<DeviceManagerAdapterMock> deviceManagerAdapterMock = nullptr;
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() {}
    void TearDown() {}
protected:
    static constexpr uint32_t DEFAULT_MTU_SIZE = 4096 * 1024u;
    static constexpr uint32_t DEFAULT_TIMEOUT = 30 * 1000;
};

void SoftbusAdapterStandardTest::SetUpTestCase(void)
{
    deviceManagerAdapterMock = std::make_shared<DeviceManagerAdapterMock>();
    BDeviceManagerAdapter::deviceManagerAdapter = deviceManagerAdapterMock;
}

void SoftbusAdapterStandardTest::TearDownTestCase()
{
    deviceManagerAdapterMock = nullptr;
}

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
    status = SoftBusAdapter::GetInstance()->StartWatchDataChange(dataListener, appId);
    delete dataListener;
    EXPECT_EQ(status, Status::ERROR);
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
    delete dataListener;
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
    status = SoftBusAdapter::GetInstance()->StopWatchDataChange(dataListener, appId);
    delete dataListener;
    EXPECT_EQ(status, Status::ERROR);
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
    delete dataListener;
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: GetExpireTime
* @tc.desc: GetExpireTime Test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(SoftbusAdapterStandardTest, GetExpireTime, TestSize.Level0)
{
    PipeInfo appId;
    appId.pipeId = "appId";
    appId.userId = "groupId";
    DeviceId di = {"DeviceId"};
    std::shared_ptr<SoftBusClient> conn = std::make_shared<SoftBusClient>(appId, di, SoftBusClient::QoSType::QOS_HML);
    EXPECT_NO_FATAL_FAILURE(SoftBusAdapter::GetInstance()->GetExpireTime(conn));
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
* @tc.name: StartCloseSessionTask
* @tc.desc: StartCloseSessionTask tests
* @tc.type: FUNC
* @tc.author:
*/
HWTEST_F(SoftbusAdapterStandardTest, StartCloseSessionTask, TestSize.Level1)
{
    std::shared_ptr<SoftBusClient> conn = nullptr;
    std::vector<std::shared_ptr<SoftBusClient>> clients;
    clients.emplace_back(conn);
    auto status = SoftBusAdapter::GetInstance()->connects_.Insert("deviceId01", clients);
    EXPECT_EQ(status, true);
    SoftBusAdapter::GetInstance()->connects_.Clear();
    EXPECT_NO_FATAL_FAILURE(SoftBusAdapter::GetInstance()->StartCloseSessionTask("deviceId02"));
}

/**
* @tc.name: OnClientShutdown
* @tc.desc: DelConnect tests
* @tc.type: FUNC
* @tc.author:
*/
HWTEST_F(SoftbusAdapterStandardTest, OnClientShutdown, TestSize.Level1)
{
    PipeInfo appId;
    appId.pipeId = "appId";
    appId.userId = "groupId";
    DeviceId di = {"DeviceId"};
    std::shared_ptr<SoftBusClient> conn1 = std::make_shared<SoftBusClient>(appId, di, SoftBusClient::QoSType::QOS_HML);
    std::shared_ptr<SoftBusClient> conn2 = nullptr;
    std::vector<std::shared_ptr<SoftBusClient>> clients;
    clients.emplace_back(conn1);
    clients.emplace_back(conn2);
    auto status = SoftBusAdapter::GetInstance()->connects_.Insert("deviceId01", clients);
    EXPECT_EQ(status, true);
    status = SoftBusAdapter::GetInstance()->connects_.Insert("deviceId02", {});
    EXPECT_EQ(status, true);
    auto name = SoftBusAdapter::GetInstance()->OnClientShutdown(-1, true);
    EXPECT_EQ(name, "deviceId01 ");
    name = SoftBusAdapter::GetInstance()->OnClientShutdown(-1, false);
    EXPECT_EQ(name, "");
    name = SoftBusAdapter::GetInstance()->OnClientShutdown(1, true);
    SoftBusAdapter::GetInstance()->connects_.Clear();
    EXPECT_EQ(name, "");
}

/**
* @tc.name: NotifyDataListeners
* @tc.desc: NotifyDataListeners tests
* @tc.type: FUNC
* @tc.author:
*/
HWTEST_F(SoftbusAdapterStandardTest, NotifyDataListeners, TestSize.Level1)
{
    PipeInfo appId;
    appId.pipeId = "appId";
    appId.userId = "groupId";
    std::string content = "Helloworlds";
    const uint8_t *t = reinterpret_cast<const uint8_t*>(content.c_str());
    SoftBusAdapter::GetInstance()->dataChangeListeners_.Clear();
    EXPECT_NO_FATAL_FAILURE(SoftBusAdapter::GetInstance()->NotifyDataListeners(t, 1, "deviceId", appId));
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    SoftBusAdapter::GetInstance()->dataChangeListeners_.Insert(appId.pipeId, dataListener);
    delete dataListener;
    SoftBusAdapter::GetInstance()->dataChangeListeners_.Clear();
    EXPECT_NO_FATAL_FAILURE(SoftBusAdapter::GetInstance()->NotifyDataListeners(t, 1, "deviceId", appId));
}

/**
* @tc.name: ListenBroadcastMsg
* @tc.desc: ListenBroadcastMsg tests
* @tc.type: FUNC
* @tc.author:
*/
HWTEST_F(SoftbusAdapterStandardTest, ListenBroadcastMsg, TestSize.Level1)
{
    SoftBusAdapter::GetInstance()->onBroadcast_= nullptr;
    PipeInfo appId;
    appId.pipeId = "appId";
    appId.userId = "groupId";
    auto result = SoftBusAdapter::GetInstance()->ListenBroadcastMsg(appId, nullptr);
    EXPECT_EQ(result, SoftBusErrNo::SOFTBUS_INVALID_PARAM);

    auto listener = [](const std::string &message, const LevelInfo &info) {};
    result = SoftBusAdapter::GetInstance()->ListenBroadcastMsg(appId, listener);
    EXPECT_EQ(result, SoftBusErrNo::SOFTBUS_INVALID_PARAM);
    result = SoftBusAdapter::GetInstance()->ListenBroadcastMsg(appId, listener);
    EXPECT_EQ(result, SoftBusErrNo::SOFTBUS_ALREADY_EXISTED);
}

/**
* @tc.name: OnBroadcast
* @tc.desc: OnBroadcast tests
* @tc.type: FUNC
* @tc.author:
*/
HWTEST_F(SoftbusAdapterStandardTest, OnBroadcast, TestSize.Level1)
{
    DeviceId di = {"DeviceId"};
    LevelInfo level;
    level.dynamic = 1;
    level.statics = 1;
    level.switches = 1;
    level.switchesLen = 1;
    EXPECT_NE(SoftBusAdapter::GetInstance()->onBroadcast_, nullptr);
    EXPECT_NO_FATAL_FAILURE(SoftBusAdapter::GetInstance()->OnBroadcast(di, level));
    SoftBusAdapter::GetInstance()->onBroadcast_ = nullptr;
    EXPECT_NO_FATAL_FAILURE(SoftBusAdapter::GetInstance()->OnBroadcast(di, level));
}

/**
* @tc.name: OnClientSocketChanged
* @tc.desc: OnClientSocketChanged tests
* @tc.type: FUNC
* @tc.author:
*/
HWTEST_F(SoftbusAdapterStandardTest, OnClientSocketChanged, TestSize.Level1)
{
    QosTV qosTv;
    qosTv.qos = QosType::QOS_TYPE_MIN_BW;
    qosTv.value = 1;
    EXPECT_NO_FATAL_FAILURE(AppDataListenerWrap::OnClientSocketChanged(1, QoSEvent::QOS_SATISFIED, &qosTv, 1));
    EXPECT_NO_FATAL_FAILURE(AppDataListenerWrap::OnClientSocketChanged(1, QoSEvent::QOS_SATISFIED, &qosTv, 0));
    qosTv.qos = QosType::QOS_TYPE_MAX_WAIT_TIMEOUT;
    EXPECT_NO_FATAL_FAILURE(AppDataListenerWrap::OnClientSocketChanged(1, QoSEvent::QOS_SATISFIED, &qosTv, 0));
    EXPECT_NO_FATAL_FAILURE(AppDataListenerWrap::OnClientSocketChanged(1, QoSEvent::QOS_SATISFIED, nullptr, 0));
    EXPECT_NO_FATAL_FAILURE(AppDataListenerWrap::OnClientSocketChanged(1, QoSEvent::QOS_NOT_SATISFIED, nullptr, 0));
}

/**
* @tc.name: OnServerBytesReceived
* @tc.desc: OnServerBytesReceived tests
* @tc.type: FUNC
* @tc.author:
*/
HWTEST_F(SoftbusAdapterStandardTest, OnServerBytesReceived, TestSize.Level1)
{
    PeerSocketInfo info;
    info.name = strdup("");
    info.networkId = strdup("peertest01");
    info.pkgName = strdup("ohos.kv.test");
    info.dataType = TransDataType::DATA_TYPE_MESSAGE;
    AppDistributedKv::SoftBusAdapter::ServerSocketInfo serinfo;
    auto result = SoftBusAdapter::GetInstance()->GetPeerSocketInfo(1, serinfo);
    EXPECT_EQ(result, false);
    char str[] = "Hello";
    const void* data = static_cast<const void*>(str);
    EXPECT_NO_FATAL_FAILURE(AppDataListenerWrap::OnServerBytesReceived(1, data, 10));
    SoftBusAdapter::GetInstance()->OnBind(1, info);
    result = SoftBusAdapter::GetInstance()->GetPeerSocketInfo(1, serinfo);
    EXPECT_EQ(result, true);
    EXPECT_NO_FATAL_FAILURE(AppDataListenerWrap::OnServerBytesReceived(1, data, 10));
    info.name = strdup("name");
    SoftBusAdapter::GetInstance()->OnBind(2, info);
    result = SoftBusAdapter::GetInstance()->GetPeerSocketInfo(2, serinfo);
    EXPECT_EQ(result, true);
    EXPECT_NO_FATAL_FAILURE(AppDataListenerWrap::OnServerBytesReceived(2, data, 10));
}

/**
* @tc.name: GetPipeId
* @tc.desc: GetPipeId tests
* @tc.type: FUNC
* @tc.author:
*/
HWTEST_F(SoftbusAdapterStandardTest, GetPipeId, TestSize.Level1)
{
    std::string names = "GetPipeId";
    auto name = AppDataListenerWrap::GetPipeId(names);
    EXPECT_EQ(name, names);
    names = "test01_GetPipeId";
    name = AppDataListenerWrap::GetPipeId(names);
    EXPECT_EQ(name, "test01");
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
* @tc.name: ReuseConnect01
* @tc.desc: reuse connect
* @tc.type: FUNC
* @tc.author: wangbin
*/
HWTEST_F(SoftbusAdapterStandardTest, ReuseConnect01, TestSize.Level1)
{
    PipeInfo pipe;
    pipe.pipeId = "appId";
    pipe.userId = "groupId";
    DeviceId device = {"DeviceId"};
    auto status = SoftBusAdapter::GetInstance()->ReuseConnect(pipe, device);
    EXPECT_EQ(status, Status::NOT_SUPPORT);
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType(testing::_)).WillOnce(testing::Return(true))
        .WillRepeatedly(testing::Return(true));
    status = SoftBusAdapter::GetInstance()->ReuseConnect(pipe, device);
    EXPECT_EQ(status, Status::NETWORK_ERROR);
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
    std::string uuid = "CloseSessionTest";
    EXPECT_CALL(*deviceManagerAdapterMock, GetUuidByNetworkId(testing::_)).WillOnce(testing::Return(uuid))
        .WillRepeatedly(testing::Return(uuid));
    std::shared_ptr<SoftBusClient> conn = nullptr;
    std::vector<std::shared_ptr<SoftBusClient>> clients;
    clients.emplace_back(conn);
    auto result = SoftBusAdapter::GetInstance()->connects_.Insert(uuid, clients);
    EXPECT_EQ(result, true);
    status = SoftBusAdapter::GetInstance()->CloseSession(networkId);
    SoftBusAdapter::GetInstance()->connects_.Clear();
    EXPECT_EQ(status, true);
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
    EXPECT_EQ(status, true);
}
} // namespace OHOS::Test