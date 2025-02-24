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
#define LOG_TAG "LocalKvStoreShamUnitTest"

#include "block_data.h"
#include "distributed_kv_data_manager.h"
#include "log_print.h"
#include "types.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <mutex>
#include <vector>

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::DistributedKv;
class LocalKvStoreShamUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static DistributedKvDataManager shamTestManger_;
    static std::shared_ptr<SingleKvStore> shamTestStore_;
    static Status shamTestStatus_;
    static AppId shamTestAppId_;
    static StoreId shamTestStoreId_;
};
std::shared_ptr<SingleKvStore> LocalKvStoreShamUnitTest::shamTestStore_ = nullptr;
Status LocalKvStoreShamUnitTest::shamTestStatus_ = Status::ERROR;
DistributedKvDataManager LocalKvStoreShamUnitTest::shamTestManger_;
AppId LocalKvStoreShamUnitTest::shamTestAppId_;
StoreId LocalKvStoreShamUnitTest::shamTestStoreId_;

void LocalKvStoreShamUnitTest::SetUpTestCase(void)
{
    mkdir("/data/service/el1/public/database/local", (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
}

void LocalKvStoreShamUnitTest::TearDownTestCase(void)
{
    shamTestManger_.CloseKvStore(shamTestAppId_, shamTestStore_);
    shamTestStore_ = nullptr;
    shamTestManger_.DeleteKvStore(shamTestAppId_, shamTestStoreId_, "/data/service/el1/public/database/local");
    (void)remove("/data/service/el1/public/database/local/kvdb");
    (void)remove("/data/service/el1/public/database/local");
}

void LocalKvStoreShamUnitTest::SetUp(void)
{
    Options shamOptions;
    shamOptions.securityLevel = S1;
    shamOptions.baseDir = std::string("/data/service/el1/public/database/local");
    shamTestAppId_.appId = "local"; // define app name.
    shamTestStoreId_.storeId = "MAN";   // define kvstore(database) name
    shamTestManger_.DeleteKvStore(shamTestAppId_, shamTestStoreId_, shamOptions.baseDir);
    // [create and] open and initialize kvstore instance.
    shamTestStatus_ = shamTestManger_.GetSingleKvStore(shamOptions, shamTestAppId_, shamTestStoreId_, shamTestStore_);
    ASSERT_EQ(Status::SUCCESS, shamTestStatus_) << "wrong status";
    ASSERT_NE(nullptr, shamTestStore_) << "kvStore is nullptr";
}

void LocalKvStoreShamUnitTest::TearDown(void)
{
    shamTestManger_.CloseKvStore(shamTestAppId_, shamTestStore_);
    shamTestStore_ = nullptr;
    shamTestManger_.DeleteKvStore(shamTestAppId_, shamTestStoreId_);
}

class DeviceObserverShamUnitTest : public KvStoreObserver {
public:
    std::vector<Entry> insertShamEntries_;
    std::vector<Entry> updateShamEntries_;
    std::vector<Entry> deleteShamEntries_;
    std::string shamDeviceId_;
    bool isShamClear_ = false;
    DeviceObserverShamUnitTest();
    ~DeviceObserverShamUnitTest() = default;

    void OnChange(const ChangeNotification &changeNotification);

    // reset the shamCallCount_ to zero.
    void ResetToZero();

    uint32_t GetCallCount(uint32_t shamTestValue = 1);

private:
    std::mutex shamMutex_;
    uint32_t shamCallCount_ = 0;
    BlockData<uint32_t> shamValue_ { 1, 0 };
};

DeviceObserverShamUnitTest::DeviceObserverShamUnitTest() { }

void DeviceObserverShamUnitTest::OnChange(const ChangeNotification &changeNotification)
{
    ZLOGD("begin.");
    insertShamEntries_ = changeNotification.GetInsertEntries();
    updateShamEntries_ = changeNotification.GetUpdateEntries();
    deleteShamEntries_ = changeNotification.GetDeleteEntries();
    shamDeviceId_ = changeNotification.GetDeviceId();
    isShamClear_ = changeNotification.IsClear();
    std::lock_guard<decltype(shamMutex_)> guard(shamMutex_);
    ++shamCallCount_;
    shamValue_.SetValue(shamCallCount_);
}

void DeviceObserverShamUnitTest::ResetToZero()
{
    std::lock_guard<decltype(shamMutex_)> guard(shamMutex_);
    shamCallCount_ = 0;
    shamValue_.Clear(0);
}

uint32_t DeviceObserverShamUnitTest::GetCallCount(uint32_t shamTestValue)
{
    int retryTimes = 0;
    uint32_t callCount = 0;
    while (retryTimes < shamTestValue) {
        callCount = shamValue_.GetValue();
        if (callCount >= shamTestValue) {
            break;
        }
        std::lock_guard<decltype(shamMutex_)> guard(shamMutex_);
        callCount = shamValue_.GetValue();
        if (callCount >= shamTestValue) {
            break;
        }
        shamValue_.Clear(callCount);
        retryTimes++;
    }
    return callCount;
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore001
 * @tc.desc: Subscribe success
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore001, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore001 begin.");
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 0);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
    shamObserver = nullptr;
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore002
 * @tc.desc: Subscribe fail, shamObserver is null
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore002, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore002 begin.");
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    std::shared_ptr<DeviceObserverShamUnitTest> shamObserver = nullptr;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::INVALID_ARGUMENT, shamStatus) << "SubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore003
 * @tc.desc: Subscribe success and OnChange callback after put
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore003, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStore003 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    Key shamTestKey = "Id1";
    Value shamTestValue = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey, shamTestValue); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
    shamObserver = nullptr;
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore004
 * @tc.desc: The same shamObserver subscribe three times and OnChange callback after put
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore004, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore004 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, shamStatus) << "SubscribeKvStore return wrong";
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, shamStatus) << "SubscribeKvStore return wrong";

    Key shamTestKey = "Id1";
    Value shamTestValue = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey, shamTestValue); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore005
 * @tc.desc: The different shamObserver subscribe three times and OnChange callback after put
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore005, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore005 begin.");
    auto observer1 = std::make_shared<DeviceObserverShamUnitTest>();
    auto observer2 = std::make_shared<DeviceObserverShamUnitTest>();
    auto observer3 = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, observer1);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore failed, wrong";
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, observer2);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore failed, wrong";
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, observer3);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore failed, wrong";

    Key shamTestKey = "Id1";
    Value shamTestValue = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey, shamTestValue); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "Putting data to KvStore failed, wrong";
    ASSERT_EQ(static_cast<int>(observer1->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observer2->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observer3->GetCallCount()), 1);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, observer1);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, observer2);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, observer3);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore006
 * @tc.desc: Unsubscribe an shamObserver and subscribe again - the map should be cleared after unsubscription.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore006, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore006 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    Key shamTestKey1 = "Id1";
    Value shamTestValue1 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey1, shamTestValue1); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";

    Key shamTestKey2 = "Id2";
    Value shamTestValue2 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey2, shamTestValue2); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);

    shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    Key shamTestKey3 = "Id3";
    Value shamTestValue3 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey3, shamTestValue3); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(2)), 2);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore007
 * @tc.desc: Subscribe to an shamObserver - OnChange callback is called multiple times after the put operation.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore007, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore007 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    Key shamTestKey1 = "Id1";
    Value shamTestValue1 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey1, shamTestValue1); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";

    Key shamTestKey2 = "Id2";
    Value shamTestValue2 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey2, shamTestValue2); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";

    Key shamTestKey3 = "Id3";
    Value shamTestValue3 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey3, shamTestValue3); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(3)), 3);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore008
