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
#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include "ut_kvstore_nb_delegate_impl.h"
#include <kvstore_data_service.h>
#include "kvstore_sync_manager.h"
#include "log_print.h"
#include "gtest/gtest.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS;
using DistributedDB::UtKvStoreNbDelegateImpl;

class KvStoreSyncManagerTest : public testing::Test {
public:
    class Waiter {
    public:
        bool GetValue(uint32_t interval = CHECK_WAITING_TIME);
        bool SetValue(bool value);
    private:
        std::mutex mutex_;
        std::condition_variable cv_;
        bool isFinished = false;
    };
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static const uint32_t CHECK_WAITING_TIME = 250; // 250ms

    void CreateKvStorePair(bool isAutoSync, const std::string &storeId,
        std::shared_ptr<SingleKvStoreImpl> &kvStore, std::shared_ptr<UtKvStoreNbDelegateImpl> &kvNb,
        std::shared_ptr<SingleKvStoreImpl> &kvStore2, std::shared_ptr<UtKvStoreNbDelegateImpl> &kvNb2);
protected:
    uint64_t seqId_ = 0;
};

bool KvStoreSyncManagerTest::Waiter::GetValue(uint32_t interval)
{
    std::unique_lock<decltype(mutex_)> lg(mutex_);
    if (isFinished) {
        return isFinished;
    }
    KvStoreSyncManager::GetInstance()->AddSyncOperation(uintptr_t(this), interval,
        [this](const auto &end)->Status {
            SetValue(true);
            return SUCCESS;
        }, nullptr);
    cv_.wait(lg, [this] { return isFinished; });
    return isFinished;
}

bool KvStoreSyncManagerTest::Waiter::SetValue(bool value)
{
    std::unique_lock<decltype(mutex_)> lg(mutex_);
    isFinished = value;
    cv_.notify_one();
    return isFinished;
}

void KvStoreSyncManagerTest::SetUpTestCase(void)
{}

void KvStoreSyncManagerTest::TearDownTestCase(void)
{}

void KvStoreSyncManagerTest::SetUp(void)
{}

void KvStoreSyncManagerTest::TearDown(void)
{}

void KvStoreSyncManagerTest::CreateKvStorePair(bool isAutoSync, const std::string &storeId,
                                               std::shared_ptr<SingleKvStoreImpl> &kvStore,
                                               std::shared_ptr<UtKvStoreNbDelegateImpl> &kvNb,
                                               std::shared_ptr<SingleKvStoreImpl> &kvStore2,
                                               std::shared_ptr<UtKvStoreNbDelegateImpl> &kvNb2)
{
    kvNb = std::make_unique<UtKvStoreNbDelegateImpl>(storeId, "DevA");
    kvNb2 = std::make_unique<UtKvStoreNbDelegateImpl>(storeId, "DevB");
    kvNb->SetNeighbor(kvNb2);
    kvNb2->SetNeighbor(kvNb);
    kvNb->SetAutoSync(isAutoSync);
    kvNb2->SetAutoSync(isAutoSync);

    Options options = { .autoSync = isAutoSync };
    std::string userId = "syncManagerTest";
    std::string appId = "syncTest";
    std::string appDir = "syncTest";
    kvStore = std::make_unique<SingleKvStoreImpl>(options, userId, appId, storeId, appId, appDir, kvNb.get());
    kvStore2 = std::make_unique<SingleKvStoreImpl>(options, userId, appId, storeId, appId, appDir, kvNb2.get());
    return;
}

