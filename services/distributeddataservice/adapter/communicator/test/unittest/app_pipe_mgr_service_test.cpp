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
#define LOG_TAG "AppPipeMgrServiceTest"

#include "app_pipe_mgr.h"
#include <gtest/gtest.h>
#include "kvstore_utils.h"
#include "reporter.h"
#include "types.h"
#include "log_print.h"

namespace OHOS::Test {
using namespace testing::ext;
using namespace OHOS::AppDistributedKv;
class AppDataChangeListenerMock : public AppDataChangeListener {
    void OnMessage(const DeviceInfo &info, const uint8_t *ptr, const int size,
                   const struct PipeInfo &id) const override;
};

void AppDataChangeListenerMock::OnMessage(const DeviceInfo &info,
    const uint8_t *ptr, const int size, const struct PipeInfo &id) const
{
    return;
}

class AppPipeMgrServiceTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

/**
* @tc.name: StopWatchDataChange001
* @tc.desc: StopWatchDataChange test
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
 */
HWTEST_F(AppPipeMgrServiceTest, StopWatchDataChange001, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    PipeInfo pipeInfo;
    const AppDataChangeListener* observer = nullptr;
    auto status = appPipeMgr.StopWatchDataChange(observer, pipeInfo);
    EXPECT_EQ(Status::INVALID_ARGUMENT, status);

    pipeInfo.pipeId = "pipeId";
    pipeInfo.userId = "userId";
    status = appPipeMgr.StopWatchDataChange(observer, pipeInfo);
    EXPECT_EQ(Status::INVALID_ARGUMENT, status);

    PipeInfo pipeInfoNull;
    const AppDataChangeListener* observerListener = new AppDataChangeListenerMock();
    status = appPipeMgr.StopWatchDataChange(observerListener, pipeInfoNull);
    EXPECT_EQ(Status::INVALID_ARGUMENT, status);
}

/**
* @tc.name: StopWatchDataChange002
* @tc.desc: StopWatchDataChange test
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
 */
HWTEST_F(AppPipeMgrServiceTest, StopWatchDataChange002, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    PipeInfo pipeInfo;
    const AppDataChangeListener* observer = new AppDataChangeListenerMock();
    pipeInfo.pipeId = "pipeId";
    pipeInfo.userId = "userId";
    auto status = appPipeMgr.StopWatchDataChange(observer, pipeInfo);
    EXPECT_EQ(Status::ERROR, status);

    auto handler = std::make_shared<AppPipeHandler>(pipeInfo);
    std::map<std::string, std::shared_ptr<AppPipeHandler>> dataBusMap = {{"pipeId", handler}};
    appPipeMgr.dataBusMap_ = dataBusMap;
    status = appPipeMgr.StopWatchDataChange(observer, pipeInfo);
    EXPECT_EQ(Status::ERROR, status);
}

/**
* @tc.name: SendData001
* @tc.desc: SendData test
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
 */
HWTEST_F(AppPipeMgrServiceTest, SendData001, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    PipeInfo pipeInfo;
    pipeInfo.pipeId = "pipeId";
    pipeInfo.userId = "userId";
    DeviceId deviceId = {"deviceId"};
    std::string content = "Helloworlds";
    const uint8_t *t = reinterpret_cast<const uint8_t*>(content.c_str());
    DataInfo dataInfo = { const_cast<uint8_t *>(t), static_cast<uint32_t>(content.length())};
    uint32_t totalLength = 0;
    MessageInfo info;
    auto handler = std::make_shared<AppPipeHandler>(pipeInfo);
    std::map<std::string, std::shared_ptr<AppPipeHandler>> dataBusMap = {{"pipeId", handler}};
    appPipeMgr.dataBusMap_ = dataBusMap;
    auto status = appPipeMgr.SendData(pipeInfo, deviceId, dataInfo, totalLength, info);
    EXPECT_EQ(std::make_pair(Status::RATE_LIMIT, 0), status);
}

/**
* @tc.name: Start001
* @tc.desc: Start test
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
 */
HWTEST_F(AppPipeMgrServiceTest, Start001, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    PipeInfo pipeInfo;
    pipeInfo.pipeId = "pipeId";
    pipeInfo.userId = "userId";
    auto handler = std::make_shared<AppPipeHandler>(pipeInfo);
    std::map<std::string, std::shared_ptr<AppPipeHandler>> dataBusMap = {{"pipeId", handler}};
    appPipeMgr.dataBusMap_ = dataBusMap;
    auto status = appPipeMgr.Start(pipeInfo);
    EXPECT_EQ(Status::SUCCESS, status);
}

/**
* @tc.name: Stop001
* @tc.desc: Stop test
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
 */
HWTEST_F(AppPipeMgrServiceTest, Stop001, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    PipeInfo pipeInfo;
    pipeInfo.pipeId = "pipeId";
    pipeInfo.userId = "userId";
    auto status = appPipeMgr.Stop(pipeInfo);
    EXPECT_EQ(Status::KEY_NOT_FOUND, status);

    auto handler = std::make_shared<AppPipeHandler>(pipeInfo);
    std::map<std::string, std::shared_ptr<AppPipeHandler>> dataBusMap = {{"pipeId", handler}};
    appPipeMgr.dataBusMap_ = dataBusMap;
    status = appPipeMgr.Stop(pipeInfo);
    EXPECT_EQ(Status::SUCCESS, status);
}

/**
* @tc.name: IsSameStartedOnPeer001
* @tc.desc: IsSameStartedOnPeer test
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
 */
HWTEST_F(AppPipeMgrServiceTest, IsSameStartedOnPeer001, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    PipeInfo pipeInfo;
    DeviceId deviceId;
    auto status = appPipeMgr.IsSameStartedOnPeer(pipeInfo, deviceId);
    EXPECT_EQ(false, status);

    pipeInfo.pipeId = "pipeId";
    pipeInfo.userId = "userId";
    status = appPipeMgr.IsSameStartedOnPeer(pipeInfo, deviceId);
    EXPECT_EQ(false, status);

    PipeInfo pipeInfoNull;
    deviceId.deviceId = "deviceId";
    status = appPipeMgr.IsSameStartedOnPeer(pipeInfoNull, deviceId);
    EXPECT_EQ(false, status);
}

/**
* @tc.name: IsSameStartedOnPeer002
* @tc.desc: IsSameStartedOnPeer test
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
 */
HWTEST_F(AppPipeMgrServiceTest, IsSameStartedOnPeer002, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    PipeInfo pipeInfo;
    DeviceId deviceId;
    pipeInfo.pipeId = "pipeId";
    pipeInfo.userId = "userId";
    deviceId.deviceId = "deviceId";
    auto status = appPipeMgr.IsSameStartedOnPeer(pipeInfo, deviceId);
    EXPECT_EQ(false, status);

    auto handler = std::make_shared<AppPipeHandler>(pipeInfo);
    std::map<std::string, std::shared_ptr<AppPipeHandler>> dataBusMap = {{"pipeId", handler}};
    appPipeMgr.dataBusMap_ = dataBusMap;
    status = appPipeMgr.IsSameStartedOnPeer(pipeInfo, deviceId);
    EXPECT_EQ(true, status);
}

/**
* @tc.name: SetMessageTransFlag001
* @tc.desc: SetMessageTransFlag test
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
 */
HWTEST_F(AppPipeMgrServiceTest, SetMessageTransFlag001, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    PipeInfo pipeInfo;
    bool flag = true;
    EXPECT_NO_FATAL_FAILURE(
        appPipeMgr.SetMessageTransFlag(pipeInfo, flag));

    pipeInfo.pipeId = "pipeId";
    pipeInfo.userId = "userId";
    auto handler = std::make_shared<AppPipeHandler>(pipeInfo);
    std::map<std::string, std::shared_ptr<AppPipeHandler>> dataBusMap = {{"pipeId", handler}};
    appPipeMgr.dataBusMap_ = dataBusMap;
    EXPECT_NO_FATAL_FAILURE(
        appPipeMgr.SetMessageTransFlag(pipeInfo, flag));
}

/**
* @tc.name: ReuseConnect001
* @tc.desc: ReuseConnect test
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
 */
HWTEST_F(AppPipeMgrServiceTest, ReuseConnect001, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    PipeInfo pipeInfo;
    DeviceId deviceId;
    auto status = appPipeMgr.ReuseConnect(pipeInfo, deviceId);
    EXPECT_EQ(Status::INVALID_ARGUMENT, status);

    pipeInfo.pipeId = "pipeId";
    pipeInfo.userId = "userId";
    status = appPipeMgr.ReuseConnect(pipeInfo, deviceId);
    EXPECT_EQ(Status::INVALID_ARGUMENT, status);

    PipeInfo pipeInfoNull;
    deviceId.deviceId = "deviceId";
    status = appPipeMgr.ReuseConnect(pipeInfoNull, deviceId);
    EXPECT_EQ(Status::INVALID_ARGUMENT, status);
}

/**
* @tc.name: ReuseConnect002
* @tc.desc: ReuseConnect test
* @tc.type: FUNC
* @tc.require:
* @tc.author: suoqilong
 */
HWTEST_F(AppPipeMgrServiceTest, ReuseConnect002, TestSize.Level0)
{
    AppPipeMgr appPipeMgr;
    PipeInfo pipeInfo;
    DeviceId deviceId;
    pipeInfo.pipeId = "pipeId";
    pipeInfo.userId = "userId";
    deviceId.deviceId = "deviceId";
    auto status = appPipeMgr.ReuseConnect(pipeInfo, deviceId);
    EXPECT_EQ(Status::INVALID_ARGUMENT, status);

    auto handler = std::make_shared<AppPipeHandler>(pipeInfo);
    std::map<std::string, std::shared_ptr<AppPipeHandler>> dataBusMap = {{"pipeId", handler}};
    appPipeMgr.dataBusMap_ = dataBusMap;
    status = appPipeMgr.ReuseConnect(pipeInfo, deviceId);
    EXPECT_EQ(Status::NOT_SUPPORT, status);
}
} // namespace OHOS::Test