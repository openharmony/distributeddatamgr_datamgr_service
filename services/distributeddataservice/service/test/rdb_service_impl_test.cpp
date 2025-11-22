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

#include "rdb_service_impl.h"

#include <gtest/gtest.h>
#include <random>

#include "account/account_delegate.h"
#include "bootstrap.h"
#include "checker_mock.h"
#include "cloud/change_event.h"
#include "cloud/schema_meta.h"
#include "device_manager_adapter.h"
#include "device_matrix.h"
#include "directory/directory_manager.h"
#include "eventcenter/event_center.h"
#include "ipc_skeleton.h"
#include "metadata/appid_meta_data.h"
#include "metadata/capability_meta_data.h"
#include "metadata/meta_data_manager.h"
#include "metadata/special_channel_data.h"
#include "metadata/store_debug_info.h"
#include "metadata/store_meta_data_local.h"
#include "mock/db_store_mock.h"
#include "mock/device_manager_adapter_mock.h"
#include "mock/general_store_mock.h"
#include "rdb_general_store.h"
#include "rdb_types.h"
#include "relational_store_manager.h"
#include "sync_mgr/sync_mgr.h"

using namespace OHOS::DistributedRdb;
using namespace OHOS::DistributedData;
using namespace DistributedDB;
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
static constexpr const char *TEST_SYNC_DEVICE = "test_sync_device1";
static constexpr const char *TEST_INVALID_DEVICE = "test_invalid_device1";
static constexpr int32_t KEY_LENGTH = 32;
static constexpr uint32_t DELY_TIME = 10000;

class RdbServiceImplTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    static void InitMetaData();
    static void InitMapping(StoreMetaData &meta);
    void SetUp();
    void TearDown();
    static std::vector<uint8_t> Random(int32_t len);
protected:
    static void InitMetaDataManager();
    static StoreMetaData GetDBMetaData(const Database &database);
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static StoreMetaData metaData_;
    static CheckerMock systemChecker_;
    static inline std::shared_ptr<DeviceManagerAdapterMock> deviceManagerAdapterMock = nullptr;
};

std::shared_ptr<DBStoreMock> RdbServiceImplTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
StoreMetaData RdbServiceImplTest::metaData_;
CheckerMock RdbServiceImplTest::systemChecker_;

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

void RdbServiceImplTest::InitMapping(StoreMetaData &metaMapping)
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

StoreMetaData RdbServiceImplTest::GetDBMetaData(const Database &database)
{
    StoreMetaData metaData = metaData_;
    RdbSyncerParam param;
    metaData.bundleName = database.bundleName;
    metaData.user = database.user;
    metaData.storeId = RdbServiceImpl::RemoveSuffix(database.name);
    return metaData;
}

void RdbServiceImplTest::SetUpTestCase()
{
    deviceManagerAdapterMock = std::make_shared<DeviceManagerAdapterMock>();
    BDeviceManagerAdapter::deviceManagerAdapter = deviceManagerAdapterMock;
    DeviceInfo deviceInfo;
    deviceInfo.uuid = "ABCD";
    EXPECT_CALL(*deviceManagerAdapterMock, GetLocalDevice()).WillRepeatedly(Return(deviceInfo));
    EXPECT_CALL(*deviceManagerAdapterMock, GetUuidByNetworkId(_)).WillRepeatedly(Return(deviceInfo.uuid));
    EXPECT_CALL(*deviceManagerAdapterMock, CalcClientUuid(_, _)).WillRepeatedly(Return(deviceInfo.uuid));
    EXPECT_CALL(*deviceManagerAdapterMock, ToUUID(deviceInfo.uuid)).WillRepeatedly(Return(deviceInfo.uuid));

    InitMetaData();
    Bootstrap::GetInstance().LoadCheckers();
    CryptoManager::GetInstance().GenerateRootKey();
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");

    auto creator = [](const StoreMetaData &metaData,
                       const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
        return { GeneralError::E_OK, new (std::nothrow) GeneralStoreMock() };
    };
    AutoCache::GetInstance().RegCreator(DistributedRdb::RDB_DEVICE_COLLABORATION, creator);
    SyncManager::AutoSyncInfo syncInfo = { 3, TEST_APPID, TEST_BUNDLE };
    SyncManager::GetInstance().Initialize({ syncInfo });
}

void RdbServiceImplTest::TearDownTestCase()
{
    deviceManagerAdapterMock = nullptr;
    BDeviceManagerAdapter::deviceManagerAdapter = nullptr;
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

void RdbServiceImplTest::SetUp()
{
    RdbServiceImpl service;
    service.SaveAutoSyncInfo(metaData_, { TEST_SYNC_DEVICE });
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
* @tc.name: ObtainDistributedTableName001
* @tc.desc: test ObtainDistributedTableName, uuid exist.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhaojh
*/
HWTEST_F(RdbServiceImplTest, ObtainDistributedTableName001, TestSize.Level0)
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
* @tc.name: ObtainDistributedTableName002
* @tc.desc: test ObtainDistributedTableName, uuid invalid.
* @tc.type: FUNC
* @tc.require:
* @tc.author: zhaojh
*/
HWTEST_F(RdbServiceImplTest, ObtainDistributedTableName002, TestSize.Level0)
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
 * @tc.name: RemoteQuery003
 * @tc.desc: test RemoteQuery, when CheckAccess pass but query failed.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, RemoteQuery003, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = TEST_BUNDLE;
    param.storeName_ = "RemoteQuery003";
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
    StoreMetaData metaData;
    RdbService::Option option;
    PredicatesMemo predicates;
    AsyncDetail async;

    auto result = service.DoSync(metaData, option, predicates, async);
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
    RdbService::Option option;
    option.mode = DistributedData::GeneralStore::AUTO_SYNC_MODE;
    option.seqNum = 1;

    PredicatesMemo predicates;
    AsyncDetail async;

    auto result = service.DoSync(metaData_, option, predicates, async);
    EXPECT_EQ(result, RDB_OK);

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
}

/**
 * @tc.name: DoSync003
 * @tc.desc: Test DoSync when meta sync with device.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Sven Wang
 */
HWTEST_F(RdbServiceImplTest, DoSync003, TestSize.Level0)
{
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyWithoutPath(), metaData_, false), true);

    RdbServiceImpl service;
    RdbService::Option option;
    option.mode = DistributedData::GeneralStore::AUTO_SYNC_MODE;
    option.seqNum = 1;

    PredicatesMemo predicates;
    AsyncDetail async;
    predicates.devices_ = { TEST_SYNC_DEVICE };
    auto result = service.DoSync(metaData_, option, predicates, async);
    EXPECT_EQ(result, RDB_OK);

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
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
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyWithoutPath(), metaData_, false), true);
    std::vector<std::string> devices = { DmAdapter::GetInstance().ToUUID(metaData_.deviceId) };
    RdbServiceImpl service;
    bool result = service.IsNeedMetaSync(metaData_, devices);

    EXPECT_EQ(result, true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
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

    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyWithoutPath(), metaData_, false), true);
    std::vector<std::string> devices = { DmAdapter::GetInstance().ToUUID(metaData_.deviceId) };
    RdbServiceImpl service;
    bool result = service.IsNeedMetaSync(metaData_, devices);

    EXPECT_EQ(result, false);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
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
    std::map<std::string, int32_t> results = { { "device1", static_cast<int32_t>(DBStatus::OK) },
        { "device2", static_cast<int32_t>(DBStatus::OK) } };

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
    std::map<std::string, int32_t> results = { { "device1", static_cast<int32_t>(DBStatus::OK) },
        { "device2", static_cast<int32_t>(DBStatus::DB_ERROR) }, { "device3", static_cast<int32_t>(DBStatus::OK) } };

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
    bindInfo.primaryKey = { { "key1", "value1" }, { "key2", "value2" } };

    BindEvent event(eventId, std::move(bindInfo));
    EventCenter::GetInstance().Subscribe(CloudEvent::LOCAL_CHANGE, [this](const Event &event) {
        auto &evt = static_cast<const SyncEvent &>(event);
        auto mode = evt.GetMode();
        EXPECT_EQ(GeneralStore::GetPriorityLevel(GeneralStore::GetHighMode(static_cast<uint32_t>(mode))), 1);
    });
    service.DoCompensateSync(event);
    EventCenter::GetInstance().Unsubscribe(CloudEvent::LOCAL_CHANGE);
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
    std::vector<std::string> devices = { "device1" };
    StoreMetaData metaData;
    auto result = service.GetReuseDevice(devices, metaData);
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
    std::vector<std::string> devices = { "device1" };
    StoreMetaData metaData;
    std::vector<std::string> tables = { "table1" };

    auto result = service.DoAutoSync(devices, metaData, tables);
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: DoAutoSync002
 * @tc.desc: Test DoAutoSync when the store is null.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: my
 */
