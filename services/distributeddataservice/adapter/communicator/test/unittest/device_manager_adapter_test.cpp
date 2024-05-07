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
#include "accesstoken_kit.h"
#include "executor_pool.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "types.h"
namespace {
using namespace testing::ext;
using namespace OHOS::AppDistributedKv;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedHardware;
using namespace OHOS::Security::AccessToken;
using DmDeviceInfo =  OHOS::DistributedHardware::DmDeviceInfo;
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

void GrantPermissionNative()
{
    const char **perms = new const char *[2];
    perms[0] = "ohos.permission.DISTRIBUTED_DATASYNC";
    perms[1] = "ohos.permission.ACCESS_SERVICE_DM";
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 2,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "distributed_data_test",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    AccessTokenKit::ReloadNativeTokenInfo();
    delete []perms;
}

class DeviceManagerAdapterTest : public testing::Test {
public:
  static void SetUpTestCase(void)
    {
        size_t max = 12;
        size_t min = 5;
        GrantPermissionNative();
        DeviceManagerAdapter::GetInstance().Init(std::make_shared<OHOS::ExecutorPool>(max, min));
    }
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}

protected:
    static std::shared_ptr<DeviceChangerListener> observer_;
    static const std::string INVALID_DEVICE_ID;
    static const std::string EMPTY_DEVICE_ID;
    static const uint32_t LOCAL_DEVICE_ID_NUM;
    static const uint32_t LOCAL_UUID_NUM;
};
std::shared_ptr<DeviceChangerListener> DeviceManagerAdapterTest::observer_;
const std::string DeviceManagerAdapterTest::INVALID_DEVICE_ID = "1234567890";
const std::string DeviceManagerAdapterTest::EMPTY_DEVICE_ID = "";
const uint32_t DeviceManagerAdapterTest::LOCAL_DEVICE_ID_NUM = 3;
const uint32_t DeviceManagerAdapterTest::LOCAL_UUID_NUM = 2;

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
    status = DeviceManagerAdapter::GetInstance().StartWatchDeviceChange(observer_.get(), {});
    EXPECT_EQ(status, Status::ERROR);
}

/**
* @tc.name: StopWatchDeviceChange
* @tc.desc: stop watch device change, the observer is nullptr
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
 */
HWTEST_F(DeviceManagerAdapterTest, StopWatchDeviceChange, TestSize.Level0)
{
    auto status = DeviceManagerAdapter::GetInstance().StopWatchDeviceChange(nullptr, {});
    EXPECT_EQ(status, Status::INVALID_ARGUMENT);
    status = DeviceManagerAdapter::GetInstance().StopWatchDeviceChange(observer_.get(), {});
    EXPECT_EQ(status, Status::SUCCESS);
    status = DeviceManagerAdapter::GetInstance().StopWatchDeviceChange(observer_.get(), {});
    EXPECT_EQ(status, Status::ERROR);
    observer_ = nullptr;
}

/**
* @tc.name: StopWatchDeviceChange002
* @tc.desc: stop watch device change, the observer is not register
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
 */
HWTEST_F(DeviceManagerAdapterTest, StopWatchDeviceChangeNotRegister, TestSize.Level0)
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
* @tc.name: GetDeviceInfo
* @tc.desc: get device info, the id is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
 */
HWTEST_F(DeviceManagerAdapterTest, GetDeviceInfoInvalidId, TestSize.Level0)
{
    auto dvInfo = DeviceManagerAdapter::GetInstance().GetDeviceInfo(EMPTY_DEVICE_ID);
    EXPECT_TRUE(dvInfo.udid.empty());
    dvInfo = DeviceManagerAdapter::GetInstance().GetDeviceInfo(INVALID_DEVICE_ID);
    EXPECT_TRUE(dvInfo.udid.empty());
}

/**
* @tc.name: GetDeviceInfo
* @tc.desc: get device info, the id is local deviceId
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
 */
HWTEST_F(DeviceManagerAdapterTest, GetDeviceInfoLocalId, TestSize.Level0)
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
* @tc.name: GetUuidByNetworkId
* @tc.desc: get uuid by networkId, the networkId is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
 */
HWTEST_F(DeviceManagerAdapterTest, GetUuidByNetworkIdInvalid, TestSize.Level0)
{
    auto uuid = DeviceManagerAdapter::GetInstance().GetUuidByNetworkId(EMPTY_DEVICE_ID);
    EXPECT_TRUE(uuid.empty());
    uuid = DeviceManagerAdapter::GetInstance().GetUuidByNetworkId(INVALID_DEVICE_ID);
    EXPECT_TRUE(uuid.empty());
}

/**
* @tc.name: GetUuidByNetworkId
* @tc.desc: get uuid by networkId, the networkId is local networkId
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
 */
HWTEST_F(DeviceManagerAdapterTest, GetUuidByNetworkIdLocal, TestSize.Level0)
{
    auto dvInfo = DeviceManagerAdapter::GetInstance().GetLocalDevice();
    auto uuid = DeviceManagerAdapter::GetInstance().GetUuidByNetworkId(dvInfo.networkId);
    EXPECT_EQ(uuid, dvInfo.uuid);
}

/**
* @tc.name: GetUdidByNetworkId
* @tc.desc: get udid by networkId, the networkId is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
 */
