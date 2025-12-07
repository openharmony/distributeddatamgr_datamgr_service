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
#include "cloud/schema_meta.h"
#include "crypto/crypto_manager.h"
#include "device_manager_adapter.h"
#include "device_matrix.h"
#include "directory/directory_manager.h"
#include "ipc_skeleton.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data_local.h"
#include "mock/access_token_mock.h"
#include "checker_mock.h"
#include "mock/db_store_mock.h"
#include "mock/general_store_mock.h"
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
    static void DeleteMultipleMetaData();
    static void InitMultipleMetaData();
    void SetUp();
    void TearDown();
protected:
    static inline std::shared_ptr<AccessTokenKitMock> accTokenMock = nullptr;
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static int32_t dbStatus_;
    static StoreMetaData metaData_;
    static StoreMetaData metaData1_;
    static StoreMetaData metaData2_;
    static StoreMetaData metaData3_;
    static StoreMetaData metaData4_;
    static Database database_;
    static Database database1_;
    static Database database2_;
    static Database database3_;
    static Database database4_;
    static CheckerMock checkerMock_;
    static void InitMetaDataManager();
    static void GetRdbSyncerParam(RdbSyncerParam &param);
};
std::shared_ptr<DBStoreMock> RdbServiceImplTokenTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
StoreMetaData RdbServiceImplTokenTest::metaData_;
StoreMetaData RdbServiceImplTokenTest::metaData1_;
StoreMetaData RdbServiceImplTokenTest::metaData2_;
StoreMetaData RdbServiceImplTokenTest::metaData3_;
StoreMetaData RdbServiceImplTokenTest::metaData4_;
Database RdbServiceImplTokenTest::database_;
Database RdbServiceImplTokenTest::database1_;
Database RdbServiceImplTokenTest::database2_;
Database RdbServiceImplTokenTest::database3_;
Database RdbServiceImplTokenTest::database4_;
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
    metaData_.instanceId = -1;
    metaData_.isAutoSync = true;
    metaData_.storeType = RDB_DEVICE_COLLABORATION;
    metaData_.storeId = TEST_STORE;
    metaData_.dataDir = DirectoryManager::GetInstance().GetStorePath(metaData_) + "/" + TEST_STORE;
}

void RdbServiceImplTokenTest::InitMultipleMetaData()
{
    metaData1_ = metaData_;
    metaData1_.bundleName = "meta1";
    metaData1_.appId = "meta1";
    metaData2_ = metaData_;
    metaData2_.bundleName = "meta2";
    metaData2_.appId = "meta2";
    metaData3_ = metaData_;
    metaData3_.bundleName = "meta3";
    metaData3_.appId = "meta3";
    metaData4_ = metaData_;
    metaData4_.bundleName = "meta4";
    metaData4_.appId = "meta4";
    database_.bundleName = metaData_.bundleName;
    database_.name = metaData_.storeId;
    database_.user = metaData_.user;
    database_.deviceId = metaData_.deviceId;
    database1_ = database_;
    database1_.bundleName = metaData1_.bundleName;
    database2_ = database_;
    database2_.bundleName = metaData2_.bundleName;
    database3_ = database_;
    database3_.bundleName = metaData3_.bundleName;
    database4_ = database_;
    database4_.bundleName = metaData4_.bundleName;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyWithoutPath(), metaData_, false), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData1_.GetKeyWithoutPath(), metaData1_, false), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData2_.GetKeyWithoutPath(), metaData2_, false), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData3_.GetKeyWithoutPath(), metaData3_, false), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData4_.GetKeyWithoutPath(), metaData4_, false), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(database_.GetKey(), database_, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(database1_.GetKey(), database1_, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(database2_.GetKey(), database2_, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(database3_.GetKey(), database3_, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(database4_.GetKey(), database4_, true), true);
}

void RdbServiceImplTokenTest::DeleteMultipleMetaData()
{
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData1_.GetKeyWithoutPath(), false), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData2_.GetKeyWithoutPath(), false), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData3_.GetKeyWithoutPath(), false), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData4_.GetKeyWithoutPath(), false), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(database_.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(database1_.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(database2_.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(database3_.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(database4_.GetKey(), true), true);
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
    InitMetaData();
    InitMetaDataManager();
    Bootstrap::GetInstance().LoadCheckers();
    Bootstrap::GetInstance().LoadDirectory();
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
    accTokenMock = nullptr;
    BAccessTokenKit::accessTokenkit = nullptr;
    AutoCache::GetInstance().RegCreator(RDB_DEVICE_COLLABORATION,
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
    GetRdbSyncerParam(param);
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
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
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyLocal(), localMeta, true), true);

    EXPECT_CALL(*accTokenMock, GetTokenType(testing::_))
        .WillOnce(testing::Return(ATokenTypeEnum::TOKEN_SHELL))
        .WillRepeatedly(testing::Return(ATokenTypeEnum::TOKEN_SHELL));
    RdbServiceImpl service;
    RdbSyncerParam param;
    GetRdbSyncerParam(param);
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
    int32_t result = service.VerifyPromiseInfo(param);
 
    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
}

/**
 * @tc.name: DoSync001
 * @tc.desc: Test DoSync when tokenId is synclimitapp, delaysync, sleep9s.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, DoSync001, TestSize.Level0)
{
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyWithoutPath(), metaData_, false), true);
    Database database;
    database.bundleName = metaData_.bundleName;
    database.name = metaData_.storeId;
    database.user = metaData_.user;
    database.deviceId = metaData_.deviceId;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(database.GetKey(), database, true), true);
    RdbServiceImpl service;
    service.executors_ = std::make_shared<ExecutorPool>(2, 0);
    RdbService::Option option;
    PredicatesMemo predicates;
    AsyncDetail async;
    for (int32_t i = 0; i < 6; i++) {
        auto result = service.DoSync(metaData_, option, predicates, async);
        EXPECT_EQ(result, RDB_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); // 模拟请求到达间隔
    }
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(database.GetKey(), true), true);
}

/**
 * @tc.name: DoSync003
 * @tc.desc: Test DoSync when app is synclimitapp, delaysync, device limit before app limit.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTokenTest, DoSync003, TestSize.Level0)
{
    InitMultipleMetaData();
    RdbServiceImpl service;
    service.executors_ = std::make_shared<ExecutorPool>(2, 0);
    PredicatesMemo predicates;
    AsyncDetail async;
    RdbService::Option option;
    for (int32_t i = 0; i < 6; i++) {
        auto result = service.DoSync(metaData_, option, predicates, async);
        EXPECT_EQ(result, RDB_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        result = service.DoSync(metaData1_, option, predicates, async);
        EXPECT_EQ(result, RDB_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        result = service.DoSync(metaData2_, option, predicates, async);
        EXPECT_EQ(result, RDB_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        result = service.DoSync(metaData3_, option, predicates, async);
        EXPECT_EQ(result, RDB_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        result = service.DoSync(metaData4_, option, predicates, async);
        EXPECT_EQ(result, RDB_OK);
        std::this_thread::sleep_for(std::chrono::milliseconds(5)); // 模拟请求到达间隔
    }
    DeleteMultipleMetaData();
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test