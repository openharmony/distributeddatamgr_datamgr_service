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

#include <random>

#include "account/account_delegate.h"
#include "bootstrap.h"
#include "checker_mock.h"
#include "cloud/change_event.h"
#include "cloud/schema_meta.h"
#include "crypto/crypto_manager.h"
#include "device_manager_adapter.h"
#include "device_matrix.h"
#include "event_center.h"
#include "ipc_skeleton.h"
#include "metadata/capability_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "metadata/store_debug_info.h"
#include "mock/db_store_mock.h"
#include "mock/general_store_mock.h"
#include "store/general_value.h"
#include "rdb_service_impl.h"
#include "rdb_types.h"
#include "relational_store_manager.h"
#include "gtest/gtest.h"
#include "directory/directory_manager.h"

using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
using namespace DistributedDB;
using namespace testing::ext;
using namespace testing;
using namespace std;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

namespace OHOS::Test {
namespace DistributedRDBTest {

static constexpr const char *TEST_BUNDLE = "test_rdb_service_impl_bundleName";
static constexpr const char *TEST_APPID = "test_rdb_service_impl_appid";
static constexpr const char *TEST_STORE = "test_rdb_service_impl_store";
static constexpr int32_t KEY_LENGTH = 32;
static constexpr uint32_t DELY_TIME = 10000;

class RdbServiceImplTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    static void InitMetaData();
    static void InitMapping(StoreMetaMapping &meta);
    void SetUp();
    void TearDown();
    static std::vector<uint8_t> Random(int32_t len);
protected:
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static StoreMetaData metaData_;
    static CheckerMock checkerMock_;
    static void InitMetaDataManager();
};

std::shared_ptr<DBStoreMock> RdbServiceImplTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
StoreMetaData RdbServiceImplTest::metaData_;
CheckerMock RdbServiceImplTest::checkerMock_;

void RdbServiceImplTest::InitMetaData()
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

void RdbServiceImplTest::InitMapping(StoreMetaMapping &metaMapping)
{
    metaMapping.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaMapping.user = "100";
    metaMapping.bundleName = "bundleName";
    metaMapping.storeId = "storeName";
}

void RdbServiceImplTest::InitMetaDataManager()
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    MetaDataManager::GetInstance().SetSyncer([](const auto &, auto) {
        DeviceMatrix::GetInstance().OnChanged(DeviceMatrix::META_STORE_MASK);
    });
}

void RdbServiceImplTest::SetUpTestCase()
{
    size_t max = 12;
    size_t min = 5;

    auto dmExecutor = std::make_shared<ExecutorPool>(max, min);
    DeviceManagerAdapter::GetInstance().Init(dmExecutor);
    InitMetaData();
    Bootstrap::GetInstance().LoadCheckers();
    CryptoManager::GetInstance().GenerateRootKey();
}

void RdbServiceImplTest::TearDownTestCase()
{
}

void RdbServiceImplTest::SetUp()
{
}

void RdbServiceImplTest::TearDown()
{
}

std::vector<uint8_t> RdbServiceImplTest::Random(int32_t len)
{
    std::random_device randomDevice;
    std::uniform_int_distribution<int> distribution(0, std::numeric_limits<uint8_t>::max());
    std::vector<uint8_t> key(len);
    for (uint32_t i = 0; i < len; i++) {
        key[i] = static_cast<uint8_t>(distribution(randomDevice));
    }
    return key;
}

/**
* @tc.name: OnRemoteRequest001
* @tc.desc: ResolveAutoLaunch LoadMeta Failed.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhaojh
*/
HWTEST_F(RdbServiceImplTest, ResolveAutoLaunch001, TestSize.Level0)
{
    auto localId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    DistributedDB::AutoLaunchParam param;
    RdbServiceImpl service;

    std::string identifier = "test_identifier";
    int32_t result = service.ResolveAutoLaunch(identifier, param);
    EXPECT_EQ(result, false);
}

/**
* @tc.name: ResolveAutoLaunch002
* @tc.desc: ResolveAutoLaunch no meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhaojh
*/
HWTEST_F(RdbServiceImplTest, ResolveAutoLaunch002, TestSize.Level0)
{
    InitMetaDataManager();
    auto localId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    DistributedDB::AutoLaunchParam param;
    RdbServiceImpl service;

    std::string identifier = "test_identifier";
    int32_t result = service.ResolveAutoLaunch(identifier, param);
    EXPECT_EQ(result, false);
}

/**
* @tc.name: ResolveAutoLaunch003
* @tc.desc: ResolveAutoLaunch has meta, identifier not match.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhaojh
*/
HWTEST_F(RdbServiceImplTest, ResolveAutoLaunch003, TestSize.Level0)
{
    auto ret = MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyWithoutPath(), metaData_, false);
    EXPECT_EQ(ret, true);
    StoreMetaData meta;
    meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta.user = std::to_string(AccountDelegate::GetInstance()->GetUserByToken(meta.tokenId));
    meta.bundleName = TEST_BUNDLE;
    meta.storeId = "ResolveAutoLaunch003";
    meta.instanceId = 0;
    meta.appId = TEST_APPID;
    meta.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    meta.area = OHOS::DistributedKv::EL1;
    meta.isAutoSync = true;

    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKeyWithoutPath(), meta, false), true);

    DistributedDB::AutoLaunchParam param;
    RdbServiceImpl service;

    std::string identifier = "test_identifier";
    int32_t result = service.ResolveAutoLaunch(identifier, param);

    EXPECT_EQ(result, false);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
}

