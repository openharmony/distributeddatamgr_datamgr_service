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

#include "softbus_adapter.h"

#include <gtest/gtest.h>

#include "account/account_delegate.h"
#include "app_data_change_listener.h"
#include "app_device_change_listener.h"
#include "communication/connect_manager.h"
#include "communicator_context.h"
#include "data_level.h"
#include "db_store_mock.h"
#include "device_manager.h"
#include "device_manager_adapter.h"
#include "executor_pool.h"
#include "inner_socket.h"
#include "metadata/appid_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "types.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::AppDistributedKv;
using namespace OHOS::DistributedData;

static constexpr size_t THREAD_MIN = 0;
static constexpr size_t THREAD_MAX = 3;
static constexpr uint16_t DYNAMIC_LEVEL = 0xFFFF;
static constexpr uint16_t STATIC_LEVEL = 0x1111;
static constexpr uint32_t SWITCH_VALUE = 0x00000001;
static constexpr uint16_t SWITCH_LENGTH = 1;
static constexpr const char *TEST_BUNDLE_NAME = "TestApplication";
static constexpr const char *TEST_STORE_NAME = "TestStore";

class DeviceChangeListenerTest : public AppDeviceChangeListener {
public:
    void OnDeviceChanged(const DeviceInfo &info, const DeviceChangeType &type) const override;
    void OnSessionReady(const DeviceInfo &info, int32_t errCode) const override;
    void ResetReadyFlag();

    mutable int32_t bindResult_;
    mutable bool isReady_ = false;
    mutable DeviceInfo info_;
};

void DeviceChangeListenerTest::OnDeviceChanged(const DeviceInfo &info, const DeviceChangeType &type) const
{
    info_ = info;
}

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

class AppDataChangeListenerTest : public AppDataChangeListener {
public:
    void OnMessage(const DeviceInfo &info, const uint8_t *ptr, const int size, const PipeInfo &pipeInfo) const override
    {
    }
};

class SoftBusAdapterStandardTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() {}
    void TearDown();

    void ProcessBroadcastMsg(const std::string &device, const std::string &networkId, const LevelInfo &levelInfo);
    static void ConfigSendParameters(bool isCancel);

    mutable bool dataLevelResult_ = false;

    static std::shared_ptr<ExecutorPool> executors_;
    static std::shared_ptr<SoftBusAdapter> softBusAdapter_;
    static std::shared_ptr<ConnectManager> connectManager_;
    static DeviceChangeListenerTest deviceListener_;
    static AppDataChangeListenerTest dataChangeListener_;
    static PipeInfo pipeInfo_;
    static DeviceId deviceId_;
    static ExtraDataInfo extraInfo_;
    static DataInfo dataInfo_;
    static MessageInfo msgInfo_;
    static LevelInfo sendLevelInfo_;
    static LevelInfo receiveLevelInfo_;
    static std::string foregroundUserId_;
    static DeviceInfo localDeviceInfo_;
    static DeviceInfo remoteDeviceInfo_;
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
};

std::shared_ptr<ExecutorPool> SoftBusAdapterStandardTest::executors_;
std::shared_ptr<SoftBusAdapter> SoftBusAdapterStandardTest::softBusAdapter_;
std::shared_ptr<ConnectManager> SoftBusAdapterStandardTest::connectManager_;
DeviceChangeListenerTest SoftBusAdapterStandardTest::deviceListener_;
AppDataChangeListenerTest SoftBusAdapterStandardTest::dataChangeListener_;
PipeInfo SoftBusAdapterStandardTest::pipeInfo_;
DeviceId SoftBusAdapterStandardTest::deviceId_;
ExtraDataInfo SoftBusAdapterStandardTest::extraInfo_;
DataInfo SoftBusAdapterStandardTest::dataInfo_;
MessageInfo SoftBusAdapterStandardTest::msgInfo_;
LevelInfo SoftBusAdapterStandardTest::sendLevelInfo_;
LevelInfo SoftBusAdapterStandardTest::receiveLevelInfo_;
std::string SoftBusAdapterStandardTest::foregroundUserId_;
DeviceInfo SoftBusAdapterStandardTest::localDeviceInfo_;
DeviceInfo SoftBusAdapterStandardTest::remoteDeviceInfo_;
std::shared_ptr<DBStoreMock> SoftBusAdapterStandardTest::dbStoreMock_;

