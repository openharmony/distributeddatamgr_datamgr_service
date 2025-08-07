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

#define LOG_TAG "ObjectServiceImpMocklTest"
#include <gmock/gmock.h>
#include <ipc_skeleton.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "object_service_impl.h"
#include "mock_dm_adapter.h"
#include "mock_object_store_manager.h"
#include "mock_account_delegate.h"
#include "mock_bootstrap.h"
#include "mock_directory_manager.h"
#include "mock_meta_data_manager.h"
#include "mock_feature.h"
#include "mock_object_dms_handler.h"

using namespace testing;
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

class ObjectServiceImpMocklTest : public testing::Test {
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
// OnInitialize测试
TEST_F(ObjectServiceImplTest, OnInitialize_LocalDeviceIdEmpty_ReturnsError)
{
    // Given
    MockDmAdapter::MockGetLocalDeviceReturnsEmptyUuid();
    
    // When
    int32_t result = objectServiceImpl->OnInitialize();
    
    // Then
    EXPECT_EQ(result, OBJECT_INNER_ERROR);
}

TEST_F(ObjectServiceImplTest, OnInitialize_ExecutorsNull_ReturnsError)
{
    // Given
    MockDmAdapter::MockGetLocalDeviceReturnsValidUuid();
    objectServiceImpl->executors_ = nullptr;
    
    // When
    int32_t result = objectServiceImpl->OnInitialize();
    
    // Then
    EXPECT_EQ(result, OBJECT_INNER_ERROR);
}

TEST_F(ObjectServiceImplTest, OnInitialize_NormalCase_ReturnsSuccess)
{
    // Given
    MockDmAdapter::MockGetLocalDeviceReturnsValidUuid();
    objectServiceImpl->executors_ = std::make_shared<MockExecutors>();
    EXPECT_CALL(*std::static_pointer_cast<MockExecutors>(objectServiceImpl->executors_), Schedule(_, _))
        .Times(1);
    
    // When
    int32_t result = objectServiceImpl->OnInitialize();
    
    // Then
    EXPECT_EQ(result, OBJECT_SUCCESS);
}

// SaveMetaData测试
TEST_F(ObjectServiceImplTest, SaveMetaData_LocalDeviceIdEmpty_ReturnsError)
{
    // Given
    MockDmAdapter::MockGetLocalDeviceReturnsEmptyUuid();
    StoreMetaData saveMeta;
    
    // When
    int32_t result = objectServiceImpl->SaveMetaData(saveMeta);
    
    // Then
    EXPECT_EQ(result, OBJECT_INNER_ERROR);
}

TEST_F(ObjectServiceImplTest, SaveMetaData_CreateDirectoryFailed_ReturnsError)
{
    // Given
    MockDmAdapter::MockGetLocalDeviceReturnsValidUuid();
    MockBootstrap::MockGetProcessLabelReturnsValidLabel();
    MockAccountDelegate::MockGetCurrentAccountIdReturnsValidId();
    MockAccountDelegate::MockQueryForegroundUserIdReturnsValidId();
    MockDirectoryManager::MockCreateDirectoryReturns(false);
    StoreMetaData saveMeta;
    
    // When
    int32_t result = objectServiceImpl->SaveMetaData(saveMeta);
    
    // Then
    EXPECT_EQ(result, OBJECT_INNER_ERROR);
}

TEST_F(ObjectServiceImplTest, SaveMetaData_SaveMetaFailed_ReturnsError)
{
    // Given
    MockDmAdapter::MockGetLocalDeviceReturnsValidUuid();
    MockBootstrap::MockGetProcessLabelReturnsValidLabel();
    MockAccountDelegate::MockGetCurrentAccountIdReturnsValidId();
    MockAccountDelegate::MockQueryForegroundUserIdReturnsValidId();
    MockDirectoryManager::MockCreateDirectoryReturns(true);
    MockMetaDataManager::MockSaveMetaReturns(false);
    StoreMetaData saveMeta;
    
    // When
    int32_t result = objectServiceImpl->SaveMetaData(saveMeta);
    
    // Then
    EXPECT_EQ(result, OBJECT_INNER_ERROR);
}

TEST_F(ObjectServiceImplTest, SaveMetaData_NormalCase_ReturnsSuccess)
{
    // Given
    MockDmAdapter::MockGetLocalDeviceReturnsValidUuid();
    MockBootstrap::MockGetProcessLabelReturnsValidLabel();
    MockAccountDelegate::MockGetCurrentAccountIdReturnsValidId();
    MockAccountDelegate::MockQueryForegroundUserIdReturnsValidId();
    MockDirectoryManager::MockCreateDirectoryReturns(true);
    MockMetaDataManager::MockSaveMetaReturns(true);
    StoreMetaData saveMeta;
    
    // When
    int32_t result = objectServiceImpl->SaveMetaData(saveMeta);
    
    // Then
    EXPECT_EQ(result, OBJECT_SUCCESS);
}

// OnUserChange测试
TEST_F(ObjectServiceImplTest, OnUserChange_AccountSwitched_ClearFailed_StillCallsParent)
{
    // Given
    uint32_t code = static_cast<uint32_t>(AccountStatus::DEVICE_ACCOUNT_SWITCHED);
    std::string user = "testUser";
    std::string account = "testAccount";
    MockObjectStoreManager::MockClearReturns(OBJECT_INNER_ERROR);
    MockDmAdapter::MockGetLocalDeviceReturnsValidUuid();
    MockBootstrap::MockGetProcessLabelReturnsValidLabel();
    MockAccountDelegate::MockGetCurrentAccountIdReturnsValidId();
    MockAccountDelegate::MockQueryForegroundUserIdReturnsValidId();
    MockDirectoryManager::MockCreateDirectoryReturns(true);
    MockMetaDataManager::MockSaveMetaReturns(true);
    MockFeature::MockOnUserChangeReturns(OBJECT_SUCCESS);
    
    // When
    int32_t result = objectServiceImpl->OnUserChange(code, user, account);
    
    // Then
    EXPECT_EQ(result, OBJECT_SUCCESS);
}

TEST_F(ObjectServiceImplTest, OnUserChange_NotAccountSwitched_OnlyCallsParent)
{
    // Given
    uint32_t code = 0; // 不是账户切换
    std::string user = "testUser";
    std::string account = "testAccount";
    MockFeature::MockOnUserChangeReturns(OBJECT_SUCCESS);
    
    // When
    int32_t result = objectServiceImpl->OnUserChange(code, user, account);
    
    // Then
    EXPECT_EQ(result, OBJECT_SUCCESS);
}

// ObjectStoreRevokeSave测试
TEST_F(ObjectServiceImplTest, ObjectStoreRevokeSave_BundleNameCheckFailed_ReturnsError)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    sptr<IRemoteObject> callback = nullptr;
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    // 假设IsBundleNameEqualTokenId会失败
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_INNER_ERROR);
    
    // When
    int32_t result = objectServiceImpl->ObjectStoreRevokeSave(bundleName, sessionId, callback);
    
    // Then
    EXPECT_EQ(result, OBJECT_INNER_ERROR);
}