* @tc.desc: Subscribe to an shamObserver - OnChange callback is
  called multiple times after the put&update operations.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore008, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore008 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    Key shamTestKey1 = "Id1";
    Value shamTestValue1 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey1, shamTestValue1); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";

    Key shamTestKey2 = "Id2";
    Value shamTestValue2 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey2, shamTestValue2); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";

    Key shamTestKey3 = "Id1";
    Value shamTestValue3 = "subscribe03";
    shamStatus = shamTestStore_->Put(shamTestKey3, shamTestValue3); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(3)), 3);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore009
 * @tc.desc: Subscribe to an shamObserver - OnChange callback is called multiple times after the putBatch operation.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore009, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore009 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    // before update.
    std::vector<Entry> shamTestEntries1;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;
    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries1.push_back(shamTestEnty1);
    shamTestEntries1.push_back(shamTestEnty2);
    shamTestEntries1.push_back(shamTestEnty3);

    std::vector<Entry> shamTestEntries2;
    Entry shamTestEnty4, shamTestEnty5;
    shamTestEnty4.shamTestKey = "Id4";
    shamTestEnty4.shamTestValue = "subscribe";
    shamTestEnty5.shamTestKey = "Id5";
    shamTestEnty5.shamTestValue = "subscribe";
    shamTestEntries2.push_back(shamTestEnty4);
    shamTestEntries2.push_back(shamTestEnty5);

    shamStatus = shamTestStore_->PutBatch(shamTestEntries1);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    shamStatus = shamTestStore_->PutBatch(shamTestEntries2);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(2)), 2);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore010
