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
#include "crypto/crypto_manager.h"
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
using DBInterceptedData = DistributedDB::InterceptedData;
using StoreId = OHOS::DistributedKv::StoreId;
using AppId = OHOS::DistributedKv::AppId;
namespace OHOS::Test {
namespace DistributedDataTest {
static constexpr const char *INVALID_APPID = "invalid_kvdb_store_test";

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

void KVDBGeneralStoreTest::TearDown()
{
    MetaDataManager::GetInstance().DelMeta(metaData_.GetSecretKey(), true);
    MetaDataManager::GetInstance().DelMeta(metaData_.GetKey(), true);
}

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
} // namespace DistributedDataTest
} // namespace OHOS::Test