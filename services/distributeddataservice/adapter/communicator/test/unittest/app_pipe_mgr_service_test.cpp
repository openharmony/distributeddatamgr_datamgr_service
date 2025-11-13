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

#include "app_pipe_mgr.h"

#include <gtest/gtest.h>
#include "kvstore_utils.h"
#include "socket.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::AppDistributedKv;

static constexpr uint32_t INVALID_TRANSFER_SIZE = 5 * 1024 * 1024;
static constexpr uint32_t VALID_TRANSFER_SIZE = 1024;
static constexpr uint32_t DATA_LENGTH = 8;
static constexpr uint32_t TOKEN_ID = 1111;
static constexpr const char *PIPE_ID = "AppPipeMgrServiceTest";
static constexpr const char *DEVICE_ID = "deviceId";
static constexpr const char *DATA_INFO = "DataInfo";
static constexpr const char *USER_ID = "100";
static constexpr const char *BUNDLE_NAME = "bundleName";
static constexpr const char *STORE_ID = "storeId";

class AppDataChangeListenerMock : public AppDataChangeListener {
    void OnMessage(const DeviceInfo &info, const uint8_t *ptr, const int size,
        const struct PipeInfo &id) const override;
};

void AppDataChangeListenerMock::OnMessage(const DeviceInfo &info,
    const uint8_t *ptr, const int size, const struct PipeInfo &id) const
{
    (void)info;
    (void)ptr;
    (void)size;
    (void)id;
    return;
}

class AppPipeMgrServiceTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown();

    static PipeInfo pipeInfo_;
    static DeviceId deviceId_;
    static DataInfo dataInfo_;
    static MessageInfo msgInfo_;
    static ExtraDataInfo extraInfo_;
};

PipeInfo AppPipeMgrServiceTest::pipeInfo_;
DeviceId AppPipeMgrServiceTest::deviceId_;
DataInfo AppPipeMgrServiceTest::dataInfo_;
MessageInfo AppPipeMgrServiceTest::msgInfo_;
ExtraDataInfo AppPipeMgrServiceTest::extraInfo_;

void AppPipeMgrServiceTest::TearDown()
{
    pipeInfo_.pipeId = "";
    deviceId_.deviceId = "";
    dataInfo_.length = 0;
    dataInfo_.data = nullptr;
    extraInfo_.userId = "";
    extraInfo_.bundleName = "";
    extraInfo_.storeId = "";
    dataInfo_.extraInfo = extraInfo_;
}

/**
* @tc.name: StartWatchDataChangeTest001
* @tc.desc: StartWatchDataChange test
* @tc.type: FUNC
*/
HWTEST_F(AppPipeMgrServiceTest, StartWatchDataChangeTest001, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    auto status = appPipeMgr.StartWatchDataChange(nullptr, pipeInfo_);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);

    AppDataChangeListenerMock* dataChangeListener = new AppDataChangeListenerMock();
    status = appPipeMgr.StartWatchDataChange(dataChangeListener, pipeInfo_);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);

    pipeInfo_.pipeId = PIPE_ID;
    status = appPipeMgr.StartWatchDataChange(dataChangeListener, pipeInfo_);
    ASSERT_EQ(status, Status::ERROR);

    appPipeMgr.dataBusMap_.insert({ PIPE_ID, nullptr });
    status = appPipeMgr.StartWatchDataChange(dataChangeListener, pipeInfo_);
    ASSERT_EQ(status, Status::ERROR);

    appPipeMgr.dataBusMap_.erase(PIPE_ID);
    appPipeMgr.dataBusMap_.insert({ PIPE_ID, std::make_shared<AppPipeHandler>(pipeInfo_) });
    status = appPipeMgr.StartWatchDataChange(dataChangeListener, pipeInfo_);
    ASSERT_EQ(status, Status::SUCCESS);

    delete dataChangeListener;
}

