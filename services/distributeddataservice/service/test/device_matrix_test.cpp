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
#include "device_manager_adapter.h"
#include "eventcenter/event_center.h"
#include "feature/feature_system.h"
#include "gtest/gtest.h"
#include "ipc_skeleton.h"
#include "matrix_event.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data_local.h"
#include "mock/db_store_mock.h"
#include "types.h"
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
        uint16_t mask_ = MatrixMetaData::DEFAULT_MASK;
        std::string deviceId_;
        Result() {};
    };
    static constexpr const char *TEST_DEVICE = "14a0a92a428005db27c40bad46bf145fede38ec37effe0347cd990fcb031f320";
    static constexpr const char *TEST_BUNDLE = "matrix_test";
    static constexpr const char *TEST_STORE = "matrix_store";
    static constexpr const char *TEST_USER = "0";
    void InitRemoteMatrixMeta();
    void InitMetaData();

    static BlockData<Result> isFinished_;
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static uint32_t selfToken_;
    StoreMetaData metaData_;
    StoreMetaDataLocal localMeta_;
};
BlockData<DeviceMatrixTest::Result> DeviceMatrixTest::isFinished_(1, Result());
std::shared_ptr<DBStoreMock> DeviceMatrixTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
uint32_t DeviceMatrixTest::selfToken_ = 0;
void DeviceMatrixTest::SetUpTestCase(void)
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr);
    MetaDataManager::GetInstance().SetSyncer([](const auto &, auto) {
        DeviceMatrix::GetInstance().OnChanged(DeviceMatrix::META_STORE_MASK);
    });
    selfToken_ = IPCSkeleton::GetCallingTokenID();
    FeatureSystem::GetInstance().GetCreator("kv_store")();
    DeviceMatrix::GetInstance().Initialize(selfToken_, "service_meta");
    EventCenter::GetInstance().Subscribe(DeviceMatrix::MATRIX_ONLINE, [](const Event &event) {
        auto &evt = static_cast<const MatrixEvent &>(event);
        auto mask = evt.GetMask();
        auto deviceId = evt.GetDeviceId();
        if ((mask & DeviceMatrix::META_STORE_MASK) != 0) {
            auto onComplete = [deviceId](const std::map<std::string, DBStatus> &) {
                DeviceMatrix::GetInstance().OnExchanged(deviceId, DeviceMatrix::META_STORE_MASK);
            };
            dbStoreMock_->Sync({ deviceId }, SYNC_MODE_PUSH_PULL, onComplete, false);
        }
#ifdef TEST_ON_DEVICE
        auto finEvent = std::make_unique<MatrixEvent>(DeviceMatrix::MATRIX_META_FINISHED, deviceId, mask);
        EventCenter::GetInstance().PostEvent(std::move(finEvent));
#endif
    });

    EventCenter::GetInstance().Subscribe(DeviceMatrix::MATRIX_META_FINISHED, [](const Event &event) {
        auto &evt = static_cast<const MatrixEvent &>(event);
        Result result;
        result.deviceId_ = evt.GetDeviceId();
        result.mask_ = evt.GetMask();
        isFinished_.SetValue(result);
    });

    mkdir("/data/service/el1/public/database/matrix_test", (S_IRWXU | S_IRWXG | S_IRWXO));
    mkdir("/data/service/el1/public/database/matrix_test/kvdb", (S_IRWXU | S_IRWXG | S_IRWXO));
}

void DeviceMatrixTest::TearDownTestCase(void)
{
    EventCenter::GetInstance().Unsubscribe(DeviceMatrix::MATRIX_ONLINE);
    EventCenter::GetInstance().Unsubscribe(DeviceMatrix::MATRIX_META_FINISHED);
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
    metaData.version = 1;
    metaData.mask = 0xF;
    metaData.deviceId = TEST_DEVICE;
    metaData.maskInfo = { "distributed_device_profile_service", "bundle_manager_service",
        "dtbhardware_manager_service" };
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
    PolicyValue value;
    value.type = PolicyType::IMMEDIATE_SYNC_ON_ONLINE;
    localMeta_.policies = { std::move(value) };
}

/**
* @tc.name: FirstOnline
* @tc.desc: the first online time;
* @tc.type: FUNC
* @tc.require:
* @tc.author: blue sky
*/
HWTEST_F(DeviceMatrixTest, FirstOnline, TestSize.Level0)
{
    DeviceMatrix::GetInstance().Online(TEST_DEVICE);
    auto result = isFinished_.GetValue();
    ASSERT_EQ(result.deviceId_, std::string(TEST_DEVICE));
    ASSERT_EQ(result.mask_, 0xF);
}

/**
* @tc.name: OnlineAgainNoData
* @tc.desc: the second online with no data change;
* @tc.type: FUNC
* @tc.require:
* @tc.author: blue sky
*/
HWTEST_F(DeviceMatrixTest, OnlineAgainNoData, TestSize.Level0)
{
    DeviceMatrix::GetInstance().Online(TEST_DEVICE);
    auto result = isFinished_.GetValue();
    ASSERT_EQ(result.deviceId_, std::string(TEST_DEVICE));
    ASSERT_EQ(result.mask_, 0xF);
    isFinished_.Clear(Result());
    DeviceMatrix::GetInstance().Offline(TEST_DEVICE);
    DeviceMatrix::GetInstance().Online(TEST_DEVICE);
    result = isFinished_.GetValue();
    ASSERT_EQ(result.deviceId_, std::string(TEST_DEVICE));
    ASSERT_EQ(result.mask_, 0xE);
}

