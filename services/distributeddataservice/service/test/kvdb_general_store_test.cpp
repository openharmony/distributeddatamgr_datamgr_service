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
#define LOG_TAG "KVDBGeneralStoreTest"

#include "kvdb_general_store.h"

#include <gtest/gtest.h>
#include <random>
#include <thread>

#include "bootstrap.h"
#include "cloud/asset_loader.h"
#include "cloud/cloud_db.h"
#include "cloud/schema_meta.h"
#include "crypto_manager.h"
#include "device_manager_adapter.h"
#include "kv_store_nb_delegate_mock.h"
#include "kvdb_query.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "mock/db_store_mock.h"
#include "mock/general_watcher_mock.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace OHOS::DistributedData;
using DBStoreMock = OHOS::DistributedData::DBStoreMock;
using StoreMetaData = OHOS::DistributedData::StoreMetaData;
using SecurityLevel = OHOS::DistributedKv::SecurityLevel;
using KVDBGeneralStore = OHOS::DistributedKv::KVDBGeneralStore;
using DMAdapter = OHOS::DistributedData::DeviceManagerAdapter;
namespace OHOS::Test {
namespace DistributedDataTest {
class KVDBGeneralStoreTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    static constexpr const char *bundleName = "test_distributeddata";
    static constexpr const char *storeName = "test_service_meta";

    void InitMetaData();
    static std::vector<uint8_t> Random(uint32_t len);
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    StoreMetaData metaData_;
};

std::shared_ptr<DBStoreMock> KVDBGeneralStoreTest::dbStoreMock_ = std::make_shared<DBStoreMock>();
static const uint32_t KEY_LENGTH = 32;
static const uint32_t ENCRYPT_KEY_LENGTH = 48;

void KVDBGeneralStoreTest::InitMetaData()
{
    metaData_.bundleName = bundleName;
    metaData_.appId = bundleName;
    metaData_.user = "0";
    metaData_.area = OHOS::DistributedKv::EL1;
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = DistributedKv::KvStoreType::SINGLE_VERSION;
    metaData_.storeId = storeName;
    metaData_.dataDir = "/data/service/el1/public/database/" + std::string(bundleName) + "/kvdb";
    metaData_.securityLevel = SecurityLevel::S2;
}

std::vector<uint8_t> KVDBGeneralStoreTest::Random(uint32_t len)
{
    std::random_device randomDevice;
    std::uniform_int_distribution<int> distribution(0, std::numeric_limits<uint8_t>::max());
    std::vector<uint8_t> key(len);
    for (uint32_t i = 0; i < len; i++) {
        key[i] = static_cast<uint8_t>(distribution(randomDevice));
    }
    return key;
}

void KVDBGeneralStoreTest::SetUpTestCase(void) {}

void KVDBGeneralStoreTest::TearDownTestCase() {}

void KVDBGeneralStoreTest::SetUp()
{
    Bootstrap::GetInstance().LoadDirectory();
    InitMetaData();
}

void KVDBGeneralStoreTest::TearDown() {}

class MockGeneralWatcher : public DistributedData::GeneralWatcher {
public:
    int32_t OnChange(const Origin &origin, const PRIFields &primaryFields, ChangeInfo &&values) override
    {
        return GeneralError::E_OK;
    }

    int32_t OnChange(const Origin &origin, const Fields &fields, ChangeData &&datas) override
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
    const std::list<DistributedDB::Entry> &GetEntriesInserted() const override
    {
        return entriesInserted;
    }

    const std::list<DistributedDB::Entry> &GetEntriesUpdated() const override
    {
        return entriesUpdated;
    }

    const std::list<Entry> &GetEntriesDeleted() const override
    {
        return entriesDeleted;
    }

