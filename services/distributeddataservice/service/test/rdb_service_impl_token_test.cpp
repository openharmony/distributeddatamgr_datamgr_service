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
#include "account/account_delegate.h"
#include "rdb_service_impl.h"
#include "ipc_skeleton.h"
#include "gmock/gmock.h"
#include "mock/access_token_mock.h"
#include "bootstrap.h"
#include "checker_mock.h"
#include "crypto_manager.h"
#include "device_manager_adapter.h"
#include "device_matrix.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "mock/db_store_mock.h"
#include "rdb_types.h"
#include "relational_store_manager.h"
#include "gtest/gtest.h"
#include "directory/directory_manager.h"
using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
using namespace DistributedDB;
using namespace OHOS::Security::AccessToken;
using namespace testing::ext;
using namespace testing;
using namespace std;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

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
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static StoreMetaData metaData_;
    static CheckerMock checkerMock_;
    static void InitMetaDataManager();
};
std::shared_ptr<DBStoreMock> RdbServiceImplTokenTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
StoreMetaData RdbServiceImplTokenTest::metaData_;
CheckerMock RdbServiceImplTokenTest::checkerMock_;


void RdbServiceImplTokenTest::InitMetaData()
{
    metaData_.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.appId = TEST_APPID;
    metaData_.bundleName = TEST_BUNDLE;
    metaData_.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    metaData_.user = std::to_string(AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId));
    metaData_.area = OHOS::DistributedKv::EL1;
    metaData_.instanceId = -1;
    metaData_.isAutoSync = true;
    metaData_.storeType = DistributedRdb::RDB_DEVICE_COLLABORATION;
    metaData_.storeId = TEST_STORE;
    metaData_.dataDir = DirectoryManager::GetInstance().GetStorePath(metaData_) + "/" + TEST_STORE;
}

void RdbServiceImplTokenTest::InitMetaDataManager()
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    MetaDataManager::GetInstance().SetSyncer([](const auto &, auto) {
        DeviceMatrix::GetInstance().OnChanged(DeviceMatrix::META_STORE_MASK);
    });
}

void RdbServiceImplTokenTest::SetUpTestCase()
{
    accTokenMock = std::make_shared<AccessTokenKitMock>();
    BAccessTokenKit::accessTokenkit = accTokenMock;
    size_t max = 12;
    size_t min = 5;
    auto dmExecutor = std::make_shared<ExecutorPool>(max, min);
    DeviceManagerAdapter::GetInstance().Init(dmExecutor);
    InitMetaData();
    Bootstrap::GetInstance().LoadCheckers();
    CryptoManager::GetInstance().GenerateRootKey();
}

void RdbServiceImplTokenTest::TearDownTestCase()
{
    accTokenMock = nullptr;
    BAccessTokenKit::accessTokenkit = nullptr;
}

void RdbServiceImplTokenTest::SetUp()
{
}

void RdbServiceImplTokenTest::TearDown()
{
}

/**
 * @tc.name: VerifyPromiseInfo001
 * @tc.desc: Test VerifyPromiseInfo when Store not exist.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, VerifyPromiseInfo001, TestSize.Level0)
{
    EXPECT_CALL(*accTokenMock, GetTokenType(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_INVALID))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_INVALID));
    RdbServiceImpl service;
    RdbSyncerParam param;
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: VerifyPromiseInfo002
 * @tc.desc: Test VerifyPromiseInfo when tokenId is INVALID.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, VerifyPromiseInfo002, TestSize.Level0)
{
    InitMetaDataManager();
    StoreMetaDataLocal localMeta;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    localMeta.isAutoSync = true;
    localMeta.promiseInfo.tokenIds = {tokenId};
    localMeta.promiseInfo.uids = {};
    localMeta.promiseInfo.permissionNames = {};
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyLocal(), localMeta, true), true);

    EXPECT_CALL(*accTokenMock, GetTokenType(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_INVALID))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_INVALID));
    RdbServiceImpl service;
    RdbSyncerParam param;
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
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
}

/**
 * @tc.name: VerifyPromiseInfo003
 * @tc.desc: Test VerifyPromiseInfo when tokenId is in promiseInfo and tokenid is TOKEN_SHELL.
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
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyLocal(), localMeta, true), true);

    EXPECT_CALL(*accTokenMock, GetTokenType(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_SHELL))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_SHELL));
    RdbServiceImpl service;
    RdbSyncerParam param;
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
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
}

/**
 * @tc.name: VerifyPromiseInfo004
 * @tc.desc: Test VerifyPromiseInfo when tokenId is not in promiseInfo and tokenid is TOKEN_SHELL.
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
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
}

/**
 * @tc.name: VerifyPromiseInfo005
 * @tc.desc: Test VerifyPromiseInfo when tokenId is in promiseInfo and tokenid is TOKEN_NATIVE.
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
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
}

/**
 * @tc.name: VerifyPromiseInfo006
 * @tc.desc: Test VerifyPromiseInfo when tokenId is not in promiseInfo and tokenid is TOKEN_NATIVE.
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
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
}
/**
 * @tc.name: VerifyPromiseInfo007
 * @tc.desc: Test VerifyPromiseInfo when permissionNames is not in promiseInfo and tokenid is TOKEN_HAP.
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
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
}

/**
 * @tc.name: VerifyPromiseInfo008
 * @tc.desc: Test VerifyPromiseInfo when permissionNames is in promiseInfo and tokenid is TOKEN_HAP.
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
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test