void SoftBusAdapterStandardTest::SetUpTestCase(void)
{
    connectManager_ = ConnectManager::GetInstance();
    softBusAdapter_ = SoftBusAdapter::GetInstance();

    CommunicatorContext::GetInstance().RegSessionListener(&deviceListener_);

    executors_ = std::make_shared<ExecutorPool>(THREAD_MAX, THREAD_MIN);
    CommunicatorContext::GetInstance().SetThreadPool(executors_);

    dbStoreMock_ = std::make_shared<DBStoreMock>();
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");

    localDeviceInfo_ = DeviceManagerAdapter::GetInstance().GetLocalDevice();
    auto remoteDeviceInfos = DeviceManagerAdapter::GetInstance().GetRemoteDevices();
    if (!remoteDeviceInfos.empty()) {
        remoteDeviceInfo_ = remoteDeviceInfos[0];
    }

    int userId = 0;
    AccountDelegate::GetInstance()->QueryForegroundUserId(userId);
    foregroundUserId_ = std::to_string(userId);
    std::shared_ptr<ExecutorPool> executors = std::make_shared<ExecutorPool>(2, 1);
    DeviceManagerAdapter::GetInstance().Init(executors);
}

void SoftBusAdapterStandardTest::TearDownTestCase(void)
{
    CommunicatorContext::GetInstance().UnRegSessionListener(&deviceListener_);
}

void SoftBusAdapterStandardTest::TearDown()
{
    softBusAdapter_->connects_.Clear();
    ConfigSendParameters(true);
}

void SoftBusAdapterStandardTest::ConfigSendParameters(bool isCancel)
{
    deviceId_.deviceId = isCancel ? "" : remoteDeviceInfo_.uuid;

    extraInfo_.userId = isCancel ? "" : foregroundUserId_;
    extraInfo_.bundleName = isCancel ? "" : TEST_BUNDLE_NAME;
    extraInfo_.storeId = isCancel ? "" : TEST_STORE_NAME;

    dataInfo_.extraInfo.userId = isCancel ? "" : foregroundUserId_;
    dataInfo_.extraInfo.appId = isCancel ? "" : TEST_BUNDLE_NAME;
    dataInfo_.extraInfo.storeId = isCancel ? "" : TEST_STORE_NAME;

    StoreMetaData localMetaData;
    localMetaData.deviceId = localDeviceInfo_.uuid;
    localMetaData.user = foregroundUserId_;
    localMetaData.bundleName = TEST_BUNDLE_NAME;
    localMetaData.storeId = TEST_STORE_NAME;

    StoreMetaData remoteMetaData;
    remoteMetaData.deviceId = remoteDeviceInfo_.uuid;
    remoteMetaData.user = foregroundUserId_;
    remoteMetaData.bundleName = TEST_BUNDLE_NAME;
    remoteMetaData.storeId = TEST_STORE_NAME;

    if (isCancel) {
        MetaDataManager::GetInstance().DelMeta(TEST_BUNDLE_NAME, true);
        MetaDataManager::GetInstance().DelMeta(localMetaData.GetKeyWithoutPath());
        MetaDataManager::GetInstance().DelMeta(remoteMetaData.GetKeyWithoutPath());
    } else {
        AppIDMetaData appIdMeta;
        appIdMeta.appId = TEST_BUNDLE_NAME;
        appIdMeta.bundleName = TEST_BUNDLE_NAME;
        MetaDataManager::GetInstance().SaveMeta(TEST_BUNDLE_NAME, appIdMeta, true);
        MetaDataManager::GetInstance().SaveMeta(localMetaData.GetKeyWithoutPath(), localMetaData);
        MetaDataManager::GetInstance().SaveMeta(remoteMetaData.GetKeyWithoutPath(), remoteMetaData);
    }
}