* @tc.desc: Subscribe to an shamObserver - OnChange callback is
  called multiple times after the putBatch update operation.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore010, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore010 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    // before update.
    std::vector<Entry> shamTestEntries1;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;
    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries1.push_back(shamTestEnty1);
    shamTestEntries1.push_back(shamTestEnty2);
    shamTestEntries1.push_back(shamTestEnty3);

    std::vector<Entry> shamTestEntries2;
    Entry shamTestEnty4, shamTestEnty5;
    shamTestEnty4.shamTestKey = "Id1";
    shamTestEnty4.shamTestValue = "modify";
    shamTestEnty5.shamTestKey = "Id2";
    shamTestEnty5.shamTestValue = "modify";
    shamTestEntries2.push_back(shamTestEnty4);
    shamTestEntries2.push_back(shamTestEnty5);

    shamStatus = shamTestStore_->PutBatch(shamTestEntries1);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    shamStatus = shamTestStore_->PutBatch(shamTestEntries2);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(2)), 2);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore011
 * @tc.desc: Subscribe to an shamObserver - OnChange callback is called after successful deletion.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore011, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore011 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;
    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    Status shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";

    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";
    shamStatus = shamTestStore_->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore Delete data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore012
 * @tc.desc: Subscribe to an shamObserver - OnChange callback is not called after deletion of non-existing keys.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore012, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore012 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;
    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    Status shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";

    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";
    shamStatus = shamTestStore_->Delete("Id4");
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore Delete data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 0);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore013
 * @tc.desc: Subscribe to an shamObserver - OnChange callback is called after KvStore is cleared.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore013, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore013 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;
    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    Status shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";

    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 0);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStore014
* @tc.desc: Subscribe to an shamObserver - OnChange callback is
  not called after non-existing data in KvStore is cleared.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore014, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore014 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 0);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore015
 * @tc.desc: Subscribe to an shamObserver - OnChange callback is called after the deleteBatch operation.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore015, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore015 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;
    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    std::vector<Key> keys;
    keys.push_back("Id1");
    keys.push_back("Id2");

    Status shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";

    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    shamStatus = shamTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore DeleteBatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore016
 * @tc.desc: Subscribe to an shamObserver - OnChange callback is called after deleteBatch of non-existing keys.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore016, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore016 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;
    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    std::vector<Key> keys;
    keys.push_back("Id4");
    keys.push_back("Id5");

    Status shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";

    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    shamStatus = shamTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore DeleteBatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 0);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStore020
 * @tc.desc: Unsubscribe an shamObserver two times.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStore020, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStore020 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::STORE_NOT_SUBSCRIBE, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification001
* @tc.desc: Subscribe to an shamObserver successfully - callback is
  called with a notification after the put operation.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification001, TestSize.Level1)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification001 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    Key shamTestKey = "Id1";
    Value shamTestValue = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey, shamTestValue); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ZLOGD("kvstore_ddm_subscribekvstore_003");
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 1);
    ASSERT_EQ("Id1", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());
    ZLOGD("kvstore_ddm_subscribekvstore_003 size:%zu.", shamObserver->insertShamEntries_.size());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification002
