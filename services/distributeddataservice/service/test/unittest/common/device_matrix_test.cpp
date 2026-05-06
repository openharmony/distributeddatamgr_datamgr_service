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
#include "device_matrix.h"

#include "block_data.h"
#include "bootstrap.h"
#include "checker/checker_manager.h"
#include "device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "feature/feature_system.h"
#include "ipc_skeleton.h"
#include "matrix_event.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data_local.h"
#include "mock/checker_mock.h"
#include "mock/db_store_mock.h"
#include "types.h"
#include "gtest/gtest.h"
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedKv;
using namespace OHOS;
using DMAdapter = DeviceManagerAdapter;
using namespace DistributedDB;
class DeviceMatrixTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    struct Result {
        uint16_t mask_ = DeviceMatrix::INVALID_LEVEL;
        std::string deviceId_;
        Result() {};
    };
    static constexpr const char *TEST_DEVICE = "14a0a92a428005db27c40bad46bf145fede38ec37effe0347cd990fcb031f320";
    static constexpr const char *TEST_BUNDLE = "matrix_test";
    static constexpr const char *TEST_STORE = "matrix_store";
    static constexpr const char *TEST_USER = "0";
    void InitRemoteMatrixMeta();
    void InitMetaData();

    static inline std::vector<std::pair<std::string, std::string>> staticStores_ = {
        { "bundle0", "store0" },
        { "bundle1", "store0" }
    };
    static inline std::vector<std::pair<std::string, std::string>> dynamicStores_ = {
        { "bundle0", "store1" },
        { "bundle3", "store0" }
    };
    static BlockData<Result> isFinished_;
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static uint32_t selfToken_;
    StoreMetaData metaData_;
    StoreMetaDataLocal localMeta_;
    static CheckerMock instance_;
    static constexpr uint32_t CURRENT_VERSION = 3;
};
BlockData<DeviceMatrixTest::Result> DeviceMatrixTest::isFinished_(1, Result());
std::shared_ptr<DBStoreMock> DeviceMatrixTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
uint32_t DeviceMatrixTest::selfToken_ = 0;
CheckerMock DeviceMatrixTest::instance_;
void DeviceMatrixTest::SetUpTestCase(void)
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    MetaDataManager::GetInstance().SetCloudSyncer([]() {
        DeviceMatrix::GetInstance().OnChanged(DeviceMatrix::META_STORE_MASK);
    });
    selfToken_ = IPCSkeleton::GetCallingTokenID();
    FeatureSystem::GetInstance().GetCreator("kv_store")();
    std::vector<CheckerManager::StoreInfo> dynamicStores;
    for (auto &[bundle, store] : dynamicStores_) {
        dynamicStores.push_back({ 0, 0, bundle, store });
        instance_.SetDynamic(dynamicStores);
    }
    std::vector<CheckerManager::StoreInfo> staticStores;
    for (auto &[bundle, store] : staticStores_) {
        staticStores.push_back({ 0, 0, bundle, store });
        instance_.SetStatic(staticStores);
    }
    Bootstrap::GetInstance().LoadCheckers();
    DeviceMatrix::GetInstance().Initialize(selfToken_, "service_meta");
    mkdir("/data/service/el1/public/database/matrix_test", (S_IRWXU | S_IRWXG | S_IRWXO));
    mkdir("/data/service/el1/public/database/matrix_test/kvdb", (S_IRWXU | S_IRWXG | S_IRWXO));
}

void DeviceMatrixTest::TearDownTestCase(void)
{
    EventCenter::GetInstance().Unsubscribe(DeviceMatrix::MATRIX_BROADCAST);
}

void DeviceMatrixTest::SetUp()
{
    isFinished_.Clear(Result());
    DeviceMatrix::GetInstance().Clear();
    InitMetaData();
    InitRemoteMatrixMeta();
}

void DeviceMatrixTest::TearDown()
{
    (void)remove("/data/service/el1/public/database/matrix_test/kvdb");
    (void)remove("/data/service/el1/public/database/matrix_test");
}

