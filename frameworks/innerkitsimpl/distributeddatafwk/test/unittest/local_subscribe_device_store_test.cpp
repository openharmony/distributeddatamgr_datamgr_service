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

#define LOG_TAG "LocalSubscribeDeviceStoreTest"
#include <cstdint>
#include <gtest/gtest.h>
#include <unistd.h>
#include <vector>

#include "distributed_kv_data_manager.h"
#include "log_print.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;
namespace {
const int USLEEP_TIME = 2000000;
}
class LocalSubscribeDeviceStoreTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static DistributedKvDataManager manager_;
    static std::shared_ptr<SingleKvStore> kvStore_;
    static Status status_;
    static AppId appId_;
    static StoreId storeId_;
    static int usleepTime_;
};
std::shared_ptr<SingleKvStore> LocalSubscribeDeviceStoreTest::kvStore_ = nullptr;
Status LocalSubscribeDeviceStoreTest::status_ = Status::ERROR;
DistributedKvDataManager LocalSubscribeDeviceStoreTest::manager_;
AppId LocalSubscribeDeviceStoreTest::appId_;
StoreId LocalSubscribeDeviceStoreTest::storeId_;

void LocalSubscribeDeviceStoreTest::SetUpTestCase(void)
{
    mkdir("/data/service/el1/public/database/dev_local_sub", (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void LocalSubscribeDeviceStoreTest::TearDownTestCase(void)
{
    manager_.CloseKvStore(appId_, kvStore_);
    kvStore_ = nullptr;
    manager_.DeleteKvStore(appId_, storeId_, "/data/service/el1/public/database/dev_local_sub");
    (void)remove("/data/service/el1/public/database/dev_local_sub/kvdb");
    (void)remove("/data/service/el1/public/database/dev_local_sub");
}

void LocalSubscribeDeviceStoreTest::SetUp(void)
{
    Options options;
    options.baseDir = std::string("/data/service/el1/public/database/dev_local_sub");
    appId_.appId = "dev_local_sub"; // define app name.
    storeId_.storeId = "student";   // define kvstore(database) name
    manager_.DeleteKvStore(appId_, storeId_, options.baseDir);
    // [create and] open and initialize kvstore instance.
    status_ = manager_.GetSingleKvStore(options, appId_, storeId_, kvStore_);
    EXPECT_EQ(Status::SUCCESS, status_) << "wrong status";
    EXPECT_NE(nullptr, kvStore_) << "kvStore is nullptr";
}

void LocalSubscribeDeviceStoreTest::TearDown(void)
{
    manager_.CloseKvStore(appId_, kvStore_);
    kvStore_ = nullptr;
    manager_.DeleteKvStore(appId_, storeId_);
}

class DeviceObserverTest : public KvStoreObserver {
public:
    std::vector<Entry> insertEntries_;
    std::vector<Entry> updateEntries_;
    std::vector<Entry> deleteEntries_;
    std::string deviceId_;
    bool isClear_;
    DeviceObserverTest();
    ~DeviceObserverTest() = default;

    void OnChange(const ChangeNotification &changeNotification);

    // reset the callCount_ to zero.
    void ResetToZero();

    uint64_t GetCallCount() const;

private:
    uint64_t callCount_;
};

DeviceObserverTest::DeviceObserverTest()
{
    callCount_ = 0;
    insertEntries_ = {};
    updateEntries_ = {};
    deleteEntries_ = {};
    isClear_ = false;
}

void DeviceObserverTest::OnChange(const ChangeNotification &changeNotification)
{
    ZLOGD("begin.");
    callCount_++;
    insertEntries_ = changeNotification.GetInsertEntries();
    updateEntries_ = changeNotification.GetUpdateEntries();
    deleteEntries_ = changeNotification.GetDeleteEntries();
    deviceId_ = changeNotification.GetDeviceId();
    isClear_ = changeNotification.IsClear();
}

void DeviceObserverTest::ResetToZero()
{
    callCount_ = 0;
}

uint64_t DeviceObserverTest::GetCallCount() const
{
    return callCount_;
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore001
* @tc.desc: Subscribe success
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore001, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore001 begin.");
    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 0);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
    observer = nullptr;
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore002
* @tc.desc: Subscribe fail, observer is null
* @tc.type: FUNC
* @tc.require: AR000CQDU9 AR000CQS37
* @tc.author: liuyuhui
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore002, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore002 begin.");
    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    std::shared_ptr<DeviceObserverTest> observer = nullptr;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::INVALID_ARGUMENT, status) << "SubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore003
* @tc.desc: Subscribe success and OnChange callback after put
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore003, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore003 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    Key key = "Id1";
    Value value = "subscribe";
    status = kvStore_->Put(key, value); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
    observer = nullptr;
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore004
* @tc.desc: The same observer subscribe three times and OnChange callback after put
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore004, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore004 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::STORE_ALREADY_SUBSCRIBE, status) << "SubscribeKvStore return wrong status";
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::STORE_ALREADY_SUBSCRIBE, status) << "SubscribeKvStore return wrong status";

    Key key = "Id1";
    Value value = "subscribe";
    status = kvStore_->Put(key, value); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore005