void SoftBusAdapterStandardTest::ProcessBroadcastMsg(const std::string &device, const std::string &networkId,
    const LevelInfo &levelInfo)
{
    (void)device;
    (void)networkId;
    receiveLevelInfo_ = levelInfo;
    dataLevelResult_ = true;
}

/**
* @tc.name: SessionServerTest001
* @tc.desc: create and remove session server
* @tc.type: FUNC
*/
HWTEST_F(SoftBusAdapterStandardTest, SessionServerTest001, TestSize.Level1)
{
    ConfigSocketId(INVALID_SOCKET);
    auto result = softBusAdapter_->CreateSessionServerAdapter("");
    ASSERT_NE(result, 0);

    ConfigSocketId(VALID_SOCKET);
    result = softBusAdapter_->CreateSessionServerAdapter("");
    ASSERT_EQ(result, 0);

    result = softBusAdapter_->RemoveSessionServerAdapter("");
    ASSERT_EQ(result, 0);
}

/**
* @tc.name: WatchDataChangeTest001
* @tc.desc: watch and unwatch data change
* @tc.type: FUNC
*/
HWTEST_F(SoftBusAdapterStandardTest, WatchDataChangeTest001, TestSize.Level1)
{
    auto status = softBusAdapter_->StartWatchDataChange(nullptr, pipeInfo_);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);

    status = softBusAdapter_->StartWatchDataChange(&dataChangeListener_, pipeInfo_);
    ASSERT_EQ(status, Status::SUCCESS);

    status = softBusAdapter_->StartWatchDataChange(&dataChangeListener_, pipeInfo_);
    ASSERT_EQ(status, Status::ERROR);

    status = softBusAdapter_->StopWatchDataChange(nullptr, pipeInfo_);
    ASSERT_EQ(status, Status::SUCCESS);

    status = softBusAdapter_->StopWatchDataChange(nullptr, pipeInfo_);
    ASSERT_EQ(status, Status::ERROR);
}

/**
* @tc.name: SendDataTest001
* @tc.desc: send data fail with invalid param
* @tc.type: FUNC
*/
HWTEST_F(SoftBusAdapterStandardTest, SendDataTest001, TestSize.Level1)
{
    auto result = softBusAdapter_->SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);

    deviceId_.deviceId = remoteDeviceInfo_.uuid;
    result = softBusAdapter_->SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);

    dataInfo_.extraInfo.userId = foregroundUserId_;
    result = softBusAdapter_->SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);

    dataInfo_.extraInfo.appId = TEST_BUNDLE_NAME;
    result = softBusAdapter_->SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);

    dataInfo_.extraInfo.storeId = TEST_STORE_NAME;
    result = softBusAdapter_->SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);

    AppIDMetaData appIdMeta;
    appIdMeta.appId = TEST_BUNDLE_NAME;
    appIdMeta.bundleName = TEST_BUNDLE_NAME;
    MetaDataManager::GetInstance().SaveMeta(TEST_BUNDLE_NAME, appIdMeta, true);

    result = softBusAdapter_->SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);
}

/**
* @tc.name: SendDataTest002
* @tc.desc: send data with bind fail
* @tc.type: FUNC
*/
HWTEST_F(SoftBusAdapterStandardTest, SendDataTest002, TestSize.Level1)
{
    ConfigSendParameters(false);
    ConfigSocketId(INVALID_MTU_SOCKET);

    auto result = softBusAdapter_->SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::RATE_LIMIT);
    while (!(deviceListener_.isReady_)) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    ASSERT_NE(deviceListener_.bindResult_, 0);

    deviceListener_.ResetReadyFlag();
}

