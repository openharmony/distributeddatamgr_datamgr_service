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
#define LOG_TAG "DistributedKvDataManagerEncryptTest"
#include <gtest/gtest.h>

#include "distributed_kv_data_manager.h"
#include "kvstore_death_recipient.h"
#include "log_print.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;

class DistributedKvDataManagerEncryptTest : public testing::Test {
public:
    static DistributedKvDataManager manager;
    static Options createEnc;

    static UserId userId;

    static AppId appId;
    static StoreId storeId;

    static void SetUpTestCase(void);
    static void TearDownTestCase(void);

    static void RemoveAllStore(DistributedKvDataManager manager);

    void SetUp();
    void TearDown();
    DistributedKvDataManagerEncryptTest();
    virtual ~DistributedKvDataManagerEncryptTest();
};

class MyDeathRecipient : public KvStoreDeathRecipient {
public:
    MyDeathRecipient() {}
    virtual ~MyDeathRecipient() {}
    void OnRemoteDied() override {}
};

DistributedKvDataManager DistributedKvDataManagerEncryptTest::manager;
Options DistributedKvDataManagerEncryptTest::createEnc;

UserId DistributedKvDataManagerEncryptTest::userId;

AppId DistributedKvDataManagerEncryptTest::appId;
StoreId DistributedKvDataManagerEncryptTest::storeId;

void DistributedKvDataManagerEncryptTest::RemoveAllStore(DistributedKvDataManager manager)
{
    manager.CloseAllKvStore(appId);
    manager.DeleteKvStore(appId, storeId, createEnc.baseDir);
    manager.DeleteAllKvStore(appId, createEnc.baseDir);
}
void DistributedKvDataManagerEncryptTest::SetUpTestCase(void)
{
    createEnc.createIfMissing = true;
    createEnc.encrypt = true;
    createEnc.autoSync = true;
    createEnc.kvStoreType = SINGLE_VERSION;

    userId.userId = "account0";
    appId.appId = "com.ohos.nb.service";

    storeId.storeId = "EncryptStoreId";

    createEnc.area = EL1;
    createEnc.baseDir = std::string("/data/service/el1/public/database/") + appId.appId;
    mkdir(createEnc.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void DistributedKvDataManagerEncryptTest::TearDownTestCase(void)
{
    RemoveAllStore(manager);
    (void)remove((createEnc.baseDir + "/kvdb").c_str());
    (void)remove(createEnc.baseDir.c_str());
}

void DistributedKvDataManagerEncryptTest::SetUp(void)
{}

DistributedKvDataManagerEncryptTest::DistributedKvDataManagerEncryptTest(void)
{}

DistributedKvDataManagerEncryptTest::~DistributedKvDataManagerEncryptTest(void)
{}

void DistributedKvDataManagerEncryptTest::TearDown(void)
{}

/**
* @tc.name: kvstore_ddm_createEncryptedStore_001
* @tc.desc: Create an encrypted KvStore.
* @tc.type: FUNC
* @tc.require: SR000D08K4 AR000D08KQ
* @tc.author: liqiao
*/
HWTEST_F(DistributedKvDataManagerEncryptTest, kvstore_ddm_createEncryptedStore_001, TestSize.Level1)
{
    ZLOGI("kvstore_ddm_createEncryptedStore_001 begin.");
    std::shared_ptr<SingleKvStore> kvStore;
    Status status = manager.GetSingleKvStore(createEnc, appId, storeId, kvStore);
    ASSERT_EQ(status, Status::SUCCESS);
    ASSERT_NE(kvStore, nullptr);

    Key key = "age";
    Value value = "18";
    status = kvStore->Put(key, value);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";

    // get value from kvstore.
    Value valueRet;
    Status statusRet = kvStore->Get(key, valueRet);
    EXPECT_EQ(Status::SUCCESS, statusRet) << "get data return wrong status";

    EXPECT_EQ(value, valueRet) << "value and valueRet are not equal";
}