    bool IsCleared() const override
    {
        return isCleared;
    }
};

/**
* @tc.name: GetDBPasswordTest_001
* @tc.desc: GetDBPassword from meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreTest, GetDBPasswordTest_001, TestSize.Level0)
{
    ZLOGI("GetDBPasswordTest start");
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true));
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData_.GetSecretKey(), metaData_, true));
    auto dbPassword = KVDBGeneralStore::GetDBPassword(metaData_);
    ASSERT_TRUE(dbPassword.GetSize() == 0);
}

/**
* @tc.name: GetDBPasswordTest_002
* @tc.desc: GetDBPassword from encrypt meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreTest, GetDBPasswordTest_002, TestSize.Level0)
{
    ZLOGI("GetDBPasswordTest_002 start");
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
    metaData_.isEncrypt = true;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true));

    auto errCode = CryptoManager::GetInstance().GenerateRootKey();
    EXPECT_EQ(errCode, CryptoManager::ErrCode::SUCCESS);

    std::vector<uint8_t> randomKey = Random(KEY_LENGTH);
    SecretKeyMetaData secretKey;
    secretKey.storeType = metaData_.storeType;
    secretKey.sKey = CryptoManager::GetInstance().Encrypt(
        randomKey, DEFAULT_ENCRYPTION_LEVEL, DEFAULT_USER,
        CryptoManager::GetInstance().vecRootKeyAlias_,
        CryptoManager::GetInstance().vecNonce_);
    EXPECT_EQ(secretKey.sKey.size(), ENCRYPT_KEY_LENGTH);
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData_.GetSecretKey(), secretKey, true));

    auto dbPassword = KVDBGeneralStore::GetDBPassword(metaData_);
    ASSERT_TRUE(dbPassword.GetSize() != 0);
    randomKey.assign(randomKey.size(), 0);
}

/**
* @tc.name: GetDBSecurityTest
* @tc.desc: GetDBSecurity
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreTest, GetDBSecurityTest, TestSize.Level0)
{
    auto dbSecurity = KVDBGeneralStore::GetDBSecurity(SecurityLevel::INVALID_LABEL);
    EXPECT_EQ(dbSecurity.securityLabel, DistributedDB::NOT_SET);
    EXPECT_EQ(dbSecurity.securityFlag, DistributedDB::ECE);

    dbSecurity = KVDBGeneralStore::GetDBSecurity(SecurityLevel::NO_LABEL);
    EXPECT_EQ(dbSecurity.securityLabel, DistributedDB::NOT_SET);
    EXPECT_EQ(dbSecurity.securityFlag, DistributedDB::ECE);

    dbSecurity = KVDBGeneralStore::GetDBSecurity(SecurityLevel::S0);
    EXPECT_EQ(dbSecurity.securityLabel, DistributedDB::S0);
    EXPECT_EQ(dbSecurity.securityFlag, DistributedDB::ECE);

    dbSecurity = KVDBGeneralStore::GetDBSecurity(SecurityLevel::S1);
    EXPECT_EQ(dbSecurity.securityLabel, DistributedDB::S1);
    EXPECT_EQ(dbSecurity.securityFlag, DistributedDB::ECE);

    dbSecurity = KVDBGeneralStore::GetDBSecurity(SecurityLevel::S2);
    EXPECT_EQ(dbSecurity.securityLabel, DistributedDB::S2);
    EXPECT_EQ(dbSecurity.securityFlag, DistributedDB::ECE);

    dbSecurity = KVDBGeneralStore::GetDBSecurity(SecurityLevel::S3);
    EXPECT_EQ(dbSecurity.securityLabel, DistributedDB::S3);
    EXPECT_EQ(dbSecurity.securityFlag, DistributedDB::SECE);

    dbSecurity = KVDBGeneralStore::GetDBSecurity(SecurityLevel::S4);
    EXPECT_EQ(dbSecurity.securityLabel, DistributedDB::S4);
    EXPECT_EQ(dbSecurity.securityFlag, DistributedDB::ECE);

    auto action = static_cast<int32_t>(SecurityLevel::S4 + 1);
    dbSecurity = KVDBGeneralStore::GetDBSecurity(action);
    EXPECT_EQ(dbSecurity.securityLabel, DistributedDB::NOT_SET);
    EXPECT_EQ(dbSecurity.securityFlag, DistributedDB::ECE);
}

/**
* @tc.name: GetDBOptionTest
* @tc.desc: GetDBOption from meta and dbPassword
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreTest, GetDBOptionTest, TestSize.Level0)
{
    metaData_.isEncrypt = true;
    auto dbPassword = KVDBGeneralStore::GetDBPassword(metaData_);
    auto dbOption = KVDBGeneralStore::GetDBOption(metaData_, dbPassword);
    EXPECT_EQ(dbOption.syncDualTupleMode, true);
    EXPECT_EQ(dbOption.createIfNecessary, false);
    EXPECT_EQ(dbOption.isMemoryDb, false);
    EXPECT_EQ(dbOption.isEncryptedDb, metaData_.isEncrypt);
    EXPECT_EQ(dbOption.isNeedCompressOnSync, metaData_.isNeedCompress);
    EXPECT_EQ(dbOption.schema, metaData_.schema);
    EXPECT_EQ(dbOption.passwd, dbPassword);
    EXPECT_EQ(dbOption.cipher, DistributedDB::CipherType::AES_256_GCM);
    EXPECT_EQ(dbOption.conflictResolvePolicy, DistributedDB::LAST_WIN);
    EXPECT_EQ(dbOption.createDirByStoreIdOnly, true);
    EXPECT_EQ(dbOption.secOption, KVDBGeneralStore::GetDBSecurity(metaData_.securityLevel));
}

/**
* @tc.name: CloseTest
* @tc.desc: Close kvdb general store and  test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreTest, CloseTest, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    auto ret = store->Close();
    EXPECT_EQ(ret, GeneralError::E_OK);

    KvStoreNbDelegateMock mockDelegate;
    mockDelegate.taskCountMock_ = 1;
    store->delegate_ = &mockDelegate;
    EXPECT_NE(store->delegate_, nullptr);
    ret = store->Close();
    EXPECT_EQ(ret, GeneralError::E_BUSY);
}

/**
* @tc.name: Close
* @tc.desc: RdbGeneralStore Close test
* @tc.type: FUNC
* @tc.require:
* @tc.author: shaoyuanzhao
*/
HWTEST_F(KVDBGeneralStoreTest, BusyClose, TestSize.Level0)
{
    auto store = std::make_shared<KVDBGeneralStore>(metaData_);
    ASSERT_NE(store, nullptr);
    std::thread thread([store]() {
        std::unique_lock<decltype(store->rwMutex_)> lock(store->rwMutex_);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    auto ret = store->Close();
    EXPECT_EQ(ret, GeneralError::E_BUSY);
    thread.join();
    ret = store->Close();
    EXPECT_EQ(ret, GeneralError::E_OK);
}

/**
* @tc.name: SyncTest
* @tc.desc: Sync.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreTest, SyncTest, TestSize.Level0)
{
    ZLOGI("SyncTest start");
    mkdir(("/data/service/el1/public/database/" + std::string(bundleName)).c_str(),
        (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    uint32_t syncMode = GeneralStore::SyncMode::NEARBY_SUBSCRIBE_REMOTE;
    uint32_t highMode = GeneralStore::HighMode::MANUAL_SYNC_MODE;
    auto mixMode = GeneralStore::MixMode(syncMode, highMode);
    std::string kvQuery = "";
    DistributedKv::KVDBQuery query(kvQuery);
    SyncParam syncParam{};
    syncParam.mode = mixMode;
    auto ret = store->Sync({}, query, [](const GenDetails &result) {}, syncParam);
    EXPECT_NE(ret.first, GeneralError::E_OK);
    auto status = store->Close();
    EXPECT_EQ(status, GeneralError::E_OK);
}

/**
* @tc.name: BindTest
* @tc.desc: Bind test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreTest, BindTest, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    DistributedData::Database database;
    std::map<uint32_t, GeneralStore::BindInfo> bindInfos;
    GeneralStore::CloudConfig config;
    EXPECT_EQ(bindInfos.empty(), true);
    auto ret = store->Bind(database, bindInfos, config);
    EXPECT_EQ(ret, GeneralError::E_OK);

    std::shared_ptr<CloudDB> db;
    std::shared_ptr<AssetLoader> loader;
    GeneralStore::BindInfo bindInfo1(db, loader);
    uint32_t key = 1;
    bindInfos[key] = bindInfo1;
    ret = store->Bind(database, bindInfos, config);
    EXPECT_EQ(ret, GeneralError::E_INVALID_ARGS);
    std::shared_ptr<CloudDB> dbs = std::make_shared<CloudDB>();
    std::shared_ptr<AssetLoader> loaders = std::make_shared<AssetLoader>();
    GeneralStore::BindInfo bindInfo2(dbs, loaders);
    bindInfos[key] = bindInfo2;
    EXPECT_EQ(store->IsBound(key), false);
    ret = store->Bind(database, bindInfos, config);
    EXPECT_EQ(ret, GeneralError::E_ALREADY_CLOSED);

    store->bindInfos_.clear();
    KvStoreNbDelegateMock mockDelegate;
    store->delegate_ = &mockDelegate;
    EXPECT_NE(store->delegate_, nullptr);
    EXPECT_EQ(store->IsBound(key), false);
    ret = store->Bind(database, bindInfos, config);
    EXPECT_EQ(ret, GeneralError::E_OK);

    EXPECT_EQ(store->IsBound(key), true);
    ret = store->Bind(database, bindInfos, config);
    EXPECT_EQ(ret, GeneralError::E_OK);
}

/**
* @tc.name: BindWithDifferentUsersTest
* @tc.desc: Bind test the functionality of different users.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreTest, BindWithDifferentUsersTest, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    KvStoreNbDelegateMock mockDelegate;
    store->delegate_ = &mockDelegate;
    EXPECT_NE(store->delegate_, nullptr);
    ASSERT_NE(store, nullptr);
    DistributedData::Database database;
    std::map<uint32_t, GeneralStore::BindInfo> bindInfos;
    GeneralStore::CloudConfig config;

    std::shared_ptr<CloudDB> dbs = std::make_shared<CloudDB>();
    std::shared_ptr<AssetLoader> loaders = std::make_shared<AssetLoader>();
    GeneralStore::BindInfo bindInfo(dbs, loaders);
    uint32_t key0 = 100;
    uint32_t key1 = 101;
    bindInfos[key0] = bindInfo;
    bindInfos[key1] = bindInfo;
    EXPECT_EQ(store->IsBound(key0), false);
    EXPECT_EQ(store->IsBound(key1), false);
    auto ret = store->Bind(database, bindInfos, config);
    EXPECT_EQ(ret, GeneralError::E_OK);
    EXPECT_EQ(store->IsBound(key0), true);
    EXPECT_EQ(store->IsBound(key1), true);

    uint32_t key2 = 102;
    bindInfos[key2] = bindInfo;
    EXPECT_EQ(store->IsBound(key2), false);
    ret = store->Bind(database, bindInfos, config);
    EXPECT_EQ(ret, GeneralError::E_OK);
    EXPECT_EQ(store->IsBound(key2), true);
}

/**
* @tc.name: GetDBSyncCompleteCB
* @tc.desc: GetDBSyncCompleteCB test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreTest, GetDBSyncCompleteCB, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    GeneralStore::DetailAsync async;
    EXPECT_EQ(async, nullptr);
    KVDBGeneralStore::DBSyncCallback ret = store->GetDBSyncCompleteCB(async);
    EXPECT_NE(ret, nullptr);
    auto asyncs = [](const GenDetails &result) {};
    EXPECT_NE(asyncs, nullptr);
    ret = store->GetDBSyncCompleteCB(asyncs);
    EXPECT_NE(ret, nullptr);
}

/**
* @tc.name: CloudSync
* @tc.desc: CloudSync test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreTest, CloudSync, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    store->SetEqualIdentifier(bundleName, storeName);
    KvStoreNbDelegateMock mockDelegate;
    store->delegate_ = &mockDelegate;
    std::vector<std::string> devices = { "device1", "device2" };
    auto asyncs = [](const GenDetails &result) {};
    store->storeInfo_.user = 0;
    auto cloudSyncMode = DistributedDB::SyncMode::SYNC_MODE_PUSH_ONLY;
    store->SetEqualIdentifier(bundleName, storeName);
    std::string prepareTraceId;
    auto ret = store->CloudSync(devices, cloudSyncMode, asyncs, 0, prepareTraceId);
    EXPECT_EQ(ret, DBStatus::OK);

    store->storeInfo_.user = 1;
    cloudSyncMode = DistributedDB::SyncMode::SYNC_MODE_CLOUD_FORCE_PUSH;
    ret = store->CloudSync(devices, cloudSyncMode, asyncs, 0, prepareTraceId);
    EXPECT_EQ(ret, DBStatus::OK);
}

/**
* @tc.name: GetIdentifierParams
* @tc.desc: GetIdentifierParams test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KVDBGeneralStoreTest, GetIdentifierParams, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    std::vector<std::string> sameAccountDevs{};
    std::vector<std::string> uuids{"uuidtest01", "uuidtest02", "uuidtest03"};
    store->GetIdentifierParams(sameAccountDevs, uuids, 0); // NO_ACCOUNT
    for (const auto &devId : uuids) {
        EXPECT_EQ(DMAdapter::GetInstance().IsOHOSType(devId), false);
        EXPECT_EQ(DMAdapter::GetInstance().GetAuthType(devId), 0); // NO_ACCOUNT
    }
    EXPECT_EQ(sameAccountDevs.empty(), false);
}

/**
* @tc.name: Sync
* @tc.desc: Sync test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreTest, Sync, TestSize.Level0)
{
    mkdir(("/data/service/el1/public/database/" + std::string(bundleName)).c_str(),
        (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    uint32_t syncMode = GeneralStore::SyncMode::CLOUD_BEGIN;
    uint32_t highMode = GeneralStore::HighMode::MANUAL_SYNC_MODE;
    auto mixMode = GeneralStore::MixMode(syncMode, highMode);
    std::string kvQuery = "";
    DistributedKv::KVDBQuery query(kvQuery);
    SyncParam syncParam{};
    syncParam.mode = mixMode;
    KvStoreNbDelegateMock mockDelegate;
    store->delegate_ = &mockDelegate;
    auto ret = store->Sync({}, query, [](const GenDetails &result) {}, syncParam);
    EXPECT_EQ(ret.first, GeneralError::E_NOT_SUPPORT);
    GeneralStore::StoreConfig storeConfig;
    storeConfig.enableCloud_ = true;
    store->SetConfig(storeConfig);
    ret = store->Sync({}, query, [](const GenDetails &result) {}, syncParam);
    EXPECT_EQ(ret.first, GeneralError::E_OK);

    syncMode = GeneralStore::SyncMode::NEARBY_END;
    mixMode = GeneralStore::MixMode(syncMode, highMode);
    syncParam.mode = mixMode;
    ret = store->Sync({}, query, [](const GenDetails &result) {}, syncParam);
    EXPECT_EQ(ret.first, GeneralError::E_INVALID_ARGS);

    std::vector<std::string> devices = { "device1", "device2" };
    syncMode = GeneralStore::SyncMode::NEARBY_SUBSCRIBE_REMOTE;
    mixMode = GeneralStore::MixMode(syncMode, highMode);
    syncParam.mode = mixMode;
    ret = store->Sync(devices, query, [](const GenDetails &result) {}, syncParam);
    EXPECT_EQ(ret.first, GeneralError::E_OK);

    syncMode = GeneralStore::SyncMode::NEARBY_UNSUBSCRIBE_REMOTE;
    mixMode = GeneralStore::MixMode(syncMode, highMode);
    syncParam.mode = mixMode;
    ret = store->Sync(devices, query, [](const GenDetails &result) {}, syncParam);
    EXPECT_EQ(ret.first, GeneralError::E_OK);

    syncMode = GeneralStore::SyncMode::NEARBY_PULL_PUSH;
    mixMode = GeneralStore::MixMode(syncMode, highMode);
    syncParam.mode = mixMode;
    ret = store->Sync(devices, query, [](const GenDetails &result) {}, syncParam);
    EXPECT_EQ(ret.first, GeneralError::E_OK);

    syncMode = GeneralStore::SyncMode::MODE_BUTT;
    mixMode = GeneralStore::MixMode(syncMode, highMode);
    syncParam.mode = mixMode;
    ret = store->Sync(devices, query, [](const GenDetails &result) {}, syncParam);
    EXPECT_EQ(ret.first, GeneralError::E_INVALID_ARGS);
}

/**
* @tc.name: Clean
* @tc.desc: Clean test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreTest, Clean, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::vector<std::string> devices = { "device1", "device2" };
    std::string tableName = "tableName";
    auto ret = store->Clean(devices, -1, tableName);
    EXPECT_EQ(ret, GeneralError::E_INVALID_ARGS);
    ret = store->Clean(devices, 5, tableName);
    EXPECT_EQ(ret, GeneralError::E_INVALID_ARGS);
    ret = store->Clean(devices, GeneralStore::CleanMode::NEARBY_DATA, tableName);
    EXPECT_EQ(ret, GeneralError::E_ALREADY_CLOSED);

    KvStoreNbDelegateMock mockDelegate;
    store->delegate_ = &mockDelegate;
    ret = store->Clean(devices, GeneralStore::CleanMode::CLOUD_INFO, tableName);
    EXPECT_EQ(ret, GeneralError::E_OK);

    ret = store->Clean(devices, GeneralStore::CleanMode::CLOUD_DATA, tableName);
    EXPECT_EQ(ret, GeneralError::E_OK);

    ret = store->Clean({}, GeneralStore::CleanMode::NEARBY_DATA, tableName);
    EXPECT_EQ(ret, GeneralError::E_OK);
    ret = store->Clean(devices, GeneralStore::CleanMode::NEARBY_DATA, tableName);
    EXPECT_EQ(ret, GeneralError::E_OK);

    ret = store->Clean(devices, GeneralStore::CleanMode::LOCAL_DATA, tableName);
    EXPECT_EQ(ret, GeneralError::E_ERROR);
}

/**
* @tc.name: Watch
* @tc.desc: Watch and Unwatch test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreTest, Watch, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    DistributedDataTest::MockGeneralWatcher watcher;
    auto ret = store->Watch(GeneralWatcher::Origin::ORIGIN_CLOUD, watcher);
    EXPECT_EQ(ret, GeneralError::E_INVALID_ARGS);
    ret = store->Unwatch(GeneralWatcher::Origin::ORIGIN_CLOUD, watcher);
    EXPECT_EQ(ret, GeneralError::E_INVALID_ARGS);

    ret = store->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(ret, GeneralError::E_OK);
    ret = store->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(ret, GeneralError::E_INVALID_ARGS);

    ret = store->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(ret, GeneralError::E_OK);
    ret = store->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(ret, GeneralError::E_INVALID_ARGS);
}

/**
* @tc.name: Release
* @tc.desc: Release and AddRef test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreTest, Release, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    auto ret = store->Release();
    EXPECT_EQ(ret, 0);
    store = new (std::nothrow) KVDBGeneralStore(metaData_);
    store->ref_ = 0;
    ret = store->Release();
    EXPECT_EQ(ret, 0);
    store->ref_ = 2;
    ret = store->Release();
    EXPECT_EQ(ret, 1);

    ret = store->AddRef();
    EXPECT_EQ(ret, 2);
    store->ref_ = 0;
    ret = store->AddRef();
    EXPECT_EQ(ret, 0);
}

/**
* @tc.name: ConvertStatus
* @tc.desc: ConvertStatus test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreTest, ConvertStatus, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    auto ret = store->ConvertStatus(DBStatus::OK);
    EXPECT_EQ(ret, GeneralError::E_OK);
    ret = store->ConvertStatus(DBStatus::CLOUD_NETWORK_ERROR);
    EXPECT_EQ(ret, GeneralError::E_NETWORK_ERROR);
    ret = store->ConvertStatus(DBStatus::CLOUD_LOCK_ERROR);
    EXPECT_EQ(ret, GeneralError::E_LOCKED_BY_OTHERS);
    ret = store->ConvertStatus(DBStatus::CLOUD_FULL_RECORDS);
    EXPECT_EQ(ret, GeneralError::E_RECODE_LIMIT_EXCEEDED);
    ret = store->ConvertStatus(DBStatus::CLOUD_ASSET_SPACE_INSUFFICIENT);
    EXPECT_EQ(ret, GeneralError::E_NO_SPACE_FOR_ASSET);
    ret = store->ConvertStatus(DBStatus::CLOUD_SYNC_TASK_MERGED);
    EXPECT_EQ(ret, GeneralError::E_SYNC_TASK_MERGED);
    ret = store->ConvertStatus(DBStatus::DB_ERROR);
    EXPECT_EQ(ret, GeneralError::E_DB_ERROR);
}

/**
* @tc.name: OnChange
* @tc.desc: OnChange test the functionality of different branches.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreTest, OnChange, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    DistributedDataTest::MockGeneralWatcher watcher;
    DistributedDataTest::MockKvStoreChangedData data;
    DistributedDB::ChangedData changedData;
    store->observer_.OnChange(data);
    store->observer_.OnChange(DistributedDB::Origin::ORIGIN_CLOUD, "originalId", std::move(changedData));
    auto result = store->Watch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_OK);
    store->observer_.OnChange(data);
    store->observer_.OnChange(DistributedDB::Origin::ORIGIN_CLOUD, "originalId", std::move(changedData));
    result = store->Unwatch(GeneralWatcher::Origin::ORIGIN_ALL, watcher);
    EXPECT_EQ(result, GeneralError::E_OK);
}

/**
* @tc.name: Delete
* @tc.desc: Delete test the function.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreTest, Delete, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    KvStoreNbDelegateMock mockDelegate;
    store->delegate_ = &mockDelegate;
    store->SetDBPushDataInterceptor(DistributedKv::KvStoreType::DEVICE_COLLABORATION);
    store->SetDBReceiveDataInterceptor(DistributedKv::KvStoreType::DEVICE_COLLABORATION);
    DistributedData::VBuckets values;
    auto ret = store->Insert("table", std::move(values));
    EXPECT_EQ(ret, GeneralError::E_NOT_SUPPORT);

    DistributedData::Values args;
    store->SetDBPushDataInterceptor(DistributedKv::KvStoreType::SINGLE_VERSION);
    store->SetDBReceiveDataInterceptor(DistributedKv::KvStoreType::SINGLE_VERSION);
    ret = store->Delete("table", "sql", std::move(args));
    EXPECT_EQ(ret, GeneralError::E_NOT_SUPPORT);
}

/**
* @tc.name: Query001
* @tc.desc: KVDBGeneralStoreTest Query function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreTest, Query001, TestSize.Level1)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string table = "table";
    std::string sql = "sql";
    auto [errCode, result] = store->Query(table, sql, {});
    EXPECT_EQ(errCode, GeneralError::E_NOT_SUPPORT);
    EXPECT_EQ(result, nullptr);
}

/**
* @tc.name: Query002
* @tc.desc: KVDBGeneralStoreTest Query function test
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(KVDBGeneralStoreTest, Query002, TestSize.Level1)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    std::string table = "table";
    MockQuery query;
    auto [errCode, result] = store->Query(table, query);
    EXPECT_EQ(errCode, GeneralError::E_NOT_SUPPORT);
    EXPECT_EQ(result, nullptr);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test