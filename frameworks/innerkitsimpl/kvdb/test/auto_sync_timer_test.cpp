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
#include "auto_sync_timer.h"
#include "store_manager.h"
using namespace testing::ext;
using namespace OHOS::DistributedKv;
class AutoSyncTimerTest : public testing::Test {
public:
    class TestSyncCallback : public KvStoreSyncCallback {
    public:
        void SyncCompleted(const std::map<std::string, Status> &results) override
        {
            ASSERT_TRUE(true);
        }
    };
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
protected:
    static std::shared_ptr<SingleKvStore> kvStore_;
};
std::shared_ptr<SingleKvStore> AutoSyncTimerTest::kvStore_;
void AutoSyncTimerTest::SetUpTestCase(void)
{
    mkdir("/data/service/el1/public/database/ut_test", (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    Options options;
    options.kvStoreType = SINGLE_VERSION;
    options.securityLevel = S1;
    options.area = EL1;
    options.baseDir = "/data/service/el1/public/database/ut_test";
    AppId appId = { "ut_test" };
    StoreId storeId = { "ut_test_store" };
    Status status = StoreManager::GetInstance().Delete(appId, storeId, options.baseDir);
    kvStore_ = StoreManager::GetInstance().GetKVStore(appId, storeId, options, status);
    ASSERT_EQ(status, SUCCESS);
}

void AutoSyncTimerTest::TearDownTestCase(void)
{
    (void)remove("/data/service/el1/public/database/ut_test/key");
    (void)remove("/data/service/el1/public/database/ut_test/kvdb");
    (void)remove("/data/service/el1/public/database/ut_test");
}

void AutoSyncTimerTest::SetUp(void)
{

}

void AutoSyncTimerTest::TearDown(void)
{

}

/**
* @tc.name: GetStoreId
* @tc.desc: get the store id of the kv store
* @tc.type: FUNC
* @tc.require: I4XVQQ
* @tc.author: Sven Wang
*/
HWTEST_F(AutoSyncTimerTest, GetStoreId, TestSize.Level0)
{
    AutoSyncTimer::GetInstance().DoAutoSync("ut_test", {{"ut_test_store"}});
    sleep(3);
    ASSERT_TRUE(true);
}