* @tc.desc: The different observer subscribe three times and OnChange callback after put
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore005, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore005 begin.");
    std::shared_ptr<DeviceObserverTest> observer1 = std::make_shared<DeviceObserverTest>();
    observer1->ResetToZero();
    std::shared_ptr<DeviceObserverTest> observer2 = std::make_shared<DeviceObserverTest>();
    observer2->ResetToZero();
    std::shared_ptr<DeviceObserverTest> observer3 = std::make_shared<DeviceObserverTest>();
    observer3->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer1);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore failed, wrong status";
    status = kvStore_->SubscribeKvStore(subscribeType, observer2);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore failed, wrong status";
    status = kvStore_->SubscribeKvStore(subscribeType, observer3);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore failed, wrong status";

    Key key = "Id1";
    Value value = "subscribe";
    status = kvStore_->Put(key, value); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "Putting data to KvStore failed, wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer1->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer2->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer3->GetCallCount()), 1);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer1);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
    status = kvStore_->UnSubscribeKvStore(subscribeType, observer2);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
    status = kvStore_->UnSubscribeKvStore(subscribeType, observer3);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore006
* @tc.desc: Unsubscribe an observer and subscribe again - the map should be cleared after unsubscription.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore006, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore006 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    Key key1 = "Id1";
    Value value1 = "subscribe";
    status = kvStore_->Put(key1, value1); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";

    Key key2 = "Id2";
    Value value2 = "subscribe";
    status = kvStore_->Put(key2, value2); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);

    kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    Key key3 = "Id3";
    Value value3 = "subscribe";
    status = kvStore_->Put(key3, value3); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 2);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore007
* @tc.desc: Subscribe to an observer - OnChange callback is called multiple times after the put operation.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore007, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore007 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    Key key1 = "Id1";
    Value value1 = "subscribe";
    status = kvStore_->Put(key1, value1); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";

    Key key2 = "Id2";
    Value value2 = "subscribe";
    status = kvStore_->Put(key2, value2); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";

    Key key3 = "Id3";
    Value value3 = "subscribe";
    status = kvStore_->Put(key3, value3); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";

    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 3);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore008
* @tc.desc: Subscribe to an observer - OnChange callback is called multiple times after the put&update operations.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore008, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore008 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    Key key1 = "Id1";
    Value value1 = "subscribe";
    status = kvStore_->Put(key1, value1); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";

    Key key2 = "Id2";
    Value value2 = "subscribe";
    status = kvStore_->Put(key2, value2); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";

    Key key3 = "Id1";
    Value value3 = "subscribe03";
    status = kvStore_->Put(key3, value3); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";

    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 3);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore009
* @tc.desc: Subscribe to an observer - OnChange callback is called multiple times after the putBatch operation.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore009, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore009 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    // before update.
    std::vector<Entry> entries1;
    Entry entry1, entry2, entry3;
    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries1.push_back(entry1);
    entries1.push_back(entry2);
    entries1.push_back(entry3);

    std::vector<Entry> entries2;
    Entry entry4, entry5;
    entry4.key = "Id4";
    entry4.value = "subscribe";
    entry5.key = "Id5";
    entry5.value = "subscribe";
    entries2.push_back(entry4);
    entries2.push_back(entry5);

    status = kvStore_->PutBatch(entries1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    status = kvStore_->PutBatch(entries2);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    usleep(1000000);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 2);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore010
