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
#include <gtest/gtest.h>
#include "delay_data_acquire_container.h"
#include "delay_data_prepare_container.h"
#include "synced_device_container.h"

using namespace OHOS::UDMF;
using namespace testing::ext;
using namespace testing;

namespace OHOS::Test {
namespace DistributedDataTest {
class UdmfDelayDataContainerTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

/**
* @tc.name: DataLoadCallbackTest001
* @tc.desc: Test Execute data load callback with return false
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, DataLoadCallbackTest001, TestSize.Level1)
{
    UnifiedData data;
    QueryOption query;
    query.key = "";
    auto ret = DelayDataPrepareContainer::GetInstance().HandleDelayLoad(query, data);
    EXPECT_EQ(ret, false);

    query.key = "udmf://drag/com.example.app/1233455";
    ret = DelayDataPrepareContainer::GetInstance().HandleDelayLoad(query, data);
    EXPECT_EQ(ret, false);
}

/**
* @tc.name: DataLoadCallbackTest002
* @tc.desc: Test Execute data load callback after registering callback
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, DataLoadCallbackTest002, TestSize.Level1)
{
    DelayDataPrepareContainer::GetInstance().dataLoadCallback_.clear();
    std::string key = "";
    sptr<UdmfNotifierProxy> callback = nullptr;
    DelayDataPrepareContainer::GetInstance().RegisterDataLoadCallback(key, callback);
    EXPECT_TRUE(DelayDataPrepareContainer::GetInstance().dataLoadCallback_.empty());

    key = "udmf://drag/com.example.app/1233455";
    DelayDataPrepareContainer::GetInstance().RegisterDataLoadCallback(key, callback);
    EXPECT_TRUE(DelayDataPrepareContainer::GetInstance().dataLoadCallback_.empty());
    sptr<IRemoteObject> re = nullptr;
    callback = new (std::nothrow) UdmfNotifierProxy(re);
    DelayDataPrepareContainer::GetInstance().RegisterDataLoadCallback(key, callback);
    EXPECT_FALSE(DelayDataPrepareContainer::GetInstance().dataLoadCallback_.empty());

    QueryOption query;
    query.key = key;
    UnifiedData data;
    auto ret = DelayDataPrepareContainer::GetInstance().HandleDelayLoad(query, data);
    EXPECT_EQ(ret, true);
    ret = DelayDataPrepareContainer::GetInstance().HandleDelayLoad(query, data);
    EXPECT_EQ(ret, true);
    DelayDataPrepareContainer::GetInstance().dataLoadCallback_.clear();
}

/**
* @tc.name: DataLoadCallbackTest003
* @tc.desc: Test Execute data load callback after registering callback
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, DataLoadCallbackTest003, TestSize.Level1)
{
    DelayDataPrepareContainer::GetInstance().dataLoadCallback_.clear();
    std::string key = "udmf://drag/com.example.app/1233455";
    sptr<IRemoteObject> re = nullptr;
    sptr<UdmfNotifierProxy> callback = new (std::nothrow) UdmfNotifierProxy(re);
    DelayDataPrepareContainer::GetInstance().RegisterDataLoadCallback(key, callback);
    EXPECT_TRUE(DelayDataPrepareContainer::GetInstance().dataLoadCallback_.size() == 1);

    using CacheData = BlockData<std::optional<UnifiedData>, std::chrono::milliseconds>;
    BlockDelayData data;
    data.tokenId = query.tokenId;
    data.blockData = std::make_shared<CacheData>(100);
    DelayDataPrepareContainer::GetInstance().blockDelayDataCache_.Insert(key, data);

    UnifiedData insertedData;
    insertedData.AddRecord(std::make_shared<UnifiedRecord>());
    data.blockData->SetValue(insertedData);

    QueryOption query;
    query.key = key;
    UnifiedData data;
    ret = DelayDataPrepareContainer::GetInstance().HandleDelayLoad(query, data);
    EXPECT_EQ(ret, true);
    DelayDataPrepareContainer::GetInstance().dataLoadCallback_.clear();
    DelayDataPrepareContainer::GetInstance().blockDelayDataCache_.clear();
}

/**
* @tc.name: ExecDataLoadCallback001
* @tc.desc: Test Execute data load callback after registering callback
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, ExecDataLoadCallback001, TestSize.Level1)
{
    DelayDataPrepareContainer::GetInstance().dataLoadCallback_.clear();
    std::string key = "udmf://drag/com.example.app/1233455";
    sptr<IRemoteObject> re = nullptr;
    sptr<UdmfNotifierProxy> callback = new (std::nothrow) UdmfNotifierProxy(re);
    DelayDataPrepareContainer::GetInstance().RegisterDataLoadCallback(key, callback);

    QueryOption query;
    query.key = key;
    UnifiedData data;
    auto ret = DelayDataPrepareContainer::GetInstance().HandleDelayLoad(query, data);
    EXPECT_EQ(ret, true);
    DataLoadInfo info;
    ret = DelayDataPrepareContainer::GetInstance().ExecDataLoadCallback(query.key, info);
    EXPECT_EQ(ret, true);
    DelayDataPrepareContainer::GetInstance().dataLoadCallback_.clear();
}

/**
* @tc.name: ExecDataLoadCallback002
* @tc.desc: Test Execute data load callback after registering callback
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, ExecDataLoadCallback002, TestSize.Level1)
{
    DelayDataPrepareContainer::GetInstance().dataLoadCallback_.clear();
    std::string key = "udmf://drag/com.example.app/1233455";
    sptr<IRemoteObject> re = nullptr;
    sptr<UdmfNotifierProxy> callback = new (std::nothrow) UdmfNotifierProxy(re);
    DelayDataPrepareContainer::GetInstance().RegisterDataLoadCallback(key, callback);
    EXPECT_TRUE(DelayDataPrepareContainer::GetInstance().dataLoadCallback_.size() == 1);
    EXPECT_NO_FATAL_FAILURE(DelayDataPrepareContainer::GetInstance().ExecAllDataLoadCallback());
    EXPECT_TRUE(DelayDataPrepareContainer::GetInstance().dataLoadCallback_.empty());
    DelayDataPrepareContainer::GetInstance().dataLoadCallback_.clear();
}

/**
* @tc.name: RegisterDelayDataCallback001
* @tc.desc: Test RegisterDelayDataCallback
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, RegisterDelayDataCallback001, TestSize.Level1)
{
    DelayDataAcquireContainer::GetInstance().delayDataCallback_.clear();
    EXPECT_TRUE(DelayDataAcquireContainer::GetInstance().delayDataCallback_.empty());
    std::string key = "";
    DelayGetDataInfo info;
    DelayDataAcquireContainer::GetInstance().RegisterDelayDataCallback(key, info);
    EXPECT_TRUE(DelayDataAcquireContainer::GetInstance().delayDataCallback_.empty());
    key = "udmf://drag/com.example.app/1233455";
    DelayDataAcquireContainer::GetInstance().RegisterDelayDataCallback(key, info);
    EXPECT_TRUE(DelayDataAcquireContainer::GetInstance().delayDataCallback_.size() == 1);

    DelayDataAcquireContainer::GetInstance().delayDataCallback_.clear();
}

/**
* @tc.name: HandleDelayDataCallback001
* @tc.desc: Test HandleDelayDataCallback
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, HandleDelayDataCallback001, TestSize.Level1)
{
    DelayDataAcquireContainer::GetInstance().delayDataCallback_.clear();
    EXPECT_TRUE(DelayDataAcquireContainer::GetInstance().delayDataCallback_.empty());
    std::string key = "udmf://drag/com.example.app/1233455";
    UnifiedData data;
    auto ret = DelayDataAcquireContainer::GetInstance().HandleDelayDataCallback(key, data);
    EXPECT_FALSE(ret);
    DelayGetDataInfo getDataInfo;
    DelayDataAcquireContainer::GetInstance().RegisterDelayDataCallback(key, getDataInfo);
    ret = DelayDataAcquireContainer::GetInstance().HandleDelayDataCallback(key, data);
    EXPECT_FALSE(ret);

    DelayDataAcquireContainer::GetInstance().delayDataCallback_.clear();
}

/**
* @tc.name: QueryDelayGetDataInfo001
* @tc.desc: Test RegisterDelayDataCallback
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, QueryDelayGetDataInfo001, TestSize.Level1)
{
    DelayDataAcquireContainer::GetInstance().delayDataCallback_.clear();
    EXPECT_TRUE(DelayDataAcquireContainer::GetInstance().delayDataCallback_.empty());
    std::string key = "udmf://drag/com.example.app/1233455";
    DelayGetDataInfo getDataInfo;
    auto ret = DelayDataAcquireContainer::GetInstance().QueryDelayGetDataInfo(key, getDataInfo);
    EXPECT_EQ(ret, false);
    DelayGetDataInfo info;
    info.tokenId = 12345;
    DelayDataAcquireContainer::GetInstance().RegisterDelayDataCallback(key, info);
    ret = DelayDataAcquireContainer::GetInstance().QueryDelayGetDataInfo(key, getDataInfo);
    EXPECT_EQ(getDataInfo.tokenId, 12345);
    EXPECT_TRUE(ret);
    DelayDataAcquireContainer::GetInstance().delayDataCallback_.clear();
}

/**
* @tc.name: QueryAllDelayKeys001
* @tc.desc: Test QueryAllDelayKeys
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, QueryAllDelayKeys001, TestSize.Level1)
{
    DelayDataAcquireContainer::GetInstance().delayDataCallback_.clear();
    EXPECT_TRUE(DelayDataAcquireContainer::GetInstance().delayDataCallback_.empty());
    DelayDataAcquireContainer::GetInstance().delayDataCallback_.insert_or_assign("key1", DelayGetDataInfo());
    DelayDataAcquireContainer::GetInstance().delayDataCallback_.insert_or_assign("", DelayGetDataInfo());
    DelayDataAcquireContainer::GetInstance().delayDataCallback_.insert_or_assign("key2", DelayGetDataInfo());
    auto keys = DelayDataAcquireContainer::GetInstance().QueryAllDelayKeys();
    EXPECT_TRUE(keys.size() == 2);
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key1") != keys.end());
    DelayDataAcquireContainer::GetInstance().delayDataCallback_.clear();
}

/**
* @tc.name: QueryBlockDelayData001
* @tc.desc: Test QueryBlockDelayData
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, QueryBlockDelayData001, TestSize.Level1)
{
    DelayDataPrepareContainer::GetInstance().blockDelayDataCache_.clear();
    std::shared_ptr<BlockData<std::optional<UnifiedData>, std::chrono::milliseconds>> blockData;
    std::string key = "udmf://drag/com.example.app/1233455";
    DelayDataPrepareContainer::GetInstance().blockDelayDataCache_.insert_or_assign(
        key, BlockDelayData{12345, blockData});
    BlockDelayData data;
    auto ret = DelayDataPrepareContainer::GetInstance().QueryBlockDelayData("111", data);
    EXPECT_FALSE(ret);
    ret = DelayDataPrepareContainer::GetInstance().QueryBlockDelayData(key, data);
    EXPECT_TRUE(ret);
    DelayDataPrepareContainer::GetInstance().blockDelayDataCache_.clear();
}

/**
* @tc.name: SaveDelayDragDeviceInfo001
* @tc.desc: Test QueryBlockDelayData
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, SaveDelayDragDeviceInfo001, TestSize.Level1)
{
    SyncedDeviceContainer::GetInstance().pulledDeviceInfo_.clear();
    SyncedDeviceContainer::GetInstance().receivedDeviceInfo_.clear();
    SyncedDeviceContainer::GetInstance().SaveSyncedDeviceInfo("", "");
    EXPECT_TRUE(SyncedDeviceContainer::GetInstance().pulledDeviceInfo_.empty());
    EXPECT_TRUE(SyncedDeviceContainer::GetInstance().receivedDeviceInfo_.empty());
    std::string deviceId = "deviceId";
    SyncedDeviceContainer::GetInstance().SaveSyncedDeviceInfo("", deviceId);
    EXPECT_TRUE(SyncedDeviceContainer::GetInstance().pulledDeviceInfo_.size() == 1);
    EXPECT_TRUE(SyncedDeviceContainer::GetInstance().receivedDeviceInfo_.empty());
    std::string key = "deviceKey";
    std::string deviceId1 = "deviceId1";
    SyncedDeviceContainer::GetInstance().SaveSyncedDeviceInfo(key, deviceId1);
    EXPECT_TRUE(SyncedDeviceContainer::GetInstance().pulledDeviceInfo_.size() == 1);
    EXPECT_TRUE(SyncedDeviceContainer::GetInstance().receivedDeviceInfo_.size() == 1);

    auto devices = SyncedDeviceContainer::GetInstance().QueryDeviceInfo(key);
    EXPECT_TRUE(devices.size() == 1);
    EXPECT_EQ(devices[0], deviceId1);

    auto devices1 = SyncedDeviceContainer::GetInstance().QueryDeviceInfo("otherKey");
    EXPECT_TRUE(devices1.size() == 1);
    EXPECT_EQ(devices1[0], deviceId);
    SyncedDeviceContainer::GetInstance().pulledDeviceInfo_.clear();
    SyncedDeviceContainer::GetInstance().receivedDeviceInfo_.clear();
}
} // namespace DistributedDataTest
} // namespace OHOS::Test