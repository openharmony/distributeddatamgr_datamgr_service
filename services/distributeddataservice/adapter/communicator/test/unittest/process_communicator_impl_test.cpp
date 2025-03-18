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
#include <gmock/gmock.h>
#include <iostream>
#include <unistd.h>
#include <vector>
#include "communication_provider.h"
#include "device_manager_adapter_mock.h"
#include "process_communicator_impl.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::AppDistributedKv;
using namespace OHOS::DistributedData;
using OnDeviceChange = DistributedDB::OnDeviceChange;
using OnDataReceive = DistributedDB::OnDataReceive;
using OnSendAble = DistributedDB::OnSendAble;
using DeviceInfos = DistributedDB::DeviceInfos;
using DeviceInfo = OHOS::AppDistributedKv::DeviceInfo;
using UserInfo = DistributedDB::UserInfo;

namespace OHOS::AppDistributedKv {
class MockCommunicationProvider : public CommunicationProvider {
public:
    ~MockCommunicationProvider() = default;
    static MockCommunicationProvider& Init()
    {
        static MockCommunicationProvider instance;
        return instance;
    }
    MOCK_METHOD(Status, StartWatchDataChange, (const AppDataChangeListener *, const PipeInfo &), (override));
    MOCK_METHOD(Status, StopWatchDataChange, (const AppDataChangeListener *, const PipeInfo &), (override));
    MOCK_METHOD(Status, Start, (const PipeInfo&), (override));
    MOCK_METHOD(Status, Stop, (const PipeInfo&), (override));
    MOCK_METHOD((std::pair<Status, int32_t>), SendData, (const PipeInfo&, const DeviceId&,
        const DataInfo&, uint32_t, const MessageInfo &), (override));
    MOCK_METHOD(bool, IsSameStartedOnPeer, (const PipeInfo &, const DeviceId &), (const));
    MOCK_METHOD(Status, ReuseConnect, (const PipeInfo&, const DeviceId&), (override));
    MOCK_METHOD(void, SetDeviceQuery, (std::shared_ptr<IDeviceQuery>), (override));
    MOCK_METHOD(void, SetMessageTransFlag, (const PipeInfo &, bool), (override));
    MOCK_METHOD(Status, Broadcast, (const PipeInfo &, const LevelInfo &), (override));
    MOCK_METHOD(int32_t, ListenBroadcastMsg, (const PipeInfo &,
        std::function<void(const std::string &, const LevelInfo &)>), (override));
};

CommunicationProvider& CommunicationProvider::GetInstance()
{
    return MockCommunicationProvider::Init();
}

std::shared_ptr<CommunicationProvider> CommunicationProvider::MakeCommunicationProvider()
{
    static std::shared_ptr<CommunicationProvider> instance(
        &MockCommunicationProvider::Init(),
        [](void*) {
        }
    );
    return instance;
}
}

class ProcessCommunicatorImplTest : public testing::Test {
public:
    static inline std::shared_ptr<DeviceManagerAdapterMock> deviceManagerAdapterMock = nullptr;
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    ProcessCommunicatorImpl* communicator_;
    MockCommunicationProvider* mockProvider;
};

namespace OHOS::DistributedData {
class ConcreteRouteHeadHandler : public RouteHeadHandler {
    public:
        bool ParseHeadDataLen(const uint8_t *data, uint32_t totalLen, uint32_t &headSize)
        {
                if (totalLen == 0) {
                    return true;
                }
                return false;
        }

        bool ParseHeadDataUser(const uint8_t *data, uint32_t totalLen, const std::string &label,
            std::vector<UserInfo> &userInfos)
        {
                if (totalLen == 0) {
                    return true;
                }
                return false;
        }
};
}

void ProcessCommunicatorImplTest::SetUp(void)
{
    communicator_ = ProcessCommunicatorImpl::GetInstance();
    communicator_->Stop();
    mockProvider = &MockCommunicationProvider::Init();
}

void ProcessCommunicatorImplTest::TearDown(void)
{
    mockProvider = nullptr;
}