void DeviceMatrixTest::InitRemoteMatrixMeta()
{
    MatrixMetaData metaData;
    metaData.version = CURRENT_VERSION;
    metaData.dynamic = 0x7;
    metaData.deviceId = TEST_DEVICE;
    metaData.origin = MatrixMetaData::Origin::REMOTE_RECEIVED;
    metaData.dynamicInfo.clear();
    for (auto &[bundleName, _] : dynamicStores_) {
        metaData.dynamicInfo.push_back(bundleName);
    }
    MetaDataManager::GetInstance().DelMeta(metaData.GetKey());
    MetaDataManager::GetInstance().SaveMeta(metaData.GetKey(), metaData);
}

void DeviceMatrixTest::InitMetaData()
{
    metaData_.deviceId = DMAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.appId = TEST_BUNDLE;
    metaData_.bundleName = TEST_BUNDLE;
    metaData_.user = TEST_USER;
    metaData_.area = EL1;
    metaData_.tokenId = IPCSkeleton::GetCallingTokenID();
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = true;
    metaData_.storeId = TEST_STORE;
    metaData_.dataType = 1;
    PolicyValue value;
    value.type = PolicyType::IMMEDIATE_SYNC_ON_ONLINE;
    localMeta_.policies = { std::move(value) };
}

/**
 * @tc.name: GetMetaStoreCode
 * @tc.desc: get the meta data store mask code;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: blue sky
 */
HWTEST_F(DeviceMatrixTest, GetMetaStoreCode, TestSize.Level0)
{
    StoreMetaData meta;
    meta.bundleName = "distributeddata";
    meta.tokenId = selfToken_;
    meta.storeId = "service_meta";
    meta.dataType = 1;
    auto code = DeviceMatrix::GetInstance().GetCode(meta);
    ASSERT_EQ(code, DeviceMatrix::META_STORE_MASK);
}

/**
 * @tc.name: GetAllCode
 * @tc.desc: get all dynamic store mask code;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: blue sky
 */
HWTEST_F(DeviceMatrixTest, GetAllCode, TestSize.Level0)
{
    StoreMetaData meta = metaData_;

    for (size_t i = 0; i < dynamicStores_.size(); ++i) {
        meta.appId = dynamicStores_[i].first;
        meta.bundleName = dynamicStores_[i].first;
        ASSERT_EQ(DeviceMatrix::GetInstance().GetCode(meta), 0x1 << (i + 1));
    }
}

/**
 * @tc.name: GetOtherStoreCode
 * @tc.desc: get the other store mask code;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: blue sky
 */
HWTEST_F(DeviceMatrixTest, GetOtherStoreCode, TestSize.Level0)
{
    StoreMetaData meta = metaData_;
    auto code = DeviceMatrix::GetInstance().GetCode(meta);
    ASSERT_EQ(code, 0);
}

/**
 * @tc.name: BroadcastMeta
 * @tc.desc: broadcast the meta store change;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: blue sky
 */
HWTEST_F(DeviceMatrixTest, BroadcastMeta, TestSize.Level0)
{
    DeviceMatrix::DataLevel level = {
        .dynamic = DeviceMatrix::META_STORE_MASK,
    };
    auto mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, TEST_DEVICE, level);
    ASSERT_EQ(mask.first, DeviceMatrix::META_STORE_MASK);
}

/**
 * @tc.name: BroadcastFirst
 * @tc.desc: broadcast all stores change;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: blue sky
 */
HWTEST_F(DeviceMatrixTest, BroadcastFirst, TestSize.Level0)
{
    StoreMetaData meta = metaData_;
    meta.appId = dynamicStores_[0].first;
    meta.bundleName = dynamicStores_[0].first;
    auto code = DeviceMatrix::GetInstance().GetCode(meta);
    ASSERT_EQ(code, 0x2);
    DeviceMatrix::DataLevel level = {
        .dynamic = code,
    };
    auto mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, TEST_DEVICE, level);
    ASSERT_EQ(mask.first, code);
}

/**
 * @tc.name: BroadcastOthers
 * @tc.desc: broadcast the device profile store change;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: blue sky
 */
HWTEST_F(DeviceMatrixTest, BroadcastOthers, TestSize.Level0)
{
    StoreMetaData meta = metaData_;
    auto code = DeviceMatrix::GetInstance().GetCode(meta);
    DeviceMatrix::DataLevel level = {
        .dynamic = code,
    };
    auto mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, TEST_DEVICE, level);
    ASSERT_EQ(mask.first, 0);
}

