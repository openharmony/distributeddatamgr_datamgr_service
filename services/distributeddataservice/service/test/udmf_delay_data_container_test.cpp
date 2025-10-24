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
#include "delay_data_container.h"

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
    int32_t res = E_OK;
    QueryOption query;
    query.key = "";
    auto ret = DelayDataContainer::GetInstance().HandleDelayLoad(query, data, res);
    EXPECT_EQ(res, E_INVALID_PARAMETERS);
    EXPECT_EQ(ret, false);

    query.key = "udmf://drag/com.example.app/1233455";
    ret = DelayDataContainer::GetInstance().HandleDelayLoad(query, data, res);
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
    DelayDataContainer::GetInstance().dataLoadCallback_.clear();
    std::string key = "";
    sptr<UdmfNotifierProxy> callback = nullptr;
    DelayDataContainer::GetInstance().RegisterDataLoadCallback(key, callback);
    EXPECT_TRUE(DelayDataContainer::GetInstance().dataLoadCallback_.empty());

    key = "udmf://drag/com.example.app/1233455";
    DelayDataContainer::GetInstance().RegisterDataLoadCallback(key, callback);
    EXPECT_TRUE(DelayDataContainer::GetInstance().dataLoadCallback_.empty());
    sptr<IRemoteObject> re = nullptr;
    callback = new (std::nothrow) UdmfNotifierProxy(re);
    DelayDataContainer::GetInstance().RegisterDataLoadCallback(key, callback);
    EXPECT_FALSE(DelayDataContainer::GetInstance().dataLoadCallback_.empty());

    QueryOption query;
    query.key = key;
    UnifiedData data;
    int32_t res = E_OK;
    auto ret = DelayDataContainer::GetInstance().HandleDelayLoad(query, data, res);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(res, E_NOT_FOUND);
    ret = DelayDataContainer::GetInstance().HandleDelayLoad(query, data, res);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(res, E_NOT_FOUND);
    DelayDataContainer::GetInstance().dataLoadCallback_.clear();
}

