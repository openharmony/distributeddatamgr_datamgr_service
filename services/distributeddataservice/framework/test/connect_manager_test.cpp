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

#include "communication/connect_manager.h"

#include <gtest/gtest.h>

using namespace testing::ext;
using namespace OHOS::AppDistributedKv;

class ConnectManagerTest : public testing::Test {};

/**
* @tc.name: RegisterInstance
* @tc.desc: RegisterInstance.
* @tc.type: FUNC
*/
HWTEST_F(ConnectManagerTest, RegisterInstance, TestSize.Level1)
{
    auto result = ConnectManager::RegisterInstance(std::make_shared<ConnectManager>());
    ASSERT_TRUE(result);
    ASSERT_NE(ConnectManager::GetInstance(), nullptr);
}

/**
* @tc.name: CloseSession
* @tc.desc: CloseSession.
* @tc.type: FUNC
*/
HWTEST_F(ConnectManagerTest, CloseSession, TestSize.Level1)
{
    auto ret = ConnectManager::GetInstance()->CloseSession("");
    ASSERT_FALSE(ret);
    ConnectManager::GetInstance()->closeSessionTask_ = [](const std::string &networkId) {
        return true;
    };
    ret = ConnectManager::GetInstance()->CloseSession("");
    ASSERT_FALSE(ret);
}

/**
* @tc.name: RegisterCloseSessionTask
* @tc.desc: RegisterCloseSessionTask.
* @tc.type: FUNC
*/
HWTEST_F(ConnectManagerTest, RegisterCloseSessionTask, TestSize.Level1)
{
    auto ret = ConnectManager::GetInstance()->RegisterCloseSessionTask([](const std::string &networkId) {
        return true;
    });
    ASSERT_FALSE(ret);
}