HWTEST_F(RdbServiceImplTest, DoAutoSync002, TestSize.Level0)
{
    StoreMetaData syncMeta;
    InitMapping(syncMeta);
    syncMeta.dataDir = "path";
    RdbServiceImpl::SaveSyncMeta(syncMeta);
    StoreMetaMapping metaMapping(syncMeta);
    metaMapping.devicePath = "path1";
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);

    RdbServiceImpl service;
    std::vector<std::string> devices = { "device1" };
    DistributedData::Database database;
    std::vector<std::string> tables = { "table1" };
    database.bundleName = "bundleName";
    database.name = "storeName";
    database.user = "100";
    auto [exists, metaData] = RdbServiceImpl::LoadSyncMeta(database);
    EXPECT_EQ(exists, true);
    auto result = service.DoAutoSync(devices, metaData, tables);
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: OnReady_DoAutoSync001
 * @tc.desc: Test OnReady_DoAutoSync001 when all tables have deviceSyncFields.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, OnReady_DoAutoSync001, TestSize.Level0)
{
    RdbServiceImpl service;
    std::vector<std::string> devices = { "device1" };
    DistributedData::Database database;
    database.name = TEST_STORE;

    DistributedData::Table table1;
    table1.name = "table1";
    table1.deviceSyncFields = { "field1", "field2" };
    DistributedData::Table table2;
    table2.name = "table2";
    table2.deviceSyncFields = {};

    database.tables = { table1, table2 };
    auto [exists, metaData] = RdbServiceImpl::LoadSyncMeta(database);
    EXPECT_EQ(exists, false);
    auto result = service.DoAutoSync(devices, metaData, database.GetSyncTables());
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
    EXPECT_EQ(result, -E_OK);
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
    service.OnInitialize();
    DistributedData::Database dataBase1;
    dataBase1.name = "test_rdb_service_impl_sync_store1";
    dataBase1.bundleName = TEST_BUNDLE;
    dataBase1.user = std::to_string(AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId));
    dataBase1.autoSyncType = AutoSyncType::SYNC_ON_READY;

    DistributedData::Database dataBase2;
    dataBase2.name = "test_rdb_service_impl_sync_store2";
    dataBase2.bundleName = TEST_BUNDLE;
    dataBase2.user = std::to_string(AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId));
    dataBase2.autoSyncType = AutoSyncType::SYNC_ON_CHANGE_READY;
    auto metaData1 = GetDBMetaData(dataBase1);
    RdbServiceImpl::SaveSyncMeta(metaData1);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(dataBase1.GetKey(), dataBase1, true), true);
    auto metaData2 = GetDBMetaData(dataBase2);
    RdbServiceImpl::SaveSyncMeta(metaData2);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(dataBase2.GetKey(), dataBase2, true), true);
    int32_t result = service.OnReady(TEST_SYNC_DEVICE);
    EXPECT_EQ(result, 2);
    result = service.OnReady(TEST_INVALID_DEVICE);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(dataBase1.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData1.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(dataBase2.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData2.GetKey(), true), true);
}

/**
 * @tc.name: OnReady002
 * @tc.desc: Test OnReady when no databases have autoSyncType SYNC_ON_READY or SYNC_ON_CHANGE_READY.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Sven Wang
 */
HWTEST_F(RdbServiceImplTest, OnReady003, TestSize.Level0)
{
    RdbServiceImpl service;
    service.OnInitialize();
    DistributedData::Database database;
    database.bundleName = TEST_BUNDLE;
    database.user = std::to_string(AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId));
    database.autoSyncType = AutoSyncType::SYNC_ON_READY;
    for (int i = 0; i < 10; ++i) {
        database.name = "test_rdb_service_impl_sync_store" + std::to_string(i);
        EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(database.GetKey(), database, true), true);
        RdbServiceImpl::SaveSyncMeta(GetDBMetaData(database));
    }

    int32_t result = service.OnReady(TEST_SYNC_DEVICE);
    EXPECT_EQ(result, -E_OVER_MAX_LIMITS);

    for (int i = 0; i < 10; ++i) {
        database.name = "test_rdb_service_impl_sync_store" + std::to_string(i);
        EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(database.GetKey(), true), true);
        EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(GetDBMetaData(database).GetKeyWithoutPath()), true);
    }
}

/**
 * @tc.name: OnReady004
 * @tc.desc: Test OnReady when no databases have autoSyncType SYNC_ON_READY or SYNC_ON_CHANGE_READY.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Sven Wang
 */
HWTEST_F(RdbServiceImplTest, OnReady004, TestSize.Level0)
{
    RdbServiceImpl service;
    service.OnInitialize();
    DistributedData::Database database;
    database.bundleName = TEST_BUNDLE;
    database.user = std::to_string(AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId));
    database.autoSyncType = AutoSyncType::SYNC_ON_READY;
    for (int i = 0; i < 3; ++i) {
        database.name = "test_rdb_service_impl_sync_store" + std::to_string(i);
        EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(database.GetKey(), database, true), true);
        RdbServiceImpl::SaveSyncMeta(GetDBMetaData(database));
    }

    int32_t result = service.OnReady("test_sync_device2");
    EXPECT_EQ(result, 0);
    service.SaveAutoSyncInfo(metaData_, { "test_sync_device2" });
    result = service.OnReady("test_sync_device2");
    EXPECT_EQ(result, 3);
    for (int i = 0; i < 3; ++i) {
        database.name = "test_rdb_service_impl_sync_store" + std::to_string(i);
        EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(database.GetKey(), true), true);
        EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(GetDBMetaData(database).GetKeyWithoutPath()), true);
    }
}

/**
 * @tc.name: SpecialChannel001
 * @tc.desc: Test OnReady when no databases have autoSyncType SYNC_ON_READY or SYNC_ON_CHANGE_READY.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Sven Wang
 */
HWTEST_F(RdbServiceImplTest, SpecialChannel001, TestSize.Level0)
{
    RdbServiceImpl service;
    service.OnInitialize();
    auto result = service.IsSpecialChannel(TEST_SYNC_DEVICE);
    EXPECT_EQ(result, true);
    result = service.IsSpecialChannel("test_sync_device2");
    EXPECT_EQ(result, false);
    service.SaveAutoSyncInfo(metaData_, { "test_sync_device2" });
    result = service.IsSpecialChannel("test_sync_device2");
    EXPECT_EQ(result, true);
}

/**
 * @tc.name: SpecialChannel002
 * @tc.desc: Test OnReady when no databases have autoSyncType SYNC_ON_READY or SYNC_ON_CHANGE_READY.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Sven Wang
 */
HWTEST_F(RdbServiceImplTest, SpecialChannel002, TestSize.Level0)
{
    {
        RdbServiceImpl service;
        service.OnInitialize();
        service.SaveAutoSyncInfo(metaData_, { "test_sync_device2" });
    }
    RdbServiceImpl service;
    service.OnInitialize();
    auto result = service.IsSpecialChannel("test_sync_device2");
    EXPECT_EQ(result, true);
}

/**
 * @tc.name: SpecialChannel003
 * @tc.desc: Test OnReady when no databases have autoSyncType SYNC_ON_READY or SYNC_ON_CHANGE_READY.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Sven Wang
 */
HWTEST_F(RdbServiceImplTest, SpecialChannel003, TestSize.Level0)
{
    {
        RdbServiceImpl service;
        service.OnInitialize();
        service.SaveAutoSyncInfo(metaData_, { "test_sync_device2" });
    }
    auto executors = std::make_shared<ExecutorPool>(1, 0);
    EXPECT_NE(executors, nullptr);
    RdbServiceImpl service;
    service.OnInitialize();
    service.OnBind({ .executors = executors });
    auto result = service.IsSpecialChannel(TEST_SYNC_DEVICE);
    EXPECT_EQ(result, true);
    executors->Remove(service.saveChannelsTask_);
    service.OnBind({ .executors = nullptr });
    executors = nullptr;
}

/**
 * @tc.name: SpecialChannel004
 * @tc.desc: Test OnReady when no databases have autoSyncType SYNC_ON_READY or SYNC_ON_CHANGE_READY.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Sven Wang
 */
HWTEST_F(RdbServiceImplTest, SpecialChannel004, TestSize.Level0)
{
    {
        RdbServiceImpl service;
        service.OnInitialize();
        service.SaveAutoSyncInfo(metaData_, { "test_sync_device2" });
    }
    RdbServiceImpl service;
    service.OnInitialize();
    std::vector<std::string> devices = { TEST_SYNC_DEVICE, "test_sync_device2", "test_sync_device3",
        "test_sync_device4" };
    if (SyncManager::GetInstance().IsAutoSyncApp(metaData_.bundleName, metaData_.appId)) {
        auto begin = std::remove_if(devices.begin(), devices.end(), [&service](const std::string &device) {
            return !service.IsSpecialChannel(device);
        });
        devices.erase(begin, devices.end());
    }
    std::vector<std::string> devTag = { TEST_SYNC_DEVICE, "test_sync_device2" };
    EXPECT_EQ(devices, devTag);
}