* @tc.desc: Subscribe to the same shamObserver three times - callback is
  called with a notification after the put operation.
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification002, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification002 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, shamStatus) << "SubscribeKvStore return wrong";
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::STORE_ALREADY_SUBSCRIBE, shamStatus) << "SubscribeKvStore return wrong";

    Key shamTestKey = "Id1";
    Value shamTestValue = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey, shamTestValue); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 1);
    ASSERT_EQ("Id1", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification003
 * @tc.desc: The different shamObserver subscribe three times and callback with notification after put
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification003, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification003 begin.");
    auto observer1 = std::make_shared<DeviceObserverShamUnitTest>();
    auto observer2 = std::make_shared<DeviceObserverShamUnitTest>();
    auto observer3 = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, observer1);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, observer2);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, observer3);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    Key shamTestKey = "Id1";
    Value shamTestValue = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey, shamTestValue); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(observer1->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observer1->insertShamEntries_.size()), 1);
    ASSERT_EQ("Id1", observer1->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", observer1->insertShamEntries_[0].shamTestValue.ToString());

    ASSERT_EQ(static_cast<int>(observer2->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observer2->insertShamEntries_.size()), 1);
    ASSERT_EQ("Id1", observer2->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", observer2->insertShamEntries_[0].shamTestValue.ToString());

    ASSERT_EQ(static_cast<int>(observer3->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(observer3->insertShamEntries_.size()), 1);
    ASSERT_EQ("Id1", observer3->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", observer3->insertShamEntries_[0].shamTestValue.ToString());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, observer1);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, observer2);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, observer3);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification004
 * @tc.desc: Verify notification after an shamObserver is unsubscribed and then subscribed again.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification004, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification004 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    Key shamTestKey1 = "Id1";
    Value shamTestValue1 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey1, shamTestValue1); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 1);
    ASSERT_EQ("Id1", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";

    Key shamTestKey2 = "Id2";
    Value shamTestValue2 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey2, shamTestValue2); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 1);
    ASSERT_EQ("Id1", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());

    shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    Key shamTestKey3 = "Id3";
    Value shamTestValue3 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey3, shamTestValue3); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 1);
    ASSERT_EQ("Id3", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification005
 * @tc.desc: Subscribe to an shamObserver, callback with notification many times after put the different data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification005, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification005 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    Key shamTestKey1 = "Id1";
    Value shamTestValue1 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey1, shamTestValue1); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 1);
    ASSERT_EQ("Id1", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());

    Key shamTestKey2 = "Id2";
    Value shamTestValue2 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey2, shamTestValue2); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 1);
    ASSERT_EQ("Id2", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());

    Key shamTestKey3 = "Id3";
    Value shamTestValue3 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey3, shamTestValue3); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(3)), 3);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 1);
    ASSERT_EQ("Id3", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification006
 * @tc.desc: Subscribe to an shamObserver, callback with notification many times after put the same data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification006, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification006 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    Key shamTestKey1 = "Id1";
    Value shamTestValue1 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey1, shamTestValue1); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 1);
    ASSERT_EQ("Id1", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());

    Key shamTestKey2 = "Id1";
    Value shamTestValue2 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey2, shamTestValue2); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(shamObserver->updateShamEntries_.size()), 1);
    ASSERT_EQ("Id1", shamObserver->updateShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->updateShamEntries_[0].shamTestValue.ToString());

    Key shamTestKey3 = "Id1";
    Value shamTestValue3 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey3, shamTestValue3); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(3)), 3);
    ASSERT_EQ(static_cast<int>(shamObserver->updateShamEntries_.size()), 1);
    ASSERT_EQ("Id1", shamObserver->updateShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->updateShamEntries_[0].shamTestValue.ToString());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification007
 * @tc.desc: Subscribe to an shamObserver, callback with notification many times after put&update
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification007, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification007 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    Key shamTestKey1 = "Id1";
    Value shamTestValue1 = "subscribe";
    Status shamStatus = shamTestStore_->Put(shamTestKey1, shamTestValue1);
    // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";

    Key shamTestKey2 = "Id2";
    Value shamTestValue2 = "subscribe";
    shamStatus = shamTestStore_->Put(shamTestKey2, shamTestValue2); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";

    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    Key shamTestKey3 = "Id1";
    Value shamTestValue3 = "subscribe03";
    shamStatus = shamTestStore_->Put(shamTestKey3, shamTestValue3); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->updateShamEntries_.size()), 1);
    ASSERT_EQ("Id1", shamObserver->updateShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe03", shamObserver->updateShamEntries_[0].shamTestValue.ToString());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification008
 * @tc.desc: Subscribe to an shamObserver, callback with notification one times after putbatch&update
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification008, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification008 begin.");
    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;

    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    Status shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";

    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";
    shamTestEntries.clear();
    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe_modify";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe_modify";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";

    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->updateShamEntries_.size()), 2);
    ASSERT_EQ("Id1", shamObserver->updateShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe_modify", shamObserver->updateShamEntries_[0].shamTestValue.ToString());
    ASSERT_EQ("Id2", shamObserver->updateShamEntries_[1].shamTestKey.ToString());
    ASSERT_EQ("subscribe_modify", shamObserver->updateShamEntries_[1].shamTestValue.ToString());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification009
 * @tc.desc: Subscribe to an shamObserver, callback with notification one times after putbatch all different data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification009, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification009 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;

    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 3);
    ASSERT_EQ("Id1", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());
    ASSERT_EQ("Id2", shamObserver->insertShamEntries_[1].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[1].shamTestValue.ToString());
    ASSERT_EQ("Id3", shamObserver->insertShamEntries_[2].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[2].shamTestValue.ToString());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification010
