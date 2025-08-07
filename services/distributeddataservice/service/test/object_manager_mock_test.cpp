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
#include <gmock/gmock.h>
#include <ipc_skeleton.h>

#include "device_manager_adapter_mock.h"
#include "device_matrix_mock.h"
#include "gtest/gtest.h"
#include "mock/access_token_mock.h"
#include "mock/account_delegate_mock.h"
#include "mock/distributed_file_daemon_manager_mock.h"
#include "mock/meta_data_manager_mock.h"
#include "object_manager.h"
#include "object_service_impl.h"


using namespace OHOS::DistributedObject;
using namespace OHOS::DistributedData;
using namespace OHOS::Storage::DistributedFile;
using namespace OHOS::Security::AccessToken;
using namespace testing::ext;
using namespace testing;
using AssetValue = OHOS::CommonType::AssetValue;
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
        fileDaemonMgrMock = std::make_shared<DistributedFileDaemonManagerMock>();
        BDistributedFileDaemonManager::fileDaemonManger_ = fileDaemonMgrMock;
        accountDelegateMock = new (std::nothrow) AccountDelegateMock();
        if (accountDelegateMock != nullptr) {
            AccountDelegate::instance_ = nullptr;
            AccountDelegate::RegisterAccountInstance(accountDelegateMock);
        }
        accTokenMock = std::make_shared<AccessTokenKitMock>();
        BAccessTokenKit::accessTokenkit = accTokenMock;
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
        fileDaemonMgrMock = nullptr;
        BDistributedFileDaemonManager::fileDaemonManger_ = nullptr;
        if (accountDelegateMock != nullptr) {
            delete accountDelegateMock;
            accountDelegateMock = nullptr;
        }
        accTokenMock = nullptr;
        BAccessTokenKit::accessTokenkit = nullptr;
    }

    static inline std::shared_ptr<MetaDataManagerMock> metaDataManagerMock = nullptr;
    static inline std::shared_ptr<MetaDataMock<StoreMetaData>> metaDataMock = nullptr;
    static inline std::shared_ptr<DeviceManagerAdapterMock> devMgrAdapterMock = nullptr;
    static inline std::shared_ptr<DeviceMatrixMock> deviceMatrixMock = nullptr;
    static inline std::shared_ptr<DistributedFileDaemonManagerMock> fileDaemonMgrMock = nullptr;
    static inline AccountDelegateMock *accountDelegateMock = nullptr;
    static inline std::shared_ptr<AccessTokenKitMock> accTokenMock = nullptr;
    void SetUp() {};
    void TearDown() {};

protected:
    std::string sessionId_ = "123";
    OHOS::ObjectStore::AssetBindInfo assetBindInfo_;
    AssetValue assetValue_;
};