/**
 * @tc.name: AfterOpen001
 * @tc.desc: Test AfterOpen when CheckParam not pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, AfterOpen001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    param.hapName_ = "test/test";
    int32_t result = service.AfterOpen(param);

    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: AfterOpen002
 * @tc.desc: Test AfterOpen when CheckAccess not pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, AfterOpen002, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    int32_t result = service.AfterOpen(param);

    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: AfterOpen003
 * @tc.desc: Test AfterOpen when CheckAccess pass and CheckParam pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, AfterOpen003, TestSize.Level0)
{
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true), true);
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    param.tokenIds_ = { 123 };
    param.uids_ = { 123 };
    param.permissionNames_ = {};
    AppIDMetaData appIdMeta;
    appIdMeta.bundleName = metaData_.bundleName;
    appIdMeta.appId = metaData_.appId;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(appIdMeta.GetKey(), appIdMeta, true), true);
    StoreMetaDataLocal localMeta;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyLocal(), localMeta, true), true);
    int32_t result = service.AfterOpen(param);
    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
    param.tokenIds_ = { 123 };
    param.uids_ = { 456 };
    result = service.AfterOpen(param);
    EXPECT_EQ(result, RDB_OK);
    param.permissionNames_ = { "com.example.myapplication" };
    result = service.AfterOpen(param);
    EXPECT_EQ(result, RDB_OK);

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyLocal(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(appIdMeta.GetKey(), true), true);
}

/**
 * @tc.name: AfterOpen004
 * @tc.desc: Test AfterOpen when CheckAccess pass and CheckParam pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, AfterOpen004, TestSize.Level0)
{
    metaData_.isSearchable = true;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyWithoutPath(), metaData_, false), true);
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    int32_t result = service.AfterOpen(param);

    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
}

/**
 * @tc.name: NotifyDataChange001
 * @tc.desc: Test NotifyDataChange when CheckParam not pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, NotifyDataChange001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    param.hapName_ = "test/test";
    RdbChangedData rdbChangedData;
    RdbNotifyConfig rdbNotifyConfig;
    int32_t result = service.NotifyDataChange(param, rdbChangedData, rdbNotifyConfig);

    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: NotifyDataChange002
 * @tc.desc: Test NotifyDataChange when CheckAccess not pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, NotifyDataChange002, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    RdbChangedData rdbChangedData;
    RdbNotifyConfig rdbNotifyConfig;
    int32_t result = service.NotifyDataChange(param, rdbChangedData, rdbNotifyConfig);

    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: NotifyDataChange003
 * @tc.desc: Test NotifyDataChange when Check pass.
 * @tc.type: FUNC
 */
HWTEST_F(RdbServiceImplTest, NotifyDataChange003, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.storeName_ = "validStoreName";
    param.bundleName_ = "validBundleName";
    param.user_ = "validUser";
    param.hapName_ = "validHapName";
    param.customDir_ = "dir1/dir2";
    RdbChangedData rdbChangedData;
    RdbNotifyConfig rdbNotifyConfig;
    auto metaData = service.GetStoreMetaData(param);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData.GetKey(), metaData, true), true);
    rdbNotifyConfig.delay_ = 0;
    int32_t result = service.NotifyDataChange(param, rdbChangedData, rdbNotifyConfig);
    EXPECT_EQ(result, RDB_OK);
    rdbNotifyConfig.delay_ = DELY_TIME;
    result = service.NotifyDataChange(param, rdbChangedData, rdbNotifyConfig);
    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData.GetKey(), true), true);
}

/**
 * @tc.name: NotifyDataChange004
 * @tc.desc: Test NotifyDataChange when Check pass.
 * @tc.type: FUNC
 */
HWTEST_F(RdbServiceImplTest, NotifyDataChange004, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.storeName_ = metaData_.storeId;
    param.bundleName_ = metaData_.bundleName;
    param.user_ = metaData_.user;
    param.hapName_ = metaData_.hapName;
    param.customDir_ = metaData_.customDir;
    param.area_ = metaData_.area;
    RdbChangedData rdbChangedData;
    RdbNotifyConfig rdbNotifyConfig;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true), true);
    Database database;
    database.bundleName = metaData_.bundleName;
    database.name = metaData_.storeId;
    database.user = metaData_.user;
    database.deviceId = metaData_.deviceId;
    database.autoSyncType = AutoSyncType::SYNC_ON_CHANGE_READY;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(database.GetKey(), database, true), true);
    rdbNotifyConfig.delay_ = 0;
    int32_t result = service.NotifyDataChange(param, rdbChangedData, rdbNotifyConfig);
    EXPECT_EQ(result, RDB_OK);
    rdbNotifyConfig.delay_ = DELY_TIME;
    result = service.NotifyDataChange(param, rdbChangedData, rdbNotifyConfig);
    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(database.GetKey(), true), true);
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
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    auto meta = metaData_;
    meta.isEncrypt = true;
    auto sKey = Random(KEY_LENGTH);
    ASSERT_FALSE(sKey.empty());
    SecretKeyMetaData secretKey;
    CryptoManager::CryptoParams encryptParams;
    secretKey.sKey = CryptoManager::GetInstance().Encrypt(sKey, encryptParams);
    secretKey.area = encryptParams.area;
    secretKey.storeType = meta.storeType;
    secretKey.nonce = encryptParams.nonce;

    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetSecretKey(), secretKey, true), true);

    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = meta.bundleName;
    param.storeName_ = meta.storeId;
    std::vector<std::vector<uint8_t>> password;
    int32_t result = service.GetPassword(param, password);

    EXPECT_EQ(result, RDB_OK);
    ASSERT_GT(password.size(), 0);
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
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    auto meta = metaData_;
    meta.isEncrypt = true;
    auto sKey = Random(KEY_LENGTH);
    ASSERT_FALSE(sKey.empty());
    SecretKeyMetaData secretKey;
    secretKey.sKey = sKey; // Invalid key for decryption
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetSecretKey(), secretKey, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);

    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = meta.bundleName;
    param.storeName_ = meta.storeId;
    std::vector<std::vector<uint8_t>> password;

    int32_t result = service.GetPassword(param, password);

    EXPECT_EQ(result, RDB_ERROR);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetSecretKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
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
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    auto meta = metaData_;
    meta.isEncrypt = true;
    auto sKey = Random(KEY_LENGTH);
    ASSERT_FALSE(sKey.empty());
    SecretKeyMetaData secretKey;
    CryptoManager::CryptoParams encryptParams;
    secretKey.sKey = CryptoManager::GetInstance().Encrypt(sKey, encryptParams);
    secretKey.area = encryptParams.area;
    secretKey.storeType = meta.storeType;
    secretKey.nonce = encryptParams.nonce;

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
    ASSERT_GT(password.size(), 0);
    EXPECT_EQ(password.at(0), sKey);
    MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(meta.GetCloneSecretKey(), true);
}

/**
 * @tc.name: GetPassword005
 * @tc.desc: Test GetPassword when no meta data.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, GetPassword005, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = TEST_BUNDLE;
    param.storeName_ = TEST_STORE;
    param.type_ = StoreMetaData::StoreType::STORE_RELATIONAL_BEGIN;
    std::vector<std::vector<uint8_t>> password;

    int32_t result = service.GetPassword(param, password);

    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: GetPassword006
 * @tc.desc: Test GetPassword when meta data is found.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, GetPassword006, TestSize.Level0)
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    auto meta = metaData_;
    meta.isEncrypt = true;
    auto sKey = Random(KEY_LENGTH);
    ASSERT_FALSE(sKey.empty());
    SecretKeyMetaData secretKey;
    CryptoManager::CryptoParams encryptParams;
    secretKey.sKey = CryptoManager::GetInstance().Encrypt(sKey, encryptParams);
    secretKey.area = encryptParams.area;
    secretKey.storeType = meta.storeType;
    secretKey.nonce = encryptParams.nonce;

    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetCloneSecretKey(), secretKey, true), true);

    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = meta.bundleName;
    param.storeName_ = meta.storeId;
    param.type_ = meta.storeType;
    param.customDir_ = "../../../base/haps/entry/files/.backup/textautofill";
    std::vector<std::vector<uint8_t>> password;

    int32_t result = service.GetPassword(param, password);

    EXPECT_EQ(result, RDB_OK);
    ASSERT_GT(password.size(), 0);
    EXPECT_EQ(password.at(0), sKey);
    MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(meta.GetCloneSecretKey(), true);
}

/**
 * @tc.name: SetDistributedTables001
 * @tc.desc: Test SetDistributedTables when CheckAccess not pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, SetDistributedTables001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    std::vector<std::string> tables;
    std::vector<OHOS::DistributedRdb::Reference> references;

    int32_t result = service.SetDistributedTables(param, tables, references, false);
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: SetDistributedTables002
 * @tc.desc: Test SetDistributedTables when type is search.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, SetDistributedTables002, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = TEST_BUNDLE;
    param.storeName_ = TEST_STORE;
    param.type_ = StoreMetaData::StoreType::STORE_RELATIONAL_BEGIN;
    std::vector<std::string> tables;
    std::vector<OHOS::DistributedRdb::Reference> references;

    int32_t result =
        service.SetDistributedTables(param, tables, references, false, DistributedTableType::DISTRIBUTED_SEARCH);
    EXPECT_EQ(result, RDB_OK);
}

/**
 * @tc.name: SetDistributedTables003
 * @tc.desc: Test SetDistributedTables when type is search.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, SetDistributedTables003, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = TEST_BUNDLE;
    param.storeName_ = TEST_STORE;
    param.type_ = StoreMetaData::StoreType::STORE_RELATIONAL_BEGIN;
    param.hapName_ = "test/test";
    std::vector<std::string> tables;
    std::vector<OHOS::DistributedRdb::Reference> references;

    int32_t result =
        service.SetDistributedTables(param, tables, references, false, DistributedTableType::DISTRIBUTED_SEARCH);
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: SetDistributedTables004
 * @tc.desc: Test SetDistributedTables when type is device.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, SetDistributedTables004, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = TEST_BUNDLE;
    param.storeName_ = "SetDistributedTables004";
    param.type_ = StoreMetaData::StoreType::STORE_RELATIONAL_BEGIN;
    std::vector<std::string> tables;
    std::vector<OHOS::DistributedRdb::Reference> references;

    auto meta = service.GetStoreMetaData(param);
    ASSERT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);

    int32_t result =
        service.SetDistributedTables(param, tables, references, false, DistributedTableType::DISTRIBUTED_DEVICE);
    EXPECT_EQ(result, RDB_OK);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: SetDistributedTables005
 * @tc.desc: Test SetDistributedTables when type is device.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: Sven Wang
 */