* @tc.desc: Subscribe to an shamObserver,
  callback with notification one times after putbatch both different and same data
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification010, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification010 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;

    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id1";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id2";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 2);
    ASSERT_EQ("Id1", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());
    ASSERT_EQ("Id2", shamObserver->insertShamEntries_[1].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[1].shamTestValue.ToString());
    ASSERT_EQ(static_cast<int>(shamObserver->updateShamEntries_.size()), 0);
    ASSERT_EQ(static_cast<int>(shamObserver->deleteShamEntries_.size()), 0);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification011
 * @tc.desc: Subscribe to an shamObserver, callback with notification one times after putbatch all same data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification011, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification011 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;

    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id1";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id1";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 1);
    ASSERT_EQ("Id1", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());
    ASSERT_EQ(static_cast<int>(shamObserver->updateShamEntries_.size()), 0);
    ASSERT_EQ(static_cast<int>(shamObserver->deleteShamEntries_.size()), 0);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification012
 * @tc.desc: Subscribe to an shamObserver, callback with notification many times after putbatch all different data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification012, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification012 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> shamTestEntries1;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;

    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries1.push_back(shamTestEnty1);
    shamTestEntries1.push_back(shamTestEnty2);
    shamTestEntries1.push_back(shamTestEnty3);

    std::vector<Entry> shamTestEntries2;
    Entry shamTestEnty4, shamTestEnty5;
    shamTestEnty4.shamTestKey = "Id4";
    shamTestEnty4.shamTestValue = "subscribe";
    shamTestEnty5.shamTestKey = "Id5";
    shamTestEnty5.shamTestValue = "subscribe";
    shamTestEntries2.push_back(shamTestEnty4);
    shamTestEntries2.push_back(shamTestEnty5);

    shamStatus = shamTestStore_->PutBatch(shamTestEntries1);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 3);
    ASSERT_EQ("Id1", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());
    ASSERT_EQ("Id2", shamObserver->insertShamEntries_[1].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[1].shamTestValue.ToString());
    ASSERT_EQ("Id3", shamObserver->insertShamEntries_[2].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[2].shamTestValue.ToString());
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification012b
 * @tc.desc: Subscribe to an shamObserver, callback with notification many times after putbatch all different data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification012b, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification012b begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> shamTestEntries1;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;

    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries1.push_back(shamTestEnty1);
    shamTestEntries1.push_back(shamTestEnty2);
    shamTestEntries1.push_back(shamTestEnty3);

    std::vector<Entry> shamTestEntries2;
    Entry shamTestEnty4, shamTestEnty5;
    shamTestEnty4.shamTestKey = "Id4";
    shamTestEnty4.shamTestValue = "subscribe";
    shamTestEnty5.shamTestKey = "Id5";
    shamTestEnty5.shamTestValue = "subscribe";
    shamTestEntries2.push_back(shamTestEnty4);
    shamTestEntries2.push_back(shamTestEnty5);

    shamStatus = shamTestStore_->PutBatch(shamTestEntries2);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 2);
    ASSERT_EQ("Id4", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());
    ASSERT_EQ("Id5", shamObserver->insertShamEntries_[1].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[1].shamTestValue.ToString());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}
