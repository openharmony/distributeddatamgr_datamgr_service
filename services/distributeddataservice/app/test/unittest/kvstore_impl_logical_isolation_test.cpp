/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include <cstdint>
#include <thread>
#include <vector>
#include "kvstore_data_service.h"
#include "kvstore_meta_manager.h"
#include "refbase.h"
#include "types.h"
#include "bootstrap.h"
#include "gtest/gtest.h"
using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributedData;
using namespace OHOS;

namespace {
sptr<KvStoreDataService> g_kvStoreDataService;
Options g_defaultOptions;
AppId g_appId;
StoreId g_storeId;
AppId g_appId1;
StoreId g_storeId1;
AppId g_appId2;
StoreId g_storeId2;
}

class SingleKvStoreImplLogicalIsolationTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void SingleKvStoreImplLogicalIsolationTest::SetUpTestCase(void)
{
    g_defaultOptions.createIfMissing = true;
    g_defaultOptions.encrypt = false;
    g_defaultOptions.autoSync = true;
    g_defaultOptions.kvStoreType = KvStoreType::SINGLE_VERSION;

    g_appId.appId = "lgc0";
    g_storeId.storeId = "store0";

    g_appId1.appId = "lgc1";
    g_storeId1.storeId = "store1";

    g_appId2.appId = "lgc2";
    g_storeId2.storeId = "store2";
}

void SingleKvStoreImplLogicalIsolationTest::TearDownTestCase(void)
{
    g_kvStoreDataService->CloseAllKvStore(g_appId);
    g_kvStoreDataService->CloseAllKvStore(g_appId1);
    g_kvStoreDataService->CloseAllKvStore(g_appId2);

    g_kvStoreDataService->DeleteAllKvStore(g_appId);
    g_kvStoreDataService->DeleteAllKvStore(g_appId1);
    g_kvStoreDataService->DeleteAllKvStore(g_appId2);
}

void SingleKvStoreImplLogicalIsolationTest::SetUp(void)
{
    g_kvStoreDataService = sptr<KvStoreDataService>(new KvStoreDataService());
    KvStoreMetaManager::GetInstance().InitMetaParameter();
    KvStoreMetaManager::GetInstance().InitMetaListener();
    Bootstrap::GetInstance().LoadComponents();
    Bootstrap::GetInstance().LoadDirectory();
    Bootstrap::GetInstance().LoadCheckers();
    Bootstrap::GetInstance().LoadNetworks();
    g_kvStoreDataService->CloseAllKvStore(g_appId);
    g_kvStoreDataService->CloseAllKvStore(g_appId1);
    g_kvStoreDataService->CloseAllKvStore(g_appId2);

    g_kvStoreDataService->DeleteAllKvStore(g_appId);
    g_kvStoreDataService->DeleteAllKvStore(g_appId1);
    g_kvStoreDataService->DeleteAllKvStore(g_appId2);
}

void SingleKvStoreImplLogicalIsolationTest::TearDown(void)
{
    g_kvStoreDataService->CloseAllKvStore(g_appId);
    g_kvStoreDataService->CloseAllKvStore(g_appId1);
    g_kvStoreDataService->CloseAllKvStore(g_appId2);

    g_kvStoreDataService->DeleteAllKvStore(g_appId);
    g_kvStoreDataService->DeleteAllKvStore(g_appId1);
    g_kvStoreDataService->DeleteAllKvStore(g_appId2);
}

/**
* @tc.name: LogicalIsolation001
* @tc.desc: Verify getting KvStore successfully
* @tc.type: FUNC
* @tc.require: AR000BVDF8 AR000CQS39 SR000CQS38
* @tc.author: liuyuhui
*/
HWTEST_F(SingleKvStoreImplLogicalIsolationTest, LogicalIsolation001, TestSize.Level1)
{
    sptr<ISingleKvStore> kvStorePtr;
    Status status = g_kvStoreDataService->GetSingleKvStore(g_defaultOptions, g_appId, g_storeId,
                                          [&](sptr<ISingleKvStore> kvStore) { kvStorePtr = std::move(kvStore); });

    EXPECT_EQ(status, Status::SUCCESS) << "GetSingleKvStore return wrong status";
    EXPECT_NE(kvStorePtr, nullptr) << "GetSingleKvStore execute fail!!";

    status = g_kvStoreDataService->CloseKvStore(g_appId, g_storeId);
}