HWTEST_F(RdbServiceImplTest, SetDistributedTables005, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    param.type_ = metaData_.storeType;
    param.area_ = metaData_.area;
    param.level_ = metaData_.securityLevel;
    param.isEncrypt_ = metaData_.isEncrypt;
    ASSERT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true), true);
    auto result = service.SetDistributedTables(param, {}, {}, false, DistributedTableType::DISTRIBUTED_DEVICE);
    EXPECT_EQ(result, RDB_OK);
    Database database;
    database.bundleName = metaData_.bundleName;
    database.name = metaData_.storeId;
    database.user = metaData_.user;
    ASSERT_EQ(MetaDataManager::GetInstance().SaveMeta(database.GetKey(), database, true), true);
    result = service.SetDistributedTables(param, {}, {}, false, DistributedTableType::DISTRIBUTED_DEVICE);

    EXPECT_EQ(result, RDB_OK);
    StoreMetaMapping metaMapping(metaData_);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(database.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath()), true);
}

/**
 * @tc.name: SetDistributedTables006
 * @tc.desc: Test SetDistributedTables when type is cloud
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(RdbServiceImplTest, SetDistributedTables006, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    param.type_ = metaData_.storeType;
    param.area_ = metaData_.area;
    param.level_ = metaData_.securityLevel;
    param.isEncrypt_ = metaData_.isEncrypt;
    param.asyncDownloadAsset_ = false;
    param.enableCloud_ = true;
    param.autoSyncSwitch_ = true;
    metaData_.asyncDownloadAsset = true;
    metaData_.enableCloud = true;
    metaData_.autoSyncSwitch = true;
    ASSERT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true), true);
    auto result = service.SetDistributedTables(param, {}, {}, false, DistributedTableType::DISTRIBUTED_CLOUD);
    EXPECT_EQ(result, RDB_OK);
    StoreMetaMapping metaMapping(metaData_);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath()), true);
}

/**
 * @tc.name: SetDistributedTables007
 * @tc.desc: Test SetDistributedTables when type is cloud.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(RdbServiceImplTest, SetDistributedTables007, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    param.type_ = metaData_.storeType;
    param.area_ = metaData_.area;
    param.level_ = metaData_.securityLevel;
    param.isEncrypt_ = metaData_.isEncrypt;
    param.asyncDownloadAsset_ = true;
    param.enableCloud_ = false;
    param.autoSyncSwitch_ = true;
    metaData_.asyncDownloadAsset = true;
    metaData_.enableCloud = true;
    metaData_.autoSyncSwitch = true;
    ASSERT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true), true);
    auto result = service.SetDistributedTables(param, {}, {}, false, DistributedTableType::DISTRIBUTED_CLOUD);
    EXPECT_EQ(result, RDB_OK);
    StoreMetaMapping metaMapping(metaData_);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath()), true);
}

/**
 * @tc.name: SetDistributedTables008
 * @tc.desc: Test SetDistributedTables when type is cloud.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(RdbServiceImplTest, SetDistributedTables008, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    param.type_ = metaData_.storeType;
    param.area_ = metaData_.area;
    param.level_ = metaData_.securityLevel;
    param.isEncrypt_ = metaData_.isEncrypt;
    param.asyncDownloadAsset_ = true;
    param.enableCloud_ = true;
    param.autoSyncSwitch_ = false;
    metaData_.asyncDownloadAsset = true;
    metaData_.enableCloud = true;
    metaData_.autoSyncSwitch = true;
    ASSERT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true), true);
    auto result = service.SetDistributedTables(param, {}, {}, false, DistributedTableType::DISTRIBUTED_CLOUD);
    EXPECT_EQ(result, RDB_OK);
    StoreMetaMapping metaMapping(metaData_);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath()), true);
}

/**
 * @tc.name: SetDistributedTables009
 * @tc.desc: Test SetDistributedTables when type is cloud.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(RdbServiceImplTest, SetDistributedTables009, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    param.type_ = metaData_.storeType;
    param.area_ = metaData_.area;
    param.level_ = metaData_.securityLevel;
    param.isEncrypt_ = metaData_.isEncrypt;
    param.asyncDownloadAsset_ = false;
    param.enableCloud_ = false;
    param.autoSyncSwitch_ = false;
    metaData_.asyncDownloadAsset = false;
    metaData_.enableCloud = false;
    metaData_.autoSyncSwitch = false;
    ASSERT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true), true);
    auto result = service.SetDistributedTables(param, {}, {}, false, DistributedTableType::DISTRIBUTED_CLOUD);
    EXPECT_EQ(result, RDB_OK);
    StoreMetaMapping metaMapping(metaData_);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath()), true);
}

/**
 * @tc.name: SetDistributedTables010
 * @tc.desc: Test SetDistributedTables when type is cloud.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(RdbServiceImplTest, SetDistributedTables010, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    param.type_ = metaData_.storeType;
    param.area_ = metaData_.area;
    param.level_ = metaData_.securityLevel;
    param.isEncrypt_ = metaData_.isEncrypt;
    param.asyncDownloadAsset_ = true;
    param.enableCloud_ = true;
    param.autoSyncSwitch_ = true;
    metaData_.asyncDownloadAsset = true;
    metaData_.enableCloud = false;
    metaData_.autoSyncSwitch = true;
    ASSERT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true), true);
    auto result = service.SetDistributedTables(param, {}, {}, false, DistributedTableType::DISTRIBUTED_CLOUD);
    EXPECT_EQ(result, RDB_OK);
    StoreMetaMapping metaMapping(metaData_);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath()), true);
}

/**
 * @tc.name: SetDistributedTables011
 * @tc.desc: Test SetDistributedTables when type is cloud.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(RdbServiceImplTest, SetDistributedTables011, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    param.type_ = metaData_.storeType;
    param.area_ = metaData_.area;
    param.level_ = metaData_.securityLevel;
    param.isEncrypt_ = metaData_.isEncrypt;
    param.asyncDownloadAsset_ = true;
    param.enableCloud_ = true;
    param.autoSyncSwitch_ = true;
    metaData_.asyncDownloadAsset = true;
    metaData_.enableCloud = true;
    metaData_.autoSyncSwitch = false;
    ASSERT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true), true);
    auto result = service.SetDistributedTables(param, {}, {}, false, DistributedTableType::DISTRIBUTED_CLOUD);
    EXPECT_EQ(result, RDB_OK);
    StoreMetaMapping metaMapping(metaData_);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath()), true);
}

/**
 * @tc.name: SetDistributedTables012
 * @tc.desc: Test SetDistributedTables when type is cloud.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(RdbServiceImplTest, SetDistributedTables012, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    param.type_ = metaData_.storeType;
    param.area_ = metaData_.area;
    param.level_ = metaData_.securityLevel;
    param.isEncrypt_ = metaData_.isEncrypt;
    param.asyncDownloadAsset_ = true;
    param.enableCloud_ = false;
    param.autoSyncSwitch_ = true;
    metaData_.asyncDownloadAsset = true;
    metaData_.enableCloud = false;
    metaData_.autoSyncSwitch = false;
    ASSERT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true), true);
    auto result = service.SetDistributedTables(param, {}, {}, false, DistributedTableType::DISTRIBUTED_CLOUD);
    EXPECT_EQ(result, RDB_OK);
    StoreMetaMapping metaMapping(metaData_);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath()), true);
}

/**
 * @tc.name: SetDistributedTables013
 * @tc.desc: Test SetDistributedTables when type is cloud.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(RdbServiceImplTest, SetDistributedTables013, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    param.type_ = metaData_.storeType;
    param.area_ = metaData_.area;
    param.level_ = metaData_.securityLevel;
    param.isEncrypt_ = metaData_.isEncrypt;
    param.asyncDownloadAsset_ = true;
    param.enableCloud_ = true;
    param.autoSyncSwitch_ = true;
    metaData_.asyncDownloadAsset = false;
    metaData_.enableCloud = false;
    metaData_.autoSyncSwitch = false;
    ASSERT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true), true);
    auto result = service.SetDistributedTables(param, {}, {}, false, DistributedTableType::DISTRIBUTED_CLOUD);
    EXPECT_EQ(result, RDB_OK);
    StoreMetaMapping metaMapping(metaData_);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true), true);
    ASSERT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath()), true);
}

/**
 * @tc.name: Sync001
 * @tc.desc: Test Sync when CheckAccess not pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, Sync001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    RdbService::Option option{};
    PredicatesMemo predicates;

    int32_t result = service.Sync(param, option, predicates, nullptr);
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: Sync002
 * @tc.desc: Test Sync when CheckParam not pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, Sync002, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = TEST_BUNDLE;
    param.storeName_ = TEST_STORE;
    param.hapName_ = "test/test";
    RdbService::Option option{};
    PredicatesMemo predicates;

    int32_t result = service.Sync(param, option, predicates, nullptr);
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: Sync003
 * @tc.desc: Test Sync when mode is nearby begin.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, Sync003, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = TEST_BUNDLE;
    param.storeName_ = "Sync003";
    RdbService::Option option{ DistributedData::GeneralStore::NEARBY_BEGIN };
    PredicatesMemo predicates;
    auto [exists, meta] = RdbServiceImpl::LoadStoreMetaData(param);
    (void)exists;
    RdbServiceImpl::SaveSyncMeta(meta);
    int32_t result = service.Sync(param, option, predicates, nullptr);
    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKeyWithoutPath()), true);
}

/**
 * @tc.name: Sync004
 * @tc.desc: Test Sync when mode is cloud begin.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, Sync004, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = TEST_BUNDLE;
    param.storeName_ = "Sync004";
    RdbService::Option option{ DistributedData::GeneralStore::CLOUD_BEGIN };
    PredicatesMemo predicates;

    int32_t result = service.Sync(param, option, predicates, nullptr);
    EXPECT_EQ(result, RDB_OK);
}

/**
 * @tc.name: QuerySharingResource001
 * @tc.desc: Test QuerySharingResource when CheckParam not pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, QuerySharingResource001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    PredicatesMemo predicates;
    std::vector<std::string> columns;
    auto result = service.QuerySharingResource(param, predicates, columns);
    EXPECT_EQ(result.first, RDB_ERROR);
}

/**
 * @tc.name: BeforeOpen001
 * @tc.desc: Test BeforeOpen when CheckParam not pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, BeforeOpen001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = TEST_BUNDLE;
    param.storeName_ = TEST_STORE;
    param.hapName_ = "test/test";
    int32_t result = service.BeforeOpen(param);
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: BeforeOpen002
 * @tc.desc: Test BeforeOpen when checkacess not pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, BeforeOpen002, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    int32_t result = service.BeforeOpen(param);
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: BeforeOpen003
 * @tc.desc: Test BeforeOpen when checkacess pass and CheckParam pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, BeforeOpen003, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = TEST_BUNDLE;
    param.storeName_ = TEST_STORE;
    int32_t result = service.BeforeOpen(param);
    EXPECT_EQ(result, RDB_NO_META);
}

/**
 * @tc.name: BeforeOpen004
 * @tc.desc: Test BeforeOpen when checkacess pass and CheckParam pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, BeforeOpen004, TestSize.Level0)
{
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true), true);
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = TEST_BUNDLE;
    param.storeName_ = TEST_STORE;
    int32_t result = service.BeforeOpen(param);
    EXPECT_EQ(result, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true), true);
}

/**
 * @tc.name: Subscribe001
 * @tc.desc: Test Subscribe when option mode invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, Subscribe001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    SubscribeOption option{};
    option.mode = SubscribeMode::SUBSCRIBE_MODE_MAX;

    int32_t result = service.Subscribe(param, option, nullptr);
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: Subscribe002
 * @tc.desc: Test Subscribe when option mode CLOUD_SYNC_TRIGGER.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(RdbServiceImplTest, Subscribe002, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    SubscribeOption option{};
    option.mode = SubscribeMode::CLOUD_SYNC_TRIGGER;

    int32_t result = service.Subscribe(param, option, nullptr);
    EXPECT_EQ(result, RDB_OK);
}


/**
 * @tc.name: Subscribe003
 * @tc.desc: Test Subscribe when option mode CLOUD.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(RdbServiceImplTest, Subscribe003, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    SubscribeOption option{};
    option.mode = SubscribeMode::CLOUD;

    int32_t result = service.Subscribe(param, option, nullptr);
    EXPECT_EQ(result, RDB_OK);
}

/**
 * @tc.name: UnSubscribe001
 * @tc.desc: Test UnSubscribe when option mode invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, UnSubscribe001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    SubscribeOption option{};
    option.mode = SubscribeMode::SUBSCRIBE_MODE_MAX;

    int32_t result = service.UnSubscribe(param, option, nullptr);
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: UnSubscribe002
 * @tc.desc: Test UnSubscribe when option mode CLOUD_SYNC_TRIGGER.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(RdbServiceImplTest, UnSubscribe002, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    SubscribeOption option{};
    option.mode = SubscribeMode::CLOUD_SYNC_TRIGGER;

    int32_t result = service.UnSubscribe(param, option, nullptr);
    EXPECT_EQ(result, RDB_OK);
}

/**
 * @tc.name: UnSubscribe003
 * @tc.desc: Test UnSubscribe when option mode CLOUD.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(RdbServiceImplTest, UnSubscribe003, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    SubscribeOption option{};
    option.mode = SubscribeMode::CLOUD;

    int32_t result = service.UnSubscribe(param, option, nullptr);
    EXPECT_EQ(result, RDB_OK);
}

/**
 * @tc.name: GetDfxInfo001
 * @tc.desc: Test GetDfxInfo when CheckAccess not pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, GetDfxInfo001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    DistributedRdb::RdbDfxInfo dfxInfo;

    int32_t result = service.GetDfxInfo(param, dfxInfo);
    EXPECT_EQ(result, RDB_ERROR);
}

/**
 * @tc.name: GetDfxInfo002
 * @tc.desc: Test GetDfxInfo when CheckAccess pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, GetDfxInfo002, TestSize.Level0)
{
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    auto meta = metaData_;
    DistributedData::StoreDfxInfo dfxMeta;
    dfxMeta.lastOpenTime = "test";
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetDfxInfoKey(), dfxMeta, true), true);

    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = meta.bundleName;
    param.storeName_ = meta.storeId;
    param.type_ = meta.storeType;
    param.customDir_ = "../../../base/haps/entry/files/.backup/textautofill";
    DistributedRdb::RdbDfxInfo dfxInfo;
    int32_t result = service.GetDfxInfo(param, dfxInfo);
    EXPECT_EQ(dfxInfo.lastOpenTime_, "test");
    EXPECT_EQ(result, RDB_OK);
    MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(meta.GetDfxInfoKey(), true);
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
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyWithoutPath(), metaData_, false), true);

    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;

    auto result = service.LockCloudContainer(param);

    // Simulate callback execution
    EXPECT_EQ(result.first, RDB_ERROR);
    EXPECT_EQ(result.second, 0);

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
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
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyWithoutPath(), metaData_, false), true);

    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;

    int32_t result = service.UnlockCloudContainer(param);

    // Simulate callback execution
    EXPECT_EQ(result, RDB_ERROR); // Assuming the callback sets status to RDB_OK

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaData_.GetKeyWithoutPath(), false), true);
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
 * @tc.name: GetDebugInfo002
 * @tc.desc: Test GetDebugInfo when CheckAccess pass.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, GetDebugInfo002, TestSize.Level0)
{
    auto meta = metaData_;
    DistributedData::StoreDebugInfo debugMeta;
    DistributedData::StoreDebugInfo::FileInfo fileInfo1;
    fileInfo1.inode = 4;
    fileInfo1.size = 5;
    fileInfo1.dev = 6;
    fileInfo1.mode = 7;
    fileInfo1.uid = 8;
    fileInfo1.gid = 9;
    debugMeta.fileInfos.insert(std::pair{ "test1", fileInfo1 });
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetDebugInfoKey(), debugMeta, true), true);
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
    param.customDir_ = "../../../base/haps/entry/files/.backup/textautofill";
    std::map<std::string, RdbDebugInfo> debugInfo;
    int32_t result = service.GetDebugInfo(param, debugInfo);
    EXPECT_EQ(result, RDB_OK);
    RdbDebugInfo rdbInfo = debugInfo["test1"];
    EXPECT_EQ(rdbInfo.inode_, 4);
    EXPECT_EQ(rdbInfo.size_, 5);
    EXPECT_EQ(rdbInfo.dev_, 6);
    EXPECT_EQ(rdbInfo.mode_, 7);
    EXPECT_EQ(rdbInfo.uid_, 8);
    EXPECT_EQ(rdbInfo.gid_, 9);
    EXPECT_EQ(debugInfo.size(), 1);
    MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true);
    MetaDataManager::GetInstance().DelMeta(meta.GetDebugInfoKey(), true);
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
    localMeta.promiseInfo.tokenIds = { tokenId };
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

/**
 * @tc.name: VerifyPromiseInfo003
 * @tc.desc: Test VerifyPromiseInfo when tokenId and uid are not in promiseInfo.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, VerifyPromiseInfo003, TestSize.Level0)
{
    StoreMetaDataLocal localMeta;
    auto tokenId = IPCSkeleton::GetCallingTokenID();
    localMeta.isAutoSync = true;
    localMeta.promiseInfo.tokenIds = { tokenId };
    localMeta.promiseInfo.uids = {};
    localMeta.promiseInfo.permissionNames = {};
    metaData_.dataDir = "path";
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKeyLocal(), localMeta, true), true);

    StoreMetaMapping storeMetaMapping(metaData_);
    storeMetaMapping.dataDir = DirectoryManager::GetInstance().GetStorePath(metaData_) + "/" + TEST_STORE;

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
 * @tc.name: CheckParam001
 * @tc.desc: Test VerifyPromiseInfo when bundleName_ contain '/'.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, CheckParam001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = "test/test";
    param.hapName_ = "test";
    param.storeName_ = "test";
    param.user_ = "test";
    param.customDir_ = "test";

    bool result = service.IsValidParam(param);

    EXPECT_EQ(result, false);
    param.bundleName_ = "..";

    result = service.IsValidParam(param);

    EXPECT_EQ(result, false);

    param.bundleName_ = "test\\..test";

    result = service.IsValidParam(param);

    EXPECT_EQ(result, false);
}

/**
 * @tc.name: CheckParam002
 * @tc.desc: Test VerifyPromiseInfo when hapName_ contain '/'.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, CheckParam002, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = "test";
    param.hapName_ = "test/test";
    param.storeName_ = "test";
    param.user_ = "test";
    param.customDir_ = "test";

    bool result = service.IsValidParam(param);

    EXPECT_EQ(result, false);
    param.hapName_ = "..";

    result = service.IsValidParam(param);

    EXPECT_EQ(result, false);

    param.hapName_ = "test\\..test";

    result = service.IsValidParam(param);

    EXPECT_EQ(result, false);
}

/**
 * @tc.name: CheckParam003
 * @tc.desc: Test CheckParam when user_ contain '/'.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, CheckParam003, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = "test";
    param.hapName_ = "test";
    param.storeName_ = "test";
    param.user_ = "test/test";
    param.customDir_ = "test";

    bool result = service.IsValidParam(param);

    EXPECT_EQ(result, false);

    param.user_ = "..";

    result = service.IsValidParam(param);

    EXPECT_EQ(result, false);
    param.user_ = "test\\..test";

    result = service.IsValidParam(param);

    EXPECT_EQ(result, false);
}

/**
 * @tc.name: CheckParam004
 * @tc.desc: Test CheckParam.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, CheckParam004, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = "test";
    param.hapName_ = "test";
    param.storeName_ = "test";
    param.user_ = "test";
    param.customDir_ = "test";

    bool result = service.IsValidParam(param);

    EXPECT_EQ(result, true);
}

/**
 * @tc.name: CheckParam005
 * @tc.desc: Test VerifyPromiseInfo when storename contain '/'.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, CheckParam005, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = "test";
    param.hapName_ = "test";
    param.storeName_ = "test/test";
    param.user_ = "test";
    param.customDir_ = "test";

    bool result = service.IsValidParam(param);

    EXPECT_EQ(result, false);
}

/**
 * @tc.name: CheckParam006
 * @tc.desc: Test VerifyPromiseInfo when customDir is invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, CheckParam006, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = "test";
    param.hapName_ = "test";
    param.storeName_ = "test";
    param.user_ = "test";
    param.customDir_ = "test/../../test/../../../";
    bool result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test/../test/../../../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test/../../../test/../../../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test/./../../test/../../../../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test/.../../../test/../../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, true);

    param.customDir_ = "test/test/../../../test/test/../test/../../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, true);

    param.customDir_ = "test/test/../../../../../test/test/test/";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, true);

    param.customDir_ = "/test";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test//////////////////..///////../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, true);

    param.customDir_ = "test/..//////////////////..///////../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test/..//////////////////..///////../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test/..////./././///////////..///////../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test/..////./././//////////////////../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, true);
}

/**
 * @tc.name: CheckParam007
 * @tc.desc: Test VerifyPromiseInfo when customDir is invalid and hapname is empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zd
 */