/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification013
* @tc.desc: Subscribe to an shamObserver,
  callback with notification many times after putbatch both different and same data
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification013, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification013 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> shamTestEntries1;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;

    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries1.push_back(shamTestEnty1);
    shamTestEntries1.push_back(shamTestEnty2);
    shamTestEntries1.push_back(shamTestEnty3);

    std::vector<Entry> shamTestEntries2;
    Entry shamTestEnty4, shamTestEnty5;
    shamTestEnty4.shamTestKey = "Id1";
    shamTestEnty4.shamTestValue = "subscribe";
    shamTestEnty5.shamTestKey = "Id4";
    shamTestEnty5.shamTestValue = "subscribe";
    shamTestEntries2.push_back(shamTestEnty4);
    shamTestEntries2.push_back(shamTestEnty5);

    shamStatus = shamTestStore_->PutBatch(shamTestEntries1);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 3);
    ASSERT_EQ("Id1", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());
    ASSERT_EQ("Id2", shamObserver->insertShamEntries_[1].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[1].shamTestValue.ToString());
    ASSERT_EQ("Id3", shamObserver->insertShamEntries_[2].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[2].shamTestValue.ToString());
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification013b
* @tc.desc: Subscribe to an shamObserver,
  callback with notification many times after putbatch both different and same data
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification013b, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification013b begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> shamTestEntries1;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;

    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries1.push_back(shamTestEnty1);
    shamTestEntries1.push_back(shamTestEnty2);
    shamTestEntries1.push_back(shamTestEnty3);

    std::vector<Entry> shamTestEntries2;
    Entry shamTestEnty4, shamTestEnty5;
    shamTestEnty4.shamTestKey = "Id1";
    shamTestEnty4.shamTestValue = "subscribe";
    shamTestEnty5.shamTestKey = "Id4";
    shamTestEnty5.shamTestValue = "subscribe";
    shamTestEntries2.push_back(shamTestEnty4);
    shamTestEntries2.push_back(shamTestEnty5);
    shamStatus = shamTestStore_->PutBatch(shamTestEntries2);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(shamObserver->updateShamEntries_.size()), 1);
    ASSERT_EQ("Id1", shamObserver->updateShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->updateShamEntries_[0].shamTestValue.ToString());
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 1);
    ASSERT_EQ("Id4", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}
/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification014
 * @tc.desc: Subscribe to an shamObserver, callback with notification many times after putbatch all same data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification014, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification014 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> shamTestEntries1;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;

    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries1.push_back(shamTestEnty1);
    shamTestEntries1.push_back(shamTestEnty2);
    shamTestEntries1.push_back(shamTestEnty3);

    std::vector<Entry> shamTestEntries2;
    Entry shamTestEnty4, shamTestEnty5;
    shamTestEnty4.shamTestKey = "Id1";
    shamTestEnty4.shamTestValue = "subscribe";
    shamTestEnty5.shamTestKey = "Id2";
    shamTestEnty5.shamTestValue = "subscribe";
    shamTestEntries2.push_back(shamTestEnty4);
    shamTestEntries2.push_back(shamTestEnty5);

    shamStatus = shamTestStore_->PutBatch(shamTestEntries1);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 3);
    ASSERT_EQ("Id1", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());
    ASSERT_EQ("Id2", shamObserver->insertShamEntries_[1].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[1].shamTestValue.ToString());
    ASSERT_EQ("Id3", shamObserver->insertShamEntries_[2].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[2].shamTestValue.ToString());

    shamStatus = shamTestStore_->PutBatch(shamTestEntries2);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(shamObserver->updateShamEntries_.size()), 2);
    ASSERT_EQ("Id1", shamObserver->updateShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->updateShamEntries_[0].shamTestValue.ToString());
    ASSERT_EQ("Id2", shamObserver->updateShamEntries_[1].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->updateShamEntries_[1].shamTestValue.ToString());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification015
 * @tc.desc: Subscribe to an shamObserver, callback with notification many times after putbatch complex data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification015, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification015 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> shamTestEntries1;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;

    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id1";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries1.push_back(shamTestEnty1);
    shamTestEntries1.push_back(shamTestEnty2);
    shamTestEntries1.push_back(shamTestEnty3);

    std::vector<Entry> shamTestEntries2;
    Entry shamTestEnty4, shamTestEnty5;
    shamTestEnty4.shamTestKey = "Id1";
    shamTestEnty4.shamTestValue = "subscribe";
    shamTestEnty5.shamTestKey = "Id2";
    shamTestEnty5.shamTestValue = "subscribe";
    shamTestEntries2.push_back(shamTestEnty4);
    shamTestEntries2.push_back(shamTestEnty5);

    shamStatus = shamTestStore_->PutBatch(shamTestEntries1);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->updateShamEntries_.size()), 0);
    ASSERT_EQ(static_cast<int>(shamObserver->deleteShamEntries_.size()), 0);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 2);
    ASSERT_EQ("Id1", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());
    ASSERT_EQ("Id3", shamObserver->insertShamEntries_[1].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[1].shamTestValue.ToString());
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification015b
 * @tc.desc: Subscribe to an shamObserver, callback with notification many times after putbatch complex data
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification015b, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification015b begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    std::vector<Entry> shamTestEntries1;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;

    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id1";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries1.push_back(shamTestEnty1);
    shamTestEntries1.push_back(shamTestEnty2);
    shamTestEntries1.push_back(shamTestEnty3);

    std::vector<Entry> shamTestEntries2;
    Entry shamTestEnty4, shamTestEnty5;
    shamTestEnty4.shamTestKey = "Id1";
    shamTestEnty4.shamTestValue = "subscribe";
    shamTestEnty5.shamTestKey = "Id2";
    shamTestEnty5.shamTestValue = "subscribe";
    shamTestEntries2.push_back(shamTestEnty4);
    shamTestEntries2.push_back(shamTestEnty5);
    shamStatus = shamTestStore_->PutBatch(shamTestEntries2);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(2)), 2);
    ASSERT_EQ(static_cast<int>(shamObserver->updateShamEntries_.size()), 1);
    ASSERT_EQ("Id1", shamObserver->updateShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->updateShamEntries_[0].shamTestValue.ToString());
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 1);
    ASSERT_EQ("Id2", shamObserver->insertShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->insertShamEntries_[0].shamTestValue.ToString());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}