/**
 * @tc.name: BroadcastAll
 * @tc.desc: broadcast the all store change;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: blue sky
 */
HWTEST_F(DeviceMatrixTest, BroadcastAll, TestSize.Level0)
{
    DeviceMatrix::DataLevel level = {
        .dynamic = DeviceMatrix::META_STORE_MASK,
    };
    auto mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, TEST_DEVICE, level);
    ASSERT_EQ(mask.first, DeviceMatrix::META_STORE_MASK);
    StoreMetaData meta = metaData_;
    for (size_t i = 0; i < dynamicStores_.size(); ++i) {
        meta.appId = dynamicStores_[i].first;
        meta.bundleName = dynamicStores_[i].first;
        auto code = DeviceMatrix::GetInstance().GetCode(meta);
        level.dynamic = code;
        mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, TEST_DEVICE, level);
        ASSERT_EQ(mask.first, (0x1 << (i + 1)) + 1);
        DeviceMatrix::GetInstance().OnExchanged(TEST_DEVICE, code);
    }
    DeviceMatrix::GetInstance().OnExchanged(TEST_DEVICE, DeviceMatrix::META_STORE_MASK);
    level.dynamic = DeviceMatrix::GetInstance().GetCode(metaData_);
    mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, TEST_DEVICE, level);
    ASSERT_EQ(mask.first, 0);

    level.dynamic = 0xFFFF;
    level.statics = 0x000D;
    mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, TEST_DEVICE, level);
    ASSERT_EQ(mask.first, 0x0);
}

/**
 * @tc.name: UpdateMatrixMeta
 * @tc.desc: update the remote matrix meta the all store change;
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: blue sky
 */
HWTEST_F(DeviceMatrixTest, UpdateMatrixMeta, TestSize.Level0)
{
    MatrixMetaData metaData;
    metaData.version = 4;
    metaData.dynamic = 0x1F;
    metaData.deviceId = TEST_DEVICE;
    metaData.origin = MatrixMetaData::Origin::REMOTE_RECEIVED;
    metaData.dynamicInfo = { TEST_BUNDLE, dynamicStores_[0].first };
    MetaDataManager::GetInstance().Subscribe(
        MatrixMetaData::GetPrefix({ TEST_DEVICE }), [](const std::string &, const std::string &value, int32_t flag) {
            if (flag != MetaDataManager::INSERT && flag != MetaDataManager::UPDATE) {
                return true;
            }
            MatrixMetaData meta;
            MatrixMetaData::Unmarshall(value, meta);
            Result result;
            result.deviceId_ = meta.deviceId;
            isFinished_.SetValue(result);
            return true;
        });
    MetaDataManager::GetInstance().DelMeta(metaData.GetKey());
    MetaDataManager::GetInstance().SaveMeta(metaData.GetKey(), metaData);

    auto result = isFinished_.GetValue();
    ASSERT_EQ(result.deviceId_, std::string(TEST_DEVICE));

    DeviceMatrix::DataLevel level = {
        .dynamic = 0x2,
    };
    auto mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, TEST_DEVICE, level);
    ASSERT_EQ(mask.first, 0);
    level.dynamic = DeviceMatrix::META_STORE_MASK;
    mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, TEST_DEVICE, level);
    ASSERT_EQ(mask.first, DeviceMatrix::META_STORE_MASK);
    level.dynamic = 0x4;
    mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, TEST_DEVICE, level);
    ASSERT_EQ(mask.first, 0x3);
    DeviceMatrix::GetInstance().OnExchanged(TEST_DEVICE, DeviceMatrix::META_STORE_MASK);
    DeviceMatrix::GetInstance().OnExchanged(TEST_DEVICE, 0x2);
    level.dynamic = 0xFFFF;
    level.statics = 0x000D;
    mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, TEST_DEVICE, level);
    ASSERT_EQ(mask.first, 0x0);
    MetaDataManager::GetInstance().Unsubscribe(MatrixMetaData::GetPrefix({ TEST_DEVICE }));
}

