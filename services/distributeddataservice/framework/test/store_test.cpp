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
#define LOG_TAG "StoreTest"

#include "access_token.h"
#include "gtest/gtest.h"
#include "general_store_mock.h"
#include "log_print.h"
#include "metadata/store_meta_data.h"
#include "rdb_query.h"
#include "rdb_types.h"
#include "store/auto_cache.h"
#include "store/general_store.h"
#include "store/general_value.h"
#include "store/general_watcher.h"
#include "screen_lock_mock.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class GeneralValueTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class GeneralStoreTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class AutoCacheTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
protected:
    static std::shared_ptr<ScreenLockMock> mock_;
};


void AutoCacheTest::SetUpTestCase(void)
{
    ScreenManager::RegisterInstance(mock_);
}

std::shared_ptr<ScreenLockMock> AutoCacheTest::mock_ = std::make_shared<ScreenLockMock>();
/**
 * @tc.name: SetQueryNodesTest
 * @tc.desc: Set and query nodes.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(GeneralValueTest, SetQueryNodesTest, TestSize.Level2)
{
    ZLOGI("GeneralValueTest SetQueryNodesTest begin.");
    std::string tableName = "test_tableName";
    QueryNode node;
    node.op = OHOS::DistributedData::QueryOperation::EQUAL_TO;
    node.fieldName =  "test_fieldName";
    node.fieldValue = {"aaa", "bbb", "ccc"};
    QueryNodes nodes{
        {node}
    };
    OHOS::DistributedRdb::RdbQuery query;
    query.SetQueryNodes(tableName, std::move(nodes));
    QueryNodes nodes1 =  query.GetQueryNodes("test_tableName");
    EXPECT_EQ(nodes1[0].fieldName, "test_fieldName");
    EXPECT_EQ(nodes1[0].op, OHOS::DistributedData::QueryOperation::EQUAL_TO);
    EXPECT_EQ(nodes1[0].fieldValue.size(), 3);
}

/**
* @tc.name: GetMixModeTest
* @tc.desc: get mix mode
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(GeneralStoreTest, GetMixModeTest, TestSize.Level2)
{
    ZLOGI("GeneralStoreTest GetMixModeTest begin.");
    auto mode1 = OHOS::DistributedRdb::TIME_FIRST;
    auto mode2 = GeneralStore::AUTO_SYNC_MODE;

    auto mixMode = GeneralStore::MixMode(mode1, mode2);
    EXPECT_EQ(mixMode, mode1 | mode2);

    auto syncMode = GeneralStore::GetSyncMode(mixMode);
    EXPECT_EQ(syncMode, OHOS::DistributedRdb::TIME_FIRST);

    auto highMode = GeneralStore::GetHighMode(mixMode);
    EXPECT_EQ(highMode, GeneralStore::AUTO_SYNC_MODE);
}

/**
* @tc.name: OnChange001
* @tc.desc: AutoCache Delegate OnChange
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(AutoCacheTest, OnChange001, TestSize.Level2)
{
    GeneralStoreMock* store = new (std::nothrow) GeneralStoreMock();
    ASSERT_NE(store, nullptr);
    AutoCache::Watchers watchers;
    int32_t user = 0;
    StoreMetaData meta;
    AutoCache::Delegate delegate(store, watchers, user, meta);
    delegate.SetObservers(watchers);
    GeneralWatcher::Origin origin;
    GeneralWatcher::PRIFields primaryFields;
    GeneralWatcher::ChangeInfo values;
    auto ret = delegate.OnChange(origin, primaryFields, std::move(values));
    EXPECT_EQ(ret, GeneralError::E_OK);
}

/**
* @tc.name: OnChange002
* @tc.desc: AutoCache Delegate OnChange
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(AutoCacheTest, OnChange002, TestSize.Level2)
{
    GeneralStoreMock* store = new (std::nothrow) GeneralStoreMock();
    ASSERT_NE(store, nullptr);
    AutoCache::Watchers watchers;
    int32_t user = 0;
    StoreMetaData meta;
    AutoCache::Delegate delegate(store, watchers, user, meta);
    delegate.SetObservers(watchers);
    GeneralWatcher::Origin origin;
    GeneralWatcher::Fields fields;
    GeneralWatcher::ChangeData datas;
    auto ret = delegate.OnChange(origin, fields, std::move(datas));
    EXPECT_EQ(ret, GeneralError::E_OK);
}

/**
* @tc.name: operatorStore
* @tc.desc: AutoCache Delegate operator Store()
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(AutoCacheTest, operatorStore, TestSize.Level2)
{
    GeneralStoreMock* store = new (std::nothrow) GeneralStoreMock();
    ASSERT_NE(store, nullptr);
    AutoCache::Watchers watchers;
    int32_t user = 0;
    StoreMetaData meta;
    AutoCache::Delegate delegate(store, watchers, user, meta);
    AutoCache::Store result = static_cast<AutoCache::Store>(delegate);
    EXPECT_NE(result, nullptr);
    GeneralWatcher::Origin origin;
    GeneralWatcher::Fields fields;
    GeneralWatcher::ChangeData datas;
    auto ret = delegate.OnChange(origin, fields, std::move(datas));
    EXPECT_EQ(ret, GeneralError::E_OK);
    EXPECT_EQ(delegate.GetUser(), user);
}

/**
* @tc.name: GetMeta
* @tc.desc: AutoCache Delegate operator GetMeta()
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(AutoCacheTest, GetMeta, TestSize.Level2)
{
    GeneralStoreMock* store = new (std::nothrow) GeneralStoreMock();
    ASSERT_NE(store, nullptr);
    AutoCache::Watchers watchers;
    int32_t user = 0;
    StoreMetaData meta;
    meta.enableCloud = true;
    AutoCache::Delegate delegate(store, watchers, user, meta);
    auto newMate = delegate.GetMeta();
    ASSERT_TRUE(newMate.enableCloud);
}

/**
* @tc.name: GetDBStore
* @tc.desc: AutoCache GetDBStore
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(AutoCacheTest, GetDBStore, TestSize.Level2)
{
    auto creator = [](const StoreMetaData &metaData) -> GeneralStore* {
        return new (std::nothrow) GeneralStoreMock();
    };
    AutoCache::GetInstance().RegCreator(DistributedRdb::RDB_DEVICE_COLLABORATION, creator);
    AutoCache::Watchers watchers;
    StoreMetaData meta;
    meta.storeType = DistributedRdb::RDB_DEVICE_COLLABORATION;
    mock_->isLocked_ = true;
    meta.area = GeneralStore::EL4;
    EXPECT_EQ(AutoCache::GetInstance().GetDBStore(meta, watchers).first, GeneralError::E_SCREEN_LOCKED);
    meta.area = GeneralStore::EL1;
    EXPECT_NE(AutoCache::GetInstance().GetDBStore(meta, watchers).first, GeneralError::E_SCREEN_LOCKED);
    meta.area = GeneralStore::EL2;
    EXPECT_NE(AutoCache::GetInstance().GetDBStore(meta, watchers).first, GeneralError::E_SCREEN_LOCKED);
    meta.area = GeneralStore::EL3;
    EXPECT_NE(AutoCache::GetInstance().GetDBStore(meta, watchers).first, GeneralError::E_SCREEN_LOCKED);
    meta.area = GeneralStore::EL5;
    EXPECT_NE(AutoCache::GetInstance().GetDBStore(meta, watchers).first, GeneralError::E_SCREEN_LOCKED);

    mock_->isLocked_ = false;
    meta.area = GeneralStore::EL4;
    EXPECT_NE(AutoCache::GetInstance().GetDBStore(meta, watchers).first, GeneralError::E_SCREEN_LOCKED);
    meta.area = GeneralStore::EL1;
    EXPECT_NE(AutoCache::GetInstance().GetDBStore(meta, watchers).first, GeneralError::E_SCREEN_LOCKED);
    meta.area = GeneralStore::EL2;
    EXPECT_NE(AutoCache::GetInstance().GetDBStore(meta, watchers).first, GeneralError::E_SCREEN_LOCKED);
    meta.area = GeneralStore::EL3;
    EXPECT_NE(AutoCache::GetInstance().GetDBStore(meta, watchers).first, GeneralError::E_SCREEN_LOCKED);
    meta.area = GeneralStore::EL5;
    EXPECT_NE(AutoCache::GetInstance().GetDBStore(meta, watchers).first, GeneralError::E_SCREEN_LOCKED);
}
} // namespace OHOS::Test