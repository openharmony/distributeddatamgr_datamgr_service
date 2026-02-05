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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "account/account_delegate.h"
#include "bootstrap.h"
#include "checker_mock.h"
#include "crypto/crypto_manager.h"
#include "device_manager_adapter.h"
#include "device_matrix.h"
#include "directory/directory_manager.h"
#include "ipc_skeleton.h"
#include "metadata/appid_meta_data.h"
#include "metadata/capability_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/meta_data_saver.h"
#include "metadata/special_channel_data.h"
#include "metadata/store_debug_info.h"
#include "metadata/store_meta_data_local.h"
#include "mock/access_token_mock.h"
#include "mock/db_store_mock.h"
#include "mock/device_manager_adapter_mock.h"
#include "mock/general_store_mock.h"
#include "rdb_general_store.h"
#include "rdb_service_impl.h"
#include "rdb_common_utils.h"
#include "rdb_types.h"
#include "relational_store_manager.h"
using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
using namespace DistributedDB;
using namespace OHOS::Security::AccessToken;
using namespace testing::ext;
using namespace testing;
using namespace std;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using RdbGeneralStore = OHOS::DistributedRdb::RdbGeneralStore;

namespace OHOS::Test {
namespace DistributedRDBTest {

static constexpr const char *TEST_BUNDLE = "test_rdb_service_impl_bundleName";
static constexpr const char *TEST_APPID = "test_rdb_service_impl_appid";
static constexpr const char *TEST_STORE = "test_rdb_service_impl_store";

class RdbServiceImplTokenTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    static void InitMetaData();
    void SetUp();
    void TearDown();
protected:
    static inline std::shared_ptr<AccessTokenKitMock> accTokenMock = nullptr;
    static inline std::shared_ptr<TokenIdKitMock> tokenIdMock = nullptr;
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static int32_t dbStatus_;
    static StoreMetaData metaData_;
    static CheckerMock checkerMock_;
    static void InitMetaDataManager();
    static void GetRdbSyncerParam(RdbSyncerParam &param);
    static inline std::shared_ptr<DeviceManagerAdapterMock> deviceManagerAdapterMock = nullptr;
};
std::shared_ptr<DBStoreMock> RdbServiceImplTokenTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
StoreMetaData RdbServiceImplTokenTest::metaData_;
CheckerMock RdbServiceImplTokenTest::checkerMock_;
int32_t RdbServiceImplTokenTest::dbStatus_ = E_OK;


void RdbServiceImplTokenTest::InitMetaData()
{
    metaData_.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.appId = TEST_APPID;
    metaData_.bundleName = TEST_BUNDLE;
    metaData_.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    metaData_.user = std::to_string(AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId));
    metaData_.area = OHOS::DistributedKv::EL1;
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = DistributedRdb::RDB_DEVICE_COLLABORATION;
    metaData_.storeId = TEST_STORE;
    metaData_.dataDir = DirectoryManager::GetInstance().GetStorePath(metaData_) + "/" + TEST_STORE;
}

void RdbServiceImplTokenTest::InitMetaDataManager()
{
    MetaDataManager::GetInstance().SetSyncer([](const auto &, auto) {
        DeviceMatrix::GetInstance().OnChanged(DeviceMatrix::META_STORE_MASK);
    });
}

void RdbServiceImplTokenTest::SetUpTestCase()
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    deviceManagerAdapterMock = std::make_shared<DeviceManagerAdapterMock>();
    BDeviceManagerAdapter::deviceManagerAdapter = deviceManagerAdapterMock;
    accTokenMock = std::make_shared<AccessTokenKitMock>();
    tokenIdMock = std::make_shared<TokenIdKitMock>();
    BAccessTokenKit::accessTokenkit = accTokenMock;
    BTokenIdKit::tokenkIdKit = tokenIdMock;
    InitMetaData();
    InitMetaDataManager();
    Bootstrap::GetInstance().LoadCheckers();
    CryptoManager::GetInstance().GenerateRootKey();
        // Construct the statisticInfo data
    AutoCache::GetInstance().RegCreator(RDB_DEVICE_COLLABORATION,
        [](const StoreMetaData &metaData, const AutoCache::StoreOption &option) -> std::pair<int32_t, GeneralStore *> {
            auto store = new (std::nothrow) GeneralStoreMock();
            if (store != nullptr) {
                store->SetMockDBStatus(dbStatus_);
                return { GeneralError::E_OK, store };
            }
            return { GeneralError::E_ERROR, nullptr };
        });
}