HWTEST_F(RdbServiceImplTest, CheckParam007, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = "test";
    param.hapName_ = "";
    param.storeName_ = "test";
    param.user_ = "test";
    param.customDir_ = "test/../../test/../../../";
    bool result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test/../test/../../../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test/../../../test/../../../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test/../../../test/../../../../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test/.../../test/../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, true);

    param.customDir_ = "test/test/../../../test/test/../test/../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, true);

    param.customDir_ = "test/test/../../../../../test/test/test/";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, true);

    param.customDir_ = "/test";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test//////////////////..///////../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, true);

    param.customDir_ = "test/..//////////////////..///////../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test/..//////////////////..///////../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test/..////./././///////////..///////../../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, false);

    param.customDir_ = "test/..////./././///////////////////../";
    result = service.IsValidParam(param);
    EXPECT_EQ(result, true);
}

/**
 * @tc.name: Delete_001
 * @tc.desc: Test Delete when param is invalid.
 * @tc.type: FUNC
 */
HWTEST_F(RdbServiceImplTest, Delete_001, TestSize.Level1)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = "";
    auto errCode = service.Delete(param);
    EXPECT_EQ(errCode, RDB_ERROR);
    param.bundleName_ = "bundleName";
    param.storeName_ = "storeName";
    param.user_ = "0";
    errCode = service.Delete(param);
    EXPECT_EQ(errCode, RDB_OK);
}

