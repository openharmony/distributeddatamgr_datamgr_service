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
#include "rdb_common_utils.h"
#include "rdb_general_store.h"
#include "rdb_service_impl.h"
#include "rdb_types.h"
#include "relational_store_manager.h"
using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
using namespace DistributedDB;
using namespace OHOS::Security::AccessToken;
using namespace testing::ext;
using namespace testing;
using namespace std;
using RdbStatus = OHOS::DistributedRdb::RdbStatus;
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
    metaData_.isManualCleanDevice = true;
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
    Bootstrap::GetInstance().LoadDirectory();
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
    param.isAutoCleanDevice_ = !metaData_.isManualCleanDevice;
    param.user_ = metaData_.user;
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
 * @tc.name: VerifyPromiseInfo010
 * @tc.desc: Test VerifyPromiseInfo non-self-access when mapping tokenId differs and no localMeta.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(RdbServiceImplTokenTest, VerifyPromiseInfo010, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);

    auto meta = service.GetStoreMetaData(param);
    meta.user = param.user_;
    MetaDataManager::GetInstance().DelMeta(meta.GetKeyLocal(), true);

    StoreMetaMapping metaMappingSave(meta);
    metaMappingSave.tokenId = 12345u;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMappingSave.GetKey(), metaMappingSave, true), true);

    int32_t result = service.VerifyPromiseInfo(param);

    EXPECT_EQ(result, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMappingSave.GetKey(), true), true);
}