/**
* @tc.name: StopWatchDataChangeTest001
* @tc.desc: StopWatchDataChange test
* @tc.type: FUNC
*/
HWTEST_F(AppPipeMgrServiceTest, StopWatchDataChangeTest001, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    auto status = appPipeMgr.StopWatchDataChange(nullptr, pipeInfo_);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);

    AppDataChangeListenerMock* dataChangeListener = new AppDataChangeListenerMock();
    status = appPipeMgr.StopWatchDataChange(dataChangeListener, pipeInfo_);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);

    pipeInfo_.pipeId = PIPE_ID;
    status = appPipeMgr.StopWatchDataChange(dataChangeListener, pipeInfo_);
    ASSERT_EQ(status, Status::ERROR);

    appPipeMgr.dataBusMap_.insert({ PIPE_ID, nullptr });
    status = appPipeMgr.StopWatchDataChange(dataChangeListener, pipeInfo_);
    ASSERT_EQ(status, Status::ERROR);

    appPipeMgr.dataBusMap_.erase(PIPE_ID);
    appPipeMgr.dataBusMap_.insert({ PIPE_ID, std::make_shared<AppPipeHandler>(pipeInfo_) });
    status = appPipeMgr.StopWatchDataChange(dataChangeListener, pipeInfo_);
    ASSERT_EQ(status, Status::SUCCESS);

    delete dataChangeListener;
}

/**
* @tc.name: IsSameStartedOnPeerTest001
* @tc.desc: IsSameStartedOnPeer test
* @tc.type: FUNC
*/
HWTEST_F(AppPipeMgrServiceTest, IsSameStartedOnPeerTest001, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    auto result = appPipeMgr.IsSameStartedOnPeer(pipeInfo_, deviceId_);
    appPipeMgr.SetMessageTransFlag(pipeInfo_, true);
    ASSERT_FALSE(result);

    pipeInfo_.pipeId = PIPE_ID;
    result = appPipeMgr.IsSameStartedOnPeer(pipeInfo_, deviceId_);
    ASSERT_FALSE(result);

    deviceId_.deviceId = DEVICE_ID;
    result = appPipeMgr.IsSameStartedOnPeer(pipeInfo_, deviceId_);
    appPipeMgr.SetMessageTransFlag(pipeInfo_, true);
    ASSERT_FALSE(result);

    appPipeMgr.dataBusMap_.insert({ PIPE_ID, nullptr });
    result = appPipeMgr.IsSameStartedOnPeer(pipeInfo_, deviceId_);
    appPipeMgr.SetMessageTransFlag(pipeInfo_, true);
    ASSERT_FALSE(result);

    appPipeMgr.dataBusMap_.erase(PIPE_ID);
    appPipeMgr.dataBusMap_.insert({ PIPE_ID, std::make_shared<AppPipeHandler>(pipeInfo_) });
    result = appPipeMgr.IsSameStartedOnPeer(pipeInfo_, deviceId_);
    appPipeMgr.SetMessageTransFlag(pipeInfo_, true);
    ASSERT_TRUE(result);
}

/**
* @tc.name: StartTest001
* @tc.desc: test start function with all branch
* @tc.type: FUNC
*/
HWTEST_F(AppPipeMgrServiceTest, StartTest001, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    auto status = appPipeMgr.Start(pipeInfo_);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);

    pipeInfo_.pipeId = PIPE_ID;
    ConfigSocketId(INVALID_SOCKET);
    status = appPipeMgr.Start(pipeInfo_);
    ASSERT_EQ(status, Status::ILLEGAL_STATE);

    ConfigSocketId(VALID_SOCKET);
    status = appPipeMgr.Start(pipeInfo_);
    ASSERT_EQ(status, Status::SUCCESS);

    status = appPipeMgr.Start(pipeInfo_);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: StopTest001
* @tc.desc: test stop function with all branch
* @tc.type: FUNC
*/
HWTEST_F(AppPipeMgrServiceTest, StopTest001, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    pipeInfo_.pipeId = PIPE_ID;
    auto status = appPipeMgr.Stop(pipeInfo_);
    ASSERT_EQ(status, Status::KEY_NOT_FOUND);

    ConfigSocketId(VALID_SOCKET);
    status = appPipeMgr.Start(pipeInfo_);
    ASSERT_EQ(status, Status::SUCCESS);

    status = appPipeMgr.Stop(pipeInfo_);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: SendDataTest001
* @tc.desc: send data fail with all exception branch
* @tc.type: FUNC
*/
HWTEST_F(AppPipeMgrServiceTest, SendDataTest001, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    auto result = appPipeMgr.SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);

    pipeInfo_.pipeId = PIPE_ID;
    result = appPipeMgr.SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);

    deviceId_.deviceId = DEVICE_ID;
    result = appPipeMgr.SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);

    dataInfo_.length = 0;
    result = appPipeMgr.SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);

    dataInfo_.length = INVALID_TRANSFER_SIZE;
    result = appPipeMgr.SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);

    dataInfo_.length = VALID_TRANSFER_SIZE;
    dataInfo_.data = nullptr;
    result = appPipeMgr.SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);

    dataInfo_.length = DATA_LENGTH;
    dataInfo_.data = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(DATA_INFO));
    result = appPipeMgr.SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);

    extraInfo_.userId = USER_ID;
    dataInfo_.extraInfo = extraInfo_;
    result = appPipeMgr.SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);

    extraInfo_.appId = BUNDLE_NAME;
    dataInfo_.extraInfo = extraInfo_;
    result = appPipeMgr.SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);

    extraInfo_.storeId = STORE_ID;
    dataInfo_.extraInfo = extraInfo_;
    result = appPipeMgr.SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::KEY_NOT_FOUND);
}