/**
* @tc.name: SendDataTest003
* @tc.desc: send data with bind success
* @tc.type: FUNC
*/
HWTEST_F(SoftBusAdapterStandardTest, SendDataTest003, TestSize.Level1)
{
    ConfigSendParameters(false);
    ConfigSocketId(VALID_SOCKET);

    auto res = softBusAdapter_->CreateSessionServerAdapter("");
    ASSERT_EQ(res, 0);

    auto result = softBusAdapter_->SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::RATE_LIMIT);
    while (!(deviceListener_.isReady_)) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    ASSERT_EQ(deviceListener_.bindResult_, 0);

    uint32_t mtuBuffer;
    GetMtuSize(VALID_SOCKET, &mtuBuffer);
    auto mtuSize = softBusAdapter_->GetMtuSize(deviceId_);
    ASSERT_EQ(mtuSize, mtuBuffer);

    auto timeOut = softBusAdapter_->GetTimeout(deviceId_);
    ASSERT_NE(timeOut, 0);

    result = softBusAdapter_->SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::SUCCESS);

    std::this_thread::sleep_for(std::chrono::seconds(10));
    mtuSize = softBusAdapter_->GetMtuSize(deviceId_);
    ASSERT_NE(mtuSize, mtuBuffer);
}

/**
* @tc.name: ReuseConnectTest001
* @tc.desc: reuse connect fail with invalid parameter
* @tc.type: FUNC
*/
HWTEST_F(SoftBusAdapterStandardTest, ReuseConnectTest001, TestSize.Level1)
{
    auto result = softBusAdapter_->ReuseConnect(pipeInfo_, deviceId_, extraInfo_);
    ASSERT_EQ(result, Status::NOT_SUPPORT);

    deviceId_.deviceId = remoteDeviceInfo_.uuid;
    extraInfo_.userId = foregroundUserId_;
    result = softBusAdapter_->ReuseConnect(pipeInfo_, deviceId_, extraInfo_);
    ASSERT_EQ(result, Status::ERROR);

    extraInfo_.bundleName = TEST_BUNDLE_NAME;
    result = softBusAdapter_->ReuseConnect(pipeInfo_, deviceId_, extraInfo_);
    ASSERT_EQ(result, Status::ERROR);

    extraInfo_.storeId = TEST_STORE_NAME;
    result = softBusAdapter_->ReuseConnect(pipeInfo_, deviceId_, extraInfo_);
    ASSERT_EQ(result, Status::ERROR);
}

/**
* @tc.name: ReuseConnectTest002
* @tc.desc: reuse connect fail with bind fail
* @tc.type: FUNC
*/
HWTEST_F(SoftBusAdapterStandardTest, ReuseConnectTest002, TestSize.Level1)
{
    ConfigSendParameters(false);
    ConfigSocketId(INVALID_MTU_SOCKET);

    auto result = softBusAdapter_->ReuseConnect(pipeInfo_, deviceId_, extraInfo_);
    ASSERT_EQ(result, Status::NETWORK_ERROR);
}

/**
* @tc.name: ReuseConnectTest003
* @tc.desc: reuse connect success
* @tc.type: FUNC
*/
HWTEST_F(SoftBusAdapterStandardTest, ReuseConnectTest003, TestSize.Level1)
{
    ConfigSendParameters(false);
    ConfigSocketId(VALID_SOCKET);

    auto res = softBusAdapter_->CreateSessionServerAdapter("");
    ASSERT_EQ(res, 0);

    auto result = softBusAdapter_->ReuseConnect(pipeInfo_, deviceId_, extraInfo_);
    ASSERT_EQ(result, Status::SUCCESS);

    std::this_thread::sleep_for(std::chrono::seconds(10));
}

/**
* @tc.name: IsSameStartedOnPeerTest001
* @tc.desc: test IsSameStartedOnPeer
* @tc.type: FUNC
*/
HWTEST_F(SoftBusAdapterStandardTest, IsSameStartedOnPeerTest001, TestSize.Level1)
{
    auto result = softBusAdapter_->IsSameStartedOnPeer(pipeInfo_, deviceId_);
    ASSERT_TRUE(result);
}