/**
* @tc.name: KvStoreSyncManagerTest001
* @tc.desc: realtime sync
* @tc.type: FUNC
* @tc.require: SR000DOGQE AR000DPUAN
* @tc.author: wangtao
*/
HWTEST_F(KvStoreSyncManagerTest, KvStoreSyncManagerTest001, TestSize.Level1)
{
    std::shared_ptr<SingleKvStoreImpl> kvStore;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb;
    std::shared_ptr<SingleKvStoreImpl> kvStore2;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb2;
    CreateKvStorePair(false, "syncTest", kvStore, kvNb, kvStore2, kvNb2);

    std::vector<std::string> syncDevices;
    syncDevices.push_back("devB");
    uint32_t syncDelayMs = 0; // ms
    Waiter waiter;
    Key key1("key1");
    Value value1("value1");
    Value value1Tmp;
    kvStore->Put(key1, value1);
    waiter.SetValue(false);
    kvStore->Sync(syncDevices, SyncMode::PUSH, syncDelayMs, ++seqId_);
    waiter.GetValue();
    kvStore2->Get(key1, value1Tmp);
    EXPECT_EQ(value1.ToString(), value1Tmp.ToString());

    Key key2("key2");
    Value value2("value2");
    Value value2Tmp;
    kvStore->Put(key2, value2);
    waiter.SetValue(false);
    kvStore->Sync(syncDevices, SyncMode::PUSH, syncDelayMs, ++seqId_);
    waiter.GetValue();
    kvStore2->Get(key2, value2Tmp);
    EXPECT_EQ(value2.ToString(), value2Tmp.ToString());

    kvStore->Delete(key2);
    waiter.SetValue(false);
    kvStore->Sync(syncDevices, SyncMode::PUSH, syncDelayMs, ++seqId_);
    waiter.GetValue();
    Status ret = kvStore2->Get(key2, value2Tmp);
    EXPECT_EQ(ret, Status::KEY_NOT_FOUND);

    ret = kvStore2->Get(key1, value1Tmp);
    EXPECT_EQ(ret, Status::SUCCESS);
    EXPECT_EQ(value1.ToString(), value1Tmp.ToString());
}

/**
* @tc.name: KvStoreSyncManagerTest002
* @tc.desc: merge delayed sync
* @tc.type: FUNC
* @tc.require: SR000DOGQE AR000DPUAN
* @tc.author: wangtao
*/
HWTEST_F(KvStoreSyncManagerTest, KvStoreSyncManagerTest002, TestSize.Level1)
{
    std::shared_ptr<SingleKvStoreImpl> kvStore;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb;
    std::shared_ptr<SingleKvStoreImpl> kvStore2;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb2;
    CreateKvStorePair(false, "syncTest", kvStore, kvNb, kvStore2, kvNb2);

    std::vector<std::string> syncDevices;
    syncDevices.push_back("devB");
    uint32_t syncDelayMs = 200; // ms

    Waiter waiter;
    Key key1("key1");
    Value value1("value1");
    Value value1Tmp;
    kvStore->Put(key1, value1);
    waiter.SetValue(false);
    kvStore->Sync(syncDevices, SyncMode::PUSH, syncDelayMs, ++seqId_);
    waiter.GetValue();
    Status ret = kvStore2->Get(key1, value1Tmp);
    EXPECT_EQ(ret, Status::SUCCESS);

    Key key2("key2");
    Value value2("value2");
    Value value2Tmp;
    kvStore->Put(key2, value2);
    waiter.SetValue(false);
    kvStore->Sync(syncDevices, SyncMode::PUSH, syncDelayMs, ++seqId_);
    waiter.GetValue();
    ret = kvStore2->Get(key2, value2Tmp);
    EXPECT_EQ(ret, Status::SUCCESS);

    kvStore->Delete(key2);
    waiter.SetValue(false);
    kvStore->Sync(syncDevices, SyncMode::PUSH, syncDelayMs, ++seqId_);
    ret = kvStore2->Get(key2, value2Tmp);
    EXPECT_EQ(ret, Status::SUCCESS);

    waiter.GetValue();
    ret = kvStore2->Get("key1", value1Tmp);
    EXPECT_EQ(ret, Status::SUCCESS);
    EXPECT_EQ(value1.ToString(), value1Tmp.ToString());
    ret = kvStore2->Get(key2, value2Tmp);
    EXPECT_EQ(ret, Status::KEY_NOT_FOUND);
}

/**
* @tc.name: KvStoreSyncManagerTest003
* @tc.desc: auto realtime sync
* @tc.type: FUNC
* @tc.require: SR000DOGQE AR000DPUAN
* @tc.author: wangtao
*/
HWTEST_F(KvStoreSyncManagerTest, KvStoreSyncManagerTest003, TestSize.Level1)
{
    std::shared_ptr<SingleKvStoreImpl> kvStore;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb;
    std::shared_ptr<SingleKvStoreImpl> kvStore2;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb2;
    CreateKvStorePair(true, "syncTest", kvStore, kvNb, kvStore2, kvNb2);
    KvSyncParam syncDelay = { 0 }; // ms
    sptr<KvParam> output;
    kvStore->Control(KvControlCmd::SET_SYNC_PARAM, TransferTypeToByteArray<KvSyncParam>(syncDelay), output);

    Key key1("key1");
    Value value1("value1");
    Value value1Tmp;
    kvStore->Put(key1, value1);
    kvStore2->Get(key1, value1Tmp);
    EXPECT_EQ(value1.ToString(), value1Tmp.ToString());

    Key key2("key2");
    Value value2("value2");
    Value value2Tmp;
    kvStore->Put(key2, value2);
    kvStore2->Get(key2, value2Tmp);
    EXPECT_EQ(value2.ToString(), value2Tmp.ToString());

    kvStore->Delete(key2);
    Status ret = kvStore2->Get(key2, value2Tmp);
    EXPECT_EQ(ret, Status::KEY_NOT_FOUND);

    ret = kvStore2->Get(key1, value1Tmp);
    EXPECT_EQ(ret, Status::SUCCESS);
    EXPECT_EQ(value1.ToString(), value1Tmp.ToString());
}