/**
* @tc.name: SendDataTest002
* @tc.desc: send data trigger success
* @tc.type: FUNC
*/
HWTEST_F(AppPipeMgrServiceTest, SendDataTest002, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    pipeInfo_.pipeId = PIPE_ID;

    ConfigSocketId(VALID_SOCKET);
    auto status = appPipeMgr.Start(pipeInfo_);
    ASSERT_EQ(status, Status::SUCCESS);

    deviceId_.deviceId = DEVICE_ID;
    dataInfo_.length = DATA_LENGTH;
    dataInfo_.data = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(DATA_INFO));
    extraInfo_.userId = USER_ID;
    extraInfo_.appId = BUNDLE_NAME;
    extraInfo_.storeId = STORE_ID;
    dataInfo_.extraInfo = extraInfo_;
    auto result = appPipeMgr.SendData(pipeInfo_, deviceId_, dataInfo_, 0, msgInfo_);
    ASSERT_EQ(result.first, Status::ERROR);

    status = appPipeMgr.Stop(pipeInfo_);
    ASSERT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: ReuseConnectTest001
* @tc.desc: reuse connect fail with all exception branch
* @tc.type: FUNC
*/
HWTEST_F(AppPipeMgrServiceTest, ReuseConnectTest001, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    auto status = appPipeMgr.ReuseConnect(pipeInfo_, deviceId_, extraInfo_);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);

    pipeInfo_.pipeId = PIPE_ID;
    status = appPipeMgr.ReuseConnect(pipeInfo_, deviceId_, extraInfo_);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);

    deviceId_.deviceId = DEVICE_ID;
    status = appPipeMgr.ReuseConnect(pipeInfo_, deviceId_, extraInfo_);
    ASSERT_EQ(status, Status::ERROR);

    extraInfo_.userId = USER_ID;
    status = appPipeMgr.ReuseConnect(pipeInfo_, deviceId_, extraInfo_);
    ASSERT_EQ(status, Status::ERROR);

    extraInfo_.bundleName = BUNDLE_NAME;
    status = appPipeMgr.ReuseConnect(pipeInfo_, deviceId_, extraInfo_);
    ASSERT_EQ(status, Status::ERROR);

    extraInfo_.storeId = STORE_ID;
    status = appPipeMgr.ReuseConnect(pipeInfo_, deviceId_, extraInfo_);
    ASSERT_EQ(status, Status::ERROR);

    extraInfo_.tokenId = TOKEN_ID;
    status = appPipeMgr.ReuseConnect(pipeInfo_, deviceId_, extraInfo_);
    ASSERT_EQ(status, Status::KEY_NOT_FOUND);
}

/**
* @tc.name: ReuseConnectTest002
* @tc.desc: reuse connect trigger success
* @tc.type: FUNC
*/
HWTEST_F(AppPipeMgrServiceTest, ReuseConnectTest002, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    pipeInfo_.pipeId = PIPE_ID;

    ConfigSocketId(VALID_SOCKET);
    auto status = appPipeMgr.Start(pipeInfo_);
    ASSERT_EQ(status, Status::SUCCESS);

    deviceId_.deviceId = DEVICE_ID;
    extraInfo_.userId = USER_ID;
    extraInfo_.bundleName = BUNDLE_NAME;
    extraInfo_.storeId = STORE_ID;
    extraInfo_.tokenId = TOKEN_ID;
    status = appPipeMgr.ReuseConnect(pipeInfo_, deviceId_, extraInfo_);
    ASSERT_EQ(status, Status::NOT_SUPPORT);

    status = appPipeMgr.Stop(pipeInfo_);
    ASSERT_EQ(status, Status::SUCCESS);
}
} // namespace OHOS::Test