/**
* @tc.name: ListenBroadcastTest001
* @tc.desc: listen broadcast test
* @tc.type: FUNC
*/
HWTEST_F(SoftBusAdapterStandardTest, ListenBroadcastTest001, TestSize.Level1)
{
    ConfigReturnCode(SoftBusErrNo::SOFTBUS_ERROR);
    auto status = softBusAdapter_->ListenBroadcastMsg(pipeInfo_, nullptr);
    ASSERT_NE(status, 0);

    ConfigReturnCode(SoftBusErrNo::SOFTBUS_OK);
    auto dataLevelListener = [this](const std::string &device, const std::string &networkId,
        const LevelInfo &levelInfo) {
        ProcessBroadcastMsg(device, networkId, levelInfo);
    };
    status = softBusAdapter_->ListenBroadcastMsg(pipeInfo_, dataLevelListener);
    ASSERT_EQ(status, 0);

    status = softBusAdapter_->ListenBroadcastMsg(pipeInfo_, dataLevelListener);
    ASSERT_NE(status, 0);
}

/**
* @tc.name: BroadcastTest001
* @tc.desc: broadcast msg fail
* @tc.type: FUNC
*/
HWTEST_F(SoftBusAdapterStandardTest, BroadcastTest001, TestSize.Level1)
{
    ConfigReturnCode(SoftBusErrNo::SOFTBUS_FUNC_NOT_SUPPORT);
    auto status = softBusAdapter_->Broadcast(pipeInfo_, sendLevelInfo_);
    ASSERT_EQ(status, Status::NOT_SUPPORT_BROADCAST);

    ConfigReturnCode(SoftBusErrNo::SOFTBUS_ERROR);
    status = softBusAdapter_->Broadcast(pipeInfo_, sendLevelInfo_);
    ASSERT_EQ(status, Status::ERROR);
}

/**
* @tc.name: BroadcastTest002
* @tc.desc: broadcast msg success and receive msg
* @tc.type: FUNC
*/
HWTEST_F(SoftBusAdapterStandardTest, BroadcastTest002, TestSize.Level1)
{
    auto dataLevelListener = [this](const std::string &device, const std::string &networkId,
        const LevelInfo &levelInfo) {
        ProcessBroadcastMsg(device, networkId, levelInfo);
    };
    ConfigReturnCode(SoftBusErrNo::SOFTBUS_OK);
    (void)softBusAdapter_->ListenBroadcastMsg(pipeInfo_, dataLevelListener);

    sendLevelInfo_.dynamic = DYNAMIC_LEVEL;
    sendLevelInfo_.statics = STATIC_LEVEL;
    sendLevelInfo_.switches = SWITCH_VALUE;
    sendLevelInfo_.switchesLen = SWITCH_LENGTH;

    auto status = softBusAdapter_->Broadcast(pipeInfo_, sendLevelInfo_);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_EQ(receiveLevelInfo_.dynamic, sendLevelInfo_.dynamic);
    ASSERT_EQ(receiveLevelInfo_.statics, sendLevelInfo_.statics);
    ASSERT_EQ(receiveLevelInfo_.switches, sendLevelInfo_.switches);
    ASSERT_EQ(receiveLevelInfo_.switchesLen, sendLevelInfo_.switchesLen);
}

/**
 * @tc.name: GetDeviceInfo001
 * @tc.desc: test when networkId is cloudNetworkId GetDeviceInfo and OnDeviceOnline
 * @tc.type: FUNC
 */
HWTEST_F(SoftBusAdapterStandardTest, GetDeviceInfo001, TestSize.Level1)
{
    DeviceChangeListenerTest listener;
    DeviceManagerAdapter::GetInstance().StartWatchDeviceChange(&listener, {});
    DistributedHardware::DmDeviceInfo info = {"cloudDeviceId", "cloudDeviceName", 14, "cloudNetworkId", 0};
    DistributedHardware::DeviceManager::GetInstance().Online(info);
    ASSERT_EQ(listener.info_.networkId, info.networkId);
    ASSERT_EQ(listener.info_.deviceName, info.deviceName);
}

/**
 * @tc.name: GetDeviceInfo002
 * @tc.desc: test when networkId is empty GetDeviceInfo and OnDeviceOnline
 * @tc.type: FUNC
 */
