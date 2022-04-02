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

#define LOG_TAG "DistributedKvDataManagerTest"

#include "distributed_kv_data_manager.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>
#include "kvstore_death_recipient.h"
#include "log_print.h"
#include "types.h"
using namespace testing::ext;
using namespace OHOS::DistributedKv;

class DistributedKvDataManagerTest : public testing::Test {
public:
    static DistributedKvDataManager manager;
    static Options create;
    static Options noCreate;

    static UserId userId;

    static AppId appId;
    static StoreId storeId64;
    static StoreId storeId65;
    static StoreId storeIdTest;
    static StoreId storeIdEmpty;

    static Entry entryA;
    static Entry entryB;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

    static void RemoveAllStore(DistributedKvDataManager manager);

    void SetUp();
    void TearDown();
    DistributedKvDataManagerTest();
};

class MyDeathRecipient : public KvStoreDeathRecipient {
public:
    MyDeathRecipient() {}
    virtual ~MyDeathRecipient() {}
    void OnRemoteDied() override {}
};

DistributedKvDataManager DistributedKvDataManagerTest::manager;
Options DistributedKvDataManagerTest::create;
Options DistributedKvDataManagerTest::noCreate;

UserId DistributedKvDataManagerTest::userId;

AppId DistributedKvDataManagerTest::appId;
StoreId DistributedKvDataManagerTest::storeId64;
StoreId DistributedKvDataManagerTest::storeId65;
StoreId DistributedKvDataManagerTest::storeIdTest;
StoreId DistributedKvDataManagerTest::storeIdEmpty;

Entry DistributedKvDataManagerTest::entryA;
Entry DistributedKvDataManagerTest::entryB;

void DistributedKvDataManagerTest::RemoveAllStore(DistributedKvDataManager manager)
{
    manager.CloseAllKvStore(appId);
    manager.DeleteAllKvStore(appId);
}
void DistributedKvDataManagerTest::SetUpTestCase(void)
{
    create.createIfMissing = true;
    create.encrypt = false;
    create.autoSync = true;
    create.kvStoreType = SINGLE_VERSION;

    noCreate.createIfMissing = false;
    noCreate.encrypt = false;
    noCreate.autoSync = true;
    noCreate.dataOwnership = true;
    noCreate.kvStoreType = SINGLE_VERSION;

    userId.userId = "account0";
    appId.appId = "com.ohos.kvdatamanager.test";

    storeId64.storeId = "a000000000b000000000c000000000d000000000e000000000f000000000g000";
    storeId65.storeId = "a000000000b000000000c000000000d000000000e000000000f000000000g000"
                        "a000000000b000000000c000000000d000000000e000000000f000000000g0000";
    storeIdTest.storeId = "test";
    storeIdEmpty.storeId = "";

    entryA.key = "a";
    entryA.value = "valueA";
    entryB.key = "b";
    entryB.value = "valueB";
    RemoveAllStore(manager);
}

void DistributedKvDataManagerTest::TearDownTestCase(void)
{
    RemoveAllStore(manager);
}

void DistributedKvDataManagerTest::SetUp(void)
{}

DistributedKvDataManagerTest::DistributedKvDataManagerTest(void)
{}

void DistributedKvDataManagerTest::TearDown(void)
{
    RemoveAllStore(manager);
}

/**
* @tc.name: GetKvStore001
* @tc.desc: Get an exist SingleKvStore
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, GetKvStore001, TestSize.Level1)
{
    ZLOGI("GetKvStore001 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore;
    Status status = manager.GetSingleKvStore(create, appId, storeId64, notExistKvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    EXPECT_NE(notExistKvStore, nullptr);

    std::shared_ptr<SingleKvStore> existKvStore;
    status = manager.GetSingleKvStore(noCreate, appId, storeId64, existKvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    EXPECT_NE(existKvStore, nullptr);
}

/**
* @tc.name: GetKvStore002
* @tc.desc: Create and get a new SingleKvStore
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, GetKvStore002, TestSize.Level1)
{
    ZLOGI("GetKvStore002 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore;
    Status status = manager.GetSingleKvStore(create, appId, storeId64, notExistKvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    EXPECT_NE(notExistKvStore, nullptr);
    manager.CloseKvStore(appId, storeId64);
    manager.DeleteKvStore(appId, storeId64);
}

/**
* @tc.name: GetKvStore003
* @tc.desc: Get a non-existing SingleKvStore, and the callback function should receive STORE_NOT_FOUND and
* get a nullptr.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, GetKvStore003, TestSize.Level1)
{
    ZLOGI("GetKvStore003 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore;
    Status status = manager.GetSingleKvStore(noCreate, appId, storeId64, notExistKvStore);
    ASSERT_EQ(status, Status::STORE_NOT_FOUND);
    EXPECT_EQ(notExistKvStore, nullptr);
}

/**
* @tc.name: GetKvStore004
* @tc.desc: Create a SingleKvStore with an empty storeId, and the callback function should receive
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, GetKvStore004, TestSize.Level1)
{
    ZLOGI("GetKvStore004 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore;
    Status status = manager.GetSingleKvStore(create, appId, storeIdEmpty, notExistKvStore);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);
    EXPECT_EQ(notExistKvStore, nullptr);
}

/**
* @tc.name: GetKvStore005
* @tc.desc: Get a SingleKvStore with an empty storeId, and the callback function should receive INVALID_ARGUMENT
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, GetKvStore005, TestSize.Level1)
{
    ZLOGI("GetKvStore005 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore;
    Status status = manager.GetSingleKvStore(noCreate, appId, storeIdEmpty, notExistKvStore);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);
    EXPECT_EQ(notExistKvStore, nullptr);
}

/**
* @tc.name: GetKvStore006
* @tc.desc: Create a SingleKvStore with a 65-byte storeId, and the callback function should receive INVALID_ARGUMENT
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, GetKvStore006, TestSize.Level1)
{
    ZLOGI("GetKvStore006 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore;
    Status status = manager.GetSingleKvStore(create, appId, storeId65, notExistKvStore);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);
    EXPECT_EQ(notExistKvStore, nullptr);
}

/**
* @tc.name: GetKvStore007
* @tc.desc: Get a SingleKvStore with a 65-byte storeId, the callback function should receive INVALID_ARGUMENT
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, GetKvStore007, TestSize.Level1)
{
    ZLOGI("GetKvStore007 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore;
    Status status = manager.GetSingleKvStore(noCreate, appId, storeId65, notExistKvStore);
    ASSERT_EQ(status, Status::INVALID_ARGUMENT);
    EXPECT_EQ(notExistKvStore, nullptr);
}

/**
* @tc.name: GetAllKvStore001
* @tc.desc: Get all KvStore IDs when no KvStore exists, and the callback function should receive a 0-length vector.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, GetAllKvStore001, TestSize.Level1)
{
    ZLOGI("GetAllKvStore001 begin.");
    std::vector<StoreId> storeIds;
    Status status = manager.GetAllKvStoreId(appId, storeIds);
    EXPECT_EQ(status, Status::SUCCESS);
    EXPECT_EQ(storeIds.size(), static_cast<size_t>(0));
}

/**
* @tc.name: GetAllKvStore002
* @tc.desc: Get all SingleKvStore IDs when no KvStore exists, and the callback function should receive a empty vector.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, GetAllKvStore002, TestSize.Level1)
{
    ZLOGI("GetAllKvStore002 begin.");
    StoreId id1;
    id1.storeId = "id1";
    StoreId id2;
    id2.storeId = "id2";
    StoreId id3;
    id3.storeId = "id3";
    std::shared_ptr<SingleKvStore> kvStore;
    Status status = manager.GetSingleKvStore(create, appId, id1, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status, Status::SUCCESS);
    status = manager.GetSingleKvStore(create, appId, id2, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status, Status::SUCCESS);
    status = manager.GetSingleKvStore(create, appId, id3, kvStore);
    ASSERT_NE(kvStore, nullptr);
    ASSERT_EQ(status, Status::SUCCESS);
    std::vector<StoreId> storeIds;
    status = manager.GetAllKvStoreId(appId, storeIds);
    EXPECT_EQ(status, Status::SUCCESS);
    bool haveId1 = false;
    bool haveId2 = false;
    bool haveId3 = false;
    for (StoreId &id : storeIds) {
        if (id.storeId == "id1") {
            haveId1 = true;
        } else if (id.storeId == "id2") {
            haveId2 = true;
        } else if (id.storeId == "id3") {
            haveId3 = true;
        } else {
            ZLOGI("got an unknown storeId.");
            EXPECT_TRUE(false);
        }
    }
    EXPECT_TRUE(haveId1);
    EXPECT_TRUE(haveId2);
    EXPECT_TRUE(haveId3);
    EXPECT_EQ(storeIds.size(), static_cast<size_t>(3));
}

/**
* @tc.name: CloseKvStore001
* @tc.desc: Close an opened KVStore, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, CloseKvStore001, TestSize.Level1)
{
    ZLOGI("CloseKvStore001 begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status status = manager.GetSingleKvStore(create, appId, storeId64, kvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);

    Status stat = manager.CloseKvStore(appId, storeId64);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: CloseKvStore002
* @tc.desc: Close a closed SingleKvStore, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, CloseKvStore002, TestSize.Level1)
{
    ZLOGI("CloseKvStore002 begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status status = manager.GetSingleKvStore(create, appId, storeId64, kvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);

    manager.CloseKvStore(appId, storeId64);
    Status stat = manager.CloseKvStore(appId, storeId64);
    EXPECT_EQ(stat, Status::STORE_NOT_OPEN);
}

/**
* @tc.name: CloseKvStore003
* @tc.desc: Close a SingleKvStore with an empty storeId, and the callback function should return INVALID_ARGUMENT.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, CloseKvStore003, TestSize.Level1)
{
    ZLOGI("CloseKvStore003 begin.");
    Status stat = manager.CloseKvStore(appId, storeIdEmpty);
    EXPECT_EQ(stat, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: CloseKvStore004
* @tc.desc: Close a SingleKvStore with a 65-byte storeId, and the callback function should return INVALID_ARGUMENT.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, CloseKvStore004, TestSize.Level1)
{
    ZLOGI("CloseKvStore004 begin.");
    Status stat = manager.CloseKvStore(appId, storeId65);
    EXPECT_EQ(stat, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: CloseKvStore005
* @tc.desc: Close a non-existing SingleKvStore, and the callback function should return STORE_NOT_OPEN.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, CloseKvStore005, TestSize.Level1)
{
    ZLOGI("CloseKvStore005 begin.");
    Status stat = manager.CloseKvStore(appId, storeId64);
    EXPECT_EQ(stat, Status::STORE_NOT_OPEN);
}

/**
* @tc.name: CloseKvStoreMulti001
* @tc.desc: Open a SingleKvStore several times and close them one by one.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000CSKRU
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, CloseKvStoreMulti001, TestSize.Level1)
{
    ZLOGI("CloseKvStoreMulti001 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore;
    Status status = manager.GetSingleKvStore(create, appId, storeId64, notExistKvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    EXPECT_NE(notExistKvStore, nullptr);

    std::shared_ptr<SingleKvStore> existKvStore;
    manager.GetSingleKvStore(noCreate, appId, storeId64, existKvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    EXPECT_NE(existKvStore, nullptr);

    Status stat = manager.CloseKvStore(appId, storeId64);
    EXPECT_EQ(stat, Status::SUCCESS);

    stat = manager.CloseKvStore(appId, storeId64);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: CloseKvStoreMulti002
* @tc.desc: Open a SingleKvStore several times and close them one by one.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000CSKRU
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, CloseKvStoreMulti002, TestSize.Level1)
{
    ZLOGI("CloseKvStoreMulti002 begin.");
    std::shared_ptr<SingleKvStore> notExistKvStore;
    Status status = manager.GetSingleKvStore(create, appId, storeId64, notExistKvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    EXPECT_NE(notExistKvStore, nullptr);

    std::shared_ptr<SingleKvStore> existKvStore1;
    status = manager.GetSingleKvStore(noCreate, appId, storeId64, existKvStore1);
    ASSERT_EQ(status, Status::SUCCESS);
    EXPECT_NE(existKvStore1, nullptr);

    std::shared_ptr<SingleKvStore> existKvStore2;
    status = manager.GetSingleKvStore(noCreate, appId, storeId64, existKvStore2);
    ASSERT_EQ(status, Status::SUCCESS);
    EXPECT_NE(existKvStore2, nullptr);

    Status stat = manager.CloseKvStore(appId, storeId64);
    EXPECT_EQ(stat, Status::SUCCESS);

    stat = manager.CloseKvStore(appId, storeId64);
    EXPECT_EQ(stat, Status::SUCCESS);

    stat = manager.CloseKvStore(appId, storeId64);
    EXPECT_EQ(stat, Status::SUCCESS);

    stat = manager.CloseKvStore(appId, storeId64);
    EXPECT_NE(stat, Status::SUCCESS);
}

/**
* @tc.name: CloseAllKvStore001
* @tc.desc: Close all opened KvStores, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, CloseAllKvStore001, TestSize.Level1)
{
    ZLOGI("CloseAllKvStore001 begin.");
    std::shared_ptr<SingleKvStore> kvStore1;
    Status status = manager.GetSingleKvStore(create, appId, storeId64, kvStore1);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore1, nullptr);

    std::shared_ptr<SingleKvStore> kvStore2;
    status = manager.GetSingleKvStore(create, appId, storeIdTest, kvStore2);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore2, nullptr);

    Status stat = manager.CloseAllKvStore(appId);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: CloseAllKvStore002
* @tc.desc: Close all KvStores which exist but are not opened, and the callback function should return STORE_NOT_OPEN.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, CloseAllKvStore002, TestSize.Level1)
{
    ZLOGI("CloseAllKvStore002 begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status status = manager.GetSingleKvStore(create, appId, storeId64, kvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);

    std::shared_ptr<SingleKvStore> kvStore2;
    status = manager.GetSingleKvStore(create, appId, storeIdTest, kvStore2);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore2, nullptr);

    Status stat = manager.CloseKvStore(appId, storeId64);
    EXPECT_EQ(stat, Status::SUCCESS);

    stat = manager.CloseAllKvStore(appId);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: DeleteKvStore001
* @tc.desc: Delete a closed KvStore, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, DeleteKvStore001, TestSize.Level1)
{
    ZLOGI("DeleteKvStore001 begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status status = manager.GetSingleKvStore(create, appId, storeId64, kvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);

    Status stat = manager.CloseKvStore(appId, storeId64);
    ASSERT_EQ(stat, Status::SUCCESS);

    stat = manager.DeleteKvStore(appId, storeId64);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: DeleteKvStore002
* @tc.desc: Delete an opened SingleKvStore, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, DeleteKvStore002, TestSize.Level1)
{
    ZLOGI("DeleteKvStore002 begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status status = manager.GetSingleKvStore(create, appId, storeId64, kvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);

    // first close it if opened, and then delete it.
    Status stat = manager.DeleteKvStore(appId, storeId64);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: DeleteKvStore003
* @tc.desc: Delete a non-existing KvStore, and the callback function should return DB_ERROR.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, DeleteKvStore003, TestSize.Level1)
{
    ZLOGI("DeleteKvStore003 begin.");
    Status stat = manager.DeleteKvStore(appId, storeId64);
    EXPECT_EQ(stat, Status::DB_ERROR);
}

/**
* @tc.name: DeleteKvStore004
* @tc.desc: Delete a KvStore with an empty storeId, and the callback function should return INVALID_ARGUMENT.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, DeleteKvStore004, TestSize.Level1)
{
    ZLOGI("DeleteKvStore004 begin.");
    Status stat = manager.DeleteKvStore(appId, storeIdEmpty);
    EXPECT_EQ(stat, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: DeleteKvStore005
* @tc.desc: Delete a KvStore with 65 bytes long storeId (which exceed storeId length limit). Should
* return INVALID_ARGUMENT.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, DeleteKvStore005, TestSize.Level1)
{
    ZLOGI("DeleteKvStore005 begin.");
    Status stat = manager.DeleteKvStore(appId, storeId65);
    EXPECT_EQ(stat, Status::INVALID_ARGUMENT);
}

/**
* @tc.name: DeleteAllKvStore001
* @tc.desc: Delete all KvStores after closing all of them, and the callback function should return SUCCESS.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, DeleteAllKvStore001, TestSize.Level1)
{
    ZLOGI("DeleteAllKvStore001 begin.");
    std::shared_ptr<SingleKvStore> kvStore1;
    Status status = manager.GetSingleKvStore(create, appId, storeId64, kvStore1);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore1, nullptr);
    std::shared_ptr<SingleKvStore> kvStore2;
    status = manager.GetSingleKvStore(create, appId, storeIdTest, kvStore2);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore2, nullptr);
    Status stat = manager.CloseKvStore(appId, storeId64);
    EXPECT_EQ(stat, Status::SUCCESS);
    stat = manager.CloseKvStore(appId, storeIdTest);
    EXPECT_EQ(stat, Status::SUCCESS);

    stat = manager.DeleteAllKvStore(appId);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: DeleteAllKvStore002
* @tc.desc: Delete all kvstore fail when any kvstore in the appId is not closed
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, DeleteAllKvStore002, TestSize.Level1)
{
    ZLOGI("DeleteAllKvStore002 begin.");
    std::shared_ptr<SingleKvStore> kvStore1;
    Status status = manager.GetSingleKvStore(create, appId, storeId64, kvStore1);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore1, nullptr);
    std::shared_ptr<SingleKvStore> kvStore2;
    status = manager.GetSingleKvStore(create, appId, storeIdTest, kvStore2);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore2, nullptr);
    Status stat = manager.CloseKvStore(appId, storeId64);
    EXPECT_EQ(stat, Status::SUCCESS);

    stat = manager.DeleteAllKvStore(appId);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: DeleteAllKvStore003
* @tc.desc: Delete all KvStores even if no KvStore exists in the appId.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000BVTDM
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, DeleteAllKvStore003, TestSize.Level1)
{
    ZLOGI("DeleteAllKvStore003 begin.");
    Status stat = manager.DeleteAllKvStore(appId);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: DeleteAllKvStore004
* @tc.desc: when delete the last active kvstore, the system will remove the app manager scene
* @tc.type: FUNC
* @tc.require: bugs
* @tc.author: Sven Wang
*/
HWTEST_F(DistributedKvDataManagerTest, DeleteAllKvStore004, TestSize.Level1)
{
    ZLOGI("DeleteAllKvStore004 begin.");
    std::shared_ptr<SingleKvStore> kvStore1;
    Status status = manager.GetSingleKvStore(create, appId, storeId64, kvStore1);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore1, nullptr);
    std::shared_ptr<SingleKvStore> kvStore2;
    status = manager.GetSingleKvStore(create, appId, storeIdTest, kvStore2);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore2, nullptr);
    Status stat = manager.CloseKvStore(appId, storeId64);
    EXPECT_EQ(stat, Status::SUCCESS);
    stat = manager.CloseKvStore(appId, storeIdTest);
    EXPECT_EQ(stat, Status::SUCCESS);
    stat = manager.DeleteKvStore(appId, storeIdTest);
    EXPECT_EQ(stat, Status::SUCCESS);
    stat = manager.DeleteAllKvStore(appId);
    EXPECT_EQ(stat, Status::SUCCESS);
}

/**
* @tc.name: RegisterKvStoreServiceDeathRecipient001
* @tc.desc: Register a callback called when the service dies.
* @tc.type: FUNC
* @tc.require: SR000CQDU0 AR000CQDU1
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, RegisterKvStoreServiceDeathRecipient001, TestSize.Level1)
{
    ZLOGI("RegisterKvStoreServiceDeathRecipient001 begin.");
    std::shared_ptr<KvStoreDeathRecipient> kvStoreDeathRecipient = std::make_shared<MyDeathRecipient>();
    manager.RegisterKvStoreServiceDeathRecipient(kvStoreDeathRecipient);
    kvStoreDeathRecipient->OnRemoteDied();
}

/**
* @tc.name: UnRegisterKvStoreServiceDeathRecipient001
* @tc.desc: Unregister the callback called when the service dies.
* @tc.type: FUNC
* @tc.require: AR000CQDUS AR000CQDU1
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerTest, UnRegisterKvStoreServiceDeathRecipient001, TestSize.Level1)
{
    ZLOGI("UnRegisterKvStoreServiceDeathRecipient001 begin.");
    std::shared_ptr<KvStoreDeathRecipient> kvStoreDeathRecipientPtr = std::make_shared<MyDeathRecipient>();
    manager.UnRegisterKvStoreServiceDeathRecipient(kvStoreDeathRecipientPtr);
}

class DeviceListenerImpl : public DeviceStatusChangeListener {
public:
    void OnDeviceChanged(const DeviceInfo &info, const DeviceChangeType &type) const override
    {
    }
    DeviceFilterStrategy GetFilterStrategy() const override
    {
        return DeviceFilterStrategy::NO_FILTER;
    }
};
/**
* @tc.name: GetDevice001
* @tc.desc: Get device id.
* @tc.type: FUNC
* @tc.require: SR000DOH1R AR000DPSGU
* @tc.author: hongbo
*/
HWTEST_F(DistributedKvDataManagerTest, GetDevice001, TestSize.Level1)
{
    ZLOGI("GetDevice001 begin.");
    DeviceInfo info;
    Status status = manager.GetLocalDevice(info);
    EXPECT_EQ(Status::SUCCESS, status) << "expected getLocalDevice true";
    EXPECT_TRUE(info.deviceId.size() > 0) << "expected deviceId exist";

    std::vector<DeviceInfo> infos;
    status = manager.GetDeviceList(infos, DeviceFilterStrategy::FILTER);
    auto listener = std::make_shared<DeviceListenerImpl>();
    status = manager.StartWatchDeviceChange(listener);
    EXPECT_EQ(Status::SUCCESS, status) << "expected StartWatchDeviceChange true";
    status = manager.StopWatchDeviceChange(listener);
    EXPECT_EQ(Status::SUCCESS, status) << "expected StopWatchDeviceChange true";
}