HWTEST_F(DeviceManagerAdapterTest, GetUdidByNetworkIdInvalid, TestSize.Level0)
{
    auto udid = DeviceManagerAdapter::GetInstance().GetUdidByNetworkId(EMPTY_DEVICE_ID);
    EXPECT_TRUE(udid.empty());
    udid = DeviceManagerAdapter::GetInstance().GetUdidByNetworkId(INVALID_DEVICE_ID);
    EXPECT_TRUE(udid.empty());
}

/**
* @tc.name: GetUdidByNetworkId
* @tc.desc: get udid by networkId, the networkId is local networkId
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
 */
HWTEST_F(DeviceManagerAdapterTest, GetUdidByNetworkIdLocal, TestSize.Level0)
{
    auto dvInfo = DeviceManagerAdapter::GetInstance().GetLocalDevice();
    auto udid = DeviceManagerAdapter::GetInstance().GetUdidByNetworkId(dvInfo.networkId);
    EXPECT_EQ(udid, dvInfo.udid);
}

/**
* @tc.name: DeviceIdToUUID
* @tc.desc: transfer deviceId to uuid, the deviceId is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
 */
HWTEST_F(DeviceManagerAdapterTest, DeviceIdToUUIDInvalid, TestSize.Level0)
{
    auto uuid = DeviceManagerAdapter::GetInstance().ToUUID(EMPTY_DEVICE_ID);
    EXPECT_TRUE(uuid.empty());
    uuid = DeviceManagerAdapter::GetInstance().ToUUID(INVALID_DEVICE_ID);
    EXPECT_TRUE(uuid.empty());
}

/**
* @tc.name: DeviceIdToUUID
* @tc.desc: transfer deviceId to uuid, the deviceId is local deviceId
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
 */
HWTEST_F(DeviceManagerAdapterTest, DeviceIdToUUIDLocal, TestSize.Level0)
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
* @tc.name: DeviceIdToUUID
* @tc.desc: transfer deviceIds to uuid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
 */
HWTEST_F(DeviceManagerAdapterTest, DeviceIdsToUUID, TestSize.Level0)
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
* @tc.name: DeviceIdToUUID
* @tc.desc: transfer deviceIds to uuid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
 */
HWTEST_F(DeviceManagerAdapterTest, DeviceIdToUUID, TestSize.Level0)
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
    EXPECT_EQ(uuids[0], INVALID_DEVICE_ID);
    EXPECT_EQ(uuids[1], dvInfo.uuid);
}

/**
* @tc.name: DeviceIdToNetworkId
* @tc.desc: transfer deviceId to networkId, the deviceId is invalid
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
 */
HWTEST_F(DeviceManagerAdapterTest, DeviceIdToNetworkIdInvalid, TestSize.Level0)
{
    auto networkId = DeviceManagerAdapter::GetInstance().ToNetworkID(EMPTY_DEVICE_ID);
    EXPECT_TRUE(networkId.empty());
    networkId = DeviceManagerAdapter::GetInstance().ToNetworkID(INVALID_DEVICE_ID);
    EXPECT_TRUE(networkId.empty());
}

/**
* @tc.name: DeviceIdToNetworkId
* @tc.desc: transfer deviceId to networkId, the deviceId is local deviceId
* @tc.type: FUNC
* @tc.require:
* @tc.author: zuojiangjiang
 */
HWTEST_F(DeviceManagerAdapterTest, DeviceIdToNetworkIdLocal, TestSize.Level0)
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
* @tc.name: StartWatchDeviceChange
* @tc.desc: start watch device change
* @tc.type: FUNC
* @tc.author: nhj
 */
HWTEST_F(DeviceManagerAdapterTest, StartWatchDeviceChange01, TestSize.Level0)
{
    std::shared_ptr<DeviceChangerListener> observer = std::make_shared<DeviceChangerListener>();
    auto status = DeviceManagerAdapter::GetInstance().StartWatchDeviceChange(observer.get(), {});
    EXPECT_EQ(status, Status::SUCCESS);
}

/**
* @tc.name: GetDeviceInfo
* @tc.desc: get device info
* @tc.type: FUNC
* @tc.author: nhj
 */
HWTEST_F(DeviceManagerAdapterTest, GetDeviceInfo, TestSize.Level0)
{
    auto executors = std::make_shared<OHOS::ExecutorPool>(0, 0);
    DeviceManagerAdapter::GetInstance().Init(executors);
    auto dvInfo = DeviceManagerAdapter::GetInstance().GetDeviceInfo(EMPTY_DEVICE_ID);
    EXPECT_TRUE(dvInfo.uuid.empty());
    EXPECT_TRUE(dvInfo.udid.empty());
    EXPECT_TRUE(dvInfo.networkId.empty());
}

/**
* @tc.name: GetDeviceInfo
* @tc.desc: get device info, the id is invalid
* @tc.type: FUNC
* @tc.author: nhj
 */
HWTEST_F(DeviceManagerAdapterTest, GetDeviceInfoInvalidId01, TestSize.Level0)
{
    auto dvInfo = DeviceManagerAdapter::GetInstance().GetDeviceInfo(EMPTY_DEVICE_ID);
    EXPECT_TRUE(dvInfo.uuid.empty());
}

/**
* @tc.name: GetOnlineDevices
* @tc.desc: get Online device
* @tc.type: FUNC
* @tc.author: nhj
 */
HWTEST_F(DeviceManagerAdapterTest, GetOnlineDevices, TestSize.Level0)
{
    auto onInfos = DeviceManagerAdapter::GetInstance().GetOnlineDevices();
    EXPECT_TRUE(onInfos.empty());
}

} // namespace