/**
* @tc.name: LogicalIsolation002
* @tc.desc: get KvStore which exists
* @tc.type: FUNC
* @tc.require: AR000BVDF8 AR000CQS39 SR000CQS38
* @tc.author: liuyuhui
*/
HWTEST_F(SingleKvStoreImplLogicalIsolationTest, LogicalIsolation002, TestSize.Level1)
{
    sptr<ISingleKvStore> kvStorePtr;
    Status status = g_kvStoreDataService->GetSingleKvStore(g_defaultOptions, g_appId, g_storeId,
                                          [&](sptr<ISingleKvStore> kvStore) { kvStorePtr = std::move(kvStore); });

    EXPECT_EQ(status, Status::SUCCESS) << "GetSingleKvStore return wrong status";
    EXPECT_NE(kvStorePtr, nullptr) << "GetSingleKvStore execute fail!!";

    sptr<ISingleKvStore> kvStorePtr1;
    status = g_kvStoreDataService->GetSingleKvStore(g_defaultOptions, g_appId, g_storeId,
                                   [&](sptr<ISingleKvStore> kvStore) { kvStorePtr1 = std::move(kvStore); });

    EXPECT_EQ(status, Status::SUCCESS) << "GetSingleKvStore return wrong status";
    EXPECT_NE(kvStorePtr1, nullptr) << "GetSingleKvStore execute fail!!";
    EXPECT_EQ(kvStorePtr, kvStorePtr1) << "Two SingleKvStoreImpl EQ fail";

    status = g_kvStoreDataService->CloseKvStore(g_appId, g_storeId);
}

/**
* @tc.name: LogicalIsolation003
* @tc.desc: get different KvStore with different g_appId and the same g_storeId
* @tc.type: FUNC
* @tc.require: AR000BVDF8 AR000CQS39 SR000CQS38
* @tc.author: liuyuhui
*/
HWTEST_F(SingleKvStoreImplLogicalIsolationTest, LogicalIsolation003, TestSize.Level1)
{
    sptr<ISingleKvStore> kvStorePtr;
    Status status = g_kvStoreDataService->GetSingleKvStore(g_defaultOptions, g_appId, g_storeId,
                                          [&](sptr<ISingleKvStore> kvStore) { kvStorePtr = std::move(kvStore); });

    EXPECT_EQ(status, Status::SUCCESS) << "GetSingleKvStore return wrong status";
    EXPECT_NE(kvStorePtr, nullptr) << "GetSingleKvStore execute fail!!";

    sptr<ISingleKvStore> kvStorePtr1;
    status = g_kvStoreDataService->GetSingleKvStore(g_defaultOptions, g_appId1, g_storeId,
                                   [&](sptr<ISingleKvStore> kvStore) { kvStorePtr1 = std::move(kvStore); });
    EXPECT_EQ(status, Status::SUCCESS) << "GetSingleKvStore return wrong status";
    EXPECT_NE(kvStorePtr1, nullptr) << "GetSingleKvStore execute fail!!";
    EXPECT_NE(kvStorePtr, kvStorePtr1) << "Two SingleKvStoreImpl NE fail";

    status = g_kvStoreDataService->CloseKvStore(g_appId, g_storeId);
    status = g_kvStoreDataService->CloseKvStore(g_appId1, g_storeId);
}

/**
* @tc.name: LogicalIsolation004
* @tc.desc: get different KvStore with the same g_appId and different g_storeId
* @tc.type: FUNC
* @tc.require: AR000BVDF8 AR000CQS39
* @tc.author: liuyuhui
*/
HWTEST_F(SingleKvStoreImplLogicalIsolationTest, LogicalIsolation004, TestSize.Level1)
{
    sptr<ISingleKvStore> kvStorePtr;
    Status status = g_kvStoreDataService->GetSingleKvStore(g_defaultOptions, g_appId, g_storeId,
                                          [&](sptr<ISingleKvStore> kvStore) { kvStorePtr = std::move(kvStore); });

    EXPECT_EQ(status, Status::SUCCESS) << "GetSingleKvStore return wrong status";
    EXPECT_NE(kvStorePtr, nullptr) << "GetSingleKvStore execute fail!!";

    sptr<ISingleKvStore> kvStorePtr1;
    status = g_kvStoreDataService->GetSingleKvStore(g_defaultOptions, g_appId, g_storeId1,
                                   [&](sptr<ISingleKvStore> kvStore) { kvStorePtr1 = std::move(kvStore); });
    EXPECT_EQ(status, Status::SUCCESS) << "GetSingleKvStore return wrong status";
    EXPECT_NE(kvStorePtr1, nullptr) << "GetSingleKvStore execute fail!!";
    EXPECT_NE(kvStorePtr, kvStorePtr1) << "Two SingleKvStoreImpl NE fail";
}

/**
* @tc.name: LogicalIsolation005
* @tc.desc: get different KvStore with different g_appId and different g_storeId
* @tc.type: FUNC
* @tc.require: AR000BVDF8 AR000CQS39
* @tc.author: liuyuhui
*/
HWTEST_F(SingleKvStoreImplLogicalIsolationTest, LogicalIsolation005, TestSize.Level1)
{
    KvStoreDataService kvDataService;
    sptr<ISingleKvStore> kvStorePtr;
    Status status = g_kvStoreDataService->GetSingleKvStore(g_defaultOptions, g_appId, g_storeId,
                                          [&](sptr<ISingleKvStore> kvStore) { kvStorePtr = std::move(kvStore); });

    EXPECT_EQ(status, Status::SUCCESS) << "GetSingleKvStore return wrong status";
    EXPECT_NE(kvStorePtr, nullptr) << "GetSingleKvStore execute fail!!";

    sptr<ISingleKvStore> kvStorePtr1;
    status = g_kvStoreDataService->GetSingleKvStore(g_defaultOptions, g_appId1, g_storeId1,
                                   [&](sptr<ISingleKvStore> kvStore) { kvStorePtr1 = std::move(kvStore); });

    EXPECT_EQ(status, Status::SUCCESS) << "GetSingleKvStore return wrong status";
    EXPECT_NE(kvStorePtr1, nullptr) << "GetSingleKvStore execute fail!!";
    EXPECT_NE(kvStorePtr, kvStorePtr1) << "Two SingleKvStoreImpl NE fail";
}