void RdbServiceImplTokenTest::TearDownTestCase()
{
    deviceManagerAdapterMock = nullptr;
    BDeviceManagerAdapter::deviceManagerAdapter = nullptr;
    accTokenMock = nullptr;
    BAccessTokenKit::accessTokenkit = nullptr;
    tokenIdMock = nullptr;
    BTokenIdKit::tokenkIdKit = nullptr;
    AutoCache::GetInstance().RegCreator(DistributedRdb::RDB_DEVICE_COLLABORATION,
        [](const StoreMetaData &metaData, const AutoCache::StoreOption &option) -> std::pair<int32_t, GeneralStore *> {
            auto store = new (std::nothrow) RdbGeneralStore(metaData, option.createRequired);
            if (store == nullptr) {
                return { GeneralError::E_ERROR, nullptr };
            }
            auto ret = store->Init();
            if (ret != GeneralError::E_OK) {
                delete store;
                store = nullptr;
            }
            return { ret, store };
        });
}

void RdbServiceImplTokenTest::SetUp()
{
}

void RdbServiceImplTokenTest::TearDown()
{
}

void RdbServiceImplTokenTest::GetRdbSyncerParam(RdbSyncerParam &param)
{
    param.bundleName_ = metaData_.bundleName;
    param.type_ = metaData_.storeType;
    param.level_ = metaData_.securityLevel;
    param.area_ = metaData_.area;
    param.hapName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    param.isEncrypt_ = metaData_.isEncrypt;
    param.isSearchable_ = metaData_.isSearchable;
    param.haMode_ = metaData_.haMode;
    param.asyncDownloadAsset_ = metaData_.asyncDownloadAsset;
    param.user_ = metaData_.user;
    std::map<std::string, std::vector<std::string>> map;
    std::vector<std::string> devices;
    devices.push_back("testdevice");
    map["test"] = devices;
    param.removeDataExceptDevicesMap_ = map;
}

