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
#define LOG_TAG "RdbGeneralStoreTest"

#include "rdb_general_store.h"

#include <random>
#include <thread>

#include "bootstrap.h"
#include "cloud/schema_meta.h"
#include "error/general_error.h"
#include "errors.h"
#include "eventcenter/event_center.h"
#include "gtest/gtest.h"
#include "log_print.h"
#include "metadata/meta_data_manager.h"
#include "metadata/secret_key_meta_data.h"
#include "metadata/store_meta_data.h"
#include "metadata/store_meta_data_local.h"
#include "mock/general_watcher_mock.h"
#include "rdb_query.h"
#include "relational_store_delegate_mock.h"
#include "store/general_store.h"
#include "store/general_value.h"
#include "store_observer.h"
#include "store_types.h"
#include "types.h"

using namespace testing::ext;
using namespace DistributedDB;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedRdb;
using DBStatus = DistributedDB::DBStatus;
RdbGeneralStore::Values g_RdbValues = { { "0000000" }, { true }, { int64_t(100) }, { double(100) }, { int64_t(1) },
    { Bytes({ 1, 2, 3, 4 }) } };
RdbGeneralStore::VBucket g_RdbVBucket = { { "#gid", { "0000000" } }, { "#flag", { true } },
    { "#value", { int64_t(100) } }, { "#float", { double(100) } } };
bool MockRelationalStoreDelegate::gTestResult = false;
namespace OHOS::Test {
namespace DistributedRDBTest {
using StoreMetaData = OHOS::DistributedData::StoreMetaData;
using namespace OHOS::DistributedRdb;
static constexpr uint32_t PRINT_ERROR_CNT = 150;
static constexpr const char *BUNDLE_NAME = "test_rdb_general_store";
static constexpr const char *STORE_NAME = "test_service_rdb";
class RdbGeneralStoreTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp()
    {
        Bootstrap::GetInstance().LoadDirectory();
        InitMetaData();
        store = std::make_shared<RdbGeneralStore>(metaData_);
        ASSERT_NE(store, nullptr);
    };
    void TearDown()
    {
        MockRelationalStoreDelegate::gTestResult = false;
    };

protected:
    void InitMetaData();
    StoreMetaData metaData_;
    std::shared_ptr<RdbGeneralStore> store;
};

void RdbGeneralStoreTest::InitMetaData()
{
    metaData_.bundleName = BUNDLE_NAME;
    metaData_.appId = BUNDLE_NAME;
    metaData_.user = "0";
    metaData_.area = DistributedKv::Area::EL1;
    metaData_.instanceId = 0;
    metaData_.isAutoSync = true;
    metaData_.storeType = 1;
    metaData_.storeId = STORE_NAME;
    metaData_.dataDir = "/data/service/el1/public/database/" + std::string(BUNDLE_NAME) + "/rdb";
    metaData_.securityLevel = DistributedKv::SecurityLevel::S2;
    metaData_.isSearchable = true;
}

class MockStoreChangedData : public DistributedDB::StoreChangedData {
public:
    std::string GetDataChangeDevice() const override
    {
        return "DataChangeDevice";
    }

    void GetStoreProperty(DistributedDB::StoreProperty &storeProperty) const override
    {
        return;
    }
};
} // namespace DistributedRDBTest
} // namespace OHOS::Test