void ProcessCommunicatorImplTest::SetUpTestCase(void)
{
    deviceManagerAdapterMock = std::make_shared<DeviceManagerAdapterMock>();
    BDeviceManagerAdapter::deviceManagerAdapter = deviceManagerAdapterMock;
}

void ProcessCommunicatorImplTest::TearDownTestCase()
{
    deviceManagerAdapterMock = nullptr;
    BDeviceManagerAdapter::deviceManagerAdapter = nullptr;
}

/**
* @tc.name: StartTest01
* @tc.desc: StartTest01 test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(ProcessCommunicatorImplTest, StartTest01, TestSize.Level0)
{
    ASSERT_NE(communicator_, nullptr);
    EXPECT_CALL(*mockProvider, Start(_)).WillOnce(Return(Status::SUCCESS));
    auto status = communicator_->Start("test_process");
    EXPECT_EQ(status, DistributedDB::OK);
    EXPECT_CALL(*mockProvider, Start(_)).WillOnce(Return(Status::INVALID_ARGUMENT));
    status = communicator_->Start("test_process");
    EXPECT_EQ(status, DistributedDB::DB_ERROR);
}

/**
* @tc.name: StopTest01
* @tc.desc: StopTest01 test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(ProcessCommunicatorImplTest, StopTest01, TestSize.Level0)
{
    ASSERT_NE(communicator_, nullptr);
    EXPECT_CALL(*mockProvider, Stop(_)).WillOnce(Return(Status::SUCCESS));
    auto status = communicator_->Stop();
    EXPECT_EQ(status, DistributedDB::OK);
    EXPECT_CALL(*mockProvider, Stop(_)).WillRepeatedly(Return(Status::INVALID_ARGUMENT));
    status = communicator_->Stop();
    EXPECT_EQ(status, DistributedDB::DB_ERROR);
}

/**
* @tc.name: RegOnDeviceChange
* @tc.desc: RegOnDeviceChange test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(ProcessCommunicatorImplTest, RegOnDeviceChange, TestSize.Level0)
{
    ASSERT_NE(communicator_, nullptr);
    EXPECT_CALL(*deviceManagerAdapterMock, StartWatchDeviceChange(_, _)).WillOnce(Return(Status::SUCCESS))
        .WillRepeatedly(Return(Status::SUCCESS));
    EXPECT_CALL(*deviceManagerAdapterMock, StopWatchDeviceChange(_, _)).WillOnce(Return(Status::SUCCESS))
        .WillRepeatedly(Return(Status::SUCCESS));
    auto myOnDeviceChange = [](const DeviceInfos &devInfo, bool isOnline) {};
    OnDeviceChange callback;
    callback = myOnDeviceChange;
    auto status = communicator_->RegOnDeviceChange(callback);
    EXPECT_EQ(status, DistributedDB::OK);
    callback = nullptr;
    status = communicator_->RegOnDeviceChange(callback);
    EXPECT_EQ(status, DistributedDB::OK);

    EXPECT_CALL(*deviceManagerAdapterMock, StartWatchDeviceChange(_, _)).WillOnce(Return(Status::INVALID_ARGUMENT))
        .WillRepeatedly(Return(Status::SUCCESS));
    EXPECT_CALL(*deviceManagerAdapterMock, StopWatchDeviceChange(_, _)).WillOnce(Return(Status::INVALID_ARGUMENT))
        .WillRepeatedly(Return(Status::SUCCESS));
    status = communicator_->RegOnDeviceChange(callback);
    EXPECT_EQ(status, DistributedDB::DB_ERROR);
    callback = myOnDeviceChange;
    status = communicator_->RegOnDeviceChange(callback);
    EXPECT_EQ(status, DistributedDB::DB_ERROR);
}

/**
* @tc.name: RegOnDataReceive
* @tc.desc: RegOnDataReceive test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(ProcessCommunicatorImplTest, RegOnDataReceive, TestSize.Level0)
{
    ASSERT_NE(communicator_, nullptr);
    auto myOnDeviceChange = [](const DeviceInfos &devInfo, const uint8_t *data, uint32_t length) {};
    OnDataReceive callback;
    callback = myOnDeviceChange;
    EXPECT_CALL(*mockProvider, StartWatchDataChange(_, _)).WillRepeatedly(Return(Status::SUCCESS));
    auto status = communicator_->RegOnDataReceive(callback);
    EXPECT_EQ(status, DistributedDB::OK);
    EXPECT_CALL(*mockProvider, StartWatchDataChange(_, _)).WillRepeatedly(Return(Status::ERROR));
    status = communicator_->RegOnDataReceive(callback);
    EXPECT_EQ(status, DistributedDB::DB_ERROR);

    callback = nullptr;
    EXPECT_CALL(*mockProvider, StopWatchDataChange(_, _)).WillRepeatedly(Return(Status::SUCCESS));
    status = communicator_->RegOnDataReceive(callback);
    EXPECT_EQ(status, DistributedDB::OK);
    EXPECT_CALL(*mockProvider, StopWatchDataChange(_, _)).WillRepeatedly(Return(Status::ERROR));
    status = communicator_->RegOnDataReceive(callback);
    EXPECT_EQ(status, DistributedDB::DB_ERROR);
}

/**
* @tc.name: RegOnSendAble
* @tc.desc: RegOnSendAble test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(ProcessCommunicatorImplTest, RegOnSendAble, TestSize.Level0)
{
    ASSERT_NE(communicator_, nullptr);
    OnSendAble sendAbleCallback = nullptr;
    EXPECT_NO_FATAL_FAILURE(communicator_->RegOnSendAble(sendAbleCallback));
}

/**
* @tc.name: SendData
* @tc.desc: SendData test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(ProcessCommunicatorImplTest, SendData, TestSize.Level0)
{
    ASSERT_NE(communicator_, nullptr);
    DeviceInfos deviceInfos = {"SendDataTest"};
    const uint8_t *data;
    uint8_t exampleData[] = {0x01, 0x02, 0x03, 0x04};
    data = exampleData;
    uint32_t length = 1;
    uint32_t totalLength = 1;
    std::pair<Status, int32_t> myPair = std::make_pair(Status::RATE_LIMIT, 1);
    EXPECT_CALL(*mockProvider, SendData(_, _, _, _, _)).WillRepeatedly(Return(myPair));
    auto status = communicator_->SendData(deviceInfos, data, length, totalLength);
    EXPECT_EQ(status, DistributedDB::RATE_LIMIT);
    myPair = {Status::SUCCESS, 1};
    EXPECT_CALL(*mockProvider, SendData(_, _, _, _, _)).WillRepeatedly(Return(myPair));
    status = communicator_->SendData(deviceInfos, data, length, totalLength);
    EXPECT_EQ(status, DistributedDB::OK);
    myPair = {Status::ERROR, static_cast<int32_t>(DistributedDB::BUSY)};
    EXPECT_CALL(*mockProvider, SendData(_, _, _, _, _)).WillRepeatedly(Return(myPair));
    status = communicator_->SendData(deviceInfos, data, length, totalLength);
    EXPECT_EQ(status, DistributedDB::BUSY);
    myPair = {Status::ERROR, 0};
    EXPECT_CALL(*mockProvider, SendData(_, _, _, _, _)).WillRepeatedly(Return(myPair));
    status = communicator_->SendData(deviceInfos, data, length, totalLength);
    EXPECT_EQ(status, DistributedDB::DB_ERROR);
}

/**
* @tc.name: GetRemoteOnlineDeviceInfosList
* @tc.desc: GetRemoteOnlineDeviceInfosList test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(ProcessCommunicatorImplTest, GetRemoteOnlineDeviceInfosList, TestSize.Level0)
{
    ASSERT_NE(communicator_, nullptr);
    auto remoteDevInfos = communicator_->GetRemoteOnlineDeviceInfosList();
    EXPECT_EQ(remoteDevInfos.empty(), true);

    DeviceInfo deviceInfo;
    deviceInfo.uuid = "GetRemoteOnlineDeviceInfosList";
    std::vector<DeviceInfo> devInfos;
    devInfos.push_back(deviceInfo);
    EXPECT_CALL(*deviceManagerAdapterMock, GetRemoteDevices()).WillRepeatedly(Return(devInfos));
    remoteDevInfos = communicator_->GetRemoteOnlineDeviceInfosList();
    EXPECT_EQ(remoteDevInfos[0].identifier, deviceInfo.uuid);
}

/**
* @tc.name: OnMessage
* @tc.desc: OnMessage test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(ProcessCommunicatorImplTest, OnMessage, TestSize.Level0)
{
    ASSERT_NE(communicator_, nullptr);
    DeviceInfo deviceInfo;
    deviceInfo.uuid = "OnMessageTest";
    uint8_t data[] = {0x10, 0x20, 0x30, 0x40, 0x50};
    uint8_t *ptr = data;
    int size = 1;
    PipeInfo pipeInfo;
    pipeInfo.pipeId = "OnMessageTest01";
    pipeInfo.userId = "OnMessageTest02";
    auto myOnDeviceChange = [](const DeviceInfos &devInfo, const uint8_t *data, uint32_t length) {};
    communicator_->onDataReceiveHandler_ = nullptr;
    EXPECT_NO_FATAL_FAILURE(communicator_->OnMessage(deviceInfo, ptr, size, pipeInfo));
    communicator_->onDataReceiveHandler_ = myOnDeviceChange;
    EXPECT_NO_FATAL_FAILURE(communicator_->OnMessage(deviceInfo, ptr, size, pipeInfo));
}

/**
* @tc.name: OnDeviceChanged
* @tc.desc: OnDeviceChanged test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(ProcessCommunicatorImplTest, OnDeviceChanged, TestSize.Level0)
{
    ASSERT_NE(communicator_, nullptr);
    DeviceInfo deviceInfo;
    deviceInfo.uuid = "cloudDeviceUuid";
    EXPECT_NO_FATAL_FAILURE(communicator_->OnDeviceChanged(deviceInfo, DeviceChangeType::DEVICE_ONREADY));
    EXPECT_NO_FATAL_FAILURE(communicator_->OnDeviceChanged(deviceInfo, DeviceChangeType::DEVICE_OFFLINE));
    deviceInfo.uuid = "OnDeviceChangedTest";
    communicator_->onDeviceChangeHandler_ = nullptr;
    EXPECT_NO_FATAL_FAILURE(communicator_->OnDeviceChanged(deviceInfo, DeviceChangeType::DEVICE_OFFLINE));
    auto myOnDeviceChange = [](const DeviceInfos &devInfo, bool isOnline) {};
    communicator_->onDeviceChangeHandler_ = myOnDeviceChange;
    EXPECT_NO_FATAL_FAILURE(communicator_->OnDeviceChanged(deviceInfo, DeviceChangeType::DEVICE_OFFLINE));
}

/**
* @tc.name: OnSessionReady
* @tc.desc: OnSessionReady test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(ProcessCommunicatorImplTest, OnSessionReady, TestSize.Level0)
{
    ASSERT_NE(communicator_, nullptr);
    DeviceInfo deviceInfo;
    deviceInfo.uuid = "OnSessionReadyTest";
    communicator_->sessionListener_ = nullptr;
    EXPECT_NO_FATAL_FAILURE(communicator_->OnSessionReady(deviceInfo, 1));
    auto myOnSendAble = [](const DeviceInfos &deviceInfo, int deviceCommErrCode) {};
    communicator_->sessionListener_ = myOnSendAble;
    EXPECT_NO_FATAL_FAILURE(communicator_->OnSessionReady(deviceInfo, 1));
}

/**
* @tc.name: GetExtendHeaderHandle
* @tc.desc: GetExtendHeaderHandle test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(ProcessCommunicatorImplTest, GetExtendHeaderHandle, TestSize.Level0)
{
    ASSERT_NE(communicator_, nullptr);
    communicator_->routeHeadHandlerCreator_ = nullptr;
    DistributedDB::ExtendInfo info;
    auto handle = communicator_->GetExtendHeaderHandle(info);
    EXPECT_EQ(handle, nullptr);
    communicator_->routeHeadHandlerCreator_ = [](const DistributedDB::ExtendInfo &info) ->
        std::shared_ptr<OHOS::DistributedData::RouteHeadHandler> {
            return std::make_shared<OHOS::DistributedData::ConcreteRouteHeadHandler>();
    };
    handle = communicator_->GetExtendHeaderHandle(info);
    EXPECT_NE(handle, nullptr);
}

/**
* @tc.name: GetDataHeadInfo
* @tc.desc: GetDataHeadInfo test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(ProcessCommunicatorImplTest, GetDataHeadInfo, TestSize.Level0)
{
    ASSERT_NE(communicator_, nullptr);
    uint8_t data[] = {0x10, 0x20, 0x30, 0x40, 0x50};
    uint8_t *ptr = data;
    uint32_t totalLen = 1;
    uint32_t headLength = 1;
    communicator_->routeHeadHandlerCreator_ = nullptr;
    auto status = communicator_->GetDataHeadInfo(ptr, totalLen, headLength);
    EXPECT_EQ(status, DistributedDB::DB_ERROR);

    communicator_->routeHeadHandlerCreator_ = [](const DistributedDB::ExtendInfo &info) ->
        std::shared_ptr<OHOS::DistributedData::RouteHeadHandler> {
            return std::make_shared<OHOS::DistributedData::ConcreteRouteHeadHandler>();
    };
    status = communicator_->GetDataHeadInfo(ptr, totalLen, headLength);
    EXPECT_EQ(status, DistributedDB::INVALID_FORMAT);
    totalLen = 0;
    status = communicator_->GetDataHeadInfo(ptr, totalLen, headLength);
    EXPECT_EQ(status, DistributedDB::OK);
}

/**
* @tc.name: GetDataUserInfo
* @tc.desc: GetDataUserInfo test
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
 */
