/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include "distributed_kv_data_manager.h"
using namespace testing::ext;
using namespace OHOS::DistributedKv;
class UpgradeTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
protected:
    static const AppId appId_;
    static const StoreId storeId_;
    static constexpr const char *APP_DIR = "/data/service/el1/public/database/ut_test";
    static constexpr const char *KEY_DIR = "/data/service/el1/public/database/ut_test/key";
    static constexpr const char *DB_DIR = "/data/service/el1/public/database/ut_test/kvdb";
};
const AppId UpgradeTest::appId_ = { "ut_test" };
const StoreId UpgradeTest::storeId_ = { "ut_test_store" };
void UpgradeTest::SetUpTestCase(void)
{
    mkdir(APP_DIR, (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void UpgradeTest::TearDownTestCase(void)
{
    remove(KEY_DIR);
    remove(DB_DIR);
    remove(APP_DIR);
}

void UpgradeTest::SetUp(void)
{
    DistributedKvDataManager manager;
    manager.DeleteKvStore(appId_, storeId_);
    manager.DeleteKvStore(appId_, storeId_, APP_DIR);
}

void UpgradeTest::TearDown(void)
{
    DistributedKvDataManager manager;
    manager.DeleteKvStore(appId_, storeId_);
    manager.DeleteKvStore(appId_, storeId_, APP_DIR);
}

/**
* @tc.name: Upgrade
* @tc.desc: upgrade normal kv store
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Sven Wang
*/
HWTEST_F(UpgradeTest, Upgrade, TestSize.Level0)
{
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S1;
    DistributedKvDataManager manager;
    std::shared_ptr<SingleKvStore> kvStore;
    auto status = manager.GetSingleKvStore(options, appId_, storeId_, kvStore);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore->Put("upgrade test", "upgrade value");
    ASSERT_EQ(status, SUCCESS);
    status = manager.CloseKvStore(appId_, kvStore);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(kvStore, nullptr);
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/ut_test";
    status = manager.GetSingleKvStore(options, appId_, storeId_, kvStore);
    ASSERT_EQ(status, SUCCESS);
    Value value;
    status = kvStore->Get("upgrade test", value);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(value.ToString(), std::string("upgrade value"));
}

/**
* @tc.name: UpgradeEncrypt
* @tc.desc: upgrade encrypt kv store
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Sven Wang
*/
HWTEST_F(UpgradeTest, UpgradeEncrypt, TestSize.Level0)
{
    Options options;
    options.encrypt = true;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S1;
    DistributedKvDataManager manager;
    std::shared_ptr<SingleKvStore> kvStore;
    auto status = manager.GetSingleKvStore(options, appId_, storeId_, kvStore);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore->Put("upgrade test", "upgrade value");
    ASSERT_EQ(status, SUCCESS);
    status = manager.CloseKvStore(appId_, kvStore);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(kvStore, nullptr);
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/ut_test";
    status = manager.GetSingleKvStore(options, appId_, storeId_, kvStore);
    ASSERT_EQ(status, SUCCESS);
    Value value;
    status = kvStore->Get("upgrade test", value);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(value.ToString(), std::string("upgrade value"));
}

/**
* @tc.name: Rollback
* @tc.desc: rollback normal kv store
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Sven Wang
*/
HWTEST_F(UpgradeTest, Rollback, TestSize.Level0)
{
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S1;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/ut_test";
    DistributedKvDataManager manager;
    std::shared_ptr<SingleKvStore> kvStore;
    auto status = manager.GetSingleKvStore(options, appId_, storeId_, kvStore);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore->Put("rollback test", "rollback value");
    ASSERT_EQ(status, SUCCESS);
    status = manager.CloseKvStore(appId_, kvStore);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(kvStore, nullptr);
    options.area = EL1;
    options.baseDir = "";
    status = manager.GetSingleKvStore(options, appId_, storeId_, kvStore);
    ASSERT_EQ(status, SUCCESS);
    Value value;
    status = kvStore->Get("rollback test", value);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(value.ToString(), std::string("rollback value"));
}

/**
* @tc.name: RollbackEncrypt
* @tc.desc: rollback encrypt kv store
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Sven Wang
*/
HWTEST_F(UpgradeTest, RollbackEncrypt, TestSize.Level0)
{
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S1;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/ut_test";
    DistributedKvDataManager manager;
    std::shared_ptr<SingleKvStore> kvStore;
    auto status = manager.GetSingleKvStore(options, appId_, storeId_, kvStore);
    ASSERT_EQ(status, SUCCESS);
    status = kvStore->Put("rollback test", "rollback value");
    ASSERT_EQ(status, SUCCESS);
    status = manager.CloseKvStore(appId_, kvStore);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(kvStore, nullptr);
    options.area = EL1;
    options.baseDir = "";
    status = manager.GetSingleKvStore(options, appId_, storeId_, kvStore);
    ASSERT_EQ(status, SUCCESS);
    Value value;
    status = kvStore->Get("rollback test", value);
    ASSERT_EQ(status, SUCCESS);
    ASSERT_EQ(value.ToString(), std::string("rollback value"));
}