/**
* @tc.name: OnlineAgainWithData
* @tc.desc: the second online with sync data change;
* @tc.type: FUNC
* @tc.require:
* @tc.author: blue sky
*/
HWTEST_F(DeviceMatrixTest, OnlineAgainWithData, TestSize.Level0)
{
    DeviceMatrix::GetInstance().Online(TEST_DEVICE);
    auto result = isFinished_.GetValue();
    ASSERT_EQ(result.deviceId_, std::string(TEST_DEVICE));
    ASSERT_EQ(result.mask_, 0xF);
    isFinished_.Clear(Result());
    DeviceMatrix::GetInstance().Offline(TEST_DEVICE);
    MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_);
    DeviceMatrix::GetInstance().Online(TEST_DEVICE);
    result = isFinished_.GetValue();
    ASSERT_EQ(result.deviceId_, std::string(TEST_DEVICE));
    ASSERT_EQ(result.mask_, 0xF);
}

/**
* @tc.name: OnlineAgainWithLocal
* @tc.desc: the second online with local data change;
* @tc.type: FUNC
* @tc.require:
* @tc.author: blue sky
*/
HWTEST_F(DeviceMatrixTest, OnlineAgainWithLocal, TestSize.Level0)
{
    DeviceMatrix::GetInstance().Online(TEST_DEVICE);
    auto result = isFinished_.GetValue();
    ASSERT_EQ(result.deviceId_, std::string(TEST_DEVICE));
    ASSERT_EQ(result.mask_, 0xF);
    isFinished_.Clear(Result());
    DeviceMatrix::GetInstance().Offline(TEST_DEVICE);
    MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyLocal(), localMeta_, true);
    DeviceMatrix::GetInstance().Online(TEST_DEVICE);
    result = isFinished_.GetValue();
    ASSERT_EQ(result.deviceId_, std::string(TEST_DEVICE));
    ASSERT_EQ(result.mask_, 0xE);
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
    auto code = DeviceMatrix::GetInstance().GetCode(meta);
    ASSERT_EQ(code, DeviceMatrix::META_STORE_MASK);
}

/**
* @tc.name: GetDPCode
* @tc.desc: get the device profile store mask code;
* @tc.type: FUNC
* @tc.require:
* @tc.author: blue sky
*/
HWTEST_F(DeviceMatrixTest, GetDPCode, TestSize.Level0)
{
    StoreMetaData meta = metaData_;
    meta.appId = "distributed_device_profile_service";
    meta.bundleName = "distributed_device_profile_service";
    auto code = DeviceMatrix::GetInstance().GetCode(meta);
    ASSERT_EQ(code, 0x2);
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
    auto mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, DeviceMatrix::META_STORE_MASK);
    ASSERT_EQ(mask, DeviceMatrix::META_STORE_MASK);
}

/**
* @tc.name: BroadcastDP
* @tc.desc: broadcast the device profile store change;
* @tc.type: FUNC
* @tc.require:
* @tc.author: blue sky
*/
HWTEST_F(DeviceMatrixTest, BroadcastDP, TestSize.Level0)
{
    StoreMetaData meta = metaData_;
    meta.appId = "distributed_device_profile_service";
    meta.bundleName = "distributed_device_profile_service";
    auto code = DeviceMatrix::GetInstance().GetCode(meta);
    auto mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, code);
    ASSERT_EQ(mask, 0x2);
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
    auto mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, code);
    ASSERT_EQ(mask, 0);
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
    auto mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, DeviceMatrix::META_STORE_MASK);
    ASSERT_EQ(mask, DeviceMatrix::META_STORE_MASK);
    StoreMetaData meta = metaData_;
    meta.appId = "distributed_device_profile_service";
    meta.bundleName = "distributed_device_profile_service";
    auto code = DeviceMatrix::GetInstance().GetCode(meta);
    mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, code);
    ASSERT_EQ(mask, 0x3);
    DeviceMatrix::GetInstance().OnExchanged(TEST_DEVICE, DeviceMatrix::META_STORE_MASK);
    DeviceMatrix::GetInstance().OnExchanged(TEST_DEVICE, 0x2);

    meta = metaData_;
    code = DeviceMatrix::GetInstance().GetCode(meta);
    mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, code);
    ASSERT_EQ(mask, 0);

    mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, 0xFFFF);
    ASSERT_EQ(mask, 0xF);
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
    metaData.version = 3;
    metaData.mask = 0x1F;
    metaData.deviceId = TEST_DEVICE;
    metaData.maskInfo = { TEST_BUNDLE, "distributed_device_profile_service" };
    MetaDataManager::GetInstance().Subscribe(MatrixMetaData::GetPrefix({ TEST_DEVICE }),
        [](const std::string &, const std::string &value, int32_t flag) {
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

    auto mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, 0x2);
    ASSERT_EQ(mask, 0);
    mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, DeviceMatrix::META_STORE_MASK);
    ASSERT_EQ(mask, DeviceMatrix::META_STORE_MASK);
    mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, 0x4);
    ASSERT_EQ(mask, 0x3);
    DeviceMatrix::GetInstance().OnExchanged(TEST_DEVICE, DeviceMatrix::META_STORE_MASK);
    DeviceMatrix::GetInstance().OnExchanged(TEST_DEVICE, 0x2);
    mask = DeviceMatrix::GetInstance().OnBroadcast(TEST_DEVICE, 0xFFFF);
    ASSERT_EQ(mask, 0x3);
    MetaDataManager::GetInstance().Unsubscribe(MatrixMetaData::GetPrefix({ TEST_DEVICE }));
}