* @tc.desc: Subscribe to an observer - OnChange callback is called multiple times after the putBatch update operation.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore010, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore010 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    // before update.
    std::vector<Entry> entries1;
    Entry entry1, entry2, entry3;
    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries1.push_back(entry1);
    entries1.push_back(entry2);
    entries1.push_back(entry3);

    std::vector<Entry> entries2;
    Entry entry4, entry5;
    entry4.key = "Id1";
    entry4.value = "modify";
    entry5.key = "Id2";
    entry5.value = "modify";
    entries2.push_back(entry4);
    entries2.push_back(entry5);

    status = kvStore_->PutBatch(entries1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    status = kvStore_->PutBatch(entries2);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 2);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore011
* @tc.desc: Subscribe to an observer - OnChange callback is called after successful deletion.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore011, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore011 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    Status status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";
    status = kvStore_->Delete("Id1");
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Delete data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore012
* @tc.desc: Subscribe to an observer - OnChange callback is not called after deletion of non-existing keys.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore012, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore012 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    Status status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";
    status = kvStore_->Delete("Id4");
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Delete data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 0);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore013
* @tc.desc: Subscribe to an observer - OnChange callback is called after KvStore is cleared.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore013, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore013 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    Status status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 0);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore014
* @tc.desc: Subscribe to an observer - OnChange callback is not called after non-existing data in KvStore is cleared.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore014, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore014 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 0);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore015
* @tc.desc: Subscribe to an observer - OnChange callback is called after the deleteBatch operation.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore015, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore015 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    std::vector<Key> keys;
    keys.push_back("Id1");
    keys.push_back("Id2");

    Status status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    status = kvStore_->DeleteBatch(keys);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore016
* @tc.desc: Subscribe to an observer - OnChange callback is called after deleteBatch of non-existing keys.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore016, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore016 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    std::vector<Key> keys;
    keys.push_back("Id4");
    keys.push_back("Id5");

    Status status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    status = kvStore_->DeleteBatch(keys);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 0);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore020
* @tc.desc: Unsubscribe an observer two times.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStore020, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore020 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::STORE_NOT_SUBSCRIBE, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification001
* @tc.desc: Subscribe to an observer successfully - callback is called with a notification after the put operation.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification001, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification001 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    Key key = "Id1";
    Value value = "subscribe";
    status = kvStore_->Put(key, value); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    ZLOGD("kvstore_ddm_subscribekvstore_003");
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());
    ZLOGD("kvstore_ddm_subscribekvstore_003 size:%zu.", observer->insertEntries_.size());

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification002
* @tc.desc: Subscribe to the same observer three times - callback is called with a notification after the put operation.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification002, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification002 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::STORE_ALREADY_SUBSCRIBE, status) << "SubscribeKvStore return wrong status";
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::STORE_ALREADY_SUBSCRIBE, status) << "SubscribeKvStore return wrong status";

    Key key = "Id1";
    Value value = "subscribe";
    status = kvStore_->Put(key, value); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification003
