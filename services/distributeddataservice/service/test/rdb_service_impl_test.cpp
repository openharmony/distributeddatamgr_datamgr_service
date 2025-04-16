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
#include "bootstrap.h"
#include "checker_mock.h"
#include "cloud/change_event.h"
#include "cloud/schema_meta.h"
#include "crypto_manager.h"
#include "device_manager_adapter.h"
#include "device_matrix.h"
#include "event_center.h"
#include "ipc_skeleton.h"
#include "metadata/capability_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "mock/db_store_mock.h"
#include "rdb_service_impl.h"
#include "relational_store_manager.h"
#include "gtest/gtest.h"

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

class RdbServiceImplTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    static void InitMetaData();
    void SetUp();
    void TearDown();
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
    auto ret = MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, false);
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

    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, false), true);

    DistributedDB::AutoLaunchParam param;
    RdbServiceImpl service;

    std::string identifier = "test_identifier";
    int32_t result = service.ResolveAutoLaunch(identifier, param);

    EXPECT_EQ(result, false);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), false), true);
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
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, false), true);
    DistributedDB::AutoLaunchParam param;
    RdbServiceImpl service;
    RelationalStoreManager userMgr(metaData_.appId, metaData_.user, metaData_.storeId);
    auto identifier = userMgr.GetRelationalStoreIdentifier(metaData_.user, metaData_.appId, metaData_.storeId, true);

    int32_t result = service.ResolveAutoLaunch(identifier, param);

    EXPECT_EQ(result, true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), false), true);
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
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, false), true);
    DistributedDB::AutoLaunchParam param;
    RdbServiceImpl service;
    RelationalStoreManager userMgr1(meta.appId, meta.user, meta.storeId);
    auto identifier = userMgr1.GetRelationalStoreIdentifier(meta.user, meta.appId, meta.storeId, true);

    int32_t result = service.ResolveAutoLaunch(identifier, param);

    EXPECT_EQ(result, true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), false), true);
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
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, false), true);
    RdbServiceImpl service;
    auto deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    auto ret = service.ObtainDistributedTableName(deviceId, TEST_STORE);
    EXPECT_GT(ret.length(), 0);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), false), true);
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
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, false), true);
    RdbServiceImpl service;
    auto ret = service.ObtainDistributedTableName("invalid_device_id", TEST_STORE);
    EXPECT_EQ(ret.length(), 0);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), false), true);
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
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, false), true);
    RdbServiceImpl service;
    auto ret = service.GetCallbacks(metaData_.tokenId, metaData_.storeId);
    EXPECT_EQ(ret, nullptr);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), false), true);
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
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, false), true);

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

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), false), true);
}