/**
 * @tc.name: VerifyPromiseInfo011
 * @tc.desc: Test VerifyPromiseInfo non-self-access with localMeta found but denied by promiseInfo.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(RdbServiceImplTokenTest, VerifyPromiseInfo011, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);

    auto meta = service.GetStoreMetaData(param);
    meta.user = param.user_;
    MetaDataManager::GetInstance().DelMeta(meta.GetKeyLocal(), true);

    StoreMetaMapping metaMappingSave(meta);
    metaMappingSave.tokenId = 12345u;
    metaMappingSave.dataDir = "test_data_dir";
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMappingSave.GetKey(), metaMappingSave, true), true);

    StoreMetaDataLocal localMeta;
    localMeta.isAutoSync = true;
    localMeta.promiseInfo.tokenIds = {};
    localMeta.promiseInfo.uids = {};
    localMeta.promiseInfo.permissionNames = {};

    StoreMetaData correctedMeta = meta;
    correctedMeta.dataDir = metaMappingSave.dataDir;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(correctedMeta.GetKeyLocal(), localMeta, true), true);

    EXPECT_CALL(*accTokenMock, GetTokenType(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_NATIVE))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_NATIVE));

    int32_t result = service.VerifyPromiseInfo(param);

    EXPECT_EQ(result, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMappingSave.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(correctedMeta.GetKeyLocal(), true), true);
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
 * @tc.name: RetainDeviceData001
 * @tc.desc: Test RetainDeviceData when user is non system app.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RetainDeviceData001, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(false));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    std::map<std::string, std::vector<std::string>> retainDevices;
    auto result = service.RetainDeviceData(param, retainDevices);
    EXPECT_EQ(result.first, RdbStatus::RDB_NON_SYSTEM_APP);
}

/**
 * @tc.name: RetainDeviceData002
 * @tc.desc: Test RetainDeviceData when no db meta.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RetainDeviceData002, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    std::map<std::string, std::vector<std::string>> retainDevices;
    auto result = service.RetainDeviceData(param, retainDevices);
    EXPECT_EQ(result.first, RdbStatus::RDB_DB_NOT_EXIST);
}

/**
 * @tc.name: RetainDeviceData003
 * @tc.desc: Test RetainDeviceData when instanceId = -1.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RetainDeviceData003, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true), true);
    std::map<std::string, std::vector<std::string>> retainDevices;
    auto result = service.RetainDeviceData(param, retainDevices);
    EXPECT_EQ(result.first, RdbStatus::RDB_DB_NOT_EXIST);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: RetainDeviceData004
 * @tc.desc: Test RetainDeviceData success empty map.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RetainDeviceData004, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_SHELL))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_SHELL));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true), true);
    std::map<std::string, std::vector<std::string>> retainDevices;
    auto result = service.RetainDeviceData(param, retainDevices);
    EXPECT_EQ(result.first, RdbCommonUtils::ConvertGeneralRdbStatus(GeneralError::E_OK));
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: RetainDeviceData005
 * @tc.desc: Test RetainDeviceData when param is invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RetainDeviceData005, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    param.user_ = "test\\..test";
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true), true);
    std::map<std::string, std::vector<std::string>> retainDevices;
    auto result = service.RetainDeviceData(param, retainDevices);
    EXPECT_EQ(result.first, RdbStatus::RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: RetainDeviceData006
 * @tc.desc: Test RetainDeviceData fail map is not empty touuid fail.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RetainDeviceData006, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_SHELL))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_SHELL));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true), true);
    std::map<std::string, std::vector<std::string>> retainDevices;
    std::vector<std::string> devices;
    devices.push_back("test");
    retainDevices["employee"] = devices;
    auto result = service.RetainDeviceData(param, retainDevices);
    EXPECT_EQ(result.first, RDB_INVALID_ARGS);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: RetainDeviceData007
 * @tc.desc: Test RetainDeviceData success empty devices.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RetainDeviceData007, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_SHELL))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_SHELL));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true), true);
    std::map<std::string, std::vector<std::string>> retainDevices;
    std::vector<std::string> devices;
    retainDevices["employee"] = devices;
    auto result = service.RetainDeviceData(param, retainDevices);
    EXPECT_EQ(result.first, RdbCommonUtils::ConvertGeneralRdbStatus(GeneralError::E_OK));
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: RetainDeviceData008
 * @tc.desc: Test RetainDeviceData fail tablename empty string.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RetainDeviceData008, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_SHELL))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_SHELL));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true), true);
    std::map<std::string, std::vector<std::string>> retainDevices;
    std::vector<std::string> devices;
    devices.push_back("device");
    retainDevices[""] = devices;
    auto result = service.RetainDeviceData(param, retainDevices);
    EXPECT_EQ(result.first, RDB_INVALID_ARGS);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: RetainDeviceData009
 * @tc.desc: Test RetainDeviceData fail device empty string.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RetainDeviceData009, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_SHELL))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_SHELL));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true), true);
    std::map<std::string, std::vector<std::string>> retainDevices;
    std::vector<std::string> devices;
    devices.push_back("");
    retainDevices["employee"] = devices;
    auto result = service.RetainDeviceData(param, retainDevices);
    EXPECT_EQ(result.first, RDB_INVALID_ARGS);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: RetainDeviceData010
 * @tc.desc: Test RetainDeviceData success map is not empty touuid success.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RetainDeviceData010, TestSize.Level0)
{
    std::vector<std::string> devices;
    devices.push_back("test");
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*deviceManagerAdapterMock, ToUUID(devices)).WillRepeatedly(Return(devices));
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_SHELL))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_SHELL));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true), true);
    std::map<std::string, std::vector<std::string>> retainDevices;
    retainDevices["employee"] = devices;
    auto result = service.RetainDeviceData(param, retainDevices);
    EXPECT_EQ(result.first, RdbCommonUtils::ConvertGeneralRdbStatus(GeneralError::E_OK));
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: RetainDeviceData011
 * @tc.desc: Test RetainDeviceData success fail due to uuids is empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RetainDeviceData011, TestSize.Level0)
{
    std::vector<std::string> devices;
    devices.push_back("test");
    std::vector<std::string> devices1;
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*deviceManagerAdapterMock, ToUUID(devices)).WillRepeatedly(Return(devices1));
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_SHELL))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_SHELL));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true), true);
    std::map<std::string, std::vector<std::string>> retainDevices;
    retainDevices["employee"] = devices;
    auto result = service.RetainDeviceData(param, retainDevices);
    EXPECT_EQ(result.first, RDB_INVALID_ARGS);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: RetainDeviceData012
 * @tc.desc: Test RetainDeviceData success fail due to uuids length is 2.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, RetainDeviceData012, TestSize.Level0)
{
    std::vector<std::string> devices;
    devices.push_back("test");
    std::vector<std::string> devices1;
    devices1.push_back("test");
    devices1.push_back("test1");
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*deviceManagerAdapterMock, ToUUID(devices)).WillRepeatedly(Return(devices1));
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_SHELL))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_SHELL));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true), true);
    std::map<std::string, std::vector<std::string>> retainDevices;
    retainDevices["employee"] = devices;
    auto result = service.RetainDeviceData(param, retainDevices);
    EXPECT_EQ(result.first, RDB_INVALID_ARGS);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: ObtainUuid001
 * @tc.desc: Test ObtainUuid fail device empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, ObtainUuid001, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillOnce(testing::Return(true));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    std::vector<std::string> devices;
    auto [result, uuids] = service.ObtainUuid(param, devices);
    EXPECT_EQ(result, RDB_INVALID_ARGS);
}

/**
 * @tc.name: ObtainUuid002
 * @tc.desc: Test ObtainUuid fail non system app.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, ObtainUuid002, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillOnce(testing::Return(false));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    std::vector<std::string> devices;
    devices.push_back(DmAdapter::GetInstance().GetLocalDevice().networkId);
    auto [result, uuids] = service.ObtainUuid(param, devices);
    EXPECT_EQ(result, RDB_NON_SYSTEM_APP);
}

/**
 * @tc.name: ObtainUuid003
 * @tc.desc: Test ObtainUuid when param is invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, ObtainUuid003, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    param.user_ = "test\\..test";
    std::vector<std::string> devices;
    devices.push_back(DmAdapter::GetInstance().GetLocalDevice().networkId);
    auto [result, uuids] = service.ObtainUuid(param, devices);
    EXPECT_EQ(result, RdbStatus::RDB_ERROR);
}

/**
 * @tc.name: ObtainUuid004
 * @tc.desc: Test ObtainUuid success.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, ObtainUuid004, TestSize.Level0)
{
    std::vector<std::string> devices;
    devices.push_back("test");
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillOnce(testing::Return(true));
    EXPECT_CALL(*deviceManagerAdapterMock, ToUUID(devices)).WillRepeatedly(Return(devices));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto [result, uuids] = service.ObtainUuid(param, devices);
    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(uuids.size(), 1);
    EXPECT_EQ(uuids[0], "test");
}

/**
 * @tc.name: ObtainUuid005
 * @tc.desc: Test ObtainUuid fail due to uuids is empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, ObtainUuid005, TestSize.Level0)
{
    std::vector<std::string> devices;
    std::vector<std::string> devices1;
    devices1.push_back("test");
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillOnce(testing::Return(true));
    EXPECT_CALL(*deviceManagerAdapterMock, ToUUID(devices1)).WillRepeatedly(Return(devices));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto [result, uuids] = service.ObtainUuid(param, devices1);
    EXPECT_EQ(result, RDB_INVALID_ARGS);
}

/**
 * @tc.name: ObtainUuid006
 * @tc.desc: Test ObtainUuid fail due to uuids length is not equal to devices length.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, ObtainUuid006, TestSize.Level0)
{
    std::vector<std::string> devices;
    devices.push_back("test");
    devices.push_back("test1");
    std::vector<std::string> devices1;
    devices1.push_back("test");
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillOnce(testing::Return(true));
    EXPECT_CALL(*deviceManagerAdapterMock, ToUUID(devices1)).WillRepeatedly(Return(devices));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto [result, uuids] = service.ObtainUuid(param, devices1);
    EXPECT_EQ(result, RDB_INVALID_ARGS);
}

/**
 * @tc.name: BeforeOpen001
 * @tc.desc: Test BeforeOpen success app is system app.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, BeforeOpen001, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true), true);
    auto result = service.BeforeOpen(param);
    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: BeforeOpen002
 * @tc.desc: Test BeforeOpen success app is not system app.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, BeforeOpen002, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(false));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    auto meta = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().LoadMeta(meta.GetKey(), meta, true), true);
    auto result = service.BeforeOpen(param);
    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: BeforeOpen003
 * @tc.desc: Test BeforeOpen success app is not system app but no meta.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, BeforeOpen003, TestSize.Level0)
{
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(false));
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_NATIVE))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_NATIVE));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    param.dbPath_ = "/data/service/el2/100/test_rdb_service_impl_bundleName/rdbtest.db";
    auto meta = service.GetStoreMetaData(param);
    auto result = service.BeforeOpen(param);
    EXPECT_EQ(result, RDB_NO_META);
}

/**
 * @tc.name: ReplicaPathRejectsNonSystemApp
 * @tc.desc: Reject a custom replica path from an ordinary HAP caller.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(RdbServiceImplTokenTest, ReplicaPathRejectsNonSystemApp, TestSize.Level0)
{
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_)).WillRepeatedly(testing::Return(TOKEN_HAP));
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(false));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    param.replicaPath_ = "/data/service/el1/public/database/replica";

    EXPECT_FALSE(service.IsValidParam(param));
}

/**
 * @tc.name: ReplicaPathAllowsSystemApp
 * @tc.desc: Allow a valid absolute replica path from a system app caller.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(RdbServiceImplTokenTest, ReplicaPathAllowsSystemApp, TestSize.Level0)
{
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_)).WillRepeatedly(testing::Return(TOKEN_HAP));
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    HapTokenInfo hapInfo{};
    hapInfo.userID = 0;
    hapInfo.instIndex = 0;
    EXPECT_CALL(*accTokenMock, GetHapTokenInfo(testing::_, testing::_))
        .WillRepeatedly(testing::DoAll(testing::SetArgReferee<1>(hapInfo), testing::Return(0)));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    // HAP caller: /data/app/el<area>/<user>/database/<bundleName>; user resolves to 0 under the token mock.
    param.replicaPath_ = "/data/app/el1/0/database/" + std::string(TEST_BUNDLE) + "/replica";

    EXPECT_TRUE(service.IsValidParam(param));
}

/**
 * @tc.name: ReplicaPathAllowsNative
 * @tc.desc: Allow a valid absolute replica path from a Native/SA caller.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(RdbServiceImplTokenTest, ReplicaPathAllowsNative, TestSize.Level0)
{
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_)).WillRepeatedly(testing::Return(TOKEN_NATIVE));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    param.replicaPath_ = "/data/service/el1/public/database/" + std::string(TEST_BUNDLE) + "/replica";

    EXPECT_TRUE(service.IsValidParam(param));
}

/**
 * @tc.name: ReplicaPathRejectsTraversal
 * @tc.desc: Reject replica paths that can escape the caller's directory through traversal syntax.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(RdbServiceImplTokenTest, ReplicaPathRejectsTraversal, TestSize.Level0)
{
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_)).WillRepeatedly(testing::Return(TOKEN_NATIVE));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);

    for (const auto &path : { std::string("relative/replica"), std::string("/"), std::string("/data/./replica"),
        std::string("/data/../replica"), std::string("/data/replica/.."), std::string("/data/replica\\dir") }) {
        param.replicaPath_ = path;
        EXPECT_FALSE(service.IsValidParam(param));
    }
    param.replicaPath_ = "/data/replica";
    param.replicaPath_.push_back('\0');
    param.replicaPath_ += "suffix";
    EXPECT_FALSE(service.IsValidParam(param));
}
/**
 * @tc.name: ReplicaPathRejectsOutsideCallerRoot
 * @tc.desc: Reject replica paths that are not under the caller-owned store root.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: agent
 */