* @tc.desc: The different observer subscribe three times and callback with notification after put
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification003, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification003 begin.");
    std::shared_ptr<DeviceObserverTest> observer1 = std::make_shared<DeviceObserverTest>();
    observer1->ResetToZero();
    std::shared_ptr<DeviceObserverTest> observer2 = std::make_shared<DeviceObserverTest>();
    observer2->ResetToZero();
    std::shared_ptr<DeviceObserverTest> observer3 = std::make_shared<DeviceObserverTest>();
    observer3->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer1);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";
    status = kvStore_->SubscribeKvStore(subscribeType, observer2);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";
    status = kvStore_->SubscribeKvStore(subscribeType, observer3);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    Key key = "Id1";
    Value value = "subscribe";
    status = kvStore_->Put(key, value); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer1->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer1->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observer1->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer1->insertEntries_[0].value.ToString());

    EXPECT_EQ(static_cast<int>(observer2->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer2->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observer2->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer2->insertEntries_[0].value.ToString());

    EXPECT_EQ(static_cast<int>(observer3->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer3->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observer3->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer3->insertEntries_[0].value.ToString());

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer1);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
    status = kvStore_->UnSubscribeKvStore(subscribeType, observer2);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
    status = kvStore_->UnSubscribeKvStore(subscribeType, observer3);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification004
* @tc.desc: Verify notification after an observer is unsubscribed and then subscribed again.
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification004, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification004 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    Key key1 = "Id1";
    Value value1 = "subscribe";
    status = kvStore_->Put(key1, value1); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";

    Key key2 = "Id2";
    Value value2 = "subscribe";
    status = kvStore_->Put(key2, value2); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());

    kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    Key key3 = "Id3";
    Value value3 = "subscribe";
    status = kvStore_->Put(key3, value3); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 2);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 1);
    EXPECT_EQ("Id3", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification005
* @tc.desc: Subscribe to an observer, callback with notification many times after put the different data
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification005, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification005 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    Key key1 = "Id1";
    Value value1 = "subscribe";
    status = kvStore_->Put(key1, value1); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());

    Key key2 = "Id2";
    Value value2 = "subscribe";
    status = kvStore_->Put(key2, value2); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 1);
    EXPECT_EQ("Id2", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());

    Key key3 = "Id3";
    Value value3 = "subscribe";
    status = kvStore_->Put(key3, value3); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 1);
    EXPECT_EQ("Id3", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());

    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 3);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification006
* @tc.desc: Subscribe to an observer, callback with notification many times after put the same data
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification006, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification006 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    Key key1 = "Id1";
    Value value1 = "subscribe";
    status = kvStore_->Put(key1, value1); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());

    Key key2 = "Id1";
    Value value2 = "subscribe";
    status = kvStore_->Put(key2, value2); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->updateEntries_.size()), 1);
    EXPECT_EQ("Id1", observer->updateEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->updateEntries_[0].value.ToString());

    Key key3 = "Id1";
    Value value3 = "subscribe";
    status = kvStore_->Put(key3, value3); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->updateEntries_.size()), 1);
    EXPECT_EQ("Id1", observer->updateEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->updateEntries_[0].value.ToString());

    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 3);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification007
* @tc.desc: Subscribe to an observer, callback with notification many times after put&update
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification007, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification007 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    Key key1 = "Id1";
    Value value1 = "subscribe";
    Status status = kvStore_->Put(key1, value1); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";

    Key key2 = "Id2";
    Value value2 = "subscribe";
    status = kvStore_->Put(key2, value2); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    Key key3 = "Id1";
    Value value3 = "subscribe03";
    status = kvStore_->Put(key3, value3); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";

    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer->updateEntries_.size()), 1);
    EXPECT_EQ("Id1", observer->updateEntries_[0].key.ToString());
    EXPECT_EQ("subscribe03", observer->updateEntries_[0].value.ToString());

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification008
* @tc.desc: Subscribe to an observer, callback with notification one times after putbatch&update
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification008, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification008 begin.");
    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;

    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    Status status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";

    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();
    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";
    entries.clear();
    entry1.key = "Id1";
    entry1.value = "subscribe_modify";
    entry2.key = "Id2";
    entry2.value = "subscribe_modify";
    entries.push_back(entry1);
    entries.push_back(entry2);
    status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";

    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer->updateEntries_.size()), 2);
    EXPECT_EQ("Id1", observer->updateEntries_[0].key.ToString());
    EXPECT_EQ("subscribe_modify", observer->updateEntries_[0].value.ToString());
    EXPECT_EQ("Id2", observer->updateEntries_[1].key.ToString());
    EXPECT_EQ("subscribe_modify", observer->updateEntries_[1].value.ToString());

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification009
* @tc.desc: Subscribe to an observer, callback with notification one times after putbatch all different data
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification009, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification009 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;

    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 3);
    EXPECT_EQ("Id1", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());
    EXPECT_EQ("Id2", observer->insertEntries_[1].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[1].value.ToString());
    EXPECT_EQ("Id3", observer->insertEntries_[2].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[2].value.ToString());

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification010
* @tc.desc: Subscribe to an observer, callback with notification one times after putbatch both different and same data
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification010, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification010 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;

    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id1";
    entry2.value = "subscribe";
    entry3.key = "Id2";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 2);
    EXPECT_EQ("Id1", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());
    EXPECT_EQ("Id2", observer->insertEntries_[1].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[1].value.ToString());
    EXPECT_EQ(static_cast<int>(observer->updateEntries_.size()), 0);
    EXPECT_EQ(static_cast<int>(observer->deleteEntries_.size()), 0);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification011