/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification016
 * @tc.desc: Pressure test subscribe, callback with notification many times after putbatch
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification016, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification016 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    int times = 100; // 100 times
    std::vector<Entry> shamTestEntries;
    for (int i = 0; i < times; i++) {
        Entry shamTestEnty;
        shamTestEnty.shamTestKey = std::to_string(i);
        shamTestEnty.shamTestValue = "subscribe";
        shamTestEntries.push_back(shamTestEnty);
    }

    shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 100);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification017
 * @tc.desc: Subscribe to an shamObserver, callback with notification after delete success
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification017, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification017 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;
    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    Status shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";

    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";
    shamStatus = shamTestStore_->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore Delete data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->deleteShamEntries_.size()), 1);
    ASSERT_EQ("Id1", shamObserver->deleteShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->deleteShamEntries_[0].shamTestValue.ToString());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification018
 * @tc.desc: Subscribe to an shamObserver, not callback after delete which shamTestKey not exist
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification018, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification018 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;
    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    Status shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";

    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";
    shamStatus = shamTestStore_->Delete("Id4");
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore Delete data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 0);
    ASSERT_EQ(static_cast<int>(shamObserver->deleteShamEntries_.size()), 0);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification019
* @tc.desc: Subscribe to an shamObserver,
  delete the same data many times and only first delete callback with notification
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification019, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification019 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;
    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    Status shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";

    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";
    shamStatus = shamTestStore_->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore Delete data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->deleteShamEntries_.size()), 1);
    ASSERT_EQ("Id1", shamObserver->deleteShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->deleteShamEntries_[0].shamTestValue.ToString());

    shamStatus = shamTestStore_->Delete("Id1");
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore Delete data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(2)), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->deleteShamEntries_.size()), 1); // not callback so not clear

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification020
 * @tc.desc: Subscribe to an shamObserver, callback with notification after deleteBatch
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification020, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification020 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;
    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    std::vector<Key> keys;
    keys.push_back("Id1");
    keys.push_back("Id2");

    Status shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";

    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    shamStatus = shamTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore DeleteBatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->deleteShamEntries_.size()), 2);
    ASSERT_EQ("Id1", shamObserver->deleteShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->deleteShamEntries_[0].shamTestValue.ToString());
    ASSERT_EQ("Id2", shamObserver->deleteShamEntries_[1].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->deleteShamEntries_[1].shamTestValue.ToString());

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification021
 * @tc.desc: Subscribe to an shamObserver, not callback after deleteBatch which all keys not exist
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification021, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification021 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;
    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    std::vector<Key> keys;
    keys.push_back("Id4");
    keys.push_back("Id5");

    Status shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";

    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    shamStatus = shamTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore DeleteBatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 0);
    ASSERT_EQ(static_cast<int>(shamObserver->deleteShamEntries_.size()), 0);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
* @tc.name: KvStoreDdmSubscribeKvStoreNotification022
* @tc.desc: Subscribe to an shamObserver,
  deletebatch the same data many times and only first deletebatch callback with