/**
* @tc.name: KvStoreSyncManagerTest004
* @tc.desc: merge delayed auto sync
* @tc.type: FUNC
* @tc.require: SR000DOGQE AR000DPUAN
* @tc.author: wangtao
*/
HWTEST_F(KvStoreSyncManagerTest, KvStoreSyncManagerTest004, TestSize.Level1)
{
    std::shared_ptr<SingleKvStoreImpl> kvStore;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb;
    std::shared_ptr<SingleKvStoreImpl> kvStore2;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb2;
    CreateKvStorePair(true, "syncTest", kvStore, kvNb, kvStore2, kvNb2);
    KvSyncParam syncDelay = { 200 }; // ms
    sptr<KvParam> output;
    kvStore->Control(KvControlCmd::SET_SYNC_PARAM, TransferTypeToByteArray<KvSyncParam>(syncDelay), output);

    Key key1("key1");
    Value value1("value1");
    Value value1Tmp;
    kvStore->Put(key1, value1);
    Status ret = kvStore2->Get(key1, value1Tmp);
    EXPECT_EQ(ret, Status::SUCCESS);

    Key key2("key2");
    Value value2("value2");
    Value value2Tmp;
    kvStore->Put(key2, value2);
    ret = kvStore2->Get(key2, value2Tmp);
    EXPECT_EQ(ret, Status::SUCCESS);

    kvStore->Delete(key2);
    ret = kvStore2->Get(key2, value2Tmp);
    EXPECT_EQ(ret, Status::KEY_NOT_FOUND);
    ret = kvStore2->Get(key1, value1Tmp);
    EXPECT_EQ(ret, Status::SUCCESS);
    EXPECT_EQ(value1.ToString(), value1Tmp.ToString());
    ret = kvStore2->Get(key2, value2Tmp);
    EXPECT_EQ(ret, Status::KEY_NOT_FOUND);
}

/**
* @tc.name: KvStoreSyncManagerTest005
* @tc.desc: realtime sync first
* @tc.type: FUNC
* @tc.require: SR000DOGQE AR000DPUAN
* @tc.author: wangtao
*/
HWTEST_F(KvStoreSyncManagerTest, KvStoreSyncManagerTest005, TestSize.Level1)
{
    std::shared_ptr<SingleKvStoreImpl> kvStore;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb;
    std::shared_ptr<SingleKvStoreImpl> kvStore2;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb2;
    CreateKvStorePair(false, "syncTest", kvStore, kvNb, kvStore2, kvNb2);

    std::shared_ptr<SingleKvStoreImpl> kvStore3;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb3;
    std::shared_ptr<SingleKvStoreImpl> kvStore4;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb4;
    CreateKvStorePair(false, "syncTest3", kvStore3, kvNb3, kvStore4, kvNb4);

    std::vector<std::string> syncDevices;
    syncDevices.push_back("devB");
    uint32_t sync1DelayMs = 0; // ms
    uint32_t sync3DelayMs = 100; // ms

    Waiter waiter;
    Key key1("key1");
    Value value1("value1");
    Value value1Tmp;
    waiter.SetValue(false);
    kvStore3->Put(key1, value1);
    kvStore3->Sync(syncDevices, SyncMode::PUSH, sync3DelayMs, ++seqId_);
    kvStore->Put(key1, value1);
    kvStore->Sync(syncDevices, SyncMode::PUSH, sync1DelayMs, ++seqId_);
    waiter.GetValue(50);
    waiter.SetValue(false);
    Status ret = kvStore2->Get(key1, value1Tmp);
    EXPECT_EQ(ret, Status::SUCCESS);
    EXPECT_EQ(value1.ToString(), value1Tmp.ToString());
    ret = kvStore4->Get(key1, value1Tmp);
    EXPECT_EQ(ret, Status::KEY_NOT_FOUND);

    waiter.GetValue(sync3DelayMs);
    ret = kvStore4->Get(key1, value1Tmp);
    EXPECT_EQ(ret, Status::SUCCESS);
    EXPECT_EQ(value1.ToString(), value1Tmp.ToString());
}

