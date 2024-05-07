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

#define LOG_TAG "CommunicationProviderImplTest"

#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include <unistd.h>
#include <iostream>
#include "app_device_change_listener.h"
#include "communication_provider.h"
#include "log_print.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::AppDistributedKv;
class CommunicationProviderImplTest : public testing::Test {
};

class AppDataChangeListenerImpl : public AppDataChangeListener {
    void OnMessage(const OHOS::AppDistributedKv::DeviceInfo &info, const uint8_t *ptr, const int size,
                   const struct PipeInfo &id) const override;
};
void AppDataChangeListenerImpl::OnMessage(const OHOS::AppDistributedKv::DeviceInfo &info,
    const uint8_t *ptr, const int size, const struct PipeInfo &id) const
{
    ZLOGI("data  %{public}s  %s", info.deviceName.c_str(), ptr);
}

/**
* @tc.name: CommunicationProvider001
* @tc.desc: Verify getting KvStore
* @tc.type: FUNC
* @tc.require: AR000CCPQ1 AR000CQDVE
* @tc.author: hongbo
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider001, TestSize.Level1)
{
    ZLOGI("begin.");
    const AppDataChangeListenerImpl* dataListener = new AppDataChangeListenerImpl();
    PipeInfo appId;
    appId.pipeId = "appId";
    appId.userId = "groupId";
    CommunicationProvider::GetInstance().StartWatchDataChange(dataListener, appId);
    auto secRegister = CommunicationProvider::GetInstance().StartWatchDataChange(dataListener, appId);
    EXPECT_EQ(Status::ERROR, secRegister);
    sleep(1); // avoid thread dnet thread died, then will have pthread;
}

/**
* @tc.name: CommunicationProvider002
* @tc.desc: Verify stop watch device change
* @tc.type: FUNC
* @tc.require: AR000CCPQ2 AR000CQS3F
* @tc.author: hongbo
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider002, TestSize.Level1)
{
    ZLOGD("CommunicationProvider004");
    const AppDataChangeListenerImpl* dataListener = new AppDataChangeListenerImpl();
    PipeInfo appId;
    appId.pipeId = "appId";
    appId.userId = "groupId";
    auto secRegister = CommunicationProvider::GetInstance().StopWatchDataChange(dataListener, appId);
    ZLOGD("CommunicationProvider004 %d", static_cast<int>(secRegister));
    EXPECT_EQ(Status::ERROR, secRegister);
    sleep(1); // avoid thread dnet thread died, then will have pthread;
}

/**
* @tc.name: CommunicationProvider003
* @tc.desc: close pipe
* @tc.type: FUNC
* @tc.require: AR000CCPQ2
* @tc.author: hongbo
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider003, TestSize.Level1)
{
    ZLOGI("CommunicationProvider015 ");
    PipeInfo appId;
    appId.pipeId = "appId";
    appId.userId = "groupId";
    auto status = CommunicationProvider::GetInstance().Stop(appId);
    EXPECT_NE(Status::ERROR, status);
    sleep(1); // avoid thread dnet thread died, then will have pthread;
}

/**
* @tc.name: CommunicationProvider004
* @tc.desc: singleton pipe
* @tc.type: FUNC
* @tc.require: AR000CCPQ2
* @tc.author: hongbo
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider004, TestSize.Level1)
{
    ZLOGI("begin.");
    auto &provider = CommunicationProvider::GetInstance();
    auto &provider1 = CommunicationProvider::GetInstance();
    EXPECT_EQ(&provider, &provider1);
    sleep(1); // avoid thread dnet thread died, then will have pthread;
}

/**
* @tc.name: CommunicationProvider005
* @tc.desc: parse sent data
* @tc.type: FUNC
* @tc.require: AR000CCPQ2 AR000CQS3M AR000CQSAI
* @tc.author: hongbo
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider005, TestSize.Level1)
{
    const AppDataChangeListenerImpl *dataListener17 = new AppDataChangeListenerImpl();
    PipeInfo id17;
    id17.pipeId = "appId";
    id17.userId = "groupId";
    CommunicationProvider::GetInstance().StartWatchDataChange(dataListener17, id17);
    CommunicationProvider::GetInstance().Start(id17);
    std::string content = "Helloworlds";
    const uint8_t *t = reinterpret_cast<const uint8_t*>(content.c_str());
    DeviceId di17 = {"127.0.0.2"};
    DataInfo data = { const_cast<uint8_t *>(t), static_cast<uint32_t>(content.length())};
    Status status = CommunicationProvider::GetInstance().SendData(id17, di17, data, 0);
    EXPECT_NE(status, Status::SUCCESS);
    CommunicationProvider::GetInstance().StopWatchDataChange(dataListener17, id17);
    CommunicationProvider::GetInstance().Stop(id17);
    delete dataListener17;
    sleep(1); // avoid thread dnet thread died, then will have pthread;
}

/**
* @tc.name: CommunicationProvider006
* @tc.desc:the observer is nullptr
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider006, TestSize.Level1)
{
    ZLOGI("begin.");
    PipeInfo appId;
    appId.pipeId = "appId06";
    appId.userId = "groupId06";
    CommunicationProvider::GetInstance().StartWatchDataChange(nullptr, appId);
    auto secRegister = CommunicationProvider::GetInstance().StartWatchDataChange(nullptr, appId);
    EXPECT_EQ(Status::INVALID_ARGUMENT, secRegister);
}

/**
* @tc.name: CommunicationProvider007
* @tc.desc:the pipeInfo is nullptr
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider007, TestSize.Level1)
{
    ZLOGI("begin.");
    auto secRegister = CommunicationProvider::GetInstance().StartWatchDataChange(nullptr, {});
    EXPECT_EQ(Status::INVALID_ARGUMENT, secRegister);
}

/**
* @tc.name: CommunicationProvider008
* @tc.desc:the observer is nullptr
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider008, TestSize.Level1)
{
    ZLOGI("begin.");
    PipeInfo appId;
    appId.pipeId = "appId06";
    appId.userId = "groupId06";
    CommunicationProvider::GetInstance().StopWatchDataChange(nullptr, appId);
    auto secRegister = CommunicationProvider::GetInstance().StopWatchDataChange(nullptr, appId);
    EXPECT_EQ(Status::INVALID_ARGUMENT, secRegister);
}

/**
* @tc.name: CommunicationProvider009
* @tc.desc: the pipeInfo is nullptr
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider009, TestSize.Level1)
{
    ZLOGI("begin.");
    auto secRegister = CommunicationProvider::GetInstance().StopWatchDataChange(nullptr, {});
    EXPECT_EQ(Status::INVALID_ARGUMENT, secRegister);
}

/**
* @tc.name: CommunicationProvider010
* @tc.desc: Start pipe
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider010, TestSize.Level1)
{
    PipeInfo appId;
    appId.pipeId = "";
    appId.userId = "groupId";
    auto status = CommunicationProvider::GetInstance().Start(appId);
    EXPECT_EQ(Status::INVALID_ARGUMENT, status);
}

/**
* @tc.name: CommunicationProvider011
* @tc.desc: parse sent data
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider011, TestSize.Level1)
{
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    PipeInfo id;
    id.pipeId = "appId";
    id.userId = "groupId";
    CommunicationProvider::GetInstance().StartWatchDataChange(dataListener, id);
    CommunicationProvider::GetInstance().Start(id);
    std::string content = "";
    const uint8_t *t = reinterpret_cast<const uint8_t*>(content.c_str());
    DeviceId di = {"DeviceId"};
    DataInfo data = { const_cast<uint8_t *>(t), static_cast<uint32_t>(content.length())};
    Status status = CommunicationProvider::GetInstance().SendData(id, di, data, 0);
    EXPECT_EQ(status, Status::ERROR);
    CommunicationProvider::GetInstance().StopWatchDataChange(dataListener, id);
    CommunicationProvider::GetInstance().Stop(id);
    delete dataListener;
}

/**
* @tc.name: CommunicationProvider012
* @tc.desc: parse sent data
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider012, TestSize.Level1)
{
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    PipeInfo id;
    id.pipeId = "appId";
    id.userId = "groupId";
    CommunicationProvider::GetInstance().StartWatchDataChange(dataListener, id);
    CommunicationProvider::GetInstance().Start(id);
    std::string content = "hello";
    const uint8_t *t = reinterpret_cast<const uint8_t*>(content.c_str());
    DeviceId di = {""};
    DataInfo data = { const_cast<uint8_t *>(t), static_cast<uint32_t>(content.length())};
    Status status = CommunicationProvider::GetInstance().SendData(id, di, data, 0);
    EXPECT_EQ(status, Status::ERROR);
    CommunicationProvider::GetInstance().StopWatchDataChange(dataListener, id);
    CommunicationProvider::GetInstance().Stop(id);
    delete dataListener;
}

/**
* @tc.name: CommunicationProvider013
* @tc.desc: parse sent data
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider013, TestSize.Level1)
{
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    PipeInfo id;
    id.pipeId = "appId";
    id.userId = "groupId";
    CommunicationProvider::GetInstance().StartWatchDataChange(dataListener, id);
    CommunicationProvider::GetInstance().Start(id);
    DeviceId di = {"DeviceId"};
    DataInfo data = {nullptr, 0};
    Status status = CommunicationProvider::GetInstance().SendData(id, di, data, 0);
    EXPECT_EQ(status, Status::ERROR);
    CommunicationProvider::GetInstance().StopWatchDataChange(dataListener, id);
    CommunicationProvider::GetInstance().Stop(id);
    delete dataListener;
}

/**
* @tc.name: CommunicationProvider014
* @tc.desc: parse sent data
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider014, TestSize.Level1)
{
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    PipeInfo id;
    id.pipeId = "";
    id.userId = "groupId";
    CommunicationProvider::GetInstance().StartWatchDataChange(dataListener, id);
    CommunicationProvider::GetInstance().Start(id);
    std::string content = "hello";
    const uint8_t *t = reinterpret_cast<const uint8_t*>(content.c_str());
    DeviceId di = {"DeviceId"};
    DataInfo data = { const_cast<uint8_t *>(t), static_cast<uint32_t>(content.length())};
    Status status = CommunicationProvider::GetInstance().SendData(id, di, data, 0);
    EXPECT_EQ(status, Status::ERROR);
    CommunicationProvider::GetInstance().StopWatchDataChange(dataListener, id);
    CommunicationProvider::GetInstance().Stop(id);
    delete dataListener;
}

/**
* @tc.name: CommunicationProvider015
* @tc.desc: parse sent data
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider015, TestSize.Level1)
{
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    PipeInfo id;
    id.pipeId = "";
    id.userId = "groupId";
    CommunicationProvider::GetInstance().StartWatchDataChange(dataListener, id);
    CommunicationProvider::GetInstance().Start(id);
    DeviceId di = {"DeviceId"};
    auto status = CommunicationProvider::GetInstance().IsSameStartedOnPeer(id, di);
    EXPECT_EQ(status, false);
    CommunicationProvider::GetInstance().StopWatchDataChange(dataListener, id);
    CommunicationProvider::GetInstance().Stop(id);
    delete dataListener;
}

/**
* @tc.name: CommunicationProvider016
* @tc.desc: IsSameStartedOnPeer
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider016, TestSize.Level1)
{
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    PipeInfo id;
    id.pipeId = "pipeid";
    id.userId = "userId";
    CommunicationProvider::GetInstance().StartWatchDataChange(dataListener, id);
    CommunicationProvider::GetInstance().Start(id);
    DeviceId di = {""};
    auto status = CommunicationProvider::GetInstance().IsSameStartedOnPeer(id, di);
    EXPECT_EQ(status, false);
    CommunicationProvider::GetInstance().StopWatchDataChange(dataListener, id);
    CommunicationProvider::GetInstance().Stop(id);
    delete dataListener;
}

/**
* @tc.name: CommunicationProvider017
* @tc.desc: SetMessageTransFlag
* @tc.type: FUNC
* @tc.author: nhj
*/
HWTEST_F(CommunicationProviderImplTest, CommunicationProvider017, TestSize.Level1)
{
    const AppDataChangeListenerImpl *dataListener = new AppDataChangeListenerImpl();
    PipeInfo id;
    id.pipeId = "pipeid";
    id.userId = "userId";
    CommunicationProvider::GetInstance().StartWatchDataChange(dataListener, id);
    CommunicationProvider::GetInstance().Start(id);
    DeviceId di = {"DeviceId"};
    auto status = CommunicationProvider::GetInstance().IsSameStartedOnPeer(id, di);
    EXPECT_EQ(status, false);
    CommunicationProvider::GetInstance().SetMessageTransFlag(id, true);
    CommunicationProvider::GetInstance().StopWatchDataChange(dataListener, id);
    CommunicationProvider::GetInstance().Stop(id);
    delete dataListener;
}