TEST_F(ObjectServiceImplTest, ObjectStoreRevokeSave_RevokeSaveFailed_ReturnsError)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    sptr<IRemoteObject> callback = nullptr;
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_SUCCESS);
    MockObjectStoreManager::MockRevokeSaveReturns(OBJECT_INNER_ERROR);
    
    // When
    int32_t result = objectServiceImpl->ObjectStoreRevokeSave(bundleName, sessionId, callback);
    
    // Then
    EXPECT_EQ(result, OBJECT_INNER_ERROR);
}

TEST_F(ObjectServiceImplTest, ObjectStoreRevokeSave_NormalCase_ReturnsSuccess)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    sptr<IRemoteObject> callback = nullptr;
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_SUCCESS);
    MockObjectStoreManager::MockRevokeSaveReturns(OBJECT_SUCCESS);
    
    // When
    int32_t result = objectServiceImpl->ObjectStoreRevokeSave(bundleName, sessionId, callback);
    
    // Then
    EXPECT_EQ(result, OBJECT_SUCCESS);
}

// ObjectStoreRetrieve测试
TEST_F(ObjectServiceImplTest, ObjectStoreRetrieve_BundleNameCheckFailed_ReturnsError)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    sptr<IRemoteObject> callback = nullptr;
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_INNER_ERROR);
    
    // When
    int32_t result = objectServiceImpl->ObjectStoreRetrieve(bundleName, sessionId, callback);
    
    // Then
    EXPECT_EQ(result, OBJECT_INNER_ERROR);
}

