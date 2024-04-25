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

#include "bootstrap.h"
#include "crypto_manager.h"
#include "kvdb_query.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "mock/db_store_mock.h"

using namespace testing::ext;
using DBStoreMock = OHOS::DistributedData::DBStoreMock;
using StoreMetaData = OHOS::DistributedData::StoreMetaData;

namespace OHOS {
namespace DistributedKv {
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
    metaData_.storeType = KvStoreType::SINGLE_VERSION;
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

void KVDBGeneralStoreTest::SetUpTestCase(void)
{
}

void KVDBGeneralStoreTest::TearDownTestCase() {}

void KVDBGeneralStoreTest::SetUp()
{
    Bootstrap::GetInstance().LoadDirectory();
    InitMetaData();
}

void KVDBGeneralStoreTest::TearDown() {}

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
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr);
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
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr);
    metaData_.isEncrypt = true;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(metaData_.GetKey(), metaData_, true));

    auto errCode = CryptoManager::GetInstance().GenerateRootKey();
    EXPECT_EQ(errCode, CryptoManager::ErrCode::SUCCESS);

    std::vector<uint8_t> randomKey = Random(KEY_LENGTH);
    SecretKeyMetaData secretKey;
    secretKey.storeType = metaData_.storeType;
    secretKey.sKey = CryptoManager::GetInstance().Encrypt(randomKey);
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
* @tc.desc: Close kvdb general store.
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
    KVDBQuery query(kvQuery);
    SyncParam syncParam{};
    syncParam.mode = mixMode;
    auto ret = store->Sync(
        {}, query, [](const GenDetails &result) {}, syncParam);
    EXPECT_NE(ret, GeneralError::E_OK);
    ret = store->Close();
    EXPECT_EQ(ret, GeneralError::E_OK);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test