/**
 * @tc.name: Online
 * @tc.desc: The test equipment is operated both online and offline.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, Online, TestSize.Level1)
{
    RefCount refCount;
    std::string device = "Online";
    DeviceMatrix::GetInstance().Online(device, refCount);

    DeviceMatrix::GetInstance().Offline(device);
    EXPECT_NO_FATAL_FAILURE(DeviceMatrix::GetInstance().Online(device, refCount));
}

/**
 * @tc.name: Offline
 * @tc.desc: Switch between offline and online status of test equipment.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, Offline, TestSize.Level1)
{
    RefCount refCount;
    std::string device = "Offline";
    DeviceMatrix::GetInstance().Offline(device);

    DeviceMatrix::GetInstance().Online(device, refCount);
    EXPECT_NO_FATAL_FAILURE(DeviceMatrix::GetInstance().Offline(device));
}

/**
 * @tc.name: OnBroadcast
 * @tc.desc: OnBroadcast testing exceptions.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, OnBroadcast, TestSize.Level1)
{
    std::string device;
    DeviceMatrix::DataLevel dataLevel;
    EXPECT_TRUE(!dataLevel.IsValid());
    auto mask = DeviceMatrix::GetInstance().OnBroadcast(device, device, dataLevel);
    EXPECT_EQ(mask.first, DeviceMatrix::INVALID_LEVEL);

    dataLevel = {
        .dynamic = DeviceMatrix::META_STORE_MASK,
    };
    EXPECT_FALSE(!dataLevel.IsValid());
    mask = DeviceMatrix::GetInstance().OnBroadcast(device, device, dataLevel);
    EXPECT_EQ(mask.first, DeviceMatrix::INVALID_LEVEL);

    EXPECT_FALSE(!dataLevel.IsValid());
    mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, device, dataLevel);
    EXPECT_EQ(mask.first, DeviceMatrix::INVALID_LEVEL);

    EXPECT_FALSE(!dataLevel.IsValid());
    mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, TEST_DEVICE, dataLevel);
    EXPECT_EQ(mask.first, DeviceMatrix::META_STORE_MASK);

    DeviceMatrix::DataLevel dataLevels;
    EXPECT_TRUE(!dataLevels.IsValid());
    mask = DeviceMatrix::GetInstance().OnBroadcast(device, device, dataLevels);
    EXPECT_EQ(mask.first, DeviceMatrix::INVALID_LEVEL);
}

/**
 * @tc.name: ConvertStatics
 * @tc.desc: ConvertStatics testing exceptions.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, ConvertStatics, TestSize.Level1)
{
    DeviceMatrix deviceMatrix;
    DistributedData::MatrixMetaData meta;
    uint16_t mask = 0;
    uint16_t result = deviceMatrix.ConvertStatics(meta, mask);
    EXPECT_EQ(result, 0);

    mask = 0xFFFF;
    result = deviceMatrix.ConvertStatics(meta, mask);
    EXPECT_EQ(result, 0);

    meta.version = 4;
    meta.dynamic = 0x1F;
    meta.deviceId = TEST_DEVICE;
    meta.origin = MatrixMetaData::Origin::REMOTE_RECEIVED;
    meta.dynamicInfo = { TEST_BUNDLE, dynamicStores_[0].first };
    result = deviceMatrix.ConvertStatics(meta, DeviceMatrix::INVALID_LEVEL);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: SaveSwitches
 * @tc.desc: SaveSwitches testing exceptions.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, SaveSwitches, TestSize.Level1)
{
    DeviceMatrix deviceMatrix;
    std::string device;
    DeviceMatrix::DataLevel dataLevel;
    dataLevel.switches = DeviceMatrix::INVALID_VALUE;
    dataLevel.switchesLen = DeviceMatrix::INVALID_LENGTH;
    EXPECT_NO_FATAL_FAILURE(deviceMatrix.SaveSwitches(device, device, dataLevel));

    device = "SaveSwitches";
    EXPECT_NO_FATAL_FAILURE(deviceMatrix.SaveSwitches(device, "", dataLevel));

    EXPECT_NO_FATAL_FAILURE(deviceMatrix.SaveSwitches(device, device, dataLevel));

    dataLevel.switches = 0;
    EXPECT_NO_FATAL_FAILURE(deviceMatrix.SaveSwitches(device, device, dataLevel));

    dataLevel.switchesLen = 0;
    EXPECT_NO_FATAL_FAILURE(deviceMatrix.SaveSwitches(device, device, dataLevel));
}

/**
 * @tc.name: Broadcast
 * @tc.desc: Broadcast testing exceptions.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, Broadcast, TestSize.Level1)
{
    DeviceMatrix deviceMatrix;
    DeviceMatrix::DataLevel dataLevel;
    EXPECT_FALSE(deviceMatrix.lasts_.IsValid());
    EXPECT_NO_FATAL_FAILURE(deviceMatrix.Broadcast(dataLevel));

    dataLevel.statics = 0;
    dataLevel.dynamic = 0;
    EXPECT_NO_FATAL_FAILURE(deviceMatrix.Broadcast(dataLevel));

    deviceMatrix.lasts_.statics = 0;
    deviceMatrix.lasts_.dynamic = 0;
    EXPECT_TRUE(deviceMatrix.lasts_.IsValid());
    EXPECT_NO_FATAL_FAILURE(deviceMatrix.Broadcast(dataLevel));

    DeviceMatrix::DataLevel dataLevels;
    dataLevel = dataLevels;
    EXPECT_NO_FATAL_FAILURE(deviceMatrix.Broadcast(dataLevel));
}

/**
 * @tc.name: UpdateConsistentMeta
 * @tc.desc: UpdateConsistentMeta testing exceptions.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, UpdateConsistentMeta, TestSize.Level1)
{
    DeviceMatrix deviceMatrix;
    std::string device = "device";
    DeviceMatrix::Mask remote;
    remote.statics = 0;
    remote.dynamic = 0;
    EXPECT_NO_FATAL_FAILURE(deviceMatrix.UpdateConsistentMeta(device, remote));

    remote.statics = 0x1;
    EXPECT_NO_FATAL_FAILURE(deviceMatrix.UpdateConsistentMeta(device, remote));

    remote.dynamic = 0x1;
    EXPECT_NO_FATAL_FAILURE(deviceMatrix.UpdateConsistentMeta(device, remote));

    remote.statics = 0;
    EXPECT_NO_FATAL_FAILURE(deviceMatrix.UpdateConsistentMeta(device, remote));
}

/**
 * @tc.name: OnChanged001
 * @tc.desc: Test the DeviceMatrix::OnChanged method exception scenario.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, OnChanged001, TestSize.Level1)
{
    StoreMetaData metaData;
    metaData.dataType = static_cast<DeviceMatrix::LevelType>(DeviceMatrix::LevelType::STATICS - 1);
    EXPECT_NO_FATAL_FAILURE(DeviceMatrix::GetInstance().OnChanged(metaData));

    metaData.dataType = DeviceMatrix::LevelType::BUTT;
    EXPECT_NO_FATAL_FAILURE(DeviceMatrix::GetInstance().OnChanged(metaData));

    metaData.bundleName = "distributeddata";
    metaData.tokenId = selfToken_;
    metaData.storeId = "service_meta";
    metaData.dataType = 1;
    metaData.dataType = DeviceMatrix::LevelType::STATICS;
    EXPECT_NO_FATAL_FAILURE(DeviceMatrix::GetInstance().OnChanged(metaData));

    StoreMetaData meta = metaData_;
    EXPECT_NO_FATAL_FAILURE(DeviceMatrix::GetInstance().OnChanged(meta));
}

/**
 * @tc.name: OnChanged002
 * @tc.desc: Test the DeviceMatrix::OnChanged method exception scenario.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, OnChanged002, TestSize.Level1)
{
    StoreMetaData metaData;
    metaData.bundleName = "distributeddata";
    metaData.tokenId = selfToken_;
    metaData.storeId = "service_meta";
    metaData.dataType = 1;
    auto code = DeviceMatrix::GetInstance().GetCode(metaData);
    DeviceMatrix::LevelType type = static_cast<DeviceMatrix::LevelType>(DeviceMatrix::LevelType::STATICS - 1);
    EXPECT_NO_FATAL_FAILURE(DeviceMatrix::GetInstance().OnChanged(code, type));

    type = DeviceMatrix::LevelType::BUTT;
    EXPECT_NO_FATAL_FAILURE(DeviceMatrix::GetInstance().OnChanged(code, type));

    StoreMetaData meta = metaData_;
    code = DeviceMatrix::GetInstance().GetCode(meta);
    EXPECT_EQ(code, 0);
    EXPECT_NO_FATAL_FAILURE(DeviceMatrix::GetInstance().OnChanged(code, type));
}

/**
 * @tc.name: OnExchanged001
 * @tc.desc: Test the DeviceMatrix::OnExchanged method exception scenario.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, OnExchanged001, TestSize.Level1)
{
    StoreMetaData metaData;
    metaData.bundleName = "distributeddata";
    metaData.tokenId = selfToken_;
    metaData.storeId = "service_meta";
    metaData.dataType = 1;
    auto code = DeviceMatrix::GetInstance().GetCode(metaData);
    DeviceMatrix::LevelType type = static_cast<DeviceMatrix::LevelType>(DeviceMatrix::LevelType::STATICS - 1);
    std::string device;
    EXPECT_NO_FATAL_FAILURE(
        DeviceMatrix::GetInstance().OnExchanged(device, code, type, DeviceMatrix::ChangeType::CHANGE_REMOTE));

    device = "OnExchanged";
    EXPECT_NO_FATAL_FAILURE(
        DeviceMatrix::GetInstance().OnExchanged(device, code, type, DeviceMatrix::ChangeType::CHANGE_REMOTE));

    type = DeviceMatrix::LevelType::BUTT;
    EXPECT_NO_FATAL_FAILURE(
        DeviceMatrix::GetInstance().OnExchanged(device, code, type, DeviceMatrix::ChangeType::CHANGE_REMOTE));
}

/**
 * @tc.name: OnExchanged002
 * @tc.desc: Test the DeviceMatrix::OnExchanged method exception scenario.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, OnExchanged002, TestSize.Level1)
{
    StoreMetaData metaData;
    metaData.dataType = static_cast<DeviceMatrix::LevelType>(DeviceMatrix::LevelType::STATICS - 1);
    std::string device = "OnExchanged";
    EXPECT_NO_FATAL_FAILURE(
        DeviceMatrix::GetInstance().OnExchanged(device, metaData, DeviceMatrix::ChangeType::CHANGE_REMOTE));

    metaData.dataType = DeviceMatrix::LevelType::BUTT;
    EXPECT_NO_FATAL_FAILURE(
        DeviceMatrix::GetInstance().OnExchanged(device, metaData, DeviceMatrix::ChangeType::CHANGE_REMOTE));
}

/**
 * @tc.name: OnExchanged003
 * @tc.desc: Test the DeviceMatrix::OnExchanged method.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, OnExchanged003, TestSize.Level1)
{
    StoreMetaData metaData;
    metaData.bundleName = "distributeddata";
    metaData.tokenId = selfToken_;
    metaData.storeId = "service_meta";
    metaData.dataType = 1;
    std::string device = "OnExchanged";
    EXPECT_NO_FATAL_FAILURE(
        DeviceMatrix::GetInstance().OnExchanged(device, metaData, DeviceMatrix::ChangeType::CHANGE_REMOTE));

    StoreMetaData meta = metaData_;
    EXPECT_NO_FATAL_FAILURE(
        DeviceMatrix::GetInstance().OnExchanged(device, meta, DeviceMatrix::ChangeType::CHANGE_REMOTE));
}

/**
 * @tc.name: GetCode
 * @tc.desc: Test the DeviceMatrix::GetCode method.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, GetCode, TestSize.Level1)
{
    StoreMetaData metaData;
    metaData.bundleName = "distributeddata";
    metaData.tokenId = selfToken_;
    metaData.storeId = "storeId";
    metaData.dataType = DeviceMatrix::LevelType::BUTT;
    auto code = DeviceMatrix::GetInstance().GetCode(metaData);
    EXPECT_EQ(code, 0);
}

/**
 * @tc.name: GetMask001
 * @tc.desc: Test the DeviceMatrix::GetMask method.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, GetMask001, TestSize.Level1)
{
    std::string device = "GetMask";
    DeviceMatrix::LevelType type = DeviceMatrix::LevelType::STATICS;
    std::pair<bool, uint16_t> mask = DeviceMatrix::GetInstance().GetMask(device, type);
    EXPECT_EQ(mask.first, false);
}

/**
 * @tc.name: GetMask002
 * @tc.desc: Test the DeviceMatrix::GetMask method.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, GetMask002, TestSize.Level1)
{
    std::string device = "GetMask";
    DeviceMatrix::LevelType type = DeviceMatrix::LevelType::STATICS;
    RefCount refCount;
    DeviceMatrix::GetInstance().Online(device, refCount);
    std::pair<bool, uint16_t> mask = DeviceMatrix::GetInstance().GetMask(device, type);
    EXPECT_EQ(mask.first, true);

    type = DeviceMatrix::LevelType::DYNAMIC;
    mask = DeviceMatrix::GetInstance().GetMask(device, type);
    EXPECT_EQ(mask.first, true);

    type = DeviceMatrix::LevelType::BUTT;
    mask = DeviceMatrix::GetInstance().GetMask(device, type);
    EXPECT_EQ(mask.first, false);
}

/**
 * @tc.name: GetRemoteMask001
 * @tc.desc: Test the DeviceMatrix::GetRemoteMask method.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, GetRemoteMask001, TestSize.Level1)
{
    std::string device = "GetRemoteMask";
    DeviceMatrix::LevelType type = DeviceMatrix::LevelType::STATICS;
    std::pair<bool, uint16_t> mask = DeviceMatrix::GetInstance().GetRemoteMask(device, type);
    EXPECT_EQ(mask.first, false);
}

/**
 * @tc.name: GetRemoteMask002
 * @tc.desc: Test the DeviceMatrix::GetRemoteMask method.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, GetRemoteMask002, TestSize.Level1)
{
    std::string device = "GetRemoteMask";
    StoreMetaData meta = metaData_;
    auto code = DeviceMatrix::GetInstance().GetCode(meta);
    DeviceMatrix::DataLevel level = {
        .dynamic = code,
    };
    DeviceMatrix::GetInstance().OnBroadcast(device, device, level);

    DeviceMatrix::LevelType type = DeviceMatrix::LevelType::STATICS;
    std::pair<bool, uint16_t> mask = DeviceMatrix::GetInstance().GetRemoteMask(device, type);
    EXPECT_EQ(mask.first, true);

    type = DeviceMatrix::LevelType::DYNAMIC;
    mask = DeviceMatrix::GetInstance().GetRemoteMask(device, type);
    EXPECT_EQ(mask.first, true);

    type = DeviceMatrix::LevelType::BUTT;
    mask = DeviceMatrix::GetInstance().GetRemoteMask(device, type);
    EXPECT_EQ(mask.first, false);
}

/**
 * @tc.name: GetRecvLevel
 * @tc.desc: Test the DeviceMatrix::GetRecvLevel method.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, GetRecvLevel, TestSize.Level1)
{
    std::string device = "GetRemoteMask";
    DeviceMatrix::LevelType type = DeviceMatrix::LevelType::STATICS;
    std::pair<bool, uint16_t> mask = DeviceMatrix::GetInstance().GetRecvLevel(device, type);
    EXPECT_EQ(mask.first, true);

    device = TEST_DEVICE;
    mask = DeviceMatrix::GetInstance().GetRecvLevel(device, type);
    EXPECT_EQ(mask.first, true);

    type = DeviceMatrix::LevelType::DYNAMIC;
    mask = DeviceMatrix::GetInstance().GetRecvLevel(device, type);
    EXPECT_EQ(mask.first, true);

    type = DeviceMatrix::LevelType::BUTT;
    mask = DeviceMatrix::GetInstance().GetRecvLevel(device, type);
    EXPECT_EQ(mask.first, false);
}

/**
 * @tc.name: GetConsLevel
 * @tc.desc: Test the DeviceMatrix::GetConsLevel method.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, GetConsLevel, TestSize.Level1)
{
    std::string device = "GetRemoteMask";
    DeviceMatrix::LevelType type = DeviceMatrix::LevelType::STATICS;
    std::pair<bool, uint16_t> mask = DeviceMatrix::GetInstance().GetConsLevel(device, type);
    EXPECT_EQ(mask.first, false);

    device = TEST_DEVICE;
    mask = DeviceMatrix::GetInstance().GetConsLevel(device, type);
    EXPECT_EQ(mask.first, true);

    type = DeviceMatrix::LevelType::DYNAMIC;
    mask = DeviceMatrix::GetInstance().GetConsLevel(device, type);
    EXPECT_EQ(mask.first, true);

    type = DeviceMatrix::LevelType::BUTT;
    mask = DeviceMatrix::GetInstance().GetConsLevel(device, type);
    EXPECT_EQ(mask.first, false);
}

/**
 * @tc.name: UpdateLevel
 * @tc.desc: Test the DeviceMatrix::UpdateLevel method exception scenario.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, UpdateLevel, TestSize.Level1)
{
    uint16_t level = 0;
    DeviceMatrix::LevelType type = static_cast<DeviceMatrix::LevelType>(DeviceMatrix::LevelType::STATICS - 1);
    std::string device;
    EXPECT_NO_FATAL_FAILURE(DeviceMatrix::GetInstance().UpdateLevel(device, level, type));

    device = "OnExchanged";
    EXPECT_NO_FATAL_FAILURE(DeviceMatrix::GetInstance().UpdateLevel(device, level, type));

    type = DeviceMatrix::LevelType::BUTT;
    EXPECT_NO_FATAL_FAILURE(DeviceMatrix::GetInstance().UpdateLevel(device, level, type));

    type = DeviceMatrix::LevelType::STATICS;
    device = TEST_DEVICE;
    EXPECT_NO_FATAL_FAILURE(DeviceMatrix::GetInstance().UpdateLevel(device, level, type));
}

/**
 * @tc.name: IsDynamic
 * @tc.desc: Test the DeviceMatrix::IsDynamic method exception scenario.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, IsDynamic, TestSize.Level1)
{
    StoreMetaData meta;
    meta.bundleName = "distributeddata";
    meta.tokenId = selfToken_;
    meta.storeId = "";
    meta.dataType = DeviceMatrix::LevelType::STATICS;
    bool isDynamic = DeviceMatrix::GetInstance().IsDynamic(meta);
    EXPECT_EQ(isDynamic, false);

    meta.dataType = DeviceMatrix::LevelType::DYNAMIC;
    isDynamic = DeviceMatrix::GetInstance().IsDynamic(meta);
    EXPECT_EQ(isDynamic, false);

    meta.storeId = "service_meta";
    isDynamic = DeviceMatrix::GetInstance().IsDynamic(meta);
    EXPECT_EQ(isDynamic, true);

    meta.tokenId = 1;
    isDynamic = DeviceMatrix::GetInstance().IsDynamic(meta);
    EXPECT_EQ(isDynamic, false);

    meta.storeId = "";
    isDynamic = DeviceMatrix::GetInstance().IsDynamic(meta);
    EXPECT_EQ(isDynamic, false);
}

/**
 * @tc.name: IsValid
 * @tc.desc: Test the DeviceMatrix::IsValid method exception scenario.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(DeviceMatrixTest, IsValid, TestSize.Level1)
{
    DistributedData::DeviceMatrix::DataLevel dataLevel;
    EXPECT_EQ(dataLevel.IsValid(), false);

    dataLevel.dynamic = 0;
    dataLevel.statics = 0;
    dataLevel.switches = 0;
    dataLevel.switchesLen = 0;
    EXPECT_EQ(dataLevel.IsValid(), true);
}

class MatrixEventTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
 * @tc.name: IsValid
 * @tc.desc: Test the MatrixEvent::IsValid method exception scenario.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: suoqilong
 */
HWTEST_F(MatrixEventTest, IsValid, TestSize.Level1)
{
    DistributedData::MatrixEvent::MatrixData matrixData;
    EXPECT_EQ(matrixData.IsValid(), false);

    matrixData.dynamic = 0;
    matrixData.statics = 0;
    matrixData.switches = 0;
    matrixData.switchesLen = 0;
    EXPECT_EQ(matrixData.IsValid(), true);
}