/**
 * @tc.name: IsNeedMetaSync001
 * @tc.desc: Test IsNeedMetaSync when LoadMeta fails for CapMetaData.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, IsNeedMetaSync001, TestSize.Level0)
{
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, false), true);
    std::vector<std::string> devices = {DmAdapter::GetInstance().ToUUID(metaData_.deviceId)};
    RdbServiceImpl service;
    bool result = service.IsNeedMetaSync(metaData_, devices);

    EXPECT_EQ(result, true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), false), true);
}

/**
 * @tc.name: IsNeedMetaSync002
 * @tc.desc: Test IsNeedMetaSync when LoadMeta fails for StoreMetaData.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, IsNeedMetaSync002, TestSize.Level0)
{
    CapMetaData capMetaData;
    auto capKey = CapMetaRow::GetKeyFor(metaData_.deviceId);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(std::string(capKey.begin(), capKey.end()), capMetaData), true);

    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, false), true);
    std::vector<std::string> devices = {DmAdapter::GetInstance().ToUUID(metaData_.deviceId)};
    RdbServiceImpl service;
    bool result = service.IsNeedMetaSync(metaData_, devices);

    EXPECT_EQ(result, false);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), false), true);
}

/**
 * @tc.name: ProcessResult001
 * @tc.desc: Test ProcessResult when all results have DBStatus::OK.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, ProcessResult001, TestSize.Level0)
{
    RdbServiceImpl service;
    std::map<std::string, int32_t> results = {{"device1", static_cast<int32_t>(DBStatus::OK)},
                                              {"device2", static_cast<int32_t>(DBStatus::OK)}};

    auto result = service.ProcessResult(results);

    EXPECT_EQ(result.second.at("device1"), DBStatus::OK);
    EXPECT_EQ(result.second.at("device2"), DBStatus::OK);
}

/**
 * @tc.name: ProcessResult002
 * @tc.desc: Test ProcessResult when some results have DBStatus::OK and others do not.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, ProcessResult002, TestSize.Level0)
{
    RdbServiceImpl service;
    std::map<std::string, int32_t> results = {{"device1", static_cast<int32_t>(DBStatus::OK)},
                                              {"device2", static_cast<int32_t>(DBStatus::DB_ERROR)},
                                              {"device3", static_cast<int32_t>(DBStatus::OK)}};

    auto result = service.ProcessResult(results);

    EXPECT_EQ(result.second.at("device1"), DBStatus::OK);
    EXPECT_EQ(result.second.at("device2"), DBStatus::DB_ERROR);
    EXPECT_EQ(result.second.at("device3"), DBStatus::OK);
}

/**
 * @tc.name: ProcessResult004
 * @tc.desc: Test ProcessResult with an empty results map.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, ProcessResult004, TestSize.Level0)
{
    RdbServiceImpl service;
    std::map<std::string, int32_t> results;

    auto result = service.ProcessResult(results);

    EXPECT_EQ(result.first.size(), 0);
    EXPECT_EQ(result.second.size(), 0);
}

/**
 * @tc.name: DoCompensateSync001
 * @tc.desc: Test DoCompensateSync when the event has valid bindInfo and COMPENSATE_SYNC event ID.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, DoCompensateSync001, TestSize.Level0)
{
    RdbServiceImpl service;

    int32_t eventId = 1;
    DistributedData::BindEvent::BindEventInfo bindInfo;
    bindInfo.bundleName = TEST_BUNDLE;
    bindInfo.tokenId = metaData_.tokenId;
    bindInfo.user = metaData_.uid;
    bindInfo.storeName = TEST_STORE;
    bindInfo.tableName = "test_table";
    bindInfo.primaryKey = {{"key1", "value1"}, {"key2", "value2"}};

    BindEvent event(eventId, std::move(bindInfo));
    EventCenter::GetInstance().Subscribe(CloudEvent::LOCAL_CHANGE, [this] (const Event &event) {
        auto &evt = static_cast<const SyncEvent &>(event);
        auto mode = evt.GetMode();
        EXPECT_EQ(GeneralStore::GetPriorityLevel(GeneralStore::GetHighMode(static_cast<uint32_t>(mode))), 1);
    });
    service.DoCompensateSync(event);
}

/**
 * @tc.name: ReportStatistic001
 * @tc.desc: Test ReportStatistic when CheckAccess fails.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, ReportStatistic001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    RdbStatEvent statEvent;

    int32_t result = service.ReportStatistic(param, statEvent);

    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: ReportStatistic002
 * @tc.desc: Test ReportStatistic when CheckAccess success.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, ReportStatistic002, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = TEST_BUNDLE;
    param.storeName_ = TEST_STORE;
    RdbStatEvent statEvent;

    int32_t result = service.ReportStatistic(param, statEvent);

    EXPECT_EQ(result, OK);
}

/**
 * @tc.name: GetReuseDevice001
 * @tc.desc: Test GetReuseDevice when all devices are reusable.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, GetReuseDevice001, TestSize.Level0)
{
    RdbServiceImpl service;
    std::vector<std::string> devices = {"device1"};

    auto result = service.GetReuseDevice(devices);
    EXPECT_EQ(result.size(), 0);
}

/**
 * @tc.name: DoAutoSync001
 * @tc.desc: Test DoAutoSync when the store is null.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, DoAutoSync001, TestSize.Level0)
{
    RdbServiceImpl service;
    std::vector<std::string> devices = {"device1"};
    DistributedData::Database dataBase;
    std::vector<std::string> tables = {"table1"};

    auto result = service.DoAutoSync(devices, dataBase, tables);
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: DoOnlineSync001
 * @tc.desc: Test DoOnlineSync when all tables have deviceSyncFields.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, DoOnlineSync001, TestSize.Level0)
{
    RdbServiceImpl service;
    std::vector<std::string> devices = {"device1"};
    DistributedData::Database dataBase;
    dataBase.name = TEST_STORE;

    DistributedData::Table table1;
    table1.name = "table1";
    table1.deviceSyncFields = {"field1", "field2"};
    DistributedData::Table table2;
    table2.name = "table2";
    table2.deviceSyncFields = {};

    dataBase.tables = {table1, table2};

    auto result = service.DoOnlineSync(devices, dataBase);
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: OnReady001
 * @tc.desc: Test OnReady when LoadMeta fails.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, OnReady001, TestSize.Level0)
{
    RdbServiceImpl service;
    std::string device = "test_device";

    int32_t result = service.OnReady(device);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: OnReady002
 * @tc.desc: Test OnReady when no databases have autoSyncType SYNC_ON_READY or SYNC_ON_CHANGE_READY.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, OnReady002, TestSize.Level0)
{
    RdbServiceImpl service;
    std::string device = metaData_.deviceId;

    DistributedData::Database dataBase1;
    dataBase1.name = "test_rdb_service_impl_sync_store2";
    dataBase1.bundleName = TEST_BUNDLE;
    dataBase1.user = std::to_string(AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId));
    dataBase1.autoSyncType = AutoSyncType::SYNC_ON_READY;

    DistributedData::Database dataBase2;
    dataBase2.name = "test_rdb_service_impl_sync_store2";
    dataBase2.bundleName = TEST_BUNDLE;
    dataBase2.user = std::to_string(AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId));
    dataBase2.autoSyncType = AutoSyncType::SYNC_ON_CHANGE_READY;

    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(dataBase1.GetKey(), metaData_, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(dataBase2.GetKey(), metaData_, true), true);
    int32_t result = service.OnReady(device);

    EXPECT_EQ(result, 0);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(dataBase1.GetKey(), metaData_, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(dataBase2.GetKey(), metaData_, true), true);
}

/**
 * @tc.name: SetSearchable001
 * @tc.desc: Test SetSearchable when CheckAccess fails.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, SetSearchable001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;

    bool isSearchable = true;
    int32_t result = service.SetSearchable(param, isSearchable);

    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: SetSearchable002
 * @tc.desc: Test SetSearchable when CheckAccess succeeds and PostSearchEvent is called.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, SetSearchable002, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;

    bool isSearchable = true;
    int32_t result = service.SetSearchable(param, isSearchable);

    EXPECT_EQ(result, RDB_OK);
}

/**
 * @tc.name: GetPassword001
 * @tc.desc: Test GetPassword when CheckAccess fails.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, GetPassword001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    std::vector<std::vector<uint8_t>> password;

    int32_t result = service.GetPassword(param, password);

    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: GetPassword002
 * @tc.desc: Test GetPassword when no meta data is found.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, GetPassword002, TestSize.Level0)
{
    auto meta = metaData_;
    meta.isEncrypt = true;
    std::vector<uint8_t> sKey{2,   249, 221, 119, 177, 216, 217, 134, 185, 139, 114, 38,  140, 64,  165, 35,
                              77,  169, 0,   226, 226, 166, 37,  73,  181, 229, 42,  88,  108, 111, 131, 104,
                              141, 43,  96,  119, 214, 34,  177, 129, 233, 96,  98,  164, 87,  115, 187, 170};
    SecretKeyMetaData secretKey;
    secretKey.sKey = CryptoManager::GetInstance().Encrypt(sKey);
    secretKey.area = 0;
    secretKey.storeType = meta.storeType;
    secretKey.time = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetSecretKey(), secretKey, true), true);

    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = meta.bundleName;
    param.storeName_ = meta.storeId;
    std::vector<std::vector<uint8_t>> password;
    int32_t result = service.GetPassword(param, password);

    EXPECT_EQ(result, RDB_OK);
    size_t KEY_COUNT = 2;
    ASSERT_EQ(password.size(), KEY_COUNT);
    EXPECT_EQ(password.at(0), sKey);
    MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(meta.GetSecretKey(), true);
}

/**
 * @tc.name: GetPassword003
 * @tc.desc: Test GetPassword when decryption fails.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, GetPassword003, TestSize.Level0)
{
    SecretKeyMetaData secretKey;
    secretKey.sKey = {0x01, 0x02, 0x03}; // Invalid key for decryption
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetSecretKey(), secretKey, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true), true);

    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    std::vector<std::vector<uint8_t>> password;

    int32_t result = service.GetPassword(param, password);

    EXPECT_EQ(result, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetSecretKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true), true);
}

/**
 * @tc.name: GetPassword004
 * @tc.desc: Test GetPassword when no meta data is found.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, GetPassword004, TestSize.Level0)
{
    auto meta = metaData_;
    meta.isEncrypt = true;
    std::vector<uint8_t> sKey{2,   249, 221, 119, 177, 216, 217, 134, 185, 139, 114, 38,  140, 64,  165, 35,
                              77,  169, 0,   226, 226, 166, 37,  73,  181, 229, 42,  88,  108, 111, 131, 104,
                              141, 43,  96,  119, 214, 34,  177, 129, 233, 96,  98,  164, 87,  115, 187, 170};
    SecretKeyMetaData secretKey;
    secretKey.sKey = CryptoManager::GetInstance().Encrypt(sKey);
    secretKey.area = 0;
    secretKey.storeType = meta.storeType;
    secretKey.time = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetCloneSecretKey(), secretKey, true), true);

    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = meta.bundleName;
    param.storeName_ = meta.storeId;
    param.type_ = meta.storeType;
    std::vector<std::vector<uint8_t>> password;

    int32_t result = service.GetPassword(param, password);

    EXPECT_EQ(result, RDB_OK);
    size_t KEY_COUNT = 2;
    ASSERT_EQ(password.size(), KEY_COUNT);
    EXPECT_EQ(password.at(1), sKey);
    MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(metaData_.GetCloneSecretKey(), true);
}

/**
 * @tc.name: LockCloudContainer001
 * @tc.desc: Test LockCloudContainer when CheckAccess fails.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, LockCloudContainer001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;

    auto result = service.LockCloudContainer(param);

    EXPECT_EQ(result.first, RDB_ERROR);
    EXPECT_EQ(result.second, 0);
}

/**
 * @tc.name: LockCloudContainer002
 * @tc.desc: Test LockCloudContainer when CheckAccess succeeds and callback updates the result.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, LockCloudContainer002, TestSize.Level0)
{
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, false), true);

    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;

    auto result = service.LockCloudContainer(param);

    // Simulate callback execution
    EXPECT_EQ(result.first, RDB_ERROR);
    EXPECT_EQ(result.second, 0);

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), false), true);
}

/**
 * @tc.name: UnlockCloudContainer001
 * @tc.desc: Test UnlockCloudContainer when CheckAccess fails.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, UnlockCloudContainer001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;

    int32_t result = service.UnlockCloudContainer(param);

    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: UnlockCloudContainer002
 * @tc.desc: Test UnlockCloudContainer when CheckAccess succeeds and callback updates the result.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, UnlockCloudContainer002, TestSize.Level0)
{
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, false), true);

    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;

    int32_t result = service.UnlockCloudContainer(param);

    // Simulate callback execution
    EXPECT_EQ(result, RDB_ERROR); // Assuming the callback sets status to RDB_OK

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), false), true);
}

/**
 * @tc.name: GetDebugInfo001
 * @tc.desc: Test GetDebugInfo when CheckAccess fails.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, GetDebugInfo001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    std::map<std::string, RdbDebugInfo> debugInfo;

    int32_t result = service.GetDebugInfo(param, debugInfo);

    EXPECT_EQ(result, RDB_ERROR);
    EXPECT_TRUE(debugInfo.empty());
}

/**
 * @tc.name: VerifyPromiseInfo001
 * @tc.desc: Test VerifyPromiseInfo when LoadMeta fails.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, VerifyPromiseInfo001, TestSize.Level0)
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
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, VerifyPromiseInfo002, TestSize.Level0)
{
    StoreMetaDataLocal localMeta;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    localMeta.isAutoSync = true;
    localMeta.promiseInfo.tokenIds = {tokenId};
    localMeta.promiseInfo.uids = {};
    localMeta.promiseInfo.permissionNames = {};

    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyLocal(), localMeta, true), true);

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