/**
 * @tc.name: VerifyPromiseInfo001
 * @tc.desc: Test VerifyPromiseInfo when tokenId and uid are not in promiseInfo.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, VerifyPromiseInfo001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: VerifyPromiseInfo002
 * @tc.desc: Test VerifyPromiseInfo when tokenId and uid are not in promiseInfo.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, VerifyPromiseInfo002, TestSize.Level0)
{
    StoreMetaDataLocal localMeta;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    localMeta.isAutoSync = true;
    localMeta.promiseInfo.tokenIds = {tokenId};
    localMeta.promiseInfo.uids = {};
    localMeta.promiseInfo.permissionNames = {};

    EXPECT_CALL(*accTokenMock, GetTokenType(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_INVALID))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_INVALID));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKeyLocal(), localMeta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKeyLocal(), localMeta, true), true);
    int32_t result = service.VerifyPromiseInfo(param);
    EXPECT_EQ(result, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKeyLocal(), true), true);
}

/**
 * @tc.name: VerifyPromiseInfo003
 * @tc.desc: Test VerifyPromiseInfo when tokenId and uid are not in promiseInfo.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, VerifyPromiseInfo003, TestSize.Level0)
{
    StoreMetaDataLocal localMeta;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    localMeta.isAutoSync = true;
    localMeta.promiseInfo.tokenIds = {tokenId};
    localMeta.promiseInfo.uids = {};
    localMeta.promiseInfo.permissionNames = {};

    EXPECT_CALL(*accTokenMock, GetTokenType(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_SHELL))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_SHELL));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKeyLocal(), localMeta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKeyLocal(), localMeta, true), true);
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
}

/**
 * @tc.name: VerifyPromiseInfo004
 * @tc.desc: Test VerifyPromiseInfo when tokenId and uid are not in promiseInfo.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, VerifyPromiseInfo004, TestSize.Level0)
{
    StoreMetaDataLocal localMeta;
    localMeta.isAutoSync = true;
    localMeta.promiseInfo.tokenIds = {};
    localMeta.promiseInfo.uids = {};
    localMeta.promiseInfo.permissionNames = {};
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyLocal(), localMeta, true), true);

    EXPECT_CALL(*accTokenMock, GetTokenType(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_SHELL))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_SHELL));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKeyLocal(), localMeta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKeyLocal(), localMeta, true), true);
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
}

/**
 * @tc.name: VerifyPromiseInfo005
 * @tc.desc: Test VerifyPromiseInfo when tokenId and uid are not in promiseInfo.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, VerifyPromiseInfo005, TestSize.Level0)
{
    StoreMetaDataLocal localMeta;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    localMeta.isAutoSync = true;
    localMeta.promiseInfo.tokenIds = {tokenId};
    localMeta.promiseInfo.uids = {};
    localMeta.promiseInfo.permissionNames = {};
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyLocal(), localMeta, true), true);

    EXPECT_CALL(*accTokenMock, GetTokenType(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_NATIVE))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_NATIVE));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKeyLocal(), localMeta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKeyLocal(), localMeta, true), true);
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
}

/**
 * @tc.name: VerifyPromiseInfo006
 * @tc.desc: Test VerifyPromiseInfo when tokenId and uid are not in promiseInfo.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, VerifyPromiseInfo006, TestSize.Level0)
{
    StoreMetaDataLocal localMeta;
    localMeta.isAutoSync = true;
    localMeta.promiseInfo.tokenIds = {};
    localMeta.promiseInfo.uids = {};
    localMeta.promiseInfo.permissionNames = {};
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyLocal(), localMeta, true), true);

    EXPECT_CALL(*accTokenMock, GetTokenType(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_NATIVE))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_NATIVE));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKeyLocal(), localMeta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKeyLocal(), localMeta, true), true);
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
}
/**
 * @tc.name: VerifyPromiseInfo007
 * @tc.desc: Test VerifyPromiseInfo when tokenId and uid are not in promiseInfo.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, VerifyPromiseInfo007, TestSize.Level0)
{
    StoreMetaDataLocal localMeta;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    localMeta.isAutoSync = true;
    localMeta.promiseInfo.tokenIds = {tokenId};
    localMeta.promiseInfo.uids = {};
    localMeta.promiseInfo.permissionNames = {};
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyLocal(), localMeta, true), true);

    EXPECT_CALL(*accTokenMock, GetTokenType(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_HAP))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_HAP));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKeyLocal(), localMeta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKeyLocal(), localMeta, true), true);
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
}

/**
 * @tc.name: VerifyPromiseInfo008
 * @tc.desc: Test VerifyPromiseInfo when tokenId and uid are not in promiseInfo.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, VerifyPromiseInfo008, TestSize.Level0)
{
    StoreMetaDataLocal localMeta;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    localMeta.isAutoSync = true;
    localMeta.promiseInfo.tokenIds = {tokenId};
    localMeta.promiseInfo.uids = {};
    localMeta.promiseInfo.permissionNames = {TEST_BUNDLE};
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyLocal(), localMeta, true), true);

    EXPECT_CALL(*accTokenMock, GetTokenType(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_HAP))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_HAP));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKeyLocal(), localMeta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKeyLocal(), localMeta, true), true);
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
}

/**
 * @tc.name: GetReuseDevice001
 * @tc.desc: Test GetReuseDevice when all devices can reusable.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, GetReuseDevice001, TestSize.Level0)
{
    EXPECT_CALL(*deviceManagerAdapterMock, GetDeviceTypeByUuid(_))
        .WillOnce(Return(DmAdapter::DmDeviceType::DEVICE_TYPE_PHONE))
        .WillOnce(Return(DmAdapter::DmDeviceType::DEVICE_TYPE_WATCH));
    RdbServiceImpl service;
    std::vector<std::string> devices = { "device1" };
    StoreMetaData metaData;
    metaData.deviceId = "device";
    auto result = service.GetReuseDevice(devices, metaData);
    EXPECT_EQ(result.size(), 0);
}

/**
 * @tc.name: GetReuseDevice002
 * @tc.desc: Test GetReuseDevice when all devices can reusable.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, GetReuseDevice002, TestSize.Level0)
{
    EXPECT_CALL(*deviceManagerAdapterMock, GetDeviceTypeByUuid(_))
        .WillOnce(Return(DmAdapter::DmDeviceType::DEVICE_TYPE_PHONE))
        .WillOnce(Return(DmAdapter::DmDeviceType::DEVICE_TYPE_PHONE));
    RdbServiceImpl service;
    std::vector<std::string> devices = { "device1" };
    StoreMetaData metaData;
    metaData.deviceId = "device";
    auto result = service.GetReuseDevice(devices, metaData);
    EXPECT_EQ(result.size(), 0);
}

/**
 * @tc.name: IsSupportAutoSyncDeviceType001
 * @tc.desc: Test IsSupportAutoSync when local and remote device can reusable.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, IsSupportAutoSyncDeviceType001, TestSize.Level0)
{
    EXPECT_CALL(*deviceManagerAdapterMock, GetDeviceTypeByUuid(_))
        .WillOnce(Return(DmAdapter::DmDeviceType::DEVICE_TYPE_PHONE))
        .WillOnce(Return(DmAdapter::DmDeviceType::DEVICE_TYPE_WATCH));
    RdbServiceImpl service;
    auto result = service.IsSupportAutoSync("device", "device1");
    EXPECT_EQ(result, true);
}

/**
 * @tc.name: IsSupportAutoSyncDeviceType002
 * @tc.desc: Test IsSupportAutoSync when local and remote device can not reusable.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, IsSupportAutoSyncDeviceType002, TestSize.Level0)
{
    EXPECT_CALL(*deviceManagerAdapterMock, GetDeviceTypeByUuid(_))
        .WillOnce(Return(DmAdapter::DmDeviceType::DEVICE_TYPE_PHONE))
        .WillOnce(Return(DmAdapter::DmDeviceType::DEVICE_TYPE_PHONE));
    RdbServiceImpl service;
    auto result = service.IsSupportAutoSync("device", "device1");
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: IsSupportAutoSyncDeviceType003
 * @tc.desc: Test IsSupportAutoSync when local and remote device can reusable.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, IsSupportAutoSyncDeviceType003, TestSize.Level0)
{
    EXPECT_CALL(*deviceManagerAdapterMock, GetDeviceTypeByUuid(_))
        .WillOnce(Return(DmAdapter::DmDeviceType::DEVICE_TYPE_WATCH))
        .WillOnce(Return(DmAdapter::DmDeviceType::DEVICE_TYPE_PHONE));
    RdbServiceImpl service;
    auto result = service.IsSupportAutoSync("device", "device1");
    EXPECT_EQ(result, true);
}

/**
 * @tc.name: RemoveExceptDeviceData001
 * @tc.desc: Test RemoveExceptDeviceData when user is non system app.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RemoveExceptDeviceData001, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillOnce(testing::Return(false));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto result = service.RemoveExceptDeviceData(param);
    EXPECT_EQ(result, RdbCommonUtils::ConvertGeneralRdbStatus(GeneralError::E_NON_SYSTEM_APP));
}

/**
 * @tc.name: RemoveExceptDeviceData002
 * @tc.desc: Test RemoveExceptDeviceData when no db meta.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RemoveExceptDeviceData002, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillOnce(testing::Return(true));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto result = service.RemoveExceptDeviceData(param);
    EXPECT_EQ(result, RdbCommonUtils::ConvertGeneralRdbStatus(GeneralError::E_DB_NOT_EXIST));
}

/**
 * @tc.name: RemoveExceptDeviceData003
 * @tc.desc: Test RemoveExceptDeviceData when instanceId = -1.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RemoveExceptDeviceData003, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillOnce(testing::Return(true));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true), true);
    auto result = service.RemoveExceptDeviceData(param);
    EXPECT_EQ(result, RdbCommonUtils::ConvertGeneralRdbStatus(GeneralError::E_DB_NOT_EXIST));
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: RemoveExceptDeviceData004
 * @tc.desc: Test RemoveExceptDeviceData when delegate return error.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RemoveExceptDeviceData004, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillOnce(testing::Return(true));
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_SHELL))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_SHELL));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true), true);
    dbStatus_ = E_TABLE_NOT_FOUND;
    auto result = service.RemoveExceptDeviceData(param);
    EXPECT_EQ(result, RdbCommonUtils::ConvertGeneralRdbStatus(GeneralError::E_TABLE_NOT_FOUND));
    dbStatus_ = E_OK;
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: RemoveExceptDeviceData005
 * @tc.desc: Test RemoveExceptDeviceData when param is invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RemoveExceptDeviceData005, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    param.user_ = "test\\..test";
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true), true);
    auto result = service.RemoveExceptDeviceData(param);
    EXPECT_EQ(result, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test