HWTEST_F(SoftBusAdapterStandardTest, GetDeviceInfo002, TestSize.Level1)
{
    DeviceChangeListenerTest listener;
    DeviceManagerAdapter::GetInstance().StartWatchDeviceChange(&listener, {});
    DistributedHardware::DmDeviceInfo info = {"cloudDeviceId", "cloudDeviceName", 0, "", 0};
    DistributedHardware::DeviceManager::GetInstance().Online(info);
    ASSERT_EQ(listener.info_.networkId, info.networkId);
}

/**
 * @tc.name: GetDeviceInfo003
 * @tc.desc: test when networkId is cloudNetworkId GetDeviceInfo and Offline
 * @tc.type: FUNC
 */
HWTEST_F(SoftBusAdapterStandardTest, GetDeviceInfo003, TestSize.Level1)
{
    DeviceChangeListenerTest listener;
    DeviceManagerAdapter::GetInstance().StartWatchDeviceChange(&listener, {});
    DistributedHardware::DmDeviceInfo info = {"", "", 109, "local_network_id", 0};
    DistributedHardware::DeviceManager::GetInstance().Offline(info);
    ASSERT_EQ(listener.info_.networkId, "");
}

/**
 * @tc.name: GetDeviceInfo004
 * @tc.desc: test when networkId is empty GetDeviceInfo and Offline
 * @tc.type: FUNC
 */
HWTEST_F(SoftBusAdapterStandardTest, GetDeviceInfo004, TestSize.Level1)
{
    DeviceChangeListenerTest listener;
    DeviceManagerAdapter::GetInstance().StartWatchDeviceChange(&listener, {});
    DistributedHardware::DmDeviceInfo info = {"", "localDeviceName", 131, "local_network_id", 0};
    DistributedHardware::DeviceManager::GetInstance().Offline(info);
    ASSERT_EQ(listener.info_.networkId, "");
}

/**
 * @tc.name: GetDeviceInfo005
 * @tc.desc: test when networkId is cloudNetworkId GetDeviceInfo and OnChanged
 * @tc.type: FUNC
 */
HWTEST_F(SoftBusAdapterStandardTest, GetDeviceInfo005, TestSize.Level1)
{
    DeviceChangeListenerTest listener;
    DeviceManagerAdapter::GetInstance().StartWatchDeviceChange(&listener, {});
    DistributedHardware::DmDeviceInfo info = {"cloudDeviceId", "cloudDeviceName", 2607, "cloudNetworkId", 0};
    DistributedHardware::DeviceManager::GetInstance().OnChanged(info);
    ASSERT_EQ(listener.info_.networkId, "");
}

/**
 * @tc.name: GetDeviceInfo006
 * @tc.desc: test when networkId is empty GetDeviceInfo and OnChanged
 * @tc.type: FUNC
 */
HWTEST_F(SoftBusAdapterStandardTest, GetDeviceInfo006, TestSize.Level1)
{
    DeviceChangeListenerTest listener;
    DeviceManagerAdapter::GetInstance().StartWatchDeviceChange(&listener, {});
    DistributedHardware::DmDeviceInfo info = {"localDeviceId", "", 17, "local_network_id", 0};
    DistributedHardware::DeviceManager::GetInstance().OnChanged(info);
    ASSERT_EQ(listener.info_.networkId, "");
}

/**
 * @tc.name: GetDeviceInfo007
 * @tc.desc: test GetDeviceInfo when networkId is empty GetDeviceInfo and OnReady
 * @tc.type: FUNC
 */
HWTEST_F(SoftBusAdapterStandardTest, GetDeviceInfo007, TestSize.Level1)
{
    DeviceChangeListenerTest listener;
    DeviceManagerAdapter::GetInstance().StartWatchDeviceChange(&listener, {});
    DistributedHardware::DmDeviceInfo info = {"", "", 0, "local_network_id", 0};
    DistributedHardware::DeviceManager::GetInstance().OnReady(info);
    ASSERT_EQ(listener.info_.networkId, "");
}
} // namespace OHOS::Test