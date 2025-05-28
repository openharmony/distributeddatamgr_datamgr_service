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

#include "upgrade_manager.h"

#include <gtest/gtest.h>

#include "device_manager_adapter.h"
#include "metadata/meta_data_manager.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedData;

static constexpr size_t THREAD_MIN = 0;
static constexpr size_t THREAD_MAX = 2;
static constexpr const char* REMOTE_DEVICE = "0123456789ABCDEF";

class UpgradeManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};

    static std::shared_ptr<ExecutorPool> executors_;
    static std::string localDevice_;
};

std::shared_ptr<ExecutorPool> UpgradeManagerTest::executors_;
std::string UpgradeManagerTest::localDevice_;

void UpgradeManagerTest::SetUpTestCase(void)
{
    localDevice_ = DeviceManagerAdapter::GetInstance().GetLocalDevice().uuid;
    executors_ = std::make_shared<ExecutorPool>(THREAD_MAX, THREAD_MIN);
    MetaDataManager::GetInstance().Init(executors_);
    UpgradeManager::GetInstance().Init(executors_);
    sleep(1);
}

/**
* @tc.name: GetCapabilityTest001
* @tc.desc: get local device capability
* @tc.type: FUNC
*/
HWTEST_F(UpgradeManagerTest, GetCapabilityTest001, TestSize.Level1)
{
    bool result = false;
    auto capMeta = UpgradeManager::GetInstance().GetCapability(localDevice_, result);
    ASSERT_EQ(result, true);
    ASSERT_EQ(capMeta.version, CapMetaData::CURRENT_VERSION);
    ASSERT_EQ(capMeta.deviceId, localDevice_);
}

/**
* @tc.name: GetCapabilityTest002
* @tc.desc: get non-exist remote device capability
* @tc.type: FUNC
*/
HWTEST_F(UpgradeManagerTest, GetCapabilityTest002, TestSize.Level1)
{
    bool result = true;
    auto capMeta = UpgradeManager::GetInstance().GetCapability(REMOTE_DEVICE, result);
    ASSERT_EQ(result, false);
    ASSERT_EQ(capMeta.version, CapMetaData::INVALID_VERSION);
    ASSERT_EQ(capMeta.deviceId, "");
}

/**
* @tc.name: GetCapabilityTest003
* @tc.desc: get remote device capability by callback of meta save-update-delete
* @tc.type: FUNC
*/
HWTEST_F(UpgradeManagerTest, GetCapabilityTest003, TestSize.Level1)
{
    CapMetaData capMetaData;
    capMetaData.version = CapMetaData::CURRENT_VERSION;
    capMetaData.deviceId = REMOTE_DEVICE;
    auto capKey = CapMetaRow::GetKeyFor(REMOTE_DEVICE);
    (void)MetaDataManager::GetInstance().SaveMeta({ capKey.begin(), capKey.end() }, capMetaData);
    sleep(1);

    bool result = false;
    auto capMeta = UpgradeManager::GetInstance().GetCapability(REMOTE_DEVICE, result);
    ASSERT_EQ(result, true);
    ASSERT_EQ(capMeta.version, capMetaData.version);
    ASSERT_EQ(capMeta.deviceId, REMOTE_DEVICE);

    capMetaData.version = CapMetaData::INVALID_VERSION;
    (void)MetaDataManager::GetInstance().UpdateMeta({ capKey.begin(), capKey.end() }, capMetaData);
    sleep(1);

    result = false;
    capMeta = UpgradeManager::GetInstance().GetCapability(REMOTE_DEVICE, result);
    ASSERT_EQ(result, true);
    ASSERT_EQ(capMeta.version, capMetaData.version);
    ASSERT_EQ(capMeta.deviceId, REMOTE_DEVICE);

    (void)MetaDataManager::GetInstance().DeleteMeta({ capKey.begin(), capKey.end() }, capMetaData);
    sleep(1);

    capMeta = UpgradeManager::GetInstance().GetCapability(REMOTE_DEVICE, result);
    ASSERT_EQ(result, false);
    ASSERT_EQ(capMeta.version, CapMetaData::INVALID_VERSION);
    ASSERT_EQ(capMeta.deviceId, "");
}

/**
* @tc.name: GetCapabilityTest004
* @tc.desc: get remote device capability by meta store
* @tc.type: FUNC
*/
HWTEST_F(UpgradeManagerTest, GetCapabilityTest004, TestSize.Level1)
{
    MetaDataManager::GetInstance().UnSubscribe();
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
}