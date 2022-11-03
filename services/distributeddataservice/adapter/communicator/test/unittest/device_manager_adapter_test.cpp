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

#include "device_manager_adapter.h"

#include "gtest/gtest.h"
#include "types.h"
using namespace testing::ext;
using namespace OHOS::AppDistributedKv;
using namespace OHOS::DistributedData;
class DeviceChangerListener final : public AppDeviceChangeListener {
public:
    void OnDeviceChanged(const DeviceInfo &info, const DeviceChangeType &type) const override
    {
    }
    ChangeLevelType GetChangeLevelType() const override
    {
        return ChangeLevelType::MIN;
    }
};

class DeviceManagerAdapterTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};

protected:
    static std::shared_ptr<DeviceChangerListener> observer_;
};
std::shared_ptr<DeviceChangerListener> DeviceManagerAdapterTest::observer_;
static const uint32_t LOCAL_DEVICE_ID_NUM = 3;
static const uint32_t LOCAL_UUID_NUM = 1;
constexpr const char *INVALID_DEVICE_ID = "1234567890";
constexpr const char *EMPTY_DEVICE_ID = "";

/**
* @tc.name: StartWatchDeviceChange
* @tc.desc: start watch device change
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, StartWatchDeviceChange, TestSize.Level0)
{
    auto status = DeviceManagerAdapter::GetInstance().StartWatchDeviceChange(nullptr, {});
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    observer_ = std::make_shared<DeviceChangerListener>();
    status = DeviceManagerAdapter::GetInstance().StartWatchDeviceChange(observer_.get(), {});
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: StopWatchDeviceChange001
* @tc.desc: stop watch device change
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, StopWatchDeviceChange001, TestSize.Level0)
{
    auto status = DeviceManagerAdapter::GetInstance().StopWatchDeviceChange(nullptr, {});
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    status = DeviceManagerAdapter::GetInstance().StopWatchDeviceChange(observer_.get(), {});
    EXPECT_EQ(status, Status::SUCCESS);
    observer_ = nullptr;
}

/**
* @tc.name: StopWatchDeviceChange002
* @tc.desc: stop watch device change, the observer is not register
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, StopWatchDeviceChange002, TestSize.Level0)
{
    std::shared_ptr<DeviceChangerListener> observer = std::make_shared<DeviceChangerListener>();
    auto status = DeviceManagerAdapter::GetInstance().StopWatchDeviceChange(observer.get(), {});
    EXPECT_EQ(status, Status::ERROR);
}

/**
* @tc.name: GetLocalDevice
* @tc.desc: get local device
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, GetLocalDevice, TestSize.Level0)
{
    auto dvInfo = DeviceManagerAdapter::GetInstance().GetLocalDevice();
    EXPECT_FALSE(dvInfo.uuid.empty());
    EXPECT_FALSE(dvInfo.udid.empty());
    EXPECT_FALSE(dvInfo.networkId.empty());
}

/**
* @tc.name: GetRemoteDevices
* @tc.desc: get remote device
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, GetRemoteDevices, TestSize.Level0)
{
    auto dvInfos = DeviceManagerAdapter::GetInstance().GetRemoteDevices();
    EXPECT_TRUE(dvInfos.empty());
}

/**
* @tc.name: GetDeviceInfo001
* @tc.desc: get device info, the id is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, GetDeviceInfo001, TestSize.Level0)
{
    auto dvInfo = DeviceManagerAdapter::GetInstance().GetDeviceInfo(EMPTY_DEVICE_ID);
    EXPECT_TRUE(dvInfo.udid.empty());
    dvInfo = DeviceManagerAdapter::GetInstance().GetDeviceInfo(INVALID_DEVICE_ID);
    EXPECT_TRUE(dvInfo.udid.empty());
}

/**
* @tc.name: GetDeviceInfo002
* @tc.desc: get device info, the id is local deviceId
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, GetDeviceInfo002, TestSize.Level0)
{
    auto localDvInfo = DeviceManagerAdapter::GetInstance().GetLocalDevice();
    auto uuidToDVInfo = DeviceManagerAdapter::GetInstance().GetDeviceInfo(localDvInfo.uuid);
    EXPECT_EQ(localDvInfo.udid, uuidToDVInfo.udid);
    auto udidToDVInfo = DeviceManagerAdapter::GetInstance().GetDeviceInfo(localDvInfo.udid);
    EXPECT_EQ(localDvInfo.uuid, udidToDVInfo.uuid);
    auto networkIdToDVInfo = DeviceManagerAdapter::GetInstance().GetDeviceInfo(localDvInfo.networkId);
    EXPECT_EQ(localDvInfo.udid, networkIdToDVInfo.udid);
}

/**
* @tc.name: GetUuidByNetworkId001
* @tc.desc: get uuid by networkId, the networkId is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, GetUuidByNetworkId001, TestSize.Level0)
{
    auto uuid = DeviceManagerAdapter::GetInstance().GetUuidByNetworkId(EMPTY_DEVICE_ID);
    EXPECT_TRUE(uuid.empty());
    uuid = DeviceManagerAdapter::GetInstance().GetUuidByNetworkId(INVALID_DEVICE_ID);
    EXPECT_TRUE(uuid.empty());
}

/**
* @tc.name: GetUuidByNetworkId002
* @tc.desc: get uuid by networkId, the networkId is local networkId
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, GetUuidByNetworkId002, TestSize.Level0)
{
    auto dvInfo = DeviceManagerAdapter::GetInstance().GetLocalDevice();
    auto uuid = DeviceManagerAdapter::GetInstance().GetUuidByNetworkId(dvInfo.networkId);
    EXPECT_EQ(uuid, dvInfo.uuid);
}

/**
* @tc.name: GetUdidByNetworkId001
* @tc.desc: get udid by networkId, the networkId is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, GetUdidByNetworkId001, TestSize.Level0)
{
    auto udid = DeviceManagerAdapter::GetInstance().GetUdidByNetworkId(EMPTY_DEVICE_ID);
    EXPECT_TRUE(udid.empty());
    udid = DeviceManagerAdapter::GetInstance().GetUdidByNetworkId(INVALID_DEVICE_ID);
    EXPECT_TRUE(udid.empty());
}

/**
* @tc.name: GetUdidByNetworkId002
* @tc.desc: get udid by networkId, the networkId is local networkId
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, GetUdidByNetworkId002, TestSize.Level0)
{
    auto dvInfo = DeviceManagerAdapter::GetInstance().GetLocalDevice();
    auto udid = DeviceManagerAdapter::GetInstance().GetUdidByNetworkId(dvInfo.networkId);
    EXPECT_EQ(udid, dvInfo.udid);
}

/**
* @tc.name: GetLocalBasicInfo
* @tc.desc: get local basic info
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, GetLocalBasicInfo, TestSize.Level0)
{
    auto info = DeviceManagerAdapter::GetInstance().GetLocalBasicInfo();
    EXPECT_FALSE(info.uuid.empty());
    EXPECT_FALSE(info.udid.empty());
    EXPECT_FALSE(info.networkId.empty());
}

/**
* @tc.name: DeviceIdToUUID001
* @tc.desc: transfer deviceId to uuid, the deviceId is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, DeviceIdToUUID001, TestSize.Level0)
{
    auto uuid = DeviceManagerAdapter::GetInstance().ToUUID(EMPTY_DEVICE_ID);
    EXPECT_TRUE(uuid.empty());
    uuid = DeviceManagerAdapter::GetInstance().ToUUID(INVALID_DEVICE_ID);
    EXPECT_TRUE(uuid.empty());
}

/**
* @tc.name: DeviceIdToUUID002
* @tc.desc: transfer deviceId to uuid, the deviceId is local deviceId
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, DeviceIdToUUID002, TestSize.Level0)
{
    auto dvInfo = DeviceManagerAdapter::GetInstance().GetLocalDevice();
    auto uuidToUuid = DeviceManagerAdapter::GetInstance().ToUUID(dvInfo.uuid);
    EXPECT_EQ(uuidToUuid, dvInfo.uuid);
    auto udidToUuid = DeviceManagerAdapter::GetInstance().ToUUID(dvInfo.udid);
    EXPECT_EQ(udidToUuid, dvInfo.uuid);
    auto networkIdToUuid = DeviceManagerAdapter::GetInstance().ToUUID(dvInfo.networkId);
    EXPECT_EQ(networkIdToUuid, dvInfo.uuid);
}

/**
* @tc.name: DeviceIdToUUID003
* @tc.desc: transfer deviceIds to uuid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, DeviceIdToUUID003, TestSize.Level0)
{
    std::vector<std::string> devices;
    devices.emplace_back(EMPTY_DEVICE_ID);
    devices.emplace_back(INVALID_DEVICE_ID);
    auto dvInfo = DeviceManagerAdapter::GetInstance().GetLocalDevice();
    devices.emplace_back(dvInfo.uuid);
    devices.emplace_back(dvInfo.udid);
    devices.emplace_back(dvInfo.networkId);
    auto uuids = DeviceManagerAdapter::GetInstance().ToUUID(devices);
    EXPECT_EQ(uuids.size(), LOCAL_DEVICE_ID_NUM);
    for (const auto &uuid : uuids) {
        EXPECT_EQ(uuid, dvInfo.uuid);
    }
}

/**
* @tc.name: DeviceIdToUUID004
* @tc.desc: transfer deviceIds to uuid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, DeviceIdToUUID004, TestSize.Level0)
{
    std::vector<DeviceInfo> devices;
    DeviceInfo dvInfo;
    devices.emplace_back(dvInfo);
    dvInfo.uuid = INVALID_DEVICE_ID;
    devices.emplace_back(dvInfo);
    dvInfo = DeviceManagerAdapter::GetInstance().GetLocalDevice();
    devices.emplace_back(dvInfo);
    auto uuids = DeviceManagerAdapter::GetInstance().ToUUID(devices);
    EXPECT_EQ(uuids.size(), LOCAL_UUID_NUM);
    for (const auto &uuid : uuids) {
        EXPECT_EQ(uuid, dvInfo.uuid);
    }
}

/**
* @tc.name: DeviceIdToNetworkId001
* @tc.desc: transfer deviceId to networkId, the deviceId is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, DeviceIdToNetworkId001, TestSize.Level0)
{
    auto networkId = DeviceManagerAdapter::GetInstance().ToNetworkID(EMPTY_DEVICE_ID);
    EXPECT_TRUE(networkId.empty());
    networkId = DeviceManagerAdapter::GetInstance().ToNetworkID(INVALID_DEVICE_ID);
    EXPECT_TRUE(networkId.empty());
}

/**
* @tc.name: DeviceIdToNetworkId002
* @tc.desc: transfer deviceId to networkId, the deviceId is local deviceId
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, DeviceIdToNetworkId002, TestSize.Level0)
{
    auto dvInfo = DeviceManagerAdapter::GetInstance().GetLocalDevice();
    auto uuidToNetworkId = DeviceManagerAdapter::GetInstance().ToNetworkID(dvInfo.uuid);
    EXPECT_EQ(uuidToNetworkId, dvInfo.networkId);
    auto udidToNetworkId = DeviceManagerAdapter::GetInstance().ToNetworkID(dvInfo.udid);
    EXPECT_EQ(udidToNetworkId, dvInfo.networkId);
    auto networkIdToNetworkId = DeviceManagerAdapter::GetInstance().ToNetworkID(dvInfo.networkId);
    EXPECT_EQ(networkIdToNetworkId, dvInfo.networkId);
}


/**
* @tc.name: NotifyReadyEvent
* @tc.desc: notify ready event, the uuid is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
*/
HWTEST_F(DeviceManagerAdapterTest, NotifyReadyEvent, TestSize.Level0)
{
    DeviceManagerAdapter::GetInstance().NotifyReadyEvent(INVALID_DEVICE_ID);
    DeviceManagerAdapter::GetInstance().NotifyReadyEvent(EMPTY_DEVICE_ID);
    auto dvInfo = DeviceManagerAdapter::GetInstance().GetLocalDevice();
    DeviceManagerAdapter::GetInstance().NotifyReadyEvent(dvInfo.uuid);
}