HWTEST_F(RdbServiceImplTokenTest, ReplicaPathRejectsOutsideCallerRoot, TestSize.Level0)
{
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_)).WillRepeatedly(testing::Return(TOKEN_NATIVE));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);

    param.replicaPath_ = "/data/service/el1/public/database/" + std::string(TEST_BUNDLE) + "/replica";
    EXPECT_TRUE(service.IsValidParam(param));
    param.replicaPath_ = "/data/service/el1/public/database/other_bundle/replica";
    EXPECT_FALSE(service.IsValidParam(param));
    param.replicaPath_ = "/mnt/custom/replica";
    EXPECT_FALSE(service.IsValidParam(param));
    // bundleName must match on a section boundary, not as a string prefix.
    param.replicaPath_ = "/data/service/el1/public/database/" + std::string(TEST_BUNDLE) + "_suffix/replica";
    EXPECT_FALSE(service.IsValidParam(param));
}

/**
 * @tc.name: AfterOpen_SandboxReplicaPath_PhysicalMetadataAndSandboxReply
 * @tc.desc: Persist physical paths and return sandbox paths for users and application clones.
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(RdbServiceImplTokenTest, AfterOpen_SandboxReplicaPath_PhysicalMetadataAndSandboxReply, TestSize.Level0)
{
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_)).WillRepeatedly(testing::Return(TOKEN_HAP));
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    RdbServiceImpl service;
    for (int32_t user : { 100, 101 }) {
        for (int32_t instance : { 0, 1 }) {
            HapTokenInfo hapInfo{};
            hapInfo.userID = user;
            hapInfo.instIndex = instance;
            EXPECT_CALL(*accTokenMock, GetHapTokenInfo(testing::_, testing::_))
                .WillRepeatedly(testing::DoAll(testing::SetArgReferee<1>(hapInfo), testing::Return(0)));
            RdbSyncerParam param;
            GetRdbSyncerParam(param);
            param.storeName_ = "sandbox_replica_roundtrip";
            param.hapName_ = "entry";
            param.customDir_ = "custom";
            std::string bundle = instance == 0 ? TEST_BUNDLE : "+clone-1+" + std::string(TEST_BUNDLE);
            auto physicalRoot = "/data/app/el1/" + std::to_string(user) + "/database/" + bundle;
            for (const auto &suffix : { "", "/", "/entry/replica", "/entry/custom/replica" }) {
                param.replicaPath_ = std::string("/data/storage/el1/database") + suffix;
                EXPECT_EQ(service.AfterOpen(param), RDB_OK);
                auto [exists, stored] = service.LoadStoreMetaData(param);
                EXPECT_TRUE(exists);
                EXPECT_EQ(stored.replicaPath, physicalRoot + suffix);
                auto reply = param;
                EXPECT_EQ(service.BeforeOpen(reply), RDB_OK);
                EXPECT_EQ(reply.replicaPath_, param.replicaPath_);
                EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(stored.GetKey(), true));
                MetaDataManager::GetInstance().DelMeta(stored.GetKeyWithoutPath(), true);
                StoreMetaMapping mapping(stored);
                MetaDataManager::GetInstance().DelMeta(mapping.GetKey(), true);
            }
        }
    }
}

/**
 * @tc.name: AfterOpen_InvalidSandboxReplicaPath_RejectsRegistration
 * @tc.desc: Reject unsupported sandbox roots, prefix collisions and traversal before saving metadata.
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(RdbServiceImplTokenTest, AfterOpen_InvalidSandboxReplicaPath_RejectsRegistration, TestSize.Level0)
{
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_)).WillRepeatedly(testing::Return(TOKEN_HAP));
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    HapTokenInfo hapInfo{};
    hapInfo.userID = 100;
    EXPECT_CALL(*accTokenMock, GetHapTokenInfo(testing::_, testing::_))
        .WillRepeatedly(testing::DoAll(testing::SetArgReferee<1>(hapInfo), testing::Return(0)));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    param.storeName_ = "invalid_sandbox_replica";
    for (const auto &path : { "/data/storage/el2/database/replica", "/data/storage/el1/base/files/replica",
        "/data/storage/el1/database_suffix/replica", "/data/storage/el1/database/../other",
        "/data/storage/el1/database/./replica", "/data/storage/el1/database/replica/..",
        "/data/storage/el1/database/replica\\other" }) {
        param.replicaPath_ = path;
        EXPECT_EQ(service.AfterOpen(param), RDB_ERROR) << path;
        EXPECT_EQ(service.BeforeOpen(param), RDB_ERROR) << path;
    }
    param.replicaPath_ = "/data/storage/el1/database/replica";
    param.replicaPath_.push_back('\0');
    param.replicaPath_ += "suffix";
    EXPECT_EQ(service.AfterOpen(param), RDB_ERROR);
    EXPECT_FALSE(service.LoadStoreMetaData(param).first);
}

/**
 * @tc.name: AfterOpen_SandboxReplicaPath_RejectsUntrustedCaller
 * @tc.desc: Ordinary applications and Native callers cannot use the HAP sandbox mapping.
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(RdbServiceImplTokenTest, AfterOpen_SandboxReplicaPath_RejectsUntrustedCaller, TestSize.Level0)
{
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_)).WillRepeatedly(testing::Return(TOKEN_HAP));
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(false));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    param.replicaPath_ = "/data/storage/el1/database/replica";
    EXPECT_EQ(service.AfterOpen(param), RDB_ERROR);
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_)).WillRepeatedly(testing::Return(TOKEN_NATIVE));
    EXPECT_EQ(service.AfterOpen(param), RDB_ERROR);
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_)).WillRepeatedly(testing::Return(TOKEN_HAP));
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*accTokenMock, GetHapTokenInfo(testing::_, testing::_)).WillRepeatedly(testing::Return(-1));
    EXPECT_EQ(service.AfterOpen(param), RDB_ERROR);
}

/**
 * @tc.name: AfterOpen_SandboxReplicaPathWithoutDirectory_RejectsRegistration
 * @tc.desc: Missing directory configuration cannot turn a custom replica path into the default path.
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(RdbServiceImplTokenTest, AfterOpen_SandboxReplicaPathWithoutDirectory_RejectsRegistration, TestSize.Level0)
{
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_)).WillRepeatedly(testing::Return(TOKEN_HAP));
    EXPECT_CALL(*tokenIdMock, IsSystemAppByFullTokenID(testing::_)).WillRepeatedly(testing::Return(true));
    HapTokenInfo hapInfo{};
    hapInfo.userID = 100;
    EXPECT_CALL(*accTokenMock, GetHapTokenInfo(testing::_, testing::_))
        .WillRepeatedly(testing::DoAll(testing::SetArgReferee<1>(hapInfo), testing::Return(0)));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    param.replicaPath_ = "/data/storage/el1/database/replica";
    DirectoryManager::GetInstance().Initialize({}, {});
    auto result = service.AfterOpen(param);
    Bootstrap::GetInstance().LoadDirectory();
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: BeforeOpen_NativeReplicaPathAndEmptyPath_PreservesValue
 * @tc.desc: Native physical paths and legacy empty paths remain unchanged in metadata and replies.
 * @tc.type: FUNC
 * @tc.author: agent
 */
