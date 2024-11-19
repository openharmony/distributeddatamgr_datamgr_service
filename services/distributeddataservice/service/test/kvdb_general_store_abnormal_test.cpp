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
#define LOG_TAG "KVDBGeneralStoreAbnormalTest"

#include "kvdb_general_store.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>
#include <thread>

#include "bootstrap.h"
#include "cloud/asset_loader.h"
#include "cloud/cloud_db.h"
#include "cloud/schema_meta.h"
#include "crypto_manager.h"
#include "device_manager_adapter_mock.h"
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
using DBProcessCB = std::function<void(const std::map<std::string, SyncProcess> &processes)>;
namespace OHOS::Test {
namespace DistributedDataTest {
class KVDBGeneralStoreAbnormalTest : public testing::Test {
public:
    static inline std::shared_ptr<DeviceManagerAdapterMock> deviceManagerAdapterMock = nullptr;
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

std::shared_ptr<DBStoreMock> KVDBGeneralStoreAbnormalTest::dbStoreMock_ = std::make_shared<DBStoreMock>();

void KVDBGeneralStoreAbnormalTest::InitMetaData()
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

std::vector<uint8_t> KVDBGeneralStoreAbnormalTest::Random(uint32_t len)
{
    std::random_device randomDevice;
    std::uniform_int_distribution<int> distribution(0, std::numeric_limits<uint8_t>::max());
    std::vector<uint8_t> key(len);
    for (uint32_t i = 0; i < len; i++) {
        key[i] = static_cast<uint8_t>(distribution(randomDevice));
    }
    return key;
}

void KVDBGeneralStoreAbnormalTest::SetUpTestCase(void)
{
    deviceManagerAdapterMock = std::make_shared<DeviceManagerAdapterMock>();
    BDeviceManagerAdapter::deviceManagerAdapter = deviceManagerAdapterMock;
}

void KVDBGeneralStoreAbnormalTest::TearDownTestCase()
{
    deviceManagerAdapterMock = nullptr;
}

void KVDBGeneralStoreAbnormalTest::SetUp()
{
    Bootstrap::GetInstance().LoadDirectory();
    InitMetaData();
}

void KVDBGeneralStoreAbnormalTest::TearDown() {}

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
* @tc.name: GetDBOption
* @tc.desc: GetDBOption.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KVDBGeneralStoreAbnormalTest, GetDBOptionTest001, TestSize.Level0)
{
    metaData_.isEncrypt = true;
    metaData_.appId = Bootstrap::GetInstance().GetProcessLabel();
    auto dbPassword = KVDBGeneralStore::GetDBPassword(metaData_);
    auto dbOption = KVDBGeneralStore::GetDBOption(metaData_, dbPassword);
    EXPECT_EQ(dbOption.compressionRate, 10);
    EXPECT_EQ(dbOption.conflictResolvePolicy, DistributedDB::LAST_WIN);
}

/**
* @tc.name: SetEqualIdentifier
* @tc.desc: sameAccountDevs & defaultAccountDevs != empty
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KVDBGeneralStoreAbnormalTest, SetEqualIdentifier, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    std::vector<std::string> uuids{"uuidtest01"};
    KvStoreNbDelegateMock mockDelegate;
    store->delegate_ = &mockDelegate;
    EXPECT_NE(store->delegate_, nullptr);
    EXPECT_CALL(*deviceManagerAdapterMock, ToUUID(testing::_)).WillOnce(testing::Return(uuids));
    auto uuids1 = DMAdapter::ToUUID(DMAdapter::GetInstance().GetRemoteDevices());
    EXPECT_CALL(*deviceManagerAdapterMock, ToUUID(testing::_)).WillOnce(testing::Return(uuids));
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType("uuidtest01"))
        .WillOnce(testing::Return(false)).WillOnce(testing::Return(false));
    store->SetEqualIdentifier(metaData_.appId, metaData_.storeId);
    EXPECT_EQ(uuids1, uuids);
}

/**
* @tc.name: GetIdentifierParams
* @tc.desc: GetIdentifierParams abnormal branch.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KVDBGeneralStoreAbnormalTest, GetIdentifierParams, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    std::vector<std::string> sameAccountDevs{};
    std::vector<std::string> uuids{"uuidtest01"};
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType("uuidtest01"))
        .WillOnce(testing::Return(true)).WillOnce(testing::Return(true));
    store->GetIdentifierParams(sameAccountDevs, uuids, 0); // test
    for (const auto &devId : uuids) {
        EXPECT_EQ(DMAdapter::GetInstance().IsOHOSType(devId), true);
        EXPECT_EQ(DMAdapter::GetInstance().GetAuthType(devId), 0); // NO_ACCOUNT
    }
    EXPECT_EQ(sameAccountDevs.empty(), true);
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType("uuidtest01"))
        .WillOnce(testing::Return(false)).WillOnce(testing::Return(false));
    store->GetIdentifierParams(sameAccountDevs, uuids, 1); // test
    for (const auto &devId : uuids) {
        EXPECT_EQ(DMAdapter::GetInstance().IsOHOSType(devId), false);
        EXPECT_EQ(DMAdapter::GetInstance().GetAuthType(devId), 0); // NO_ACCOUNT
    }
    EXPECT_EQ(sameAccountDevs.empty(), true);
}

/**
* @tc.name: ConvertStatus
* @tc.desc: ConvertStatus test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KVDBGeneralStoreAbnormalTest, ConvertStatus, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    auto status = store->ConvertStatus(DistributedDB::DBStatus::BUTT_STATUS); // test
    EXPECT_EQ(status, DistributedData::GeneralError::E_ERROR);
}

/**
* @tc.name: ConvertStatus
* @tc.desc: ConvertStatus test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KVDBGeneralStoreAbnormalTest, GetDBProcessCB, TestSize.Level0)
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    ASSERT_NE(store, nullptr);
    GeneralStore::DetailAsync async;
    EXPECT_EQ(async, nullptr);
    auto process = store->GetDBProcessCB(async);
    EXPECT_NE(process, nullptr);
    auto asyncs = [](const GenDetails &result) {};
    EXPECT_NE(asyncs, nullptr);
    process = store->GetDBProcessCB(asyncs);
    EXPECT_NE(process, nullptr);
}

/**
* @tc.name: SetDBPushDataInterceptor
* @tc.desc: SetDBPushDataInterceptor test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KVDBGeneralStoreAbnormalTest, SetDBPushDataInterceptor, TestSize.Level0) //SINGLE_VERSION
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    KvStoreNbDelegateMock mockDelegate;
    store->delegate_ = &mockDelegate;
    EXPECT_NE(store->delegate_, nullptr);
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType("uuidtest01")).WillOnce(testing::Return(false));
    store->SetDBPushDataInterceptor(metaData_.storeType);
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType("uuidtest01")).WillOnce(testing::Return(true));
    store->SetDBPushDataInterceptor(metaData_.storeType);
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType("uuidtest01")).WillOnce(testing::Return(true));
    store->SetDBPushDataInterceptor(DistributedKv::KvStoreType::DEVICE_COLLABORATION);
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType("uuidtest01")).WillOnce(testing::Return(false));
    store->SetDBPushDataInterceptor(DistributedKv::KvStoreType::DEVICE_COLLABORATION);
    EXPECT_NE(metaData_.storeType, DistributedKv::KvStoreType::DEVICE_COLLABORATION);
}

/**
* @tc.name: SetDBReceiveDataInterceptor
* @tc.desc: SetDBReceiveDataInterceptor test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: wangbin
*/
HWTEST_F(KVDBGeneralStoreAbnormalTest, SetDBReceiveDataInterceptor, TestSize.Level0) //SINGLE_VERSION
{
    auto store = new (std::nothrow) KVDBGeneralStore(metaData_);
    KvStoreNbDelegateMock mockDelegate;
    store->delegate_ = &mockDelegate;
    EXPECT_NE(store->delegate_, nullptr);
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType("uuidtest_1")).WillOnce(testing::Return(false));
    store->SetDBReceiveDataInterceptor(metaData_.storeType);
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType("uuidtest_1")).WillOnce(testing::Return(true));
    store->SetDBReceiveDataInterceptor(metaData_.storeType);
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType("uuidtest_1")).WillOnce(testing::Return(true));
    store->SetDBReceiveDataInterceptor(DistributedKv::KvStoreType::DEVICE_COLLABORATION);
    EXPECT_CALL(*deviceManagerAdapterMock, IsOHOSType("uuidtest_1")).WillOnce(testing::Return(false));
    store->SetDBReceiveDataInterceptor(DistributedKv::KvStoreType::DEVICE_COLLABORATION);
    EXPECT_NE(metaData_.storeType, DistributedKv::KvStoreType::DEVICE_COLLABORATION);
}
} // namespace DistributedDataTest
} // namespace OHOS::Test