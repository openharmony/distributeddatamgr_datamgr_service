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

#define LOG_TAG "ObjectManagerMockTest"
#include "object_manager.h"

#include <gmock/gmock.h>
#include "gtest/gtest.h"

#include "device_matrix_mock.h"
#include "mock/meta_data_manager_mock.h"
#include "device_manager_adapter_mock.h"

using namespace OHOS::DistributedObject;
using namespace OHOS::DistributedData;
using namespace testing::ext;
using namespace testing;
using DeviceInfo = OHOS::AppDistributedKv::DeviceInfo;
using OnComplete = OHOS::DistributedData::MetaDataManager::OnComplete;

namespace OHOS::Test {
namespace DistributedDataTest {
class ObjectManagerMockTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        metaDataManagerMock = std::make_shared<MetaDataManagerMock>();
        BMetaDataManager::metaDataManager = metaDataManagerMock;
        metaDataMock = std::make_shared<MetaDataMock<StoreMetaData>>();
        BMetaData<StoreMetaData>::metaDataManager = metaDataMock;
        devMgrAdapterMock = std::make_shared<DeviceManagerAdapterMock>();
        BDeviceManagerAdapter::deviceManagerAdapter = devMgrAdapterMock;
        deviceMatrixMock = std::make_shared<DeviceMatrixMock>();
        BDeviceMatrix::deviceMatrix = deviceMatrixMock;
    }
    static void TearDownTestCase(void)
    {
        metaDataManagerMock = nullptr;
        BMetaDataManager::metaDataManager = nullptr;
        metaDataMock = nullptr;
        BMetaData<StoreMetaData>::metaDataManager = nullptr;
        devMgrAdapterMock = nullptr;
        BDeviceManagerAdapter::deviceManagerAdapter = nullptr;
        deviceMatrixMock = nullptr;
        BDeviceMatrix::deviceMatrix = nullptr;
    }

    static inline std::shared_ptr<MetaDataManagerMock> metaDataManagerMock = nullptr;
    static inline std::shared_ptr<MetaDataMock<StoreMetaData>> metaDataMock = nullptr;
    static inline std::shared_ptr<DeviceManagerAdapterMock> devMgrAdapterMock = nullptr;
    static inline std::shared_ptr<DeviceMatrixMock> deviceMatrixMock = nullptr;
    void SetUp() {};
    void TearDown() {};
};

/**
 * @tc.name: IsNeedMetaSync001
 * @tc.desc: Test IsNeedMetaSync when LoadMeta fails for CapMetaData.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(ObjectManagerMockTest, IsNeedMetaSync001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    StoreMetaData meta;
    meta.deviceId = "test_device_id";
    meta.user = "0";
    meta.storeId = "distributedObject_";
    meta.bundleName = "test_bundle";
    std::vector<std::string> uuids = {"test_uuid"};

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(false));
    bool isNeedSync = manager->IsNeedMetaSync(meta, uuids);
    EXPECT_EQ(isNeedSync, true);
 
    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(true));
    isNeedSync = manager->IsNeedMetaSync(meta, uuids);
    EXPECT_EQ(isNeedSync, true);
 
    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    isNeedSync = manager->IsNeedMetaSync(meta, uuids);
    EXPECT_EQ(isNeedSync, true);
}

/**
 * @tc.name: IsNeedMetaSync002
 * @tc.desc: Test IsNeedMetaSync when LoadMeta fails for StoreMetaData.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(ObjectManagerMockTest, IsNeedMetaSync002, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    StoreMetaData meta;
    meta.deviceId = "test_device_id";
    meta.user = "0";
    meta.storeId = "distributedObject_";
    meta.bundleName = "test_bundle";
    
    std::vector<std::string> uuids = {"test_uuid"};

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
        .WillRepeatedly(Return((true)));

    EXPECT_CALL(*deviceMatrixMock, GetRemoteMask(_, _))
        .WillRepeatedly(Return(std::make_pair(true, DeviceMatrix::META_STORE_MASK)));
    
    bool result = manager->IsNeedMetaSync(meta, uuids);
    EXPECT_EQ(result, true);
}

/**
 * @tc.name: IsNeedMetaSync003
 * @tc.desc: Test IsNeedMetaSync when LoadMeta fails for StoreMetaData.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(ObjectManagerMockTest, IsNeedMetaSync003, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    StoreMetaData meta;
    meta.deviceId = "test_device_id";
    meta.user = "0";
    meta.storeId = "distributedObject_";
    meta.bundleName = "test_bundle";
    
    std::vector<std::string> uuids = {"test_uuid"};

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*deviceMatrixMock, GetRemoteMask(_, _))
        .WillOnce(Return(std::make_pair(true, 0)));

    EXPECT_CALL(*deviceMatrixMock, GetMask(_, _))
        .WillOnce(Return(std::make_pair(true, DeviceMatrix::META_STORE_MASK)));
    
    bool result = manager->IsNeedMetaSync(meta, uuids);
    EXPECT_EQ(result, true);
}

HWTEST_F(ObjectManagerMockTest, SyncOnStore001, TestSize.Level0)
{
    auto manager = ObjectStoreManager::GetInstance();
    auto func = [](const std::map<std::string, int32_t> &results) {
        return results;
    };
    std::string prefix = "ObjectManagerTest";
    StoreMetaData meta;
    meta.deviceId = "test_device_id";
    meta.user = "0";
    meta.storeId = "distributedObject_";
    meta.bundleName = "test_bundle";
    std::vector<std::string> uuids = {"test_uuid"};


    // local device
    {
        std::vector<std::string> localDeviceList = {"local"};
        auto result = manager->SyncOnStore(prefix, localDeviceList, func);
        EXPECT_EQ(result, OBJECT_SUCCESS);
    }

    // remote device. IsNeedMetaSync: true; Sync: true
    {
        std::vector<std::string> remoteDeviceList = {"remote_device_1"};

        EXPECT_CALL(*devMgrAdapterMock, GetUuidByNetworkId(_)).WillRepeatedly(Return("mock_uuid"));
        EXPECT_CALL(*devMgrAdapterMock, ToUUID(_))
            .WillOnce(Return(std::vector<std::string>{"mock_uuid_1"}));
        EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
            .WillOnce(testing::Return(false))
            .WillOnce(testing::Return(false));
        EXPECT_CALL(*metaDataManagerMock, Sync(_, _, _)).WillOnce(testing::Return(true));
        auto result = manager->SyncOnStore(prefix, remoteDeviceList, func);
        EXPECT_EQ(result, OBJECT_SUCCESS);
    }

    // remote device. IsNeedMetaSync: true; Sync: false
    {
        std::vector<std::string> remoteDeviceList = {"remote_device_1"};

        EXPECT_CALL(*devMgrAdapterMock, GetUuidByNetworkId(_)).WillRepeatedly(Return("mock_uuid"));
        EXPECT_CALL(*devMgrAdapterMock, ToUUID(_))
            .WillOnce(Return(std::vector<std::string>{"mock_uuid_1"}));
        EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
            .WillOnce(testing::Return(false))
            .WillOnce(testing::Return(false));
        EXPECT_CALL(*metaDataManagerMock, Sync(_, _, _)).WillOnce(testing::Return(false));
        auto result = manager->SyncOnStore(prefix, remoteDeviceList, func);
        EXPECT_EQ(result, E_DB_ERROR);
    }
}
}; // namespace OHOS::Test
}