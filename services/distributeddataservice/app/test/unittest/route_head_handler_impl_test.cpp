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

#include "route_head_handler_impl.h"

#include "upgrade_manager.h"

#include <gtest/gtest.h>

#include "device_manager_adapter.h"
#include "metadata/meta_data_manager.h"
using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedKv;
using namespace DistributedDB;

static constexpr size_t THREAD_MIN = 0;
static constexpr size_t THREAD_MAX = 2;
static constexpr const char* REMOTE_DEVICE = "0123456789ABCDEF";

class RouteHeadHandlerImplTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};

    static std::shared_ptr<ExecutorPool> executors_;
    static std::string localDevice_;
};
static inline std::shared_ptr<DeviceManagerAdapterMock> deviceManagerAdapterMock = nullptr;
std::shared_ptr<ExecutorPool> RouteHeadHandlerImplTest::executors_;
std::string RouteHeadHandlerImplTest::localDevice_;

void RouteHeadHandlerImplTest::SetUpTestCase(void)
{
    localDevice_ = DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    executors_ = std::make_shared<ExecutorPool>(THREAD_MAX, THREAD_MIN);
    MetaDataManager::GetInstance().Init(executors_);
    UpgradeManager::GetInstance().Init(executors_);
    deviceManagerAdapterMock = std::make_shared<DeviceManagerAdapterMock>();
    BDeviceManagerAdapter::deviceManagerAdapter = deviceManagerAdapterMock;
    sleep(1);
}

/**
* @tc.name: GetHeadDataSize_Test1
* @tc.desc: get headSize for udmf store
* @tc.type: FUNC
*/
HWTEST_F(RouteHeadHandlerImplTest, GetHeadDataSize_Test1, TestSize.Level0)
{
    DeviceInfo deviceInfo;
    deviceInfo.osType = OH_OS_TYPE;
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*deviceManagerAdapterMock, GetDeviceInfo(_)).WillRepeatedly(Return(deviceInfo));

    const DistributedDB::ExtendInfo info = {
        .appId = "otherAppId", .storeId = "test_store", .userId = "100", .dstTarget = REMOTE_DEVICE
    };
    auto sendHandler = RouteHeadHandlerImpl::Create(info);
    ASSERT_NE(sendHandler, nullptr);

    CapMetaData capMetaData;
    capMetaData.version = CapMetaData::CURRENT_VERSION;
    capMetaData.deviceId = REMOTE_DEVICE;
    auto capKey = CapMetaRow::GetKeyFor(REMOTE_DEVICE);
    (void)MetaDataManager::GetInstance().SaveMeta({ capKey.begin(), capKey.end() }, capMetaData);

    bool result = false;
    auto capMeta = UpgradeManager::GetInstance().GetCapability(REMOTE_DEVICE, result);
    ASSERT_EQ(result, true);
    ASSERT_EQ(capMeta.version, capMetaData.version);
    ASSERT_EQ(capMeta.deviceId, REMOTE_DEVICE);

    uint32_t headSize = 0;
    EXPECT_EQ(sendHandler->GetHeadDataSize(headSize), DBStatus::OK);
    EXPECT_NE(headSize, 0);

}