/**
* @tc.name: ResolveAutoLaunch004
* @tc.desc: ResolveAutoLaunch has meta, identifier match.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhaojh
*/
HWTEST_F(RdbServiceImplTest, ResolveAutoLaunch004, TestSize.Level0)
{
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyWithoutPath(), metaData_, false), true);
    DistributedDB::AutoLaunchParam param;
    RdbServiceImpl service;
    RelationalStoreManager userMgr(metaData_.appId, metaData_.user, metaData_.storeId);
    auto identifier = userMgr.GetRelationalStoreIdentifier(metaData_.user, metaData_.appId, metaData_.storeId, true);

    int32_t result = service.ResolveAutoLaunch(identifier, param);

    EXPECT_EQ(result, true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
}

/**
* @tc.name: ResolveAutoLaunch005
* @tc.desc: ResolveAutoLaunch has meta, identifier match, is encrypt.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhaojh
*/
HWTEST_F(RdbServiceImplTest, ResolveAutoLaunch005, TestSize.Level0)
{
    auto meta = metaData_;
    meta.isEncrypt = true;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKeyWithoutPath(), meta, false), true);
    DistributedDB::AutoLaunchParam param;
    RdbServiceImpl service;
    RelationalStoreManager userMgr1(meta.appId, meta.user, meta.storeId);
    auto identifier = userMgr1.GetRelationalStoreIdentifier(meta.user, meta.appId, meta.storeId, true);

    int32_t result = service.ResolveAutoLaunch(identifier, param);

    EXPECT_EQ(result, true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKeyWithoutPath(), false), true);
}

/**
* @tc.name: ResolveAutoLaunch006
* @tc.desc: test ObtainDistributedTableName, uuid exist.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhaojh
*/
HWTEST_F(RdbServiceImplTest, ResolveAutoLaunch006, TestSize.Level0)
{
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyWithoutPath(), metaData_, false), true);
    RdbServiceImpl service;
    auto deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    RdbSyncerParam param{ .bundleName_ = TEST_BUNDLE };
    auto ret = service.ObtainDistributedTableName(param, deviceId, TEST_STORE);
    EXPECT_GT(ret.length(), 0);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
}

/**
* @tc.name: ObtainDistributedTableName001
* @tc.desc: test ObtainDistributedTableName, uuid invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhaojh
*/
HWTEST_F(RdbServiceImplTest, ObtainDistributedTableName001, TestSize.Level0)
{
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyWithoutPath(), metaData_, false), true);
    RdbServiceImpl service;
    RdbSyncerParam param;
    auto ret = service.ObtainDistributedTableName(param, "invalid_device_id", TEST_STORE);
    EXPECT_EQ(ret.length(), 0);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
}

/**
 * @tc.name: RemoteQuery001
 * @tc.desc: test RemoteQuery, param invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, RemoteQuery001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = TEST_BUNDLE;
    param.storeName_ = TEST_STORE;
    param.hapName_ = "test/test";
    std::vector<std::string> selectionArgs;
    auto deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    auto ret = service.RemoteQuery(param, deviceId, "", selectionArgs);
    EXPECT_EQ(ret.first, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
}

/**
 * @tc.name: RemoteQuery002
 * @tc.desc: test RemoteQuery, when CheckAccess fails.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, RemoteQuery002, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    std::vector<std::string> selectionArgs;
    auto deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    auto ret = service.RemoteQuery(param, deviceId, "", selectionArgs);
    EXPECT_EQ(ret.first, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
}

/**
* @tc.name: TransferStringToHex001
* @tc.desc: test TransferStringToHex, param empty.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhaojh
*/
HWTEST_F(RdbServiceImplTest, TransferStringToHex001, TestSize.Level0)
{
    RdbServiceImpl service;
    auto ret = service.TransferStringToHex("");
    EXPECT_EQ(ret.length(), 0);
}

/**
* @tc.name: GetCallbacks001
* @tc.desc: test GetCallbacks.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhaojh
*/
HWTEST_F(RdbServiceImplTest, GetCallbacks001, TestSize.Level0)
{
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyWithoutPath(), metaData_, false), true);
    RdbServiceImpl service;
    auto ret = service.GetCallbacks(metaData_.tokenId, metaData_.storeId);
    EXPECT_EQ(ret, nullptr);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
}

/**
 * @tc.name: DoSync001
 * @tc.desc: Test DoSync when the store is null.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, DoSync001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    RdbService::Option option;
    PredicatesMemo predicates;
    AsyncDetail async;

    auto result = service.DoSync(param, option, predicates, async);
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: DoSync002
 * @tc.desc: Test DoSync when meta sync is needed and succeeds.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, DoSync002, TestSize.Level0)
{
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyWithoutPath(), metaData_, false), true);

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

    RdbService::Option option;
    option.mode = DistributedData::GeneralStore::AUTO_SYNC_MODE;
    option.seqNum = 1;

    PredicatesMemo predicates;
    AsyncDetail async;

    auto result = service.DoSync(param, option, predicates, async);
    EXPECT_EQ(result, RDB_ERROR);

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test