HWTEST_F(ProcessCommunicatorImplTest, GetDataUserInfo, TestSize.Level0)
{
    ASSERT_NE(communicator_, nullptr);
    uint8_t data[] = {0x10, 0x20, 0x30, 0x40, 0x50};
    uint8_t *ptr = data;
    uint32_t totalLen = 1;
    std::string label = "GetDataUserInfoTest";
    std::vector<UserInfo> userInfos;
    UserInfo user1{"GetDataUserInfo01"};
    UserInfo user2{"GetDataUserInfo02"};
    UserInfo user3{"GetDataUserInfo03"};
    communicator_->routeHeadHandlerCreator_ = nullptr;
    auto status = communicator_->GetDataUserInfo(ptr, totalLen, label, userInfos);
    EXPECT_EQ(status, DistributedDB::DB_ERROR);

    communicator_->routeHeadHandlerCreator_ = [](const DistributedDB::ExtendInfo &info) ->
        std::shared_ptr<OHOS::DistributedData::RouteHeadHandler> {
            return std::make_shared<OHOS::DistributedData::ConcreteRouteHeadHandler>();
    };
    status = communicator_->GetDataUserInfo(ptr, totalLen, label, userInfos);
    EXPECT_EQ(status, DistributedDB::INVALID_FORMAT);
    totalLen = 0;
    EXPECT_EQ(userInfos.empty(), true);
    status = communicator_->GetDataUserInfo(ptr, totalLen, label, userInfos);
    EXPECT_EQ(status, DistributedDB::NO_PERMISSION);
    userInfos.push_back(user1);
    userInfos.push_back(user2);
    userInfos.push_back(user3);
    status = communicator_->GetDataUserInfo(ptr, totalLen, label, userInfos);
    EXPECT_EQ(status, DistributedDB::OK);
}