TEST_F(ObjectServiceImplTest, ObjectStoreRetrieve_RetrieveFailed_ReturnsError)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    sptr<IRemoteObject> callback = nullptr;
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_SUCCESS);
    MockObjectStoreManager::MockRetrieveReturns(OBJECT_INNER_ERROR);
    
    // When
    int32_t result = objectServiceImpl->ObjectStoreRetrieve(bundleName, sessionId, callback);
    
    // Then
    EXPECT_EQ(result, OBJECT_INNER_ERROR);
}

TEST_F(ObjectServiceImplTest, ObjectStoreRetrieve_NormalCase_ReturnsSuccess)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    sptr<IRemoteObject> callback = nullptr;
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_SUCCESS);
    MockObjectStoreManager::MockRetrieveReturns(OBJECT_SUCCESS);
    
    // When
    int32_t result = objectServiceImpl->ObjectStoreRetrieve(bundleName, sessionId, callback);
    
    // Then
    EXPECT_EQ(result, OBJECT_SUCCESS);
}

// RegisterDataObserver测试
TEST_F(ObjectServiceImplTest, RegisterDataObserver_BundleNameCheckFailed_ReturnsError)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    sptr<IRemoteObject> callback = nullptr;
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    MockIPCSkeleton::MockGetCallingPidReturns(456);
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_INNER_ERROR);
    
    // When
    int32_t result = objectServiceImpl->RegisterDataObserver(bundleName, sessionId, callback);
    
    // Then
    EXPECT_EQ(result, OBJECT_INNER_ERROR);
}

TEST_F(ObjectServiceImplTest, RegisterDataObserver_NormalCase_ReturnsSuccess)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    sptr<IRemoteObject> callback = nullptr;
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    MockIPCSkeleton::MockGetCallingPidReturns(456);
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_SUCCESS);
    MockObjectStoreManager::MockRegisterRemoteCallbackReturns();
    
    // When
    int32_t result = objectServiceImpl->RegisterDataObserver(bundleName, sessionId, callback);
    
    // Then
    EXPECT_EQ(result, OBJECT_SUCCESS);
}

// UnregisterDataChangeObserver测试
TEST_F(ObjectServiceImplTest, UnregisterDataChangeObserver_BundleNameCheckFailed_ReturnsError)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    MockIPCSkeleton::MockGetCallingPidReturns(456);
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_INNER_ERROR);
    
    // When
    int32_t result = objectServiceImpl->UnregisterDataChangeObserver(bundleName, sessionId);
    
    // Then
    EXPECT_EQ(result, OBJECT_INNER_ERROR);
}

TEST_F(ObjectServiceImplTest, UnregisterDataChangeObserver_NormalCase_ReturnsSuccess)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    MockIPCSkeleton::MockGetCallingPidReturns(456);
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_SUCCESS);
    MockObjectStoreManager::MockUnregisterRemoteCallbackReturns();
    
    // When
    int32_t result = objectServiceImpl->UnregisterDataChangeObserver(bundleName, sessionId);
    
    // Then
    EXPECT_EQ(result, OBJECT_SUCCESS);
}

// RegisterProgressObserver测试
TEST_F(ObjectServiceImplTest, RegisterProgressObserver_BundleNameCheckFailed_ReturnsError)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    sptr<IRemoteObject> callback = nullptr;
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    MockIPCSkeleton::MockGetCallingPidReturns(456);
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_INNER_ERROR);
    
    // When
    int32_t result = objectServiceImpl->RegisterProgressObserver(bundleName, sessionId, callback);
    
    // Then
    EXPECT_EQ(result, OBJECT_INNER_ERROR);
}