* @tc.desc: Subscribe to an observer, callback with notification one times after putbatch all same data
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification011, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification011 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;

    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id1";
    entry2.value = "subscribe";
    entry3.key = "Id1";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 1);
    EXPECT_EQ("Id1", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());
    EXPECT_EQ(static_cast<int>(observer->updateEntries_.size()), 0);
    EXPECT_EQ(static_cast<int>(observer->deleteEntries_.size()), 0);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification012
* @tc.desc: Subscribe to an observer, callback with notification many times after putbatch all different data
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification012, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification012 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    std::vector<Entry> entries1;
    Entry entry1, entry2, entry3;

    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries1.push_back(entry1);
    entries1.push_back(entry2);
    entries1.push_back(entry3);

    std::vector<Entry> entries2;
    Entry entry4, entry5;
    entry4.key = "Id4";
    entry4.value = "subscribe";
    entry5.key = "Id5";
    entry5.value = "subscribe";
    entries2.push_back(entry4);
    entries2.push_back(entry5);

    status = kvStore_->PutBatch(entries1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 3);
    EXPECT_EQ("Id1", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());
    EXPECT_EQ("Id2", observer->insertEntries_[1].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[1].value.ToString());
    EXPECT_EQ("Id3", observer->insertEntries_[2].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[2].value.ToString());

    status = kvStore_->PutBatch(entries2);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 2);
    EXPECT_EQ("Id4", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());
    EXPECT_EQ("Id5", observer->insertEntries_[1].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[1].value.ToString());

    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 2);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification013
* @tc.desc: Subscribe to an observer, callback with notification many times after putbatch both different and same data
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification013, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification013 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    std::vector<Entry> entries1;
    Entry entry1, entry2, entry3;

    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries1.push_back(entry1);
    entries1.push_back(entry2);
    entries1.push_back(entry3);

    std::vector<Entry> entries2;
    Entry entry4, entry5;
    entry4.key = "Id1";
    entry4.value = "subscribe";
    entry5.key = "Id4";
    entry5.value = "subscribe";
    entries2.push_back(entry4);
    entries2.push_back(entry5);

    status = kvStore_->PutBatch(entries1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 3);
    EXPECT_EQ("Id1", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());
    EXPECT_EQ("Id2", observer->insertEntries_[1].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[1].value.ToString());
    EXPECT_EQ("Id3", observer->insertEntries_[2].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[2].value.ToString());

    status = kvStore_->PutBatch(entries2);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->updateEntries_.size()), 1);
    EXPECT_EQ("Id1", observer->updateEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->updateEntries_[0].value.ToString());
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 1);
    EXPECT_EQ("Id4", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());

    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 2);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification014
* @tc.desc: Subscribe to an observer, callback with notification many times after putbatch all same data
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification014, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification014 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    std::vector<Entry> entries1;
    Entry entry1, entry2, entry3;

    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries1.push_back(entry1);
    entries1.push_back(entry2);
    entries1.push_back(entry3);

    std::vector<Entry> entries2;
    Entry entry4, entry5;
    entry4.key = "Id1";
    entry4.value = "subscribe";
    entry5.key = "Id2";
    entry5.value = "subscribe";
    entries2.push_back(entry4);
    entries2.push_back(entry5);

    status = kvStore_->PutBatch(entries1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 3);
    EXPECT_EQ("Id1", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());
    EXPECT_EQ("Id2", observer->insertEntries_[1].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[1].value.ToString());
    EXPECT_EQ("Id3", observer->insertEntries_[2].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[2].value.ToString());

    status = kvStore_->PutBatch(entries2);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->updateEntries_.size()), 2);
    EXPECT_EQ("Id1", observer->updateEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->updateEntries_[0].value.ToString());
    EXPECT_EQ("Id2", observer->updateEntries_[1].key.ToString());
    EXPECT_EQ("subscribe", observer->updateEntries_[1].value.ToString());

    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 2);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification015