/**
* @tc.name: ExecDataLoadCallback001
* @tc.desc: Test Execute data load callback after registering callback
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, ExecDataLoadCallback001, TestSize.Level1)
{
    DelayDataContainer::GetInstance().dataLoadCallback_.clear();
    std::string key = "udmf://drag/com.example.app/1233455";
    sptr<IRemoteObject> re = nullptr;
    sptr<UdmfNotifierProxy> callback = new (std::nothrow) UdmfNotifierProxy(re);
    DelayDataContainer::GetInstance().RegisterDataLoadCallback(key, callback);

    QueryOption query;
    query.key = key;
    UnifiedData data;
    int32_t res = E_OK;
    auto ret = DelayDataContainer::GetInstance().HandleDelayLoad(query, data, res);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(res, E_NOT_FOUND);
    DataLoadInfo info;
    ret = DelayDataContainer::GetInstance().ExecDataLoadCallback(query.key, info);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(res, E_NOT_FOUND);
    DelayDataContainer::GetInstance().dataLoadCallback_.clear();
}

/**
* @tc.name: ExecDataLoadCallback002
* @tc.desc: Test Execute data load callback after registering callback
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, ExecDataLoadCallback002, TestSize.Level1)
{
    DelayDataContainer::GetInstance().dataLoadCallback_.clear();
    std::string key = "udmf://drag/com.example.app/1233455";
    sptr<IRemoteObject> re = nullptr;
    sptr<UdmfNotifierProxy> callback = new (std::nothrow) UdmfNotifierProxy(re);
    DelayDataContainer::GetInstance().RegisterDataLoadCallback(key, callback);
    EXPECT_TRUE(DelayDataContainer::GetInstance().dataLoadCallback_.size() == 1);
    EXPECT_NO_FATAL_FAILURE(DelayDataContainer::GetInstance().ExecAllDataLoadCallback());
    EXPECT_TRUE(DelayDataContainer::GetInstance().dataLoadCallback_.empty());
    DelayDataContainer::GetInstance().dataLoadCallback_.clear();
}

/**
* @tc.name: RegisterDelayDataCallback001
* @tc.desc: Test RegisterDelayDataCallback
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, RegisterDelayDataCallback001, TestSize.Level1)
{
    DelayDataContainer::GetInstance().delayDataCallback_.clear();
    EXPECT_TRUE(DelayDataContainer::GetInstance().delayDataCallback_.empty());
    std::string key = "";
    DelayGetDataInfo info;
    DelayDataContainer::GetInstance().RegisterDelayDataCallback(key, info);
    EXPECT_TRUE(DelayDataContainer::GetInstance().delayDataCallback_.empty());
    key = "udmf://drag/com.example.app/1233455";
    DelayDataContainer::GetInstance().RegisterDelayDataCallback(key, info);
    EXPECT_TRUE(DelayDataContainer::GetInstance().delayDataCallback_.size() == 1);

    DelayDataContainer::GetInstance().delayDataCallback_.clear();
}

/**
* @tc.name: HandleDelayDataCallback001
* @tc.desc: Test HandleDelayDataCallback
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, HandleDelayDataCallback001, TestSize.Level1)
{
    DelayDataContainer::GetInstance().delayDataCallback_.clear();
    EXPECT_TRUE(DelayDataContainer::GetInstance().delayDataCallback_.empty());
    std::string key = "udmf://drag/com.example.app/1233455";
    UnifiedData data;
    auto ret = DelayDataContainer::GetInstance().HandleDelayDataCallback(key, data);
    EXPECT_FALSE(ret);
    DelayGetDataInfo getDataInfo;
    DelayDataContainer::GetInstance().RegisterDelayDataCallback(key, getDataInfo);
    ret = DelayDataContainer::GetInstance().HandleDelayDataCallback(key, data);
    EXPECT_FALSE(ret);

    DelayDataContainer::GetInstance().delayDataCallback_.clear();
}

/**
* @tc.name: QueryDelayGetDataInfo001
* @tc.desc: Test RegisterDelayDataCallback
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, QueryDelayGetDataInfo001, TestSize.Level1)
{
    DelayDataContainer::GetInstance().delayDataCallback_.clear();
    EXPECT_TRUE(DelayDataContainer::GetInstance().delayDataCallback_.empty());
    std::string key = "udmf://drag/com.example.app/1233455";
    DelayGetDataInfo getDataInfo;
    auto ret = DelayDataContainer::GetInstance().QueryDelayGetDataInfo(key, getDataInfo);
    EXPECT_EQ(ret, false);
    DelayGetDataInfo info;
    info.tokenId = 12345;
    DelayDataContainer::GetInstance().RegisterDelayDataCallback(key, info);
    ret = DelayDataContainer::GetInstance().QueryDelayGetDataInfo(key, getDataInfo);
    EXPECT_EQ(getDataInfo.tokenId, 12345);
    EXPECT_TRUE(ret);
    DelayDataContainer::GetInstance().delayDataCallback_.clear();
}

/**
* @tc.name: QueryAllDelayKeys001
* @tc.desc: Test QueryAllDelayKeys
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, QueryAllDelayKeys001, TestSize.Level1)
{
    DelayDataContainer::GetInstance().delayDataCallback_.clear();
    EXPECT_TRUE(DelayDataContainer::GetInstance().delayDataCallback_.empty());
    DelayDataContainer::GetInstance().delayDataCallback_.insert_or_assign("key1", DelayGetDataInfo());
    DelayDataContainer::GetInstance().delayDataCallback_.insert_or_assign("", DelayGetDataInfo());
    DelayDataContainer::GetInstance().delayDataCallback_.insert_or_assign("key2", DelayGetDataInfo());
    auto keys = DelayDataContainer::GetInstance().QueryAllDelayKeys();
    EXPECT_TRUE(keys.size == 2);
    EXPECT_TRUE(std::find(keys.begin(), keys.end(), "key1") != keys.end());
    DelayDataContainer::GetInstance().delayDataCallback_.clear();
}

/**
* @tc.name: QueryBlockDelayData001
* @tc.desc: Test QueryBlockDelayData
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, QueryBlockDelayData001, TestSize.Level1)
{
    DelayDataContainer::GetInstance().blockDelayDataCache_.clear();
    std::shared_ptr<BlockData<std::optional<UnifiedData>, std::chrono::milliseconds>> blockData;
    std::string key = "udmf://drag/com.example.app/1233455";
    DelayDataContainer::GetInstance().blockDelayDataCache_.insert_or_assign(key, BlockDelayData{12345, blockData});
    BlockDelayData data;
    auto ret = DelayDataContainer::GetInstance().QueryBlockDelayData("111", data);
    EXPECT_FALSE(ret);
    auto ret = DelayDataContainer::GetInstance().QueryBlockDelayData(key, data);
    EXPECT_TRUE(ret);
    DelayDataContainer::GetInstance().blockDelayDataCache_.clear();
}

/**
* @tc.name: SaveDelayDragDeviceInfo001
* @tc.desc: Test QueryBlockDelayData
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, SaveDelayDragDeviceInfo001, TestSize.Level1)
{
    DelayDataContainer::GetInstance().delayDragDeviceInfo_.clear();
    DelayDataContainer::GetInstance().SaveDelayDragDeviceInfo("");
    EXPECT_TRUE(DelayDataContainer::GetInstance().delayDragDeviceInfo_.empty());
    std::string deviceId = "saavsasd11213"
    DelayDataContainer::GetInstance().SaveDelayDragDeviceInfo(deviceId);
    EXPECT_TRUE(DelayDataContainer::GetInstance().delayDragDeviceInfo_.size() == 1);

    auto devices = DelayDataContainer::GetInstance().QueryDelayDragDeviceInfo();
    EXPECT_TRUE(devices.size() == 1);
    EXPECT_EQ(devices[0] == deviceId);

    DelayDataContainer::GetInstance().ClearDelayDragDeviceInfo();
    EXPECT_TRUE(DelayDataContainer::GetInstance().delayDragDeviceInfo_.empty());
}

/**
* @tc.name: SaveDelayAcceptableInfo001
* @tc.desc: Test SaveDelayAcceptableInfo
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfDelayDataContainerTest, SaveDelayAcceptableInfo001, TestSize.Level1)
{
    DelayDataContainer::GetInstance().delayAcceptableInfos_.clear();
    std::string key = "key1";
    DataLoadInfo info;
    info.sequenceKey = "11";
    DelayDataContainer::GetInstance().SaveDelayAcceptableInfo(key, info);

    DataLoadInfo getInfo;
    auto ret = DelayDataContainer::GetInstance().QueryDelayAcceptableInfo("empty", getInfo);
    EXPECT_FALSE(ret);
    ret = DelayDataContainer::GetInstance().QueryDelayAcceptableInfo(key, getInfo);
    EXPECT_TRUE(ret);
    EXPECT_EQ(info.sequenceKey, getInfo.sequenceKey);
    
    EXPECT_TRUE(DelayDataContainer::GetInstance().delayAcceptableInfos_.empty());
}

} // namespace DistributedDataTest
} // namespace OHOS::Test