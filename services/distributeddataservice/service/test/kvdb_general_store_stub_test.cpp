/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "KVDBGeneralStoreStubTest"

#include "bootstrap.h"
#include "cloud/asset_loader.h"
#include "cloud/cloud_db.h"
#include "cloud/schema_meta.h"
#include "crypto_manager.h"
#include "device_manager_adapter.h"
#include "kv_store_nb_delegate_mock.h"
#include "kvdb_query.h"
#include "log_print.h"
#include "kvdb_general_store.h"
#include <gtest/gtest.h>
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "mock/db_store_mock.h"
#include "mock/general_watcher_mock.h"
#include <random>
#include <thread>

using namespace testing::ext;
using namespace DistributedDB;
using namespace OHOS::DistributedData;
using DBStoreMock = OHOS::DistributedData::DBStoreMock;
using DMAdapter = OHOS::DistributedData::DeviceManagerAdapter;
using KVDBGeneralStore = OHOS::DistributedKv::KVDBGeneralStore;
using StoreMetaData = OHOS::DistributedData::StoreMetaData;
using SecurityLevel = OHOS::DistributedKv::SecurityLevel;
namespace OHOS::Test {
namespace DistributedDataTest {
class KVDBGeneralStoreStubTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    static constexpr const char *bundleName = "test_distributeddata";
    static constexpr const char *storeName = "test_service";

    void InitMetaData();
    static std::vector<uint8_t> Random(uint32_t len);
    static std::shared_ptr<DBStoreMock> dbStoreMock;
    StoreMetaData metaData;
};

std::shared_ptr<DBStoreMock> KVDBGeneralStoreStubTest::dbStoreMock = std::make_shared<DBStoreMock>();
static const uint32_t KEY_LENGTH = 32;
static const uint32_t ENCRYPT_KEY_LENGTH = 48;

void KVDBGeneralStoreStubTest::InitMetaData()
{
    metaData.bundleName = bundleName;
    metaData.appId = bundleName;
    metaData.user = "0";
    metaData.area = OHOS::DistributedKv::EL1;
    metaData.instanceId = 0;
    metaData.isAutoSync = true;
    metaData.storeType = DistributedKv::KvStoreType::SINGLE_VERSION;
    metaData.storeId = storeName;
    metaData.dataDir = "/data/service/el1/public/database/" + std::string(bundleName) + "/kvdb";
    metaData.securityLevel = SecurityLevel::S2;
}

std::vector<uint8_t> KVDBGeneralStoreStubTest::Random(uint32_t len)
{
    std::random_device device;
    std::uniform_int_distribution<int> distribution(0, std::numeric_limits<uint8_t>::max());
    std::vector<uint8_t> key(len);
    for (uint32_t i = 0; i < len; i++) {
        key[i] = static_cast<uint8_t>(distribution(device));
    }
    return key;
}

void KVDBGeneralStoreStubTest::SetUpTestCase(void) {}

void KVDBGeneralStoreStubTest::TearDownTestCase() {}

void KVDBGeneralStoreStubTest::SetUp()
{
    Bootstrap::GetInstance().LoadDirectory();
    InitMetaData();
}

void KVDBGeneralStoreStubTest::TearDown() {}

class MockGeneralWatcher : public DistributedData::GeneralWatcher {
public:
    int32_t OnChange(const Origin &origin, const Fields &fields, ChangeData &&datas) override
    {
        return GeneralError::E_OK;
    }

    int32_t OnChange(const Origin &origin, const PRIFields &primaryFields, ChangeInfo &&values) override
    {
        return GeneralError::E_OK;
    }
};

class MockKvStoreChangedData : public DistributedDB::KvStoreChangedData {
public:
    MockKvStoreChangedData() {}
    virtual ~MockKvStoreChangedData() {}
    std::list<DistributedDB::Entry> entriesInserted = {};
    std::list<DistributedDB::Entry> entriesUpdated = {};
    std::list<DistributedDB::Entry> entriesDeleted = {};
    bool isCleared = true;
    const std::list<DistributedDB::Entry> &GetEntriesUpdated() const override
    {
        return entriesUpdated;
    }

    const std::list<DistributedDB::Entry> &GetEntriesInserted() const override
    {
        return entriesInserted;
    }

    bool IsCleared() const override
    {
        return isCleared;
    }