* @tc.desc: Subscribe to an observer, callback with notification many times after putbatch complex data
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification015, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification015 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    std::vector<Entry> entries1;
    Entry entry1, entry2, entry3;

    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id1";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries1.push_back(entry1);
    entries1.push_back(entry2);
    entries1.push_back(entry3);

    std::vector<Entry> entries2;
    Entry entry4, entry5;
    entry4.key = "Id1";
    entry4.value = "subscribe";
    entry5.key = "Id2";
    entry5.value = "subscribe";
    entries2.push_back(entry4);
    entries2.push_back(entry5);

    status = kvStore_->PutBatch(entries1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->updateEntries_.size()), 0);
    EXPECT_EQ(static_cast<int>(observer->deleteEntries_.size()), 0);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 2);
    EXPECT_EQ("Id1", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());
    EXPECT_EQ("Id3", observer->insertEntries_[1].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[1].value.ToString());

    status = kvStore_->PutBatch(entries2);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->updateEntries_.size()), 1);
    EXPECT_EQ("Id1", observer->updateEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->updateEntries_[0].value.ToString());
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 1);
    EXPECT_EQ("Id2", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->insertEntries_[0].value.ToString());

    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 2);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification016
* @tc.desc: Pressure test subscribe, callback with notification many times after putbatch
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification016, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification016 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    const int ENTRIES_MAX_LEN = 100;
    std::vector<Entry> entries;
    for (int i = 0; i < ENTRIES_MAX_LEN; i++) {
        Entry entry;
        entry.key = std::to_string(i);
        entry.value = "subscribe";
        entries.push_back(entry);
    }

    status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    usleep(USLEEP_TIME);

    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 100);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification017
* @tc.desc: Subscribe to an observer, callback with notification after delete success
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification017, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification017 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    Status status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";
    status = kvStore_->Delete("Id1");
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Delete data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer->deleteEntries_.size()), 1);
    EXPECT_EQ("Id1", observer->deleteEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->deleteEntries_[0].value.ToString());

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification018
* @tc.desc: Subscribe to an observer, not callback after delete which key not exist
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification018, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification018 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    Status status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";
    status = kvStore_->Delete("Id4");
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Delete data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 0);
    EXPECT_EQ(static_cast<int>(observer->deleteEntries_.size()), 0);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification019
* @tc.desc: Subscribe to an observer, delete the same data many times and only first delete callback with notification
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification019, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification019 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    Status status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";
    status = kvStore_->Delete("Id1");
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Delete data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer->deleteEntries_.size()), 1);
    EXPECT_EQ("Id1", observer->deleteEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->deleteEntries_[0].value.ToString());

    status = kvStore_->Delete("Id1");
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Delete data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer->deleteEntries_.size()), 1); // not callback so not clear

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification020
* @tc.desc: Subscribe to an observer, callback with notification after deleteBatch
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification020, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification020 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    std::vector<Key> keys;
    keys.push_back("Id1");
    keys.push_back("Id2");

    Status status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    status = kvStore_->DeleteBatch(keys);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer->deleteEntries_.size()), 2);
    EXPECT_EQ("Id1", observer->deleteEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->deleteEntries_[0].value.ToString());
    EXPECT_EQ("Id2", observer->deleteEntries_[1].key.ToString());
    EXPECT_EQ("subscribe", observer->deleteEntries_[1].value.ToString());

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification021
* @tc.desc: Subscribe to an observer, not callback after deleteBatch which all keys not exist
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification021, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification021 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    std::vector<Key> keys;
    keys.push_back("Id4");
    keys.push_back("Id5");

    Status status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    status = kvStore_->DeleteBatch(keys);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 0);
    EXPECT_EQ(static_cast<int>(observer->deleteEntries_.size()), 0);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification022