* notification
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification022, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification022 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;
    shamTestEnty1.shamTestKey = "Id1";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id2";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id3";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    std::vector<Key> keys;
    keys.push_back("Id1");
    keys.push_back("Id2");

    Status shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";

    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    shamStatus = shamTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore DeleteBatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->deleteShamEntries_.size()), 2);
    ASSERT_EQ("Id1", shamObserver->deleteShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->deleteShamEntries_[0].shamTestValue.ToString());
    ASSERT_EQ("Id2", shamObserver->deleteShamEntries_[1].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->deleteShamEntries_[1].shamTestValue.ToString());

    shamStatus = shamTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore DeleteBatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(2)), 1);
    ASSERT_EQ(static_cast<int>(shamObserver->deleteShamEntries_.size()), 2); // not callback so not clear

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification023
 * @tc.desc: Subscribe to an shamObserver, include Clear Put PutBatch Delete DeleteBatch
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification023, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification023 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    Key shamTestKey1 = "Id1";
    Value shamTestValue1 = "subscribe";

    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;
    shamTestEnty1.shamTestKey = "Id2";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id3";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id4";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    std::vector<Key> keys;
    keys.push_back("Id2");
    keys.push_back("Id3");

    shamStatus = shamTestStore_->Put(shamTestKey1, shamTestValue1); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    shamStatus = shamTestStore_->Delete(shamTestKey1);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore delete data return wrong";
    shamStatus = shamTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore DeleteBatch data return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount(4)), 4);
    // every callback will clear vector
    ASSERT_EQ(static_cast<int>(shamObserver->deleteShamEntries_.size()), 2);
    ASSERT_EQ("Id2", shamObserver->deleteShamEntries_[0].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->deleteShamEntries_[0].shamTestValue.ToString());
    ASSERT_EQ("Id3", shamObserver->deleteShamEntries_[1].shamTestKey.ToString());
    ASSERT_EQ("subscribe", shamObserver->deleteShamEntries_[1].shamTestValue.ToString());
    ASSERT_EQ(static_cast<int>(shamObserver->updateShamEntries_.size()), 0);
    ASSERT_EQ(static_cast<int>(shamObserver->insertShamEntries_.size()), 0);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}

/**
 * @tc.name: KvStoreDdmSubscribeKvStoreNotification024
 * @tc.desc: Subscribe to an shamObserver[use transaction], include Clear Put PutBatch Delete DeleteBatch
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(LocalKvStoreShamUnitTest, KvStoreDdmSubscribeKvStoreNotification024, TestSize.Level2)
{
    ZLOGI("KvStoreDdmSubscribeKvStoreNotification024 begin.");
    auto shamObserver = std::make_shared<DeviceObserverShamUnitTest>();
    SubscribeType shamSubscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status shamStatus = shamTestStore_->SubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "SubscribeKvStore return wrong";

    Key shamTestKey1 = "Id1";
    Value shamTestValue1 = "subscribe";

    std::vector<Entry> shamTestEntries;
    Entry shamTestEnty1, shamTestEnty2, shamTestEnty3;
    shamTestEnty1.shamTestKey = "Id2";
    shamTestEnty1.shamTestValue = "subscribe";
    shamTestEnty2.shamTestKey = "Id3";
    shamTestEnty2.shamTestValue = "subscribe";
    shamTestEnty3.shamTestKey = "Id4";
    shamTestEnty3.shamTestValue = "subscribe";
    shamTestEntries.push_back(shamTestEnty1);
    shamTestEntries.push_back(shamTestEnty2);
    shamTestEntries.push_back(shamTestEnty3);

    std::vector<Key> keys;
    keys.push_back("Id2");
    keys.push_back("Id3");

    shamStatus = shamTestStore_->StartTransaction();
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore startTransaction return wrong";
    shamStatus = shamTestStore_->Put(shamTestKey1, shamTestValue1); // insert or update shamTestKey-shamTestValue
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore put data return wrong";
    shamStatus = shamTestStore_->PutBatch(shamTestEntries);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore putbatch data return wrong";
    shamStatus = shamTestStore_->Delete(shamTestKey1);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore delete data return wrong";
    shamStatus = shamTestStore_->DeleteBatch(keys);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore DeleteBatch data return wrong";
    shamStatus = shamTestStore_->Commit();
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "KvStore Commit return wrong";
    ASSERT_EQ(static_cast<int>(shamObserver->GetCallCount()), 1);

    shamStatus = shamTestStore_->UnSubscribeKvStore(shamSubscribeType, shamObserver);
    ASSERT_EQ(Status::SUCCESS, shamStatus) << "UnSubscribeKvStore return wrong";
}