    const std::list<Entry> &GetEntriesDeleted() const override
    {
        return entriesDeleted;
    }
};

/**
* @tc.name: GetDBPasswordTest_001
* @tc.desc: GetDBPassword from meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreStubTest, GetPasswordTest001, TestSize.Level0)
{
    ZLOGI("GetDBPasswordTest start");
    MetaDataManager::GetInstance().Initialize(dbStoreMock, nullptr, "");
    ASSERT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKey(), metaData, true));
    ASSERT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetSecretKey(), metaData, true));
    auto password = KVDBGeneralStore::GetDBPassword(metaData);
    ASSERT_TRUE(password.GetSize() == 0);
}

/**
* @tc.name: GetDBPasswordTest_002
* @tc.desc: GetDBPassword from encrypt meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreStubTest, GetPasswordTest002, TestSize.Level0)
{
    ZLOGI("GetDBPasswordTest002 start");
    MetaDataManager::GetInstance().Initialize(dbStoreMock, nullptr, "");
    metaData.isEncrypt = true;
    ASSERT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetKey(), metaData, true));

    auto errCode = CryptoManager::GetInstance().GenerateRootKey();
    ASSERT_EQ(errCode, CryptoManager::ErrCode::SUCCESS);

    std::vector<uint8_t> randomKey01 = Random(KEY_LENGTH);
    SecretKeyMetaData secretKey;
    secretKey.storeType = metaData.storeType;
    secretKey.sKey = CryptoManager::GetInstance().Encrypt(randomKey01);
    ASSERT_EQ(secretKey.sKey.size(), ENCRYPT_KEY_LENGTH);
    ASSERT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData.GetSecretKey(), secretKey, true));

    auto password = KVDBGeneralStore::GetDBPassword(metaData);
    ASSERT_TRUE(password.GetSize() != 0);
    randomKey01.assign(randomKey01.size(), 0);
}

/**
* @tc.name: GetDBSecurityTest
* @tc.desc: GetDBSecurity
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreStubTest, GetSecurity, TestSize.Level0)
{
    auto dbSecurity01 = KVDBGeneralStore::GetDBSecurity(SecurityLevel::INVALID_LABEL);
    ASSERT_EQ(dbSecurity01.securityLabel, DistributedDB::NOT_SET);
    ASSERT_EQ(dbSecurity01.securityFlag, DistributedDB::ECE);

    dbSecurity01 = KVDBGeneralStore::GetDBSecurity(SecurityLevel::NO_LABEL);
    ASSERT_EQ(dbSecurity01.securityLabel, DistributedDB::NOT_SET);
    ASSERT_EQ(dbSecurity01.securityFlag, DistributedDB::ECE);

    dbSecurity01 = KVDBGeneralStore::GetDBSecurity(SecurityLevel::S0);
    ASSERT_EQ(dbSecurity01.securityLabel, DistributedDB::S0);
    ASSERT_EQ(dbSecurity01.securityFlag, DistributedDB::ECE);

    dbSecurity01 = KVDBGeneralStore::GetDBSecurity(SecurityLevel::S1);
    ASSERT_EQ(dbSecurity01.securityLabel, DistributedDB::S1);
    ASSERT_EQ(dbSecurity01.securityFlag, DistributedDB::ECE);

    dbSecurity01 = KVDBGeneralStore::GetDBSecurity(SecurityLevel::S2);
    ASSERT_EQ(dbSecurity01.securityLabel, DistributedDB::S2);
    ASSERT_EQ(dbSecurity01.securityFlag, DistributedDB::ECE);

    dbSecurity01 = KVDBGeneralStore::GetDBSecurity(SecurityLevel::S3);
    ASSERT_EQ(dbSecurity01.securityLabel, DistributedDB::S3);
    ASSERT_EQ(dbSecurity01.securityFlag, DistributedDB::SECE);

    dbSecurity01 = KVDBGeneralStore::GetDBSecurity(SecurityLevel::S4);
    ASSERT_EQ(dbSecurity01.securityLabel, DistributedDB::S4);
    ASSERT_EQ(dbSecurity01.securityFlag, DistributedDB::ECE);

    auto action = static_cast<int32_t>(SecurityLevel::S4 + 1);
    dbSecurity01 = KVDBGeneralStore::GetDBSecurity(action);
    ASSERT_EQ(dbSecurity01.securityLabel, DistributedDB::NOT_SET);
    ASSERT_EQ(dbSecurity01.securityFlag, DistributedDB::ECE);
}

/**
* @tc.name: GetDBOptionTest
* @tc.desc: GetDBOption from meta and password
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreStubTest, GetOptionTest, TestSize.Level0)
{
    metaData.isEncrypt = true;
    auto password = KVDBGeneralStore::GetDBPassword(metaData);
    auto option = KVDBGeneralStore::GetDBOption(metaData, password);
    ASSERT_EQ(option.syncDualTupleMode, true);
    ASSERT_EQ(option.createIfNecessary, false);
    ASSERT_EQ(option.isMemoryDb, false);
    ASSERT_EQ(option.isEncryptedDb, metaData.isEncrypt);
    ASSERT_EQ(option.isNeedCompressOnSync, metaData.isNeedCompress);
    ASSERT_EQ(option.schema, metaData.schema);
    ASSERT_EQ(option.passwd, password);
    ASSERT_EQ(option.cipher, DistributedDB::CipherType::AES_256_GCM);
    ASSERT_EQ(option.conflictResolvePolicy, DistributedDB::LAST_WIN);
    ASSERT_EQ(option.createDirByStoreIdOnly, true);
    ASSERT_EQ(option.secOption, KVDBGeneralStore::GetDBSecurity(metaData.securityLevel));
}

/**
* @tc.name: CloseTest
* @tc.desc: Close kvdb general store and  test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreStubTest, CloseTest001, TestSize.Level0)
{
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    ASSERT_NE(generalStore, nullptr);
    auto ret = generalStore->Close();
    ASSERT_EQ(ret, GeneralError::E_OK);

    KvStoreNbDelegateMock mockKvDelegate;
    mockKvDelegate.taskCountMock_ = 1;
    generalStore->delegate_ = &mockKvDelegate;
    ASSERT_NE(generalStore->delegate_, nullptr);
    ret = generalStore->Close();
    ASSERT_EQ(ret, GeneralError::E_BUSY);
}

/**
* @tc.name: Close
* @tc.desc: RdbGeneralStore Close test
* @tc.type: FUNC
* @tc.require:
* @tc.author: shaoyuanzhao
*/
HWTEST_F(KVDBGeneralStoreStubTest, BusyClose001, TestSize.Level0)
{
    auto generalStore = std::make_shared<KVDBGeneralStore>(metaData);
    ASSERT_NE(generalStore, nullptr);
    std::thread thread([generalStore]() {
        std::unique_lock<decltype(generalStore->rwMutex_)> lock(generalStore->rwMutex_);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto ret = generalStore->Close();
    ASSERT_EQ(ret, GeneralError::E_BUSY);
    thread.join();
    ret = generalStore->Close();
    ASSERT_EQ(ret, GeneralError::E_OK);
}

/**
* @tc.name: SyncTest
* @tc.desc: Sync.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreStubTest, SyncTest001, TestSize.Level0)
{
    ZLOGI("SyncTest start");
    mkdir(("/data/service/el1/public/database/" + std::string(bundleName)).c_str(),
        (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    ASSERT_NE(generalStore, nullptr);
    uint32_t syncMode11 = GeneralStore::SyncMode::NEARBY_SUBSCRIBE_REMOTE;
    uint32_t highMode11 = GeneralStore::HighMode::MANUAL_SYNC_MODE;
    auto mixMode11 = GeneralStore::MixMode(syncMode11, highMode11);
    std::string kvQuery = "";
    DistributedKv::KVDBQuery query(kvQuery);
    SyncParam syncParam{};
    syncParam.mode = mixMode11;
    auto ret = generalStore->Sync({}, query, [](const GenDetails &result) {}, syncParam);
    ASSERT_NE(ret.first, GeneralError::E_OK);
    auto status = generalStore->Close();
    ASSERT_EQ(status, GeneralError::E_OK);
}

/**
* @tc.name: BindTest
* @tc.desc: Bind test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreStubTest, BindTest001, TestSize.Level0)
{
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    ASSERT_NE(generalStore, nullptr);
    DistributedData::Database database;
    std::map<uint32_t, GeneralStore::BindInfo> bindInfos01;
    GeneralStore::CloudConfig config;
    ASSERT_EQ(bindInfos01.empty(), true);
    auto ret = generalStore->Bind(database, bindInfos01, config);
    ASSERT_EQ(ret, GeneralError::E_OK);

    std::shared_ptr<CloudDB> db;
    std::shared_ptr<AssetLoader> loader;
    GeneralStore::BindInfo bindInfo1(db, loader);
    uint32_t key = 1;
    bindInfos01[key] = bindInfo1;
    ret = generalStore->Bind(database, bindInfos01, config);
    ASSERT_EQ(ret, GeneralError::E_INVALID_ARGS);
    std::shared_ptr<CloudDB> dbs = std::make_shared<CloudDB>();
    std::shared_ptr<AssetLoader> loaders = std::make_shared<AssetLoader>();
    GeneralStore::BindInfo bindInfo2(dbs, loaders);
    bindInfos01[key] = bindInfo2;
    ASSERT_EQ(generalStore->IsBound(key), false);
    ret = store->Bind(database, bindInfos01, config);
    ASSERT_EQ(ret, GeneralError::E_ALREADY_CLOSED);

    generalStore->users_.clear();
    KvStoreNbDelegateMock mockDelegate;
    generalStore->delegate_ = &mockDelegate;
    ASSERT_NE(generalStore->delegate_, nullptr);
    ASSERT_EQ(generalStore->IsBound(key), false);
    ret = generalStore->Bind(database, bindInfos01, config);
    ASSERT_EQ(ret, GeneralError::E_OK);

    ASSERT_EQ(generalStore->IsBound(key), true);
    ret = generalStore->Bind(database, bindInfos01, config);
    ASSERT_EQ(ret, GeneralError::E_OK);
}

/**
* @tc.name: BindWithDifferentUsersTest
* @tc.desc: Bind test the functionality of different users.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreStubTest, DifferentUsersTest, TestSize.Level0)
{
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    KvStoreNbDelegateMock mockDelegate;
    generalStore->delegate_ = &mockDelegate;
    ASSERT_NE(generalStore->delegate_, nullptr);
    ASSERT_NE(generalStore, nullptr);
    DistributedData::Database database;
    std::map<uint32_t, GeneralStore::BindInfo> bindInfos01;
    GeneralStore::CloudConfig config;

    std::shared_ptr<CloudDB> dbs = std::make_shared<CloudDB>();
    std::shared_ptr<AssetLoader> loaders = std::make_shared<AssetLoader>();
    GeneralStore::BindInfo bindInfo(dbs, loaders);
    uint32_t key0 = 100;
    uint32_t key1 = 101;
    bindInfos01[key0] = bindInfo;
    bindInfos01[key1] = bindInfo;
    ASSERT_EQ(generalStore->IsBound(key0), false);
    ASSERT_EQ(generalStore->IsBound(key1), false);
    auto ret = generalStore->Bind(database, bindInfos01, config);
    ASSERT_EQ(ret, GeneralError::E_OK);
    ASSERT_EQ(generalStore->IsBound(key0), true);
    ASSERT_EQ(generalStore->IsBound(key1), true);

    uint32_t key2 = 102;
    bindInfos01[key2] = bindInfo;
    ASSERT_EQ(generalStore->IsBound(key2), false);
    ret = generalStore->Bind(database, bindInfos01, config);
    ASSERT_EQ(ret, GeneralError::E_OK);
    ASSERT_EQ(generalStore->IsBound(key2), true);
}

/**
* @tc.name: GetDBSyncCompleteCB
* @tc.desc: GetDBSyncCompleteCB test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreStubTest, GetDBSyncCompleteTest, TestSize.Level0)
{
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    ASSERT_NE(generalStore, nullptr);
    GeneralStore::DetailAsync async01;
    ASSERT_EQ(async01, nullptr);
    KVDBGeneralStore::DBSyncCallback ret = generalStore->GetDBSyncCompleteCB(async01);
    ASSERT_NE(ret, nullptr);
    auto async01s = [](const GenDetails &result) {};
    ASSERT_NE(async01s, nullptr);
    ret = generalStore->GetDBSyncCompleteCB(async01s);
    ASSERT_NE(ret, nullptr);
}

/**
* @tc.name: CloudSync
* @tc.desc: CloudSync test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreStubTest, CloudSync001, TestSize.Level0)
{
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    ASSERT_NE(generalStore, nullptr);
    generalStore->SetEqualIdentifier(bundleName, storeName);
    KvStoreNbDelegateMock mockDelegate;
    generalStore->delegate_ = &mockDelegate;
    std::vector<std::string> device = { "device1", "device2" };
    auto async01s = [](const GenDetails &result) {};
    generalStore->storeInfo_.user = 0;
    auto cloudSyncMode = DistributedDB::SyncMode::SYNC_MODE_PUSH_ONLY;
    generalStore->SetEqualIdentifier(bundleName, storeName);
    std::string prepareTraceId;
    auto ret = generalStore->CloudSync(device, cloudSyncMode, async01s, 0, prepareTraceId);
    ASSERT_EQ(ret, DBStatus::OK);

    generalStore->storeInfo_.user = 1;
    cloudSyncMode = DistributedDB::SyncMode::SYNC_MODE_CLOUD_FORCE_PUSH;
    ret = generalStore->CloudSync(device, cloudSyncMode, async01s, 0, prepareTraceId);
    ASSERT_EQ(ret, DBStatus::OK);
}

/**
* @tc.name: GetIdentifierParams
* @tc.desc: GetIdentifierParams test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KVDBGeneralStoreStubTest, GetIdentifier, TestSize.Level0)
{
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    std::vector<std::string> sameAccountDevs{};
    std::vector<std::string> uuid{"uuidtest01", "uuidtest02", "uuidtest03"};
    generalStore->GetIdentifierParams(sameAccountDevs, uuid, 0); // NO_ACCOUNT
    for (const auto &devId : uuid) {
        ASSERT_EQ(DMAdapter::GetInstance().IsOHOSType(devId), false);
        ASSERT_EQ(DMAdapter::GetInstance().GetAuthType(devId), 0); // NO_ACCOUNT
    }
    ASSERT_EQ(sameAccountDevs.empty(), false);
}

/**
* @tc.name: Sync
* @tc.desc: Sync test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreStubTest, Sync, TestSize.Level0)
{
    mkdir(("/data/service/el1/public/database/" + std::string(bundleName)).c_str(),
        (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    ASSERT_NE(generalStore, nullptr);
    uint32_t syncMode11 = GeneralStore::SyncMode::CLOUD_BEGIN;
    uint32_t highMode11 = GeneralStore::HighMode::MANUAL_SYNC_MODE;
    auto mixMode11 = GeneralStore::MixMode(syncMode11, highMode11);
    std::string kvQuery = "";
    DistributedKv::KVDBQuery query(kvQuery);
    SyncParam syncParam{};
    syncParam.mode = mixMode11;
    KvStoreNbDelegateMock mockDelegate;
    generalStore->delegate_ = &mockDelegate;
    auto ret = generalStore->Sync({}, query, [](const GenDetails &result) {}, syncParam);
    ASSERT_EQ(ret.first, GeneralError::E_NOT_SUPPORT);
    GeneralStore::StoreConfig storeConfig;
    storeConfig.enableCloud_ = true;
    generalStore->SetConfig(storeConfig);
    ret = generalStore->Sync({}, query, [](const GenDetails &result) {}, syncParam);
    ASSERT_EQ(ret.first, GeneralError::E_OK);

    syncMode11 = GeneralStore::SyncMode::NEARBY_END;
    mixMode11 = GeneralStore::MixMode(syncMode11, highMode11);
    syncParam.mode = mixMode11;
    ret = generalStore->Sync({}, query, [](const GenDetails &result) {}, syncParam);
    ASSERT_EQ(ret.first, GeneralError::E_INVALID_ARGS);

    std::vector<std::string> device = { "device1", "device2" };
    syncMode11 = GeneralStore::SyncMode::NEARBY_SUBSCRIBE_REMOTE;
    mixMode11 = GeneralStore::MixMode(syncMode11, highMode11);
    syncParam.mode = mixMode11;
    ret = generalStore->Sync(device, query, [](const GenDetails &result) {}, syncParam);
    ASSERT_EQ(ret.first, GeneralError::E_OK);

    syncMode11 = GeneralStore::SyncMode::NEARBY_UNSUBSCRIBE_REMOTE;
    mixMode11 = GeneralStore::MixMode(syncMode11, highMode11);
    syncParam.mode = mixMode11;
    ret = generalStore->Sync(device, query, [](const GenDetails &result) {}, syncParam);
    ASSERT_EQ(ret.first, GeneralError::E_OK);

    syncMode11 = GeneralStore::SyncMode::NEARBY_PULL_PUSH;
    mixMode11 = GeneralStore::MixMode(syncMode11, highMode11);
    syncParam.mode = mixMode11;
    ret = generalStore->Sync(device, query, [](const GenDetails &result) {}, syncParam);
    ASSERT_EQ(ret.first, GeneralError::E_OK);

    syncMode11 = GeneralStore::SyncMode::MODE_BUTT;
    mixMode11 = GeneralStore::MixMode(syncMode11, highMode11);
    syncParam.mode = mixMode11;
    ret = generalStore->Sync(device, query, [](const GenDetails &result) {}, syncParam);
    ASSERT_EQ(ret.first, GeneralError::E_INVALID_ARGS);
}

/**
* @tc.name: Clean
* @tc.desc: Clean test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreStubTest, CleanTest, TestSize.Level0)
{
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    ASSERT_NE(generalStore, nullptr);
    std::vector<std::string> device = { "device1", "device2" };
    std::string tableName = "tableName";
    auto ret = generalStore->Clean(device, -1, tableName);
    ASSERT_EQ(ret, GeneralError::E_INVALID_ARGS);
    ret = stogeneralStorere->Clean(device, 5, tableName);
    ASSERT_EQ(ret, GeneralError::E_INVALID_ARGS);
    ret = generalStore->Clean(device, GeneralStore::CleanMode::NEARBY_DATA, tableName);
    ASSERT_EQ(ret, GeneralError::E_ALREADY_CLOSED);

    KvStoreNbDelegateMock mockDelegate;
    generalStore->delegate_ = &mockDelegate;
    ret = generalStore->Clean(device, GeneralStore::CleanMode::CLOUD_INFO, tableName);
    ASSERT_EQ(ret, GeneralError::E_OK);

    ret = v->Clean(device, GeneralStore::CleanMode::CLOUD_DATA, tableName);
    ASSERT_EQ(ret, GeneralError::E_OK);

    ret = generalStore->Clean({}, GeneralStore::CleanMode::NEARBY_DATA, tableName);
    ASSERT_EQ(ret, GeneralError::E_OK);
    ret = generalStore->Clean(device, GeneralStore::CleanMode::NEARBY_DATA, tableName);
    ASSERT_EQ(ret, GeneralError::E_OK);

    ret = generalStore->Clean(device, GeneralStore::CleanMode::LOCAL_DATA, tableName);
    ASSERT_EQ(ret, GeneralError::E_ERROR);
}

/**
* @tc.name: Watch
* @tc.desc: Watch and Unwatch test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreStubTest, WatchTest, TestSize.Level0)
{
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    ASSERT_NE(generalStore, nullptr);
    DistributedDataTest::MockGeneralWatcher generalWatcher;
    auto ret = generalStore->Watch(GeneralWatcher::Origin::ORIGIN_CLOUD, generalWatcher);
    ASSERT_EQ(ret, GeneralError::E_INVALID_ARGS);
    ret = generalStore->Unwatch(GeneralWatcher::Origin::ORIGIN_CLOUD, generalWatcher);
    ASSERT_EQ(ret, GeneralError::E_INVALID_ARGS);

    ret = generalStore->Watch(GeneralWatcher::Origin::ORIGIN_ALL, generalWatcher);
    ASSERT_EQ(ret, GeneralError::E_OK);
    ret = generalStore->Watch(GeneralWatcher::Origin::ORIGIN_ALL, generalWatcher);
    ASSERT_EQ(ret, GeneralError::E_INVALID_ARGS);

    ret = generalStore->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, generalWatcher);
    ASSERT_EQ(ret, GeneralError::E_OK);
    ret = generalStore->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, generalWatcher);
    ASSERT_EQ(ret, GeneralError::E_INVALID_ARGS);
}

/**
* @tc.name: Release
* @tc.desc: Release and AddRef test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreStubTest, ReleaseTest, TestSize.Level0)
{
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    ASSERT_NE(generalStore, nullptr);
    auto ret = generalStore->Release();
    ASSERT_EQ(ret, 0);
    generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    generalStore->ref_ = 0;
    ret = generalStore->Release();
    ASSERT_EQ(ret, 0);
    generalStore->ref_ = 2;
    ret = storgeneralStoree->Release();
    ASSERT_EQ(ret, 1);

    ret = generalStore->AddRef();
    ASSERT_EQ(ret, 2);
    generalStore->ref_ = 0;
    ret = generalStore->AddRef();
    ASSERT_EQ(ret, 0);
}

/**
* @tc.name: ConvertStatus
* @tc.desc: ConvertStatus test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreStubTest, ConvertStatusTest, TestSize.Level0)
{
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    ASSERT_NE(generalStore, nullptr);
    auto ret = generalStore->ConvertStatus(DBStatus::OK);
    ASSERT_EQ(ret, GeneralError::E_OK);
    ret = ->ConvertStatus(DBStatus::CLOUD_NETWORK_ERROR);
    ASSERT_EQ(ret, GeneralError::E_NETWORK_ERROR);
    ret = generalStore->ConvertStatus(DBStatus::CLOUD_LOCK_ERROR);
    ASSERT_EQ(ret, GeneralError::E_LOCKED_BY_OTHERS);
    ret = generalStore->ConvertStatus(DBStatus::CLOUD_FULL_RECORDS);
    ASSERT_EQ(ret, GeneralError::E_RECODE_LIMIT_EXCEEDED);
    ret = generalStore->ConvertStatus(DBStatus::CLOUD_ASSET_SPACE_INSUFFICIENT);
    ASSERT_EQ(ret, GeneralError::E_NO_SPACE_FOR_ASSET);
    ret = generalStore->ConvertStatus(DBStatus::CLOUD_SYNC_TASK_MERGED);
    ASSERT_EQ(ret, GeneralError::E_SYNC_TASK_MERGED);
    ret = generalStore->ConvertStatus(DBStatus::DB_ERROR);
    ASSERT_EQ(ret, GeneralError::E_DB_ERROR);
}

/**
* @tc.name: OnChange
* @tc.desc: OnChange test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreStubTest, OnChangeTest, TestSize.Level0)
{
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    ASSERT_NE(generalStore, nullptr);
    DistributedDataTest::MockGeneralWatcher generalWatcher;
    DistributedDataTest::MockKvStoreChangedData data;
    DistributedDB::ChangedData changedData;
    generalStore->observer_.OnChange(data);
    generalStore->observer_.OnChange(DistributedDB::Origin::ORIGIN_CLOUD, "originalId", std::move(changedData));
    auto result = generalStore->Watch(GeneralWatcher::Origin::ORIGIN_ALL, generalWatcher);
    ASSERT_EQ(result, GeneralError::E_OK);
    generalStore->observer_.OnChange(data);
    generalStore->observer_.OnChange(DistributedDB::Origin::ORIGIN_CLOUD, "originalId", std::move(changedData));
    result = generalStore->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, generalWatcher);
    ASSERT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Delete
* @tc.desc: Delete test the function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreStubTest, DeleteTest, TestSize.Level0)
{
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    ASSERT_NE(generalStore, nullptr);
    KvStoreNbDelegateMock mockDelegate;
    generalStore->delegate_ = &mockDelegate;
    generalStore->SetDBPushDataInterceptor(DistributedKv::KvStoreType::DEVICE_COLLABORATION);
    generalStore->SetDBReceiveDataInterceptor(DistributedKv::KvStoreType::DEVICE_COLLABORATION);
    DistributedData::VBuckets values;
    auto ret = generalStore->Insert("table", std::move(values));
    ASSERT_EQ(ret, GeneralError::E_NOT_SUPPORT);

    DistributedData::Values args;
    generalStore->SetDBPushDataInterceptor(DistributedKv::KvStoreType::SINGLE_VERSION);
    generalStore->SetDBReceiveDataInterceptor(DistributedKv::KvStoreType::SINGLE_VERSION);
    ret = generalStore->Delete("table", "sql", std::move(args));
    ASSERT_EQ(ret, GeneralError::E_NOT_SUPPORT);
}

/**
* @tc.name: Query001
* @tc.desc: KVDBGeneralStoreStubTest Query function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreStubTest, Query001Test, TestSize.Level1)
{
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    ASSERT_NE(generalStore, nullptr);
    std::string table = "table";
    std::string sql = "sql";
    auto [errCode, result] = generalStore->Query(table, sql, {});
    ASSERT_EQ(errCode, GeneralError::E_NOT_SUPPORT);
    ASSERT_EQ(result, nullptr);
}

/**
* @tc.name: Query002
* @tc.desc: KVDBGeneralStoreStubTest Query function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreStubTest, Query002Test, TestSize.Level1)
{
    auto generalStore = new (std::nothrow) KVDBGeneralStore(metaData);
    ASSERT_NE(generalStore, nullptr);
    std::string table = "table";
    MockQuery query;
    auto [errCode, result] = generalStore->Query(table, query);
    ASSERT_EQ(errCode, GeneralError::E_NOT_SUPPORT);
    ASSERT_EQ(result, nullptr);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test