* @tc.desc: Subscribe to an observer, deletebatch the same data many times and only first deletebatch callback with
* notification
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification022, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification022 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "Id1";
    entry1.value = "subscribe";
    entry2.key = "Id2";
    entry2.value = "subscribe";
    entry3.key = "Id3";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    std::vector<Key> keys;
    keys.push_back("Id1");
    keys.push_back("Id2");

    Status status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    status = kvStore_->DeleteBatch(keys);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer->deleteEntries_.size()), 2);
    EXPECT_EQ("Id1", observer->deleteEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->deleteEntries_[0].value.ToString());
    EXPECT_EQ("Id2", observer->deleteEntries_[1].key.ToString());
    EXPECT_EQ("subscribe", observer->deleteEntries_[1].value.ToString());

    status = kvStore_->DeleteBatch(keys);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);
    EXPECT_EQ(static_cast<int>(observer->deleteEntries_.size()), 2); // not callback so not clear

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification023
* @tc.desc: Subscribe to an observer, include Clear Put PutBatch Delete DeleteBatch
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification023, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification023 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    Key key1 = "Id1";
    Value value1 = "subscribe";

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "Id2";
    entry1.value = "subscribe";
    entry2.key = "Id3";
    entry2.value = "subscribe";
    entry3.key = "Id4";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    std::vector<Key> keys;
    keys.push_back("Id2");
    keys.push_back("Id3");

    status = kvStore_->Put(key1, value1); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    status = kvStore_->Delete(key1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore delete data return wrong status";
    status = kvStore_->DeleteBatch(keys);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong status";
    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 4);
    // every callback will clear vector
    EXPECT_EQ(static_cast<int>(observer->deleteEntries_.size()), 2);
    EXPECT_EQ("Id2", observer->deleteEntries_[0].key.ToString());
    EXPECT_EQ("subscribe", observer->deleteEntries_[0].value.ToString());
    EXPECT_EQ("Id3", observer->deleteEntries_[1].key.ToString());
    EXPECT_EQ("subscribe", observer->deleteEntries_[1].value.ToString());
    EXPECT_EQ(static_cast<int>(observer->updateEntries_.size()), 0);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 0);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification024