HWTEST_F(RdbServiceImplTokenTest, BeforeOpen_NativeReplicaPathAndEmptyPath_PreservesValue, TestSize.Level0)
{
    EXPECT_CALL(*accTokenMock, GetTokenTypeFlag(testing::_)).WillRepeatedly(testing::Return(TOKEN_NATIVE));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
    param.storeName_ = "native_replica_roundtrip";
    auto root = "/data/service/el1/public/database/" + std::string(TEST_BUNDLE);
    for (const auto &path : { std::string(), root + "/replica" }) {
        param.replicaPath_ = path;
        EXPECT_EQ(service.AfterOpen(param), RDB_OK);
        auto [exists, stored] = service.LoadStoreMetaData(param);
        EXPECT_TRUE(exists);
        EXPECT_EQ(stored.replicaPath, path);
        EXPECT_EQ(service.BeforeOpen(param), RDB_OK);
        EXPECT_EQ(param.replicaPath_, path);
        EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(stored.GetKey(), true));
        MetaDataManager::GetInstance().DelMeta(stored.GetKeyWithoutPath(), true);
        StoreMetaMapping mapping(stored);
        MetaDataManager::GetInstance().DelMeta(mapping.GetKey(), true);
    }
}

} // namespace DistributedRDBTest
} // namespace OHOS::Test