TEST_F(ObjectServiceImplTest, RegisterProgressObserver_NormalCase_ReturnsSuccess)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    sptr<IRemoteObject> callback = nullptr;
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    MockIPCSkeleton::MockGetCallingPidReturns(456);
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_SUCCESS);
    MockObjectStoreManager::MockRegisterProgressObserverCallbackReturns();
    
    // When
    int32_t result = objectServiceImpl->RegisterProgressObserver(bundleName, sessionId, callback);
    
    // Then
    EXPECT_EQ(result, OBJECT_SUCCESS);
}

// UnregisterProgressObserver测试
TEST_F(ObjectServiceImplTest, UnregisterProgressObserver_BundleNameCheckFailed_ReturnsError)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    MockIPCSkeleton::MockGetCallingPidReturns(456);
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_INNER_ERROR);
    
    // When
    int32_t result = objectServiceImpl->UnregisterProgressObserver(bundleName, sessionId);
    
    // Then
    EXPECT_EQ(result, OBJECT_INNER_ERROR);
}

TEST_F(ObjectServiceImplTest, UnregisterProgressObserver_NormalCase_ReturnsSuccess)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    MockIPCSkeleton::MockGetCallingPidReturns(456);
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_SUCCESS);
    MockObjectStoreManager::MockUnregisterProgressObserverCallbackReturns();
    
    // When
    int32_t result = objectServiceImpl->UnregisterProgressObserver(bundleName, sessionId);
    
    // Then
    EXPECT_EQ(result, OBJECT_SUCCESS);
}

// DeleteSnapshot测试
TEST_F(ObjectServiceImplTest, DeleteSnapshot_BundleNameCheckFailed_ReturnsError)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_INNER_ERROR);
    
    // When
    int32_t result = objectServiceImpl->DeleteSnapshot(bundleName, sessionId);
    
    // Then
    EXPECT_EQ(result, OBJECT_INNER_ERROR);
}

TEST_F(ObjectServiceImplTest, DeleteSnapshot_NormalCase_ReturnsSuccess)
{
    // Given
    std::string bundleName = "testBundle";
    std::string sessionId = "testSession";
    MockIPCSkeleton::MockGetCallingTokenIDReturns(123);
    MockObjectServiceImpl::MockIsBundleNameEqualTokenIdReturns(OBJECT_SUCCESS);
    MockObjectStoreManager::MockDeleteSnapshotReturns();
    
    // When
    int32_t result = objectServiceImpl->DeleteSnapshot(bundleName, sessionId);
    
    // Then
    EXPECT_EQ(result, OBJECT_SUCCESS);
}


/**
 * @tc.name: IsNeedMetaSync001
 * @tc.desc: Test IsNeedMetaSync when LoadMeta fails for CapMetaData.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(ObjectServiceImpMocklTest, IsNeedMetaSync001, TestSize.Level0)
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
HWTEST_F(ObjectServiceImpMocklTest, IsNeedMetaSync002, TestSize.Level0)
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
HWTEST_F(ObjectServiceImpMocklTest, IsNeedMetaSync003, TestSize.Level0)
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
HWTEST_F(ObjectServiceImpMocklTest, SyncOnStore001, TestSize.Level0)
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
HWTEST_F(ObjectServiceImpMocklTest, GetCurrentUser001, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, GetCurrentUser002, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, GetCurrentUser003, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, WaitAssets001, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, NotifyAssetsReady001, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, DoNotifyAssetsReady001, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, DoNotifyWaitAssetTimeout001, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, FlushClosedStore001, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, CloseAfterMinute001, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, UnRegisterAssetsLister001, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, InitUserMeta001, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, InitUserMeta002, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, InitUserMeta003, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, BindAsset001, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, BindAsset002, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, IsContinue001, TestSize.Level1)
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
HWTEST_F(ObjectServiceImpMocklTest, IsContinue002, TestSize.Level1)
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