* @tc.desc: Subscribe to an observer[use transaction], include Clear Put PutBatch Delete DeleteBatch
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification024, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification024 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    Key key1 = "Id1";
    Value value1 = "subscribe";

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "Id2";
    entry1.value = "subscribe";
    entry2.key = "Id3";
    entry2.value = "subscribe";
    entry3.key = "Id4";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    std::vector<Key> keys;
    keys.push_back("Id2");
    keys.push_back("Id3");

    status = kvStore_->StartTransaction();
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore startTransaction return wrong status";
    status = kvStore_->Put(key1, value1); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    status = kvStore_->Delete(key1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore delete data return wrong status";
    status = kvStore_->DeleteBatch(keys);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong status";
    status = kvStore_->Commit();
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Commit return wrong status";

    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification025
* @tc.desc: Subscribe to an observer[use transaction], include Clear Put PutBatch Delete DeleteBatch
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification025, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification025 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    Key key1 = "Id1";
    Value value1 = "subscribe";

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "Id2";
    entry1.value = "subscribe";
    entry2.key = "Id3";
    entry2.value = "subscribe";
    entry3.key = "Id4";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    std::vector<Key> keys;
    keys.push_back("Id2");
    keys.push_back("Id3");

    status = kvStore_->StartTransaction();
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore startTransaction return wrong status";
    status = kvStore_->Put(key1, value1); // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore put data return wrong status";
    status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";
    status = kvStore_->Delete(key1);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore delete data return wrong status";
    status = kvStore_->DeleteBatch(keys);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore DeleteBatch data return wrong status";
    status = kvStore_->Rollback();
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore Commit return wrong status";

    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 0);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 0);
    EXPECT_EQ(static_cast<int>(observer->updateEntries_.size()), 0);
    EXPECT_EQ(static_cast<int>(observer->deleteEntries_.size()), 0);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
    observer = nullptr;
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification026
* @tc.desc: Subscribe to an observer[use transaction], include bigData PutBatch  update  insert delete
* @tc.type: FUNC
* @tc.require: I5GG0N
* @tc.author: blue sky
*/
HWTEST_F(LocalSubscribeDeviceStoreTest, KvStoreDdmSubscribeKvStoreNotification026, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification026 begin.");
    std::shared_ptr<DeviceObserverTest> observer = std::make_shared<DeviceObserverTest>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    std::vector<Entry> entries;
    Entry entry0, entry1, entry2, entry3, entry4, entry5, entry6, entry7;

    int maxValueSize = 2 * 1024 * 1024; // max value size is 2M.
    std::vector<uint8_t> val(maxValueSize);
    for (int i = 0; i < maxValueSize; i++) {
        val[i] = static_cast<uint8_t>(i);
    }
    Value value = val;

    int maxValueSize2 = 1000 * 1024; // max value size is 1000k.
    std::vector<uint8_t> val2(maxValueSize2);
    for (int i = 0; i < maxValueSize2; i++) {
        val2[i] = static_cast<uint8_t>(i);
    }
    Value value2 = val2;

    entry0.key = "SingleKvStoreDdmPutBatch006_0";
    entry0.value = "beijing";
    entry1.key = "SingleKvStoreDdmPutBatch006_1";
    entry1.value = value;
    entry2.key = "SingleKvStoreDdmPutBatch006_2";
    entry2.value = value;
    entry3.key = "SingleKvStoreDdmPutBatch006_3";
    entry3.value = "ZuiHouBuZhiTianZaiShui";
    entry4.key = "SingleKvStoreDdmPutBatch006_4";
    entry4.value = value;

    entries.push_back(entry0);
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);
    entries.push_back(entry4);

    status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putbatch data return wrong status";

    usleep(USLEEP_TIME);

    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 5);
    EXPECT_EQ("SingleKvStoreDdmPutBatch006_0", observer->insertEntries_[0].key.ToString());
    EXPECT_EQ("beijing", observer->insertEntries_[0].value.ToString());
    EXPECT_EQ("SingleKvStoreDdmPutBatch006_1", observer->insertEntries_[1].key.ToString());
    EXPECT_EQ("SingleKvStoreDdmPutBatch006_2", observer->insertEntries_[2].key.ToString());
    EXPECT_EQ("SingleKvStoreDdmPutBatch006_3", observer->insertEntries_[3].key.ToString());
    EXPECT_EQ("ZuiHouBuZhiTianZaiShui", observer->insertEntries_[3].value.ToString());

    entry5.key = "SingleKvStoreDdmPutBatch006_2";
    entry5.value = val2;
    entry6.key = "SingleKvStoreDdmPutBatch006_3";
    entry6.value = "ManChuanXingMengYaXingHe";
    entry7.key = "SingleKvStoreDdmPutBatch006_4";
    entry7.value = val2;
    std::vector<Entry> updateEntries;
    updateEntries.push_back(entry5);
    updateEntries.push_back(entry6);
    updateEntries.push_back(entry7);
    status = kvStore_->PutBatch(updateEntries);
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore putBatch update data return wrong status";

    usleep(USLEEP_TIME);
    EXPECT_EQ(static_cast<int>(observer->updateEntries_.size()), 3);
    EXPECT_EQ("SingleKvStoreDdmPutBatch006_2", observer->updateEntries_[0].key.ToString());
    EXPECT_EQ("SingleKvStoreDdmPutBatch006_3", observer->updateEntries_[1].key.ToString());
    EXPECT_EQ("ManChuanXingMengYaXingHe", observer->updateEntries_[1].value.ToString());
    EXPECT_EQ("SingleKvStoreDdmPutBatch006_4", observer->updateEntries_[2].key.ToString());
    EXPECT_EQ(false, observer->isClear_);

    status = kvStore_->Delete("SingleKvStoreDdmPutBatch006_3");
    EXPECT_EQ(Status::SUCCESS, status) << "KvStore delete data return wrong status";
    usleep(1000000);
    EXPECT_EQ(static_cast<int>(observer->deleteEntries_.size()), 1);
    EXPECT_EQ("SingleKvStoreDdmPutBatch006_3", observer->deleteEntries_[0].key.ToString());

    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 3);
    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}