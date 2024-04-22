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

#include "account/account_delegate.h"
#include "communicator/device_manager_adapter.h"
#include "device_matrix.h"
#include "distributed_kv_data_manager.h"
#include "eventcenter/event_center.h"
#include "feature/feature_system.h"
#include "ipc_skeleton.h"
#include "kvdb_query.h"
#include "metadata/meta_data_manager.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "mock/db_store_mock.h"
#include "rdb_types.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedKv;
using DmAdapter = OHOS::DistributedData::DeviceManagerAdapter;

namespace OHOS::Test {
namespace DistributedDataTest {
class KVDBGeneralStoreTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    static constexpr const char *TEST_DISTRIBUTEDDATA_BUNDLE = "test_distributeddata";
    static constexpr const char *TEST_DISTRIBUTEDDATA_STORE = "test_service_meta";

    void InitMetaData();
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    StoreMetaData metaData_;
};

void KVDBGeneralStoreTest::InitMetaData()
{
    metaData_.deviceId = DmAdapter::GetInstance().GetLocalDevice().uuid;
    metaData_.appId = TEST_DISTRIBUTEDDATA_BUNDLE;
    metaData_.bundleName = TEST_DISTRIBUTEDDATA_BUNDLE;
    metaData_.tokenId = OHOS::IPCSkeleton::GetCallingTokenID();
    metaData_.user = std::to_string(DistributedKv::AccountDelegate::GetInstance()->GetUserByToken(metaData_.tokenId));
    metaData_.area = OHOS::DistributedKv::EL1;
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = 1;
    metaData_.storeId = TEST_DISTRIBUTEDDATA_STORE;
    PolicyValue value;
    value.type = OHOS::DistributedKv::PolicyType::IMMEDIATE_SYNC_ON_ONLINE;
}

void KVDBGeneralStoreTest::SetUpTestCase(void)
{
}

void KVDBGeneralStoreTest::TearDownTestCase() {}

void KVDBGeneralStoreTest::SetUp()
{
}

void KVDBGeneralStoreTest::TearDown() {}

/**
* @tc.name: GetDBPasswordTest
* @tc.desc: GetDBPassword from meta.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreTest, GetDBPasswordTest, TestSize.Level0)
{
    InitMetaData();
    auto dbPassword = KVDBGeneralStore::GetDBPassword(metaData_);
    ASSERT_TRUE(dbPassword.GetSize() == 0);
    metaData_.isEncrypt = true;
    dbPassword = KVDBGeneralStore::GetDBPassword(metaData_);
    ASSERT_TRUE(dbPassword.GetSize() != 0);
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
    EXPECT_EQ(dbSecurity.securityLabel, SecurityLevel::NO_LABEL);
    EXPECT_EQ(dbSecurity.securityFlag, DistributedDB::ECE);

    dbSecurity = KVDBGeneralStore::GetDBSecurity(SecurityLevel::S0);
    EXPECT_EQ(dbSecurity.securityLabel, SecurityLevel::S0);
    EXPECT_EQ(dbSecurity.securityFlag, DistributedDB::ECE);

    dbSecurity = KVDBGeneralStore::GetDBSecurity(SecurityLevel::S1);
    EXPECT_EQ(dbSecurity.securityLabel, SecurityLevel::S1);
    EXPECT_EQ(dbSecurity.securityFlag, DistributedDB::ECE);

    dbSecurity = KVDBGeneralStore::GetDBSecurity(SecurityLevel::S2);
    EXPECT_EQ(dbSecurity.securityLabel, SecurityLevel::S2);
    EXPECT_EQ(dbSecurity.securityFlag, DistributedDB::ECE);

    dbSecurity = KVDBGeneralStore::GetDBSecurity(SecurityLevel::S3);
    EXPECT_EQ(dbSecurity.securityLabel, SecurityLevel::S3);
    EXPECT_EQ(dbSecurity.securityFlag, DistributedDB::SECE);

    dbSecurity = KVDBGeneralStore::GetDBSecurity(SecurityLevel::S4);
    EXPECT_EQ(dbSecurity.securityLabel, SecurityLevel::S4);
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
    EXPECT_EQ(ret, GeneralError::E_INVALID_ARGS);
    ret = store->Sync(
        { "default_deviceId" }, query, [](const GenDetails &result) {}, syncParam);
    EXPECT_NE(ret, GeneralError::E_OK);
    ret = store->Close();
    EXPECT_EQ(ret, GeneralError::E_OK);
}

/**
* @tc.name: CleanTest
* @tc.desc: Clean.
* @tc.type: FUNC
* @tc.require:
* @tc.author: Hollokin
*/
HWTEST_F(KVDBGeneralStoreTest, CleanTest, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    EXPECT_EQ(store->IsValid(), true);
    auto ret = store->Clean({}, GeneralStore::CleanMode::NEARBY_DATA, "");
    EXPECT_EQ(ret, GeneralError::E_OK);
    ret = store->Close();
    EXPECT_EQ(ret, GeneralError::E_OK);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test