/**
 * @tc.name: IsNeedMetaSync001
 * @tc.desc: Test IsNeedMetaSync when LoadMeta fails for CapMetaData.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ObjectManagerMockTest, IsNeedMetaSync001, TestSize.Level0)
{
    EXPECT_CALL(*fileDaemonMgrMock, RegisterAssetCallback(_)).WillOnce(testing::Return(0));
    auto &manager = ObjectStoreManager::GetInstance();
    StoreMetaData meta;
    meta.deviceId = "test_device_id";
    meta.user = "0";
    meta.storeId = "distributedObject_";
    meta.bundleName = "test_bundle";
    std::vector<std::string> uuids = { "test_uuid" };

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_)).WillOnce(testing::Return(false));
    bool isNeedSync = manager.IsNeedMetaSync(meta, uuids);
    EXPECT_EQ(isNeedSync, true);
    EXPECT_CALL(*metaDataManagerMock, LoadMeta(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    isNeedSync = manager.IsNeedMetaSync(meta, uuids);
    EXPECT_EQ(isNeedSync, true);
}

/**
 * @tc.name: IsNeedMetaSync002
 * @tc.desc: Test IsNeedMetaSync when LoadMeta fails for StoreMetaData.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ObjectManagerMockTest, IsNeedMetaSync002, TestSize.Level0)
{
    auto &manager = ObjectStoreManager::GetInstance();
    StoreMetaData meta;
    meta.deviceId = "test_device_id";
    meta.user = "0";
    meta.storeId = "distributedObject_";
    meta.bundleName = "test_bundle";
    std::vector<std::string> uuids = { "test_uuid" };

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _)).WillRepeatedly(Return((true)));
    EXPECT_CALL(*deviceMatrixMock, GetRemoteMask(_, _))
        .WillRepeatedly(Return(std::make_pair(true, DeviceMatrix::META_STORE_MASK)));

    bool result = manager.IsNeedMetaSync(meta, uuids);
    EXPECT_EQ(result, true);
}

/**
 * @tc.name: IsNeedMetaSync003
 * @tc.desc: Test IsNeedMetaSync when LoadMeta fails for StoreMetaData.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ObjectManagerMockTest, IsNeedMetaSync003, TestSize.Level0)
{
    auto &manager = ObjectStoreManager::GetInstance();
    StoreMetaData meta;
    meta.deviceId = "test_device_id";
    meta.user = "0";
    meta.storeId = "distributedObject_";
    meta.bundleName = "test_bundle";
    std::vector<std::string> uuids = { "test_uuid" };

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(*deviceMatrixMock, GetRemoteMask(_, _)).WillOnce(Return(std::make_pair(true, 0)));
    EXPECT_CALL(*deviceMatrixMock, GetMask(_, _)).WillOnce(Return(std::make_pair(true, DeviceMatrix::META_STORE_MASK)));

    bool result = manager.IsNeedMetaSync(meta, uuids);
    EXPECT_EQ(result, true);

    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _)).WillRepeatedly(Return(true));

    EXPECT_CALL(*deviceMatrixMock, GetRemoteMask(_, _)).WillOnce(Return(std::make_pair(true, 0)));

    EXPECT_CALL(*deviceMatrixMock, GetMask(_, _)).WillOnce(Return(std::make_pair(true, 0)));

    result = manager.IsNeedMetaSync(meta, uuids);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: SyncOnStore001
 * @tc.desc: Test SyncOnStore.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ObjectManagerMockTest, SyncOnStore001, TestSize.Level0)
{
    // 2 means that the GetUserByToken interface will be called twice
    EXPECT_CALL(*accountDelegateMock, GetUserByToken(_)).Times(2).WillRepeatedly(Return(0));
    auto &manager = ObjectStoreManager::GetInstance();
    std::function<void(const std::map<std::string, int32_t> &results)> func;
    func = [](const std::map<std::string, int32_t> &results) { return results; };
    std::string prefix = "ObjectManagerTest";
    StoreMetaData meta;
    meta.deviceId = "test_device_id";
    meta.user = "0";
    meta.storeId = "distributedObject_";
    meta.bundleName = "test_bundle";
    std::vector<std::string> uuids = { "test_uuid" };

    // local device
    {
        std::vector<std::string> localDeviceList = { "local" };
        auto result = manager.SyncOnStore(prefix, localDeviceList, func);
        EXPECT_EQ(result, OBJECT_SUCCESS);
    }

    // remote device. IsNeedMetaSync: true; Sync: true
    {
        std::vector<std::string> remoteDeviceList = { "remote_device_1" };
        EXPECT_CALL(*devMgrAdapterMock, GetUuidByNetworkId(_)).WillRepeatedly(Return("mock_uuid"));
        EXPECT_CALL(*devMgrAdapterMock, ToUUID(testing::A<const std::vector<std::string> &>()))
            .WillOnce(Return(std::vector<std::string>{ "mock_uuid_1" }));
        EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _)).WillOnce(testing::Return(false));
        EXPECT_CALL(*metaDataManagerMock, Sync(_, _, _)).WillOnce(testing::Return(true));
        auto result = manager.SyncOnStore(prefix, remoteDeviceList, func);
        EXPECT_EQ(result, OBJECT_SUCCESS);
    }

    // remote device. IsNeedMetaSync: false
    {
        std::vector<std::string> remoteDeviceList = { "remote_device_1" };
        EXPECT_CALL(*devMgrAdapterMock, GetUuidByNetworkId(_)).WillRepeatedly(Return("mock_uuid"));
        EXPECT_CALL(*devMgrAdapterMock, ToUUID(testing::A<const std::vector<std::string> &>()))
            .WillOnce(Return(std::vector<std::string>{ "mock_uuid_1" }));
        EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
            .WillOnce(testing::Return(true))
            .WillOnce(testing::Return(true));
        EXPECT_CALL(*deviceMatrixMock, GetRemoteMask(_, _)).WillOnce(Return(std::make_pair(true, 0)));
        EXPECT_CALL(*deviceMatrixMock, GetMask(_, _)).WillOnce(Return(std::make_pair(true, 0)));
        auto result = manager.SyncOnStore(prefix, remoteDeviceList, func);
        EXPECT_EQ(result, E_DB_ERROR);
    }
}

/**
* @tc.name: GetCurrentUser001
* @tc.desc: Test the scenario where the QueryUsers return false in the GetCurrentUser function.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(ObjectManagerMockTest, GetCurrentUser001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::vector<int> users;
    EXPECT_CALL(*accountDelegateMock, QueryUsers(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(users), Return(false)));
    auto result = manager.GetCurrentUser();
    EXPECT_EQ(result, "");
}

/**
* @tc.name: GetCurrentUser002
* @tc.desc: Test the scenario where the QueryUsers users empty in the GetCurrentUser function.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(ObjectManagerMockTest, GetCurrentUser002, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::vector<int> users;
    EXPECT_CALL(*accountDelegateMock, QueryUsers(_))
        .Times(1)
        .WillOnce(
            DoAll(SetArgReferee<0>(users), Invoke([](std::vector<int> &users) { users.clear(); }), Return(true)));
    auto result = manager.GetCurrentUser();
    EXPECT_EQ(result, "");
}

/**
* @tc.name: GetCurrentUser003
* @tc.desc: Test the scenario where the QueryUsers return true in the GetCurrentUser function.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(ObjectManagerMockTest, GetCurrentUser003, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::vector<int> users = { 0, 1 };
    EXPECT_CALL(*accountDelegateMock, QueryUsers(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(users), Return(true)));
    auto result = manager.GetCurrentUser();
    EXPECT_EQ(result, std::to_string(users[0]));
}

/**
* @tc.name: WaitAssets001
* @tc.desc: WaitAssets test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerMockTest, WaitAssets001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string objectKey = "objectKey";
    ObjectStoreManager::SaveInfo info;
    std::map<std::string, ObjectRecord> data;
    auto ret = manager.WaitAssets(objectKey, info, data);
    EXPECT_EQ(ret, DistributedObject::OBJECT_INNER_ERROR);
}

/**
* @tc.name: NotifyAssetsReady001
* @tc.desc: NotifyAssetsReady test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerMockTest, NotifyAssetsReady001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string objectKey = "objectKey";
    std::string bundleName = "bundleName";
    std::string srcNetworkId = "srcNetworkId";
    manager.NotifyAssetsReady(objectKey, bundleName, srcNetworkId);
    EXPECT_EQ(manager.executors_, nullptr);
}

/**
* @tc.name: DoNotifyAssetsReady001
* @tc.desc: DoNotifyAssetsReady test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerMockTest, DoNotifyAssetsReady001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    uint32_t tokenId = 0;
    ObjectStoreManager::CallbackInfo info;
    std::string objectKey = "objectKey";
    bool isReady = true;
    manager.DoNotifyAssetsReady(tokenId, info, objectKey, isReady);
    EXPECT_EQ(manager.executors_, nullptr);
}

/**
* @tc.name: DoNotifyWaitAssetTimeout001
* @tc.desc: DoNotifyWaitAssetTimeout test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerMockTest, DoNotifyWaitAssetTimeout001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string objectKey = "objectKey";
    manager.DoNotifyWaitAssetTimeout(objectKey);
    EXPECT_EQ(manager.executors_, nullptr);
}

/**
* @tc.name: FlushClosedStore001
* @tc.desc: FlushClosedStore test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerMockTest, FlushClosedStore001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    manager.FlushClosedStore();
    EXPECT_EQ(manager.executors_, nullptr);
}

/**
* @tc.name: CloseAfterMinute001
* @tc.desc: CloseAfterMinute test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerMockTest, CloseAfterMinute001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    manager.CloseAfterMinute();
    EXPECT_EQ(manager.executors_, nullptr);
}

/**
* @tc.name: UnRegisterAssetsLister001
* @tc.desc: UnRegisterAssetsLister test.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerMockTest, UnRegisterAssetsLister001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    manager.objectAssetsRecvListener_ = nullptr;
    auto ret = manager.UnRegisterAssetsLister();
    EXPECT_EQ(ret, true);
    EXPECT_CALL(*fileDaemonMgrMock, RegisterAssetCallback(_)).WillOnce(testing::Return(0));
    manager.RegisterAssetsLister();
    EXPECT_CALL(*fileDaemonMgrMock, UnRegisterAssetCallback(_)).WillOnce(testing::Return(-1));
    ret = manager.UnRegisterAssetsLister();
    EXPECT_EQ(ret, false);
    EXPECT_CALL(*fileDaemonMgrMock, UnRegisterAssetCallback(_)).WillOnce(testing::Return(0));
    ret = manager.UnRegisterAssetsLister();
    EXPECT_EQ(ret, true);
}

/**
* @tc.name: InitUserMeta001
* @tc.desc: Test the scenario where the QueryUsers return false in the GetCurrentUser function.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerMockTest, InitUserMeta001, TestSize.Level1)
{
    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
            .WillOnce(testing::Return(false));
    auto &manager = ObjectStoreManager::GetInstance();
    std::vector<int> users;
    EXPECT_CALL(*accountDelegateMock, QueryUsers(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(users), Return(false)));
    auto status = manager.InitUserMeta();
    EXPECT_EQ(status, DistributedObject::OBJECT_INNER_ERROR);
}

/**
* @tc.name: InitUserMeta002
* @tc.desc: Test the scenario where the QueryUsers users empty in the GetCurrentUser function.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerMockTest, InitUserMeta002, TestSize.Level1)
{
    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
            .WillOnce(testing::Return(false));
    auto &manager = ObjectStoreManager::GetInstance();
    std::vector<int> users;
    EXPECT_CALL(*accountDelegateMock, QueryUsers(_))
        .Times(1)
        .WillOnce(
            DoAll(SetArgReferee<0>(users), Invoke([](std::vector<int> &users) { users.clear(); }), Return(true)));
    auto status = manager.InitUserMeta();
    EXPECT_EQ(status, DistributedObject::OBJECT_INNER_ERROR);
}

/**
* @tc.name: InitUserMeta003
* @tc.desc: Test the scenario where the QueryUsers return true in the GetCurrentUser function.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerMockTest, InitUserMeta003, TestSize.Level1)
{
    EXPECT_CALL(*metaDataManagerMock, LoadMeta(_, _, _))
            .WillOnce(testing::Return(false));
    DeviceInfo devInfo = { .uuid = "666" };
    EXPECT_CALL(*devMgrAdapterMock, GetLocalDevice()).WillOnce(Return(devInfo));
    auto &manager = ObjectStoreManager::GetInstance();
    std::vector<int> users = { 0, 1 };
    EXPECT_CALL(*accountDelegateMock, QueryUsers(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(users), Return(true)));
    auto status = manager.InitUserMeta();
    EXPECT_EQ(status, DistributedObject::OBJECT_INNER_ERROR);
}

/**
* @tc.name: BindAsset001
* @tc.desc: Test BindAsset function when GetTokenTypeFlag is not TOKEN_HAP.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerMockTest, BindAsset001, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string bundleName = "BindAsset";
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(_))
        .Times(1)
        .WillOnce(Return(ATokenTypeEnum::TOKEN_NATIVE));
    auto result = manager.BindAsset(tokenId, bundleName, sessionId_, assetValue_, assetBindInfo_);
    EXPECT_EQ(result, DistributedObject::OBJECT_DBSTATUS_ERROR);
}

/**
* @tc.name: BindAsset002
* @tc.desc: Test BindAsset function when GetTokenTypeFlag is TOKEN_HAP.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerMockTest, BindAsset002, TestSize.Level1)
{
    auto &manager = ObjectStoreManager::GetInstance();
    std::string bundleName = "BindAsset";
    uint32_t tokenId = IPCSkeleton::GetCallingTokenID();
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(_))
        .Times(1)
        .WillOnce(Return(ATokenTypeEnum::TOKEN_HAP));
    EXPECT_CALL(*accTokenMock, GetHapTokenInfo(_, _))
        .Times(1)
        .WillOnce(Return(0));
    auto result = manager.BindAsset(tokenId, bundleName, sessionId_, assetValue_, assetBindInfo_);
    EXPECT_EQ(result, DistributedObject::OBJECT_SUCCESS);
}

/**
* @tc.name: IsContinue001
* @tc.desc: Test IsContinue function when GetTokenTypeFlag is not TOKEN_HAP.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerMockTest, IsContinue001, TestSize.Level1)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    bool isContinue = false;
    EXPECT_CALL(*fileDaemonMgrMock, UnRegisterAssetCallback(_)).WillRepeatedly(testing::Return(0));
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(_))
        .Times(1)
        .WillOnce(Return(ATokenTypeEnum::TOKEN_NATIVE));
    auto ret = objectServiceImpl->IsContinue(isContinue);
    EXPECT_EQ(ret, DistributedObject::OBJECT_INNER_ERROR);
}

/**
* @tc.name: IsContinue002
* @tc.desc: Test IsContinue function when GetTokenTypeFlag is TOKEN_HAP.
* @tc.type: FUNC
*/
HWTEST_F(ObjectManagerMockTest, IsContinue002, TestSize.Level1)
{
    std::shared_ptr<ObjectServiceImpl> objectServiceImpl = std::make_shared<ObjectServiceImpl>();
    bool isContinue = false;
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(_))
        .Times(2)
        .WillRepeatedly(Return(ATokenTypeEnum::TOKEN_HAP));
    EXPECT_CALL(*accTokenMock, GetHapTokenInfo(_, _))
        .Times(1)
        .WillOnce(Return(0));
    auto ret = objectServiceImpl->IsContinue(isContinue);
    EXPECT_EQ(ret, DistributedObject::OBJECT_SUCCESS);
}
}; // namespace DistributedDataTest
} // namespace OHOS::Test