/**
 * @tc.name: Delete_002
 * @tc.desc: Test Delete_002.
 * @tc.type: FUNC
 */
HWTEST_F(RdbServiceImplTest, Delete_002, TestSize.Level1)
{
    StoreMetaMapping metaMapping;
    metaMapping.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaMapping.user = "0";
    metaMapping.bundleName = "bundleName";
    metaMapping.storeId = "storeName";
    metaMapping.instanceId = 1;

    StoreMetaData meta1;
    meta1.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta1.user = "0";
    meta1.bundleName = "bundleName";
    meta1.storeId = "storeName";
    meta1.dataDir = DirectoryManager::GetInstance().GetStorePath(meta1) + "/" + metaMapping.storeId;
    meta1.instanceId = 1;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta1.GetKey(), meta1, true), true);

    metaMapping.cloudPath = meta1.dataDir;
    metaMapping.devicePath = meta1.dataDir;
    metaMapping.searchPath = meta1.dataDir;
    metaMapping.dataDir = meta1.dataDir;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);

    StoreMetaData meta2;
    meta2.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta2.user = "0";
    meta2.bundleName = "bundleName";
    meta2.storeId = "storeName";
    meta2.dataDir = "path2";
    meta2.instanceId = 1;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta2.GetKey(), meta2, true), true);

    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaMapping.bundleName;
    param.storeName_ = metaMapping.storeId;
    param.user_ = metaMapping.user;
    auto errCode = service.Delete(param);
    EXPECT_EQ(errCode, RDB_OK);
    MetaDataManager::GetInstance().LoadMeta(metaMapping.GetKey(), metaMapping, true);
    EXPECT_EQ(metaMapping.cloudPath, "");
    EXPECT_EQ(metaMapping.devicePath, "");
    EXPECT_EQ(metaMapping.searchPath, "");
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta1.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta2.GetKey(), true), true);
}

/**
 * @tc.name: Delete_003
 * @tc.desc: Test Delete_003.
 * @tc.type: FUNC
 */
HWTEST_F(RdbServiceImplTest, Delete_003, TestSize.Level1)
{
    StoreMetaMapping metaMapping;
    metaMapping.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaMapping.user = "0";
    metaMapping.bundleName = "bundleName";
    metaMapping.storeId = "storeName";

    StoreMetaData meta1;
    meta1.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta1.user = "0";
    meta1.bundleName = "bundleName";
    meta1.storeId = "storeName";
    meta1.dataDir = DirectoryManager::GetInstance().GetStorePath(meta1) + "/" + metaMapping.storeId;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta1.GetKey(), meta1, true), true);

    metaMapping.cloudPath = "path2";
    metaMapping.devicePath = "path2";
    metaMapping.searchPath = "path2";
    metaMapping.dataDir = meta1.dataDir;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);

    StoreMetaData meta2;
    meta2.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta2.user = "0";
    meta2.bundleName = "bundleName";
    meta2.storeId = "storeName";
    meta2.dataDir = "path2";
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta2.GetKey(), meta2, true), true);

    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaMapping.bundleName;
    param.storeName_ = metaMapping.storeId;
    param.user_ = metaMapping.user;
    auto errCode = service.Delete(param);
    EXPECT_EQ(errCode, RDB_OK);
    MetaDataManager::GetInstance().LoadMeta(metaMapping.GetKey(), metaMapping, true);
    EXPECT_EQ(metaMapping.cloudPath, "path2");
    EXPECT_EQ(metaMapping.devicePath, "path2");
    EXPECT_EQ(metaMapping.searchPath, "path2");
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta1.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta2.GetKey(), true), true);
}

/**
 * @tc.name: Delete_004
 * @tc.desc: Test Delete_004.
 * @tc.type: FUNC
 */
HWTEST_F(RdbServiceImplTest, Delete_004, TestSize.Level1)
{
    StoreMetaMapping metaMapping;
    metaMapping.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaMapping.user = "0";
    metaMapping.bundleName = "bundleName";
    metaMapping.storeId = "storeName";
    metaMapping.instanceId = 1;

    StoreMetaData meta1;
    meta1.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta1.user = "0";
    meta1.bundleName = "bundleName";
    meta1.storeId = "storeName";
    meta1.dataDir = "path1";
    meta1.instanceId = 1;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta1.GetKey(), meta1, true), true);

    metaMapping.cloudPath = "path2";
    metaMapping.devicePath = "path2";
    metaMapping.searchPath = "path2";
    metaMapping.dataDir = DirectoryManager::GetInstance().GetStorePath(meta1) + "/" + metaMapping.storeId;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);

    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaMapping.bundleName;
    param.storeName_ = metaMapping.storeId;
    param.user_ = metaMapping.user;
    auto errCode = service.Delete(param);
    EXPECT_EQ(errCode, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta1.GetKey(), true), true);
}

/**
 * @tc.name: RegisterEvent_001
 * @tc.desc: Test RegisterEvent_001.
 * @tc.type: FUNC
 */
HWTEST_F(RdbServiceImplTest, RegisterEvent_001, TestSize.Level1)
{
    RdbServiceImpl service;
    DistributedData::StoreInfo storeInfo;
    storeInfo.bundleName = "RegisterEvent_bundleName";
    storeInfo.storeName = "RegisterEvent_storeName";
    storeInfo.user = 100;
    storeInfo.path = "RegisterEvent_path";
    auto event = std::make_unique<CloudEvent>(CloudEvent::CLOUD_SYNC, storeInfo);
    EXPECT_NE(event, nullptr);
    auto result = EventCenter::GetInstance().PostEvent(move(event));
    EXPECT_EQ(result, 1); // CODE_SYNC
}

/**
 * @tc.name: RegisterEvent_002
 * @tc.desc: Test RegisterEvent_002.
 * @tc.type: FUNC
 */
HWTEST_F(RdbServiceImplTest, RegisterEvent_002, TestSize.Level1)
{
    StoreMetaMapping metaMapping;
    InitMapping(metaMapping);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);

    RdbServiceImpl service;
    DistributedData::StoreInfo storeInfo;
    storeInfo.bundleName = "bundleName";
    storeInfo.storeName = "storeName";
    storeInfo.user = 100;
    storeInfo.path = "path";
    auto event = std::make_unique<CloudEvent>(CloudEvent::CLOUD_SYNC, storeInfo);
    EXPECT_NE(event, nullptr);
    auto result = EventCenter::GetInstance().PostEvent(move(event));
    EXPECT_EQ(result, 1); // CODE_SYNC

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
}

/**
 * @tc.name: RegisterEvent_003
 * @tc.desc: Test RegisterEvent_003.
 * @tc.type: FUNC
 */
HWTEST_F(RdbServiceImplTest, RegisterEvent_003, TestSize.Level1)
{
    StoreMetaMapping metaMapping;
    InitMapping(metaMapping);
    metaMapping.cloudPath = "path1";
    metaMapping.dataDir = "path";
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);

    StoreMetaData meta;
    meta.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    meta.user = "100";
    meta.bundleName = "bundleName";
    meta.storeId = "storeName";
    meta.dataDir = "path1";
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);
    RdbServiceImpl service;
    DistributedData::StoreInfo storeInfo;
    storeInfo.bundleName = "bundleName";
    storeInfo.storeName = "storeName";
    storeInfo.user = 100;
    storeInfo.path = "path";
    auto event = std::make_unique<CloudEvent>(CloudEvent::CLOUD_SYNC, storeInfo);
    EXPECT_NE(event, nullptr);
    auto result = EventCenter::GetInstance().PostEvent(move(event));
    EXPECT_EQ(result, 1); // CODE_SYNC

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: RegisterEvent_004
 * @tc.desc: Test RegisterEvent_005.
 * @tc.type: FUNC
 */
HWTEST_F(RdbServiceImplTest, RegisterEvent_004, TestSize.Level1)
{
    StoreMetaMapping metaMapping;
    InitMapping(metaMapping);
    metaMapping.cloudPath = "path";
    metaMapping.dataDir = "path";
    metaMapping.storeType = StoreMetaData::STORE_KV_BEGIN;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);

    RdbServiceImpl service;
    DistributedData::StoreInfo storeInfo;
    storeInfo.bundleName = "bundleName";
    storeInfo.storeName = "storeName";
    storeInfo.user = 100;
    storeInfo.path = "path";
    auto event = std::make_unique<CloudEvent>(CloudEvent::CLOUD_SYNC, storeInfo);
    EXPECT_NE(event, nullptr);
    auto result = EventCenter::GetInstance().PostEvent(move(event));
    EXPECT_EQ(result, 1); // CODE_SYNC

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
}

/**
 * @tc.name: RegisterEvent_005
 * @tc.desc: Test RegisterEvent_005
 * @tc.type: FUNC
 */
HWTEST_F(RdbServiceImplTest, RegisterEvent_005, TestSize.Level1)
{
    StoreMetaMapping metaMapping;
    InitMapping(metaMapping);
    metaMapping.cloudPath = "path";
    metaMapping.dataDir = "path";
    metaMapping.storeType = StoreMetaData::STORE_OBJECT_BEGIN;
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);

    RdbServiceImpl service;
    DistributedData::StoreInfo storeInfo;
    storeInfo.bundleName = "bundleName";
    storeInfo.storeName = "storeName";
    storeInfo.user = 100;
    storeInfo.path = "path";
    auto event = std::make_unique<CloudEvent>(CloudEvent::CLOUD_SYNC, storeInfo);
    EXPECT_NE(event, nullptr);
    auto result = EventCenter::GetInstance().PostEvent(move(event));
    EXPECT_EQ(result, 1); // CODE_SYNC

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
}

/**
 * @tc.name: RegisterEvent_006
 * @tc.desc: Test RegisterEvent_006
 * @tc.type: FUNC
 */
HWTEST_F(RdbServiceImplTest, RegisterEvent_006, TestSize.Level1)
{
    StoreMetaMapping metaMapping;
    InitMapping(metaMapping);
    metaMapping.cloudPath = "path1";
    metaMapping.dataDir = "path";
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaMapping.GetKey(), metaMapping, true), true);

    RdbServiceImpl service;
    DistributedData::StoreInfo storeInfo;
    storeInfo.bundleName = "bundleName";
    storeInfo.storeName = "storeName";
    storeInfo.user = 100;
    storeInfo.path = "path";
    auto event = std::make_unique<CloudEvent>(CloudEvent::CLOUD_SYNC, storeInfo);
    EXPECT_NE(event, nullptr);
    auto result = EventCenter::GetInstance().PostEvent(move(event));
    EXPECT_EQ(result, 1); // CODE_SYNC

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(metaMapping.GetKey(), true), true);
}

/**
 * @tc.name: QuerySharingResource_PermissionDenied_001
 * @tc.desc: Test QuerySharingResource returns RDB_ERROR when CheckAccess fails.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, QuerySharingResource_PermissionDenied_001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    // param.bundleName_ and param.storeName_ left empty to trigger CheckAccess failure
    PredicatesMemo predicates;
    predicates.tables_ = { "table1" };
    std::vector<std::string> columns = { "col1", "col2" };

    auto result = service.QuerySharingResource(param, predicates, columns);
    EXPECT_EQ(result.first, RDB_ERROR);
    EXPECT_EQ(result.second, nullptr);
}

/**
 * @tc.name: QuerySharingResource_PermissionDenied_002
 * @tc.desc: Test QuerySharingResource returns RDB_ERROR when not system app.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, QuerySharingResource_PermissionDenied_002, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = TEST_BUNDLE;
    param.storeName_ = TEST_STORE;
    PredicatesMemo predicates;
    predicates.tables_ = { "table1" };
    std::vector<std::string> columns = { "col1", "col2" };

    auto result = service.QuerySharingResource(param, predicates, columns);
    EXPECT_EQ(result.first, RDB_ERROR);
    EXPECT_EQ(result.second, nullptr);
}

/**
 * @tc.name: SaveSecretKeyMeta_CloneKeyUpdate_001
 * @tc.desc: Test SaveSecretKeyMeta updates clone secret key when area < 0 or nonce is empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, SaveSecretKeyMeta_CloneKeyUpdate_001, TestSize.Level0)
{
    // Prepare metaData and secret key
    auto meta = metaData_;
    meta.isEncrypt = true;
    std::vector<uint8_t> password = Random(KEY_LENGTH);

    // Prepare cloneKey with area < 0 and empty nonce
    SecretKeyMetaData cloneKey;
    CryptoManager::CryptoParams params;
    cloneKey.sKey = CryptoManager::GetInstance().Encrypt(password, params);
    cloneKey.area = -1;
    cloneKey.nonce.clear();
    cloneKey.storeType = meta.storeType;

    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetCloneSecretKey(), cloneKey, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);

    // Call SaveSecretKeyMeta, should trigger UpdateSecretMeta for cloneKey
    RdbServiceImpl service;
    service.SaveSecretKeyMeta(meta, password);

    // Clean up
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetCloneSecretKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: SaveSecretKeyMeta_CloneKeyUpdate_EmptySKey_002
 * @tc.desc: Test SaveSecretKeyMeta does not update clone secret key if sKey is empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, SaveSecretKeyMeta_CloneKeyUpdate_EmptySKey_002, TestSize.Level0)
{
    auto meta = metaData_;
    meta.isEncrypt = true;
    std::vector<uint8_t> password = Random(KEY_LENGTH);

    // Prepare cloneKey with empty sKey
    SecretKeyMetaData cloneKey;
    cloneKey.sKey.clear();
    cloneKey.area = -1;
    cloneKey.nonce.clear();
    cloneKey.storeType = meta.storeType;

    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetCloneSecretKey(), cloneKey, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);

    RdbServiceImpl service;
    service.SaveSecretKeyMeta(meta, password);

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetCloneSecretKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: SaveSecretKeyMeta_CloneKeyUpdate_NoUpdate_003
 * @tc.desc: Test SaveSecretKeyMeta does not update clone secret key if area >= 0 and nonce not empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: zhaojh
 */
HWTEST_F(RdbServiceImplTest, SaveSecretKeyMeta_CloneKeyUpdate_NoUpdate_003, TestSize.Level0)
{
    auto meta = metaData_;
    meta.isEncrypt = true;
    std::vector<uint8_t> password = Random(KEY_LENGTH);

    // Prepare cloneKey with area >= 0 and nonce not empty
    SecretKeyMetaData cloneKey;
    CryptoManager::CryptoParams params;
    cloneKey.sKey = CryptoManager::GetInstance().Encrypt(password, params);
    cloneKey.area = 1;
    cloneKey.nonce = { 1, 2, 3, 4 };
    cloneKey.storeType = meta.storeType;

    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetCloneSecretKey(), cloneKey, true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(meta.GetKey(), meta, true), true);

    RdbServiceImpl service;
    service.SaveSecretKeyMeta(meta, password);

    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetCloneSecretKey(), true), true);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}

/**
 * @tc.name: PostHeartbeatTask001
 * @tc.desc: Test that tasks does not contain path and delay == 0.
 * @tc.type: FUNC
 * @tc.expect: The taskId is invalid in PostHeartbeatTask
 */
HWTEST_F(RdbServiceImplTest, PostHeartbeatTask001, TestSize.Level0)
{
    int32_t callingPid = 123;
    uint32_t delay = 0;
    DistributedData::StoreInfo storeInfo;
    DataChangeEvent::EventInfo eventInfo;
    storeInfo.path = "/test/path";
    RdbServiceImpl service;
    service.PostHeartbeatTask(callingPid, delay, storeInfo, eventInfo);
    auto it = service.heartbeatTaskIds_.Find(callingPid);
    auto taskId = it.second[storeInfo.path];
    EXPECT_EQ(taskId, ExecutorPool::INVALID_TASK_ID);
}

/**
 * @tc.name: PostHeartbeatTask002
 * @tc.desc: Test that tasks does not contain path and delay != 0.
 * @tc.type: FUNC
 * @tc.expect: The taskId is invalid in PostHeartbeatTask
 */
HWTEST_F(RdbServiceImplTest, PostHeartbeatTask002, TestSize.Level0)
{
    int32_t callingPid = 123;
    uint32_t delay = 1000;
    DistributedData::StoreInfo storeInfo;
    DataChangeEvent::EventInfo eventInfo;
    storeInfo.path = "/test/path";
    RdbServiceImpl service;
    service.executors_ = std::make_shared<ExecutorPool>(2, 0);
    service.PostHeartbeatTask(callingPid, delay, storeInfo, eventInfo);
    auto it = service.heartbeatTaskIds_.Find(callingPid);
    auto taskId = it.second[storeInfo.path];
    EXPECT_NE(taskId, ExecutorPool::INVALID_TASK_ID);
    service.executors_->Remove(taskId);
}

/**
 * @tc.name: PostHeartbeatTask003
 * @tc.desc: Test if the task already exists, delay is not 0.
 * @tc.type: FUNC
 * @tc.expect: The tableProperties value in the global variable changes
 */
HWTEST_F(RdbServiceImplTest, PostHeartbeatTask003, TestSize.Level0)
{
    int32_t callingPid = 456;
    uint32_t delay = 1000;
    DistributedData::StoreInfo storeInfo;
    DataChangeEvent::EventInfo eventInfo;
    eventInfo.isFull = true;
    eventInfo.tableProperties["table1"] = {1, 0};
    storeInfo.path = "/test/path";
    RdbServiceImpl service;
    service.executors_ = std::make_shared<ExecutorPool>(2, 0);
    service.PostHeartbeatTask(callingPid, delay, storeInfo, eventInfo);
    DataChangeEvent::EventInfo eventInfo_again;
    eventInfo_again.isFull = false;
    eventInfo_again.tableProperties["table1"] = {0, 0};
    eventInfo_again.tableProperties["table2"] = {1, 0};

    service.PostHeartbeatTask(callingPid, delay, storeInfo, eventInfo);
    auto globalEvents = service.eventContainer_->events_[storeInfo.path];
    EXPECT_EQ(globalEvents.tableProperties["table1"].isTrackedDataChange, 1);
    EXPECT_EQ(globalEvents.tableProperties["table1"].isP2pSyncDataChange, 0);
    auto it = service.heartbeatTaskIds_.Find(callingPid);
    auto taskId = it.second[storeInfo.path];
    service.executors_->Remove(taskId);
}

/**
 * @tc.name: StealEvent001
 * @tc.desc: Test path is not in events_.
 * @tc.type: FUNC
 * @tc.expect: StealEvent returns nullopt
 */
HWTEST_F(RdbServiceImplTest, StealEvent001, TestSize.Level0)
{
    const std::string testPath = "/test/path";
    DataChangeEvent::EventInfo testEventInfo;
    RdbServiceImpl service;
    auto result = service.eventContainer_->StealEvent(testPath);
    EXPECT_EQ(result, std::nullopt);
}

/**
 * @tc.name: StopCloudSync001
 * @tc.desc: Test StopCloudSync, param invalid.
 * @tc.type: FUNC
 * @tc.expect: StopCloudSync returns RDB_ERROR
 */
HWTEST_F(RdbServiceImplTest, StopCloudSync001, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = TEST_BUNDLE;
    param.storeName_ = TEST_STORE;
    param.hapName_ = "test/test";
    auto errCode = service.StopCloudSync(param);
    EXPECT_EQ(errCode, RDB_ERROR);
}

/**
 * @tc.name: StopCloudSync002
 * @tc.desc: Test StopCloudSync, when CheckAccess fails.
 * @tc.type: FUNC
 * @tc.expect: StopCloudSync returns RDB_ERROR
 */
HWTEST_F(RdbServiceImplTest, StopCloudSync002, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    auto errCode = service.StopCloudSync(param);
    EXPECT_EQ(errCode, RDB_ERROR);
}

/**
 * @tc.name: StopCloudSync003
 * @tc.desc: Test StopCloudSync, param invalid and CheckAccess fails.
 * @tc.type: FUNC
 * @tc.expect: StopCloudSync returns RDB_ERROR
 */
HWTEST_F(RdbServiceImplTest, StopCloudSync003, TestSize.Level0)
{
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.hapName_ = "test/test";
    auto errCode = service.StopCloudSync(param);
    EXPECT_EQ(errCode, RDB_ERROR);
}

/**
 * @tc.name: StopCloudSync004
 * @tc.desc: Test StopCloudSync, when CheckAccess succ.
 * @tc.type: FUNC
 * @tc.expect: StopCloudSync returns RDB_OK
 */
HWTEST_F(RdbServiceImplTest, StopCloudSync004, TestSize.Level0)
{
    EXPECT_EQ(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true), true);
    RdbServiceImpl service;
    RdbSyncerParam param;
    param.bundleName_ = metaData_.bundleName;
    param.storeName_ = metaData_.storeId;
    auto errCode = service.StopCloudSync(param);
    EXPECT_EQ(errCode, RDB_OK);
    EXPECT_EQ(MetaDataManager::GetInstance().DelMeta(meta.GetKey(), true), true);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test
