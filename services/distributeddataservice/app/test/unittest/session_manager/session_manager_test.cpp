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

#include <gtest/gtest.h>

#include "gmock/gmock.h"
#include "route_head_handler_impl.h"
#include "upgrade_manager.h"
#include "user_delegate.h"

namespace OHOS::DistributedData {
using namespace testing::ext;
class SessionManagerTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        // init peer device
        UserMetaData userMetaData;
        userMetaData.deviceId = "PEER_DEVICE_ID";

        UserStatus status;
        status.isActive = true;
        constexpr const int MOCK_PEER_USER = 101;
        status.id = MOCK_PEER_USER;
        userMetaData.users = { status };
        UserDelegate::GetInstance();

        CapMetaData capMetaData;
        capMetaData.version = CapMetaData::CURRENT_VERSION;
        UpgradeManager::GetInstance().Init();
    }
    static void TearDownTestCase()
    {
    }
    void SetUp()
    {
    }
    void TearDown()
    {
    }
};

/**
* @tc.name: PackAndUnPack01
* @tc.desc: test get db dir
* @tc.type: FUNC
* @tc.require:
* @tc.author: illybyy
*/
HWTEST_F(SessionManagerTest, PackAndUnPack01, TestSize.Level2)
{
    const DistributedDB::ExtendInfo info = {
        .userId = "100", .appId = "com.sample.helloworld", .storeId = "test_store", .dstTarget = "PEER_DEVICE_ID"
    };
    auto sendHandler = RouteHeadHandlerImpl::Create(info);
    ASSERT_NE(sendHandler, nullptr);
    uint32_t routeHeadSize = 0;
    auto result = sendHandler->GetHeadDataSize(routeHeadSize);
    ASSERT_GT(routeHeadSize, 0);
    std::unique_ptr<uint8_t> data = std::make_unique<uint8_t>(routeHeadSize);
    sendHandler->FillHeadData(data.get(), routeHeadSize, routeHeadSize);

    std::vector<std::string> users;
    auto recvHandler = RouteHeadHandlerImpl::Create({});
    uint32_t parseSize = 0;
    recvHandler->ParseHeadData(data.get(), routeHeadSize, parseSize, users);
    EXPECT_EQ(routeHeadSize, parseSize);
    EXPECT_EQ(users.size(), 1);
    EXPECT_EQ(users[0], "101");
}
} // namespace OHOS::DistributedData