/**
* @tc.name: LogicalIsolation006
* @tc.desc: multithread create the same kvstore.
* @tc.type: FUNC
* @tc.require: AR000BVDF8 AR000CQS39
* @tc.author: liuyuhui
*/
HWTEST_F(SingleKvStoreImplLogicalIsolationTest, LogicalIsolation006, TestSize.Level1)
{
    sptr<ISingleKvStore> kvStorePtr;
    sptr<ISingleKvStore> kvStorePtr1;
    sptr<ISingleKvStore> kvStorePtr2;
    Status status = g_kvStoreDataService->GetSingleKvStore(g_defaultOptions, g_appId, g_storeId,
                                          [&](sptr<ISingleKvStore> kvStore) { kvStorePtr = std::move(kvStore); });
    EXPECT_EQ(status, Status::SUCCESS) << "GetSingleKvStore return wrong status";

    std::thread thread1([&]() {
        Status status1 = g_kvStoreDataService->GetSingleKvStore(g_defaultOptions, g_appId, g_storeId,
                                               [&](sptr<ISingleKvStore> kvStore) { kvStorePtr1 = std::move(kvStore); });
        EXPECT_EQ(status1, Status::SUCCESS) << "GetSingleKvStore return wrong status";
    });

    std::thread thread2([&]() {
        Status status2 = g_kvStoreDataService->GetSingleKvStore(g_defaultOptions, g_appId, g_storeId,
                                               [&](sptr<ISingleKvStore> kvStore) { kvStorePtr2 = std::move(kvStore); });
        EXPECT_EQ(status2, Status::SUCCESS) << "GetSingleKvStore return wrong status";
    });

    thread1.join();
    thread2.join();

    EXPECT_NE(kvStorePtr, nullptr) << "GetSingleKvStore execute fail!!";
    EXPECT_NE(kvStorePtr1, nullptr) << "GetSingleKvStore execute fail!!";
    EXPECT_NE(kvStorePtr2, nullptr) << "GetSingleKvStore execute fail!!";
    EXPECT_EQ(kvStorePtr, kvStorePtr1) << "Two SingleKvStoreImpl EQ fail";
    EXPECT_EQ(kvStorePtr, kvStorePtr2) << "Two SingleKvStoreImpl EQ fail";
}

/**
* @tc.name: LogicalIsolation007
* @tc.desc: multithread create different kvstore.
* @tc.type: FUNC
* @tc.require: AR000BVDF8 AR000CQS39
* @tc.author: liuyuhui
*/
HWTEST_F(SingleKvStoreImplLogicalIsolationTest, LogicalIsolation007, TestSize.Level1)
{
    sptr<ISingleKvStore> kvStorePtr;
    sptr<ISingleKvStore> kvStorePtr1;
    sptr<ISingleKvStore> kvStorePtr2;

    Status status = g_kvStoreDataService->GetSingleKvStore(g_defaultOptions, g_appId, g_storeId,
                                          [&](sptr<ISingleKvStore> kvStore) { kvStorePtr = std::move(kvStore); });
    EXPECT_EQ(status, Status::SUCCESS) << "GetSingleKvStore return wrong status";

    std::thread thread1([&]() {
        Status status1 = g_kvStoreDataService->GetSingleKvStore(g_defaultOptions, g_appId1, g_storeId1,
                                               [&](sptr<ISingleKvStore> kvStore) { kvStorePtr1 = std::move(kvStore); });
        EXPECT_EQ(status1, Status::SUCCESS) << "GetSingleKvStore return wrong status";
    });

    std::thread thread2([&]() {
        Status status2 = g_kvStoreDataService->GetSingleKvStore(g_defaultOptions, g_appId2, g_storeId2,
                                               [&](sptr<ISingleKvStore> kvStore) { kvStorePtr2 = std::move(kvStore); });
        EXPECT_EQ(status2, Status::SUCCESS) << "GetSingleKvStore return wrong status";
    });

    thread1.join();
    thread2.join();

    EXPECT_NE(kvStorePtr, nullptr) << "GetSingleKvStore execute fail!!";
    EXPECT_NE(kvStorePtr1, nullptr) << "GetSingleKvStore execute fail!!";
    EXPECT_NE(kvStorePtr2, nullptr) << "GetSingleKvStore execute fail!!";
    EXPECT_NE(kvStorePtr, kvStorePtr1) << "Two SingleKvStoreImpl NE fail";
    EXPECT_NE(kvStorePtr, kvStorePtr2) << "Two SingleKvStoreImpl NE fail";
}