/**
* @tc.name: KvStoreSyncManagerTest006
* @tc.desc: merge multiple sync
* @tc.type: FUNC
* @tc.require: SR000DOGQE AR000DPUAN
* @tc.author: wangtao
*/
HWTEST_F(KvStoreSyncManagerTest, KvStoreSyncManagerTest006, TestSize.Level1)
{
    std::shared_ptr<SingleKvStoreImpl> kvStore;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb;
    std::shared_ptr<SingleKvStoreImpl> kvStore2;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb2;
    CreateKvStorePair(true, "syncTest", kvStore, kvNb, kvStore2, kvNb2);
    KvSyncParam syncDelay = { 1000 }; // ms
    sptr<KvParam> output;
    kvStore->Control(KvControlCmd::SET_SYNC_PARAM, TransferTypeToByteArray<KvSyncParam>(syncDelay), output);

    uint32_t batch = 200;   // check count
    uint32_t count = 1000;  // entries
    std::string prefix = "keyTest";
    std::vector<uint8_t> data(2000); // 2k bytes
    for (uint32_t i = 0; i < count; i++) {
        Key key(prefix + std::to_string(i));
        data[0] = static_cast<uint8_t>(i);
        Value value(data);
        kvStore->Put(key, value);

        if ((i % batch) == (batch - 1)) {
            std::vector<Entry> entries;
            Status ret1 = kvStore2->GetEntries(prefix, entries);
            EXPECT_EQ(ret1, Status::SUCCESS);
        }
    }

    std::vector<Entry> entries;
    Status ret = kvStore2->GetEntries(prefix, entries);
    EXPECT_EQ(ret, Status::SUCCESS);
    EXPECT_EQ(entries.size(), count);
}

/**
* @tc.name: KvStoreSyncManagerTest007
* @tc.desc: if realtime sync timeout, do not blocking delayed sync long time
* @tc.type: FUNC
* @tc.require: SR000DOGQE AR000DPUAN
* @tc.author: wangtao
*/
HWTEST_F(KvStoreSyncManagerTest, KvStoreSyncManagerTest007, TestSize.Level1)
{
    std::shared_ptr<SingleKvStoreImpl> kvStore;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb;
    std::shared_ptr<SingleKvStoreImpl> kvStore2;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb2;
    CreateKvStorePair(false, "syncTest", kvStore, kvNb, kvStore2, kvNb2);

    std::shared_ptr<SingleKvStoreImpl> kvStore3;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb3;
    std::shared_ptr<SingleKvStoreImpl> kvStore4;
    std::shared_ptr<UtKvStoreNbDelegateImpl> kvNb4;
    CreateKvStorePair(false, "syncTest3", kvStore3, kvNb3, kvStore4, kvNb4);

    std::vector<std::string> syncDevices;
    syncDevices.push_back("devB");
    uint32_t sync1DelayMs = 0; // ms
    uint32_t sync3DelayMs = 200; // ms

    Waiter waiter;
    Key key1("key1");
    Value value1("value1");
    Value value1Tmp;
    waiter.SetValue(false);
    kvStore->Put(key1, value1);
    kvNb->SetSyncStatus(DistributedDB::TIME_OUT);
    kvStore->Sync(syncDevices, SyncMode::PUSH, sync1DelayMs, ++seqId_);
    kvStore3->Put(key1, value1);
    kvStore3->Sync(syncDevices, SyncMode::PUSH, sync3DelayMs, ++seqId_);

    waiter.GetValue(50);
    Status ret = kvStore2->Get(key1, value1Tmp);
    EXPECT_EQ(ret, Status::KEY_NOT_FOUND);
    ret = kvStore4->Get(key1, value1Tmp);
    EXPECT_EQ(ret, Status::KEY_NOT_FOUND);
    waiter.SetValue(false);
    waiter.GetValue(sync3DelayMs);
    ret = kvStore4->Get(key1, value1Tmp);
    EXPECT_EQ(ret, Status::KEY_NOT_FOUND);

    usleep(300 * 1000);
    ret = kvStore2->Get(key1, value1Tmp);
    EXPECT_EQ(ret, Status::KEY_NOT_FOUND);
    ret = kvStore4->Get(key1, value1Tmp);
    EXPECT_EQ(ret, Status::SUCCESS);
    EXPECT_EQ(value1.ToString(), value1Tmp.ToString());
}
