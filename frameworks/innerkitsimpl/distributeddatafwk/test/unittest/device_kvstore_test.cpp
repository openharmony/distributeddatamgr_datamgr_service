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
#include <unistd.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "distributed_kv_data_manager.h"
#include "types.h"

using namespace testing::ext;
using namespace OHOS::DistributedKv;

class DeviceKvStoreTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp();

    void TearDown();
    static std::string GetKey(const std::string &key);

    static std::shared_ptr<SingleKvStore> kvStore_; // declare kvstore instance.
    static Status status_;
    static std::string deviceId_;
    static Options options_;
    static int MAX_VALUE_SIZE;
};

const std::string VALID_SCHEMA = "{\"SCHEMA_VERSION\":\"1.0\","
                                 "\"SCHEMA_MODE\":\"STRICT\","
                                 "\"SCHEMA_SKIPSIZE\":0,"
                                 "\"SCHEMA_DEFINE\":{"
                                 "\"age\":\"INTEGER, NOT NULL\""
                                 "},"
                                 "\"SCHEMA_INDEXES\":[\"$.age\"]}";

std::shared_ptr<SingleKvStore> DeviceKvStoreTest::kvStore_ = nullptr;
Status DeviceKvStoreTest::status_ = Status::ERROR;
std::string DeviceKvStoreTest::deviceId_;
Options DeviceKvStoreTest::options_;
int DeviceKvStoreTest::MAX_VALUE_SIZE = 4 * 1024 * 1024; // max value size is 4M.

void DeviceKvStoreTest::SetUpTestCase(void)
{
    DistributedKvDataManager manager;
    options_.area = EL1;
    options_.baseDir = std::string("/data/service/el1/public/database/odmf");
    mkdir(options_.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    AppId appId = { "odmf" };
    StoreId storeId = { "student_device" }; // define kvstore(database) name.
    // [create and] open and initialize kvstore instance.
    status_ = manager.GetSingleKvStore(options_, appId, storeId, kvStore_);
    DeviceInfo deviceInfo;
    manager.GetLocalDevice(deviceInfo);
    deviceId_ = deviceInfo.deviceId;
}

void DeviceKvStoreTest::TearDownTestCase(void)
{
    DistributedKvDataManager manager;
    AppId appId = { "odmf" };
    manager.DeleteAllKvStore(appId, options_.baseDir);
    (void)remove("/data/service/el1/public/database/odmf/key");
    (void)remove("/data/service/el1/public/database/odmf/kvdb");
    (void)remove("/data/service/el1/public/database/odmf");
}

void DeviceKvStoreTest::SetUp(void)
{}

void DeviceKvStoreTest::TearDown(void)
{}

std::string DeviceKvStoreTest::GetKey(const std::string& key)
{
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(sizeof(uint32_t)) << deviceId_.length();
    oss << deviceId_ << std::string(key.begin(), key.end());
    return oss.str();
}

class DeviceObserverTestImpl : public KvStoreObserver {
public:
    std::vector<Entry> insertEntries_;
    std::vector<Entry> updateEntries_;
    std::vector<Entry> deleteEntries_;
    bool isClear_ = false;
    DeviceObserverTestImpl();
    ~DeviceObserverTestImpl()
    {}

    DeviceObserverTestImpl(const DeviceObserverTestImpl &) = delete;
    DeviceObserverTestImpl &operator=(const DeviceObserverTestImpl &) = delete;
    DeviceObserverTestImpl(DeviceObserverTestImpl &&) = delete;
    DeviceObserverTestImpl &operator=(DeviceObserverTestImpl &&) = delete;

    void OnChange(const ChangeNotification &changeNotification);

    // reset the callCount_ to zero.
    void ResetToZero();

    uint64_t GetCallCount() const;

private:
    uint64_t callCount_;
};

void DeviceObserverTestImpl::OnChange(const ChangeNotification &changeNotification)
{
    callCount_++;
    const auto &insert = changeNotification.GetInsertEntries();
    insertEntries_.clear();
    for (const auto &entry : insert) {
        insertEntries_.push_back(entry);
    }

    const auto &update = changeNotification.GetUpdateEntries();
    updateEntries_.clear();
    for (const auto &entry : update) {
        updateEntries_.push_back(entry);
    }

    const auto &del = changeNotification.GetDeleteEntries();
    deleteEntries_.clear();
    for (const auto &entry : del) {
        deleteEntries_.push_back(entry);
    }

    isClear_ = changeNotification.IsClear();
}

DeviceObserverTestImpl::DeviceObserverTestImpl()
{
    callCount_ = 0;
    insertEntries_ = {};
    updateEntries_ = {};
    deleteEntries_ = {};
    isClear_ = false;
}

void DeviceObserverTestImpl::ResetToZero()
{
    callCount_ = 0;
}

uint64_t DeviceObserverTestImpl::GetCallCount() const
{
    return callCount_;
}

class DeviceSyncCallbackTestImpl : public KvStoreSyncCallback {
public:
    void SyncCompleted(const std::map<std::string, Status> &results);
};

void DeviceSyncCallbackTestImpl::SyncCompleted(const std::map<std::string, Status> &results)
{}

/**
* @tc.name: GetStoreId001
* @tc.desc: Get a Device KvStore instance.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, GetStoreId001, TestSize.Level1)
{
    EXPECT_NE(kvStore_, nullptr) << "kvStore is null.";

    auto storID = kvStore_->GetStoreId();
    EXPECT_EQ(storID.storeId, "student_device");
}

/**
* @tc.name: PutGetDelete001
* @tc.desc: put value and delete value
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, PutGetDelete001, TestSize.Level1)
{
    EXPECT_NE(kvStore_, nullptr) << "kvStore is null.";

    Key skey = {"single_001"};
    Value sval = {"value_001"};
    auto status = kvStore_->Put(skey, sval);
    EXPECT_EQ(status, Status::SUCCESS) << "Put data failed";

    auto delStatus = kvStore_->Delete(skey);
    EXPECT_EQ(delStatus, Status::SUCCESS) << "Delete data failed";

    auto notExistStatus = kvStore_->Delete(skey);
    EXPECT_EQ(notExistStatus, Status::SUCCESS) << "Delete non-existing data failed";

    auto spaceStatus = kvStore_->Put(skey, {""});
    EXPECT_EQ(spaceStatus, Status::SUCCESS) << "Put space failed";

    auto spaceKeyStatus = kvStore_->Put({""}, {""});
    EXPECT_NE(spaceKeyStatus, Status::SUCCESS) << "Put space keys failed";

    Status validStatus = kvStore_->Put(skey, sval);
    EXPECT_EQ(validStatus, Status::SUCCESS) << "Put valid keys and values failed";

    Value rVal;
    auto validPutStatus = kvStore_->Get({ GetKey("single_001")}, rVal);
    EXPECT_EQ(validPutStatus, Status::SUCCESS) << "Get value failed";
    EXPECT_EQ(sval, rVal) << "Got and put values not equal";
}

/**
* @tc.name: GetDataQueryEntriesAndResultSet
* @tc.desc: get entries and result set by data query.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, GetDataQueryEntriesAndResultSet, TestSize.Level1)
{
    EXPECT_NE(kvStore_, nullptr) << "kvStore is nullptr.";

    // prepare 10
    size_t sum = 10;
    int sum_1 = 10;
    std::string prefix = "prefix_";
    for (size_t i = 0; i < sum; i++) {
        kvStore_->Put({prefix + std::to_string(i)}, {std::to_string(i)});
    }

    DataQuery dataQuery;
    dataQuery.KeyPrefix(prefix);
    dataQuery.DeviceId(deviceId_);
    kvStore_->GetCount(dataQuery, sum_1);
    EXPECT_EQ(sum_1, sum) << "count is not equal 10.";

    std::vector<Entry> results;
    kvStore_->GetEntries(dataQuery, results);
    EXPECT_EQ(results.size(), sum) << "entries size is not equal 10.";

    std::shared_ptr<KvStoreResultSet> resultSet;
    Status status = kvStore_->GetResultSet(dataQuery, resultSet);
    EXPECT_EQ(status, Status::SUCCESS);
    EXPECT_EQ(resultSet->GetCount(), sum_1) << "resultSet size is not equal 10.";
    resultSet->IsFirst();
    resultSet->IsAfterLast();
    resultSet->IsBeforeFirst();
    resultSet->MoveToPosition(1);
    resultSet->IsLast();
    resultSet->MoveToPrevious();
    resultSet->MoveToNext();
    resultSet->MoveToLast();
    resultSet->MoveToFirst();
    resultSet->GetPosition();
    Entry entry;
    resultSet->GetEntry(entry);

    for (size_t i = 0; i < sum; i++) {
        kvStore_->Delete({GetKey(prefix + std::to_string(i))});
    }

    status = kvStore_->CloseResultSet(resultSet);
    EXPECT_EQ(status, Status::SUCCESS) << "Close resultSet failed.";
}

/**
* @tc.name: GetPrefixQueryEntriesAndResultSet
* @tc.desc: get entries and result set by prefix query.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, GetPrefixQueryEntriesAndResultSet, TestSize.Level1)
{
    EXPECT_NE(kvStore_, nullptr) << "kvStore is nullptr.";
    if (options_.baseDir.empty()) {
        return;
    }

    // prepare 10
    size_t sum = 10;
    int sum_1 = 10;
    std::string prefix = "prefix_";
    for (size_t i = 0; i < sum; i++) {
        kvStore_->Put({prefix + std::to_string(i)}, {std::to_string(i)});
    }

    DataQuery dataQuery;
    dataQuery.KeyPrefix(GetKey(prefix));
    kvStore_->GetCount(dataQuery, sum_1);
    EXPECT_EQ(sum_1, sum) << "count is not equal 10.";

    std::vector<Entry> results;
    kvStore_->GetEntries(dataQuery, results);
    EXPECT_EQ(results.size(), sum) << "entries size is not equal 10.";

    std::shared_ptr<KvStoreResultSet> resultSet;
    Status status = kvStore_->GetResultSet(dataQuery, resultSet);
    EXPECT_EQ(status, Status::SUCCESS);
    EXPECT_EQ(resultSet->GetCount(), sum_1) << "resultSet size is not equal 10.";
    resultSet->IsFirst();
    resultSet->IsAfterLast();
    resultSet->IsBeforeFirst();
    resultSet->MoveToPosition(1);
    resultSet->IsLast();
    resultSet->MoveToPrevious();
    resultSet->MoveToNext();
    resultSet->MoveToLast();
    resultSet->MoveToFirst();
    resultSet->GetPosition();
    Entry entry;
    resultSet->GetEntry(entry);

    for (size_t i = 0; i < sum; i++) {
        kvStore_->Delete({GetKey(prefix + std::to_string(i))});
    }

    status = kvStore_->CloseResultSet(resultSet);
    EXPECT_EQ(status, Status::SUCCESS) << "Close resultSet failed.";
}

/**
* @tc.name: GetPrefixEntriesAndResultSet
* @tc.desc: get entries and result set by prefix.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, GetPrefixEntriesAndResultSet, TestSize.Level1)
{
    EXPECT_NE(kvStore_, nullptr) << "kvStore is nullptr.";

    // prepare 10
    size_t sum = 10;
    int sum_1 = 10;
    std::string prefix = "prefix_";
    for (size_t i = 0; i < sum; i++) {
        kvStore_->Put({prefix + std::to_string(i)}, {std::to_string(i)});
    }
    std::vector<Entry> results;
    kvStore_->GetEntries(GetKey(prefix + "      "), results);
    EXPECT_EQ(results.size(), sum) << "entries size is not equal 10.";

    std::shared_ptr<KvStoreResultSet> resultSet;
    Status status = kvStore_->GetResultSet({ GetKey(std::string("    ") + prefix + "      ") }, resultSet);
    EXPECT_EQ(status, Status::SUCCESS);
    EXPECT_EQ(resultSet->GetCount(), sum_1) << "resultSet size is not equal 10.";
    resultSet->IsFirst();
    resultSet->IsAfterLast();
    resultSet->IsBeforeFirst();
    resultSet->MoveToPosition(1);
    resultSet->IsLast();
    resultSet->MoveToPrevious();
    resultSet->MoveToNext();
    resultSet->MoveToLast();
    resultSet->MoveToFirst();
    resultSet->GetPosition();
    Entry entry;
    resultSet->GetEntry(entry);

    for (size_t i = 0; i < sum; i++) {
        kvStore_->Delete({GetKey(prefix + std::to_string(i))});
    }

    status = kvStore_->CloseResultSet(resultSet);
    EXPECT_EQ(status, Status::SUCCESS) << "Close resultSet failed.";
}

/**
* @tc.name: Subscribe001
* @tc.desc: Put data and get callback.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, Subscribe001, TestSize.Level1)
{
    auto observer = std::make_shared<DeviceObserverTestImpl>();
    auto subStatus = kvStore_->SubscribeKvStore(SubscribeType::SUBSCRIBE_TYPE_ALL, observer);
    EXPECT_EQ(subStatus, Status::SUCCESS) << "Subscribe observer failed.";
    // subscribe repeated observer;
    auto repeatedSubStatus = kvStore_->SubscribeKvStore(SubscribeType::SUBSCRIBE_TYPE_ALL, observer);
    EXPECT_NE(repeatedSubStatus, Status::SUCCESS) << "Repeat subscribe kvStore observer failed.";

    auto unSubStatus = kvStore_->UnSubscribeKvStore(SubscribeType::SUBSCRIBE_TYPE_ALL, observer);
    EXPECT_EQ(unSubStatus, Status::SUCCESS) << "Unsubscribe observer failed.";
}

/**
* @tc.name: SyncCallback001
* @tc.desc: Register sync callback.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, SyncCallback001, TestSize.Level1)
{
    EXPECT_NE(kvStore_, nullptr) << "kvStore is nullptr.";

    auto syncCallback = std::make_shared<DeviceSyncCallbackTestImpl>();
    auto syncStatus = kvStore_->RegisterSyncCallback(syncCallback);
    EXPECT_EQ(syncStatus, Status::SUCCESS) << "Register sync callback failed.";

    auto unRegStatus = kvStore_->UnRegisterSyncCallback();
    EXPECT_EQ(unRegStatus, Status::SUCCESS) << "Unregister sync callback failed.";

    Key skey = {"single_001"};
    Value sval = {"value_001"};
    kvStore_->Put(skey, sval);
    kvStore_->Delete(skey);

    std::map<std::string, Status> results;
    results.insert({"aaa", Status::INVALID_ARGUMENT});
    syncCallback->SyncCompleted(results);
}

/**
* @tc.name: RemoveDeviceData001
* @tc.desc: Remove device data.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, RemoveDeviceData001, TestSize.Level1)
{
    EXPECT_NE(kvStore_, nullptr) << "kvStore is nullptr.";

    Key skey = {"single_001"};
    Value sval = {"value_001"};
    kvStore_->Put(skey, sval);

    std::string deviceId = "no_exist_device_id";
    auto removeStatus = kvStore_->RemoveDeviceData(deviceId);
    EXPECT_NE(removeStatus, Status::SUCCESS) << "Remove device should not return success";

    Value retVal;
    auto getRet = kvStore_->Get(GetKey(skey.ToString()), retVal);
    EXPECT_EQ(getRet, Status::SUCCESS) << "Get value failed.";
    EXPECT_EQ(retVal.Size(), sval.Size()) << "data base should be null.";
}

/**
* @tc.name: SyncData001
* @tc.desc: Synchronize device data.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, SyncData001, TestSize.Level1)
{
    EXPECT_NE(kvStore_, nullptr) << "kvStore is nullptr.";
    std::string deviceId = "no_exist_device_id";
    std::vector<std::string> deviceIds = { deviceId };
    auto syncStatus = kvStore_->Sync(deviceIds, SyncMode::PUSH);
    EXPECT_NE(syncStatus, Status::SUCCESS) << "Sync device should not return success";
}

/**
* @tc.name: TestSchemaStoreC001
* @tc.desc: Test schema device store.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, TestSchemaStoreC001, TestSize.Level1)
{
    std::shared_ptr<SingleKvStore> deviceKvStore;
    DistributedKvDataManager manager;
    Options options;
    options.encrypt = true;
    options.schema = VALID_SCHEMA;
    options.area = EL1;
    options.baseDir = std::string("/data/service/el1/public/database/odmf");
    AppId appId = { "odmf" };
    StoreId storeId = { "schema_device_id" };
    (void)manager.GetSingleKvStore(options, appId, storeId, deviceKvStore);
    ASSERT_NE(deviceKvStore, nullptr) << "kvStorePtr is null.";
    auto result = deviceKvStore->GetStoreId();
    EXPECT_EQ(result.storeId, "schema_device_id");

    Key testKey = {"TestSchemaStoreC001_key"};
    Value testValue = {"{\"age\":10}"};
    auto testStatus = deviceKvStore->Put(testKey, testValue);
    EXPECT_EQ(testStatus, Status::SUCCESS) << "putting data failed";
    Value resultValue;
    auto status = deviceKvStore->Get(GetKey(testKey.ToString()), resultValue);
    EXPECT_EQ(status, Status::SUCCESS) << "get value failed.";
    manager.DeleteKvStore(appId, storeId, options.baseDir);
}

/**
* @tc.name: SyncData001
* @tc.desc: Synchronize device data.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, SyncData002, TestSize.Level1)
{
    EXPECT_NE(kvStore_, nullptr) << "kvStorePtr is null.";
    std::string deviceId = "no_exist_device_id";
    std::vector<std::string> deviceIds = { deviceId };
    uint32_t allowedDelayMs = 200;
    auto syncStatus = kvStore_->Sync(deviceIds, SyncMode::PUSH, allowedDelayMs);
    EXPECT_EQ(syncStatus, Status::SUCCESS) << "sync device should return success";
}

/**
* @tc.name: SyncData002
* @tc.desc: Set sync parameters - success.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, SetSync001, TestSize.Level1)
{
    EXPECT_NE(kvStore_, nullptr) << "kvStore is null.";
    KvSyncParam syncParam{ 500 }; // 500ms
    auto ret = kvStore_->SetSyncParam(syncParam);
    EXPECT_EQ(ret, Status::SUCCESS) << "set sync param should return success";

    KvSyncParam syncParamRet;
    kvStore_->GetSyncParam(syncParamRet);
    EXPECT_EQ(syncParamRet.allowedDelayMs, syncParam.allowedDelayMs);
}

/**
* @tc.name: SyncData002
* @tc.desc: Set sync parameters - failed.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, SetSync002, TestSize.Level1)
{
    EXPECT_NE(kvStore_, nullptr) << "kvStore is null.";
    KvSyncParam syncParam2{ 50 }; // 50ms
    auto ret = kvStore_->SetSyncParam(syncParam2);
    EXPECT_NE(ret, Status::SUCCESS) << "set sync param should not return success";

    KvSyncParam syncParamRet2;
    ret = kvStore_->GetSyncParam(syncParamRet2);
    EXPECT_NE(syncParamRet2.allowedDelayMs, syncParam2.allowedDelayMs);
}

/**
* @tc.name: SingleKvStoreDdmPutBatch001
* @tc.desc: Batch put data.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, SingleKvStoreDdmPutBatch001, TestSize.Level2)
{
    EXPECT_NE(nullptr, kvStore_) << "kvStore is nullptr";

    // store entries to kvstore.
    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "KvStoreDdmPutBatch001_1";
    entry1.value = "age:20";
    entry2.key = "KvStoreDdmPutBatch001_2";
    entry2.value = "age:19";
    entry3.key = "KvStoreDdmPutBatch001_3";
    entry3.value = "age:23";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    Status status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "PutBatch data return wrong status";
    // get value from kvstore.
    Value valueRet1;
    Status statusRet1 = kvStore_->Get(GetKey(entry1.key.ToString()), valueRet1);
    EXPECT_EQ(Status::SUCCESS, statusRet1) << "Get data return wrong status";
    EXPECT_EQ(entry1.value, valueRet1) << "value and valueRet are not equal";

    Value valueRet2;
    Status statusRet2 = kvStore_->Get(GetKey(entry2.key.ToString()), valueRet2);
    EXPECT_EQ(Status::SUCCESS, statusRet2) << "Get data return wrong status";
    EXPECT_EQ(entry2.value, valueRet2) << "value and valueRet are not equal";

    Value valueRet3;
    Status statusRet3 = kvStore_->Get(GetKey(entry3.key.ToString()), valueRet3);
    EXPECT_EQ(Status::SUCCESS, statusRet3) << "Get data return wrong status";
    EXPECT_EQ(entry3.value, valueRet3) << "value and valueRet are not equal";
}

/**
* @tc.name: SingleKvStoreDdmPutBatch002
* @tc.desc: Batch update data.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, SingleKvStoreDdmPutBatch002, TestSize.Level2)
{
    EXPECT_NE(nullptr, kvStore_) << "kvStore is nullptr";

    // before update.
    std::vector<Entry> entriesBefore;
    Entry entry1, entry2, entry3;
    entry1.key = "SingleKvStoreDdmPutBatch002_1";
    entry1.value = "age:20";
    entry2.key = "SingleKvStoreDdmPutBatch002_2";
    entry2.value = "age:19";
    entry3.key = "SingleKvStoreDdmPutBatch002_3";
    entry3.value = "age:23";
    entriesBefore.push_back(entry1);
    entriesBefore.push_back(entry2);
    entriesBefore.push_back(entry3);

    Status status = kvStore_->PutBatch(entriesBefore);
    EXPECT_EQ(Status::SUCCESS, status) << "PutBatch data return wrong status";

    // after update.
    std::vector<Entry> entriesAfter;
    Entry entry4, entry5, entry6;
    entry4.key = "SingleKvStoreDdmPutBatch002_1";
    entry4.value = "age:20, sex:girl";
    entry5.key = "SingleKvStoreDdmPutBatch002_2";
    entry5.value = "age:19, sex:boy";
    entry6.key = "SingleKvStoreDdmPutBatch002_3";
    entry6.value = "age:23, sex:girl";
    entriesAfter.push_back(entry4);
    entriesAfter.push_back(entry5);
    entriesAfter.push_back(entry6);

    status = kvStore_->PutBatch(entriesAfter);
    EXPECT_EQ(Status::SUCCESS, status) << "PutBatch failed, wrong status";

    // get value from kvstore.
    Value valueRet1;
    Status statusRet1 = kvStore_->Get(GetKey(entry4.key.ToString()), valueRet1);
    EXPECT_EQ(Status::SUCCESS, statusRet1) << "Get data failed, wrong status";
    EXPECT_EQ(entry4.value, valueRet1) << "value and valueRet are not equal";

    Value valueRet2;
    Status statusRet2 = kvStore_->Get(GetKey(entry5.key.ToString()), valueRet2);
    EXPECT_EQ(Status::SUCCESS, statusRet2) << "Get data failed, wrong status";
    EXPECT_EQ(entry5.value, valueRet2) << "value and valueRet are not equal";

    Value valueRet3;
    Status statusRet3 = kvStore_->Get(GetKey(entry6.key.ToString()), valueRet3);
    EXPECT_EQ(Status::SUCCESS, statusRet3) << "Get data return wrong status";
    EXPECT_EQ(entry6.value, valueRet3) << "value and valueRet are not equal";
}

/**
* @tc.name: DdmPutBatch003
* @tc.desc: Batch put data that contains invalid data.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, DdmPutBatch003, TestSize.Level2)
{
    EXPECT_NE(nullptr, kvStore_) << "kvStore is nullptr";

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "         ";
    entry1.value = "age:20";
    entry2.key = "student_name_caixu";
    entry2.value = "         ";
    entry3.key = "student_name_liuyue";
    entry3.value = "age:23";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    Status status = kvStore_->PutBatch(entries);
    Status target = options_.baseDir.empty() ? Status::SUCCESS : Status::INVALID_ARGUMENT;
    EXPECT_EQ(target, status) << "PutBatch data return wrong status";
}

/**
* @tc.name: DdmPutBatch004
* @tc.desc: Batch put data that contains invalid data.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, DdmPutBatch004, TestSize.Level2)
{
    EXPECT_NE(nullptr, kvStore_) << "kvStore is nullptr";

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "";
    entry1.value = "age:20";
    entry2.key = "student_name_caixu";
    entry2.value = "";
    entry3.key = "student_name_liuyue";
    entry3.value = "age:23";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    Status status = kvStore_->PutBatch(entries);
    Status target = options_.baseDir.empty() ? Status::SUCCESS : Status::INVALID_ARGUMENT;
    EXPECT_EQ(target, status) << "PutBatch data return wrong status";
}

static std::string SingleGenerate1025KeyLen()
{
    std::string str("prefix");
    // Generate a key with a length of more than 1024 bytes.
    for (int i = 0; i < 1024; i++) {
        str += "a";
    }
    return str;
}
/**
* @tc.name: DdmPutBatch005
* @tc.desc: Batch put data that contains invalid data.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, DdmPutBatch005, TestSize.Level2)
{
    EXPECT_NE(nullptr, kvStore_) << "kvStore is nullptr";
    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = SingleGenerate1025KeyLen();
    entry1.value = "age:20";
    entry2.key = "student_name_caixu";
    entry2.value = "age:19";
    entry3.key = "student_name_liuyue";
    entry3.value = "age:23";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    Status status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::INVALID_ARGUMENT, status) << "PutBatch data return wrong status";
}

/**
* @tc.name: DdmPutBatch006
* @tc.desc: Batch put large data.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, DdmPutBatch006, TestSize.Level2)
{
    EXPECT_NE(nullptr, kvStore_) << "kvStore is nullptr";

    std::vector<uint8_t> val(MAX_VALUE_SIZE);
    for (int i = 0; i < MAX_VALUE_SIZE; i++) {
        val[i] = static_cast<uint8_t>(i);
    }
    Value value = val;

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "SingleKvStoreDdmPutBatch006_1";
    entry1.value = value;
    entry2.key = "SingleKvStoreDdmPutBatch006_2";
    entry2.value = value;
    entry3.key = "SingleKvStoreDdmPutBatch006_3";
    entry3.value = value;
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);
    Status status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "PutBatch data return wrong status";

    // get value from kvstore.
    Value valueRet1;
    Status statusRet1 = kvStore_->Get(GetKey("SingleKvStoreDdmPutBatch006_1"), valueRet1);
    EXPECT_EQ(Status::SUCCESS, statusRet1) << "Get data return wrong status";
    EXPECT_EQ(entry1.value, valueRet1) << "value and valueRet are not equal";

    Value valueRet2;
    Status statusRet2 = kvStore_->Get(GetKey("SingleKvStoreDdmPutBatch006_2"), valueRet2);
    EXPECT_EQ(Status::SUCCESS, statusRet2) << "Get data return wrong status";
    EXPECT_EQ(entry2.value, valueRet2) << "value and valueRet are not equal";

    Value valueRet3;
    Status statusRet3 = kvStore_->Get(GetKey("SingleKvStoreDdmPutBatch006_3"), valueRet3);
    EXPECT_EQ(Status::SUCCESS, statusRet3) << "Get data return wrong status";
    EXPECT_EQ(entry3.value, valueRet3) << "value and valueRet are not equal";
}

/**
* @tc.name: DdmDeleteBatch001
* @tc.desc: Batch delete data.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, DdmDeleteBatch001, TestSize.Level2)
{
    EXPECT_NE(nullptr, kvStore_) << "kvStore is nullptr";

    // store entries to kvstore.
    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "SingleKvStoreDdmDeleteBatch001_1";
    entry1.value = "age:20";
    entry2.key = "SingleKvStoreDdmDeleteBatch001_2";
    entry2.value = "age:19";
    entry3.key = "SingleKvStoreDdmDeleteBatch001_3";
    entry3.value = "age:23";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    std::vector<Key> keys;
    keys.push_back("SingleKvStoreDdmDeleteBatch001_1");
    keys.push_back("SingleKvStoreDdmDeleteBatch001_2");
    keys.push_back("SingleKvStoreDdmDeleteBatch001_3");

    Status status1 = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status1) << "PutBatch data return wrong status";

    Status status2 = kvStore_->DeleteBatch(keys);
    EXPECT_EQ(Status::SUCCESS, status2) << "DeleteBatch data return wrong status";
    std::vector<Entry> results;
    kvStore_->GetEntries(GetKey("SingleKvStoreDdmDeleteBatch001_"), results);
    size_t sum = 0;
    EXPECT_EQ(results.size(), sum) << "entries size is not equal 0.";
}

/**
* @tc.name: DdmDeleteBatch002
* @tc.desc: Batch delete data when some keys are not in KvStore.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, DdmDeleteBatch002, TestSize.Level2)
{
    EXPECT_NE(nullptr, kvStore_) << "kvStore is nullptr";

    // store entries to kvstore.
    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "SingleKvStoreDdmDeleteBatch002_1";
    entry1.value = "age:20";
    entry2.key = "SingleKvStoreDdmDeleteBatch002_2";
    entry2.value = "age:19";
    entry3.key = "SingleKvStoreDdmDeleteBatch002_3";
    entry3.value = "age:23";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    std::vector<Key> keys;
    keys.push_back("SingleKvStoreDdmDeleteBatch002_1");
    keys.push_back("SingleKvStoreDdmDeleteBatch002_2");
    keys.push_back("SingleKvStoreDdmDeleteBatch002_3");
    keys.push_back("SingleKvStoreDdmDeleteBatch002_4");

    Status status1 = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status1) << "PutBatch data return wrong status";

    Status status2 = kvStore_->DeleteBatch(keys);
    EXPECT_EQ(Status::SUCCESS, status2) << "DeleteBatch data return wrong status";
    std::vector<Entry> results;
    kvStore_->GetEntries(GetKey("SingleKvStoreDdmDeleteBatch002_"), results);
    size_t sum = 0;
    EXPECT_EQ(results.size(), sum) << "entries size is not equal 0.";
}

/**
* @tc.name: DdmDeleteBatch003
* @tc.desc: Batch delete data when some keys are invalid.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, DdmDeleteBatch003, TestSize.Level2)
{
    EXPECT_NE(nullptr, kvStore_) << "kvStore is nullptr";

    // Store entries to KvStore.
    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "SingleKvStoreDdmDeleteBatch003_1";
    entry1.value = "age:20";
    entry2.key = "SingleKvStoreDdmDeleteBatch003_2";
    entry2.value = "age:19";
    entry3.key = "SingleKvStoreDdmDeleteBatch003_3";
    entry3.value = "age:23";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    std::vector<Key> keys;
    keys.push_back("SingleKvStoreDdmDeleteBatch003_1");
    keys.push_back("SingleKvStoreDdmDeleteBatch003_2");
    keys.push_back("");

    Status status1 = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status1) << "PutBatch data return wrong status";

    Status status2 = kvStore_->DeleteBatch(keys);
    Status target = options_.baseDir.empty() ? Status::SUCCESS : Status::INVALID_ARGUMENT;
    size_t sum = options_.baseDir.empty() ? 1 : 3;
    EXPECT_EQ(target, status2) << "DeleteBatch data return wrong status";
    std::vector<Entry> results;
    kvStore_->GetEntries(GetKey("SingleKvStoreDdmDeleteBatch003_"), results);
    EXPECT_EQ(results.size(), sum) << "entries size is not equal 3.";
}

/**
* @tc.name: DdmDeleteBatch004
* @tc.desc: Batch delete data when some keys are invalid.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, DdmDeleteBatch004, TestSize.Level2)
{
    EXPECT_NE(nullptr, kvStore_) << "kvStore is nullptr";

    // store entries to kvstore.
    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "SingleKvStoreDdmDeleteBatch004_1";
    entry1.value = "age:20";
    entry2.key = "SingleKvStoreDdmDeleteBatch004_2";
    entry2.value = "age:19";
    entry3.key = "SingleKvStoreDdmDeleteBatch004_3";
    entry3.value = "age:23";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    std::vector<Key> keys;
    keys.push_back("SingleKvStoreDdmDeleteBatch004_1");
    keys.push_back("SingleKvStoreDdmDeleteBatch004_2");
    keys.push_back("          ");

    Status status1 = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status1) << "PutBatch data return wrong status";

    std::vector<Entry> results1;
    kvStore_->GetEntries(GetKey("SingleKvStoreDdmDeleteBatch004_"), results1);
    size_t sum1 = 3;
    EXPECT_EQ(results1.size(), sum1) << "entries size1111 is not equal 3.";

    Status status2 = kvStore_->DeleteBatch(keys);
    Status target = options_.baseDir.empty() ? Status::SUCCESS : Status::INVALID_ARGUMENT;
    size_t sum = options_.baseDir.empty() ? 1 : 3;
    EXPECT_EQ(target, status2) << "DeleteBatch data return wrong status";
    std::vector<Entry> results;
    kvStore_->GetEntries(GetKey("SingleKvStoreDdmDeleteBatch004_"), results);
    EXPECT_EQ(results.size(), sum) << "entries size is not equal 3.";
}

/**
* @tc.name: DdmDeleteBatch005
* @tc.desc: Batch delete data when some keys are invalid.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, DdmDeleteBatch005, TestSize.Level2)
{
    EXPECT_NE(nullptr, kvStore_) << "kvStore is nullptr";

    // store entries to kvstore.
    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "SingleKvStoreDdmDeleteBatch005_1";
    entry1.value = "age:20";
    entry2.key = "SingleKvStoreDdmDeleteBatch005_2";
    entry2.value = "age:19";
    entry3.key = "SingleKvStoreDdmDeleteBatch005_3";
    entry3.value = "age:23";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    std::vector<Key> keys;
    keys.push_back("SingleKvStoreDdmDeleteBatch005_1");
    keys.push_back("SingleKvStoreDdmDeleteBatch005_2");
    Key keyTmp = SingleGenerate1025KeyLen();
    keys.push_back(keyTmp);

    Status status1 = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status1) << "PutBatch data return wrong status";

    std::vector<Entry> results1;
    kvStore_->GetEntries(GetKey("SingleKvStoreDdmDeleteBatch005_"), results1);
    size_t sum1 = 3;
    EXPECT_EQ(results1.size(), sum1) << "entries111 size is not equal 3.";

    Status status2 = kvStore_->DeleteBatch(keys);
    EXPECT_EQ(Status::INVALID_ARGUMENT, status2) << "DeleteBatch data return wrong status";
    std::vector<Entry> results;
    kvStore_->GetEntries(GetKey("SingleKvStoreDdmDeleteBatch005_"), results);
    size_t sum = 3;
    EXPECT_EQ(results.size(), sum) << "entries size is not equal 3.";
}

/**
* @tc.name: Transaction001
* @tc.desc: Batch delete data when some keys are invalid.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, Transaction001, TestSize.Level2)
{
    EXPECT_NE(nullptr, kvStore_) << "kvStore is nullptr";
    std::shared_ptr<DeviceObserverTestImpl> observer = std::make_shared<DeviceObserverTestImpl>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    Key key1 = "SingleKvStoreTransaction001_1";
    Value value1 = "subscribe";

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "SingleKvStoreTransaction001_2";
    entry1.value = "subscribe";
    entry2.key = "SingleKvStoreTransaction001_3";
    entry2.value = "subscribe";
    entry3.key = "SingleKvStoreTransaction001_4";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    std::vector<Key> keys;
    keys.push_back("SingleKvStoreTransaction001_2");
    keys.push_back("ISingleKvStoreTransaction001_3");

    status = kvStore_->StartTransaction();
    EXPECT_EQ(Status::SUCCESS, status) << "StartTransaction return wrong status";

    status = kvStore_->Put(key1, value1);  // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "Put data return wrong status";
    status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "PutBatch data return wrong status";
    status = kvStore_->Delete(key1);
    EXPECT_EQ(Status::SUCCESS, status) << "Delete data return wrong status";
    status = kvStore_->DeleteBatch(keys);
    EXPECT_EQ(Status::SUCCESS, status) << "DeleteBatch data return wrong status";
    status = kvStore_->Commit();
    EXPECT_EQ(Status::SUCCESS, status) << "Commit return wrong status";

    usleep(200000);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 1);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
}

/**
* @tc.name: Transaction002
* @tc.desc: Batch delete data when some keys are invalid.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, Transaction002, TestSize.Level2)
{
    EXPECT_NE(nullptr, kvStore_) << "kvStore is nullptr";
    std::shared_ptr<DeviceObserverTestImpl> observer = std::make_shared<DeviceObserverTestImpl>();
    observer->ResetToZero();

    SubscribeType subscribeType = SubscribeType::SUBSCRIBE_TYPE_ALL;
    Status status = kvStore_->SubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "SubscribeKvStore return wrong status";

    Key key1 = "SingleKvStoreTransaction002_1";
    Value value1 = "subscribe";

    std::vector<Entry> entries;
    Entry entry1, entry2, entry3;
    entry1.key = "SingleKvStoreTransaction002_2";
    entry1.value = "subscribe";
    entry2.key = "SingleKvStoreTransaction002_3";
    entry2.value = "subscribe";
    entry3.key = "SingleKvStoreTransaction002_4";
    entry3.value = "subscribe";
    entries.push_back(entry1);
    entries.push_back(entry2);
    entries.push_back(entry3);

    std::vector<Key> keys;
    keys.push_back("SingleKvStoreTransaction002_2");
    keys.push_back("SingleKvStoreTransaction002_3");

    status = kvStore_->StartTransaction();
    EXPECT_EQ(Status::SUCCESS, status) << "StartTransaction return wrong status";

    status = kvStore_->Put(key1, value1);  // insert or update key-value
    EXPECT_EQ(Status::SUCCESS, status) << "Put data return wrong status";
    status = kvStore_->PutBatch(entries);
    EXPECT_EQ(Status::SUCCESS, status) << "PutBatch data return wrong status";
    status = kvStore_->Delete(key1);
    EXPECT_EQ(Status::SUCCESS, status) << "Delete data return wrong status";
    status = kvStore_->DeleteBatch(keys);
    EXPECT_EQ(Status::SUCCESS, status) << "DeleteBatch data return wrong status";
    status = kvStore_->Rollback();
    EXPECT_EQ(Status::SUCCESS, status) << "Commit return wrong status";

    usleep(200000);
    EXPECT_EQ(static_cast<int>(observer->GetCallCount()), 0);
    EXPECT_EQ(static_cast<int>(observer->insertEntries_.size()), 0);
    EXPECT_EQ(static_cast<int>(observer->updateEntries_.size()), 0);
    EXPECT_EQ(static_cast<int>(observer->deleteEntries_.size()), 0);

    status = kvStore_->UnSubscribeKvStore(subscribeType, observer);
    EXPECT_EQ(Status::SUCCESS, status) << "UnSubscribeKvStore return wrong status";
    observer = nullptr;
}

/**
* @tc.name: DeviceSync001
* @tc.desc: Test sync enable.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, DeviceSync001, TestSize.Level1)
{
    std::shared_ptr<SingleKvStore> kvStore;
    DistributedKvDataManager manager;
    Options options;
    options.encrypt = true;
    options.area = EL1;
    options.baseDir = std::string("/data/service/el1/public/database/odmf");
    AppId appId = { "odmf" };
    StoreId storeId = { "schema_device_id001" };
    manager.GetSingleKvStore(options, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr) << "kvStore is null.";
    auto result = kvStore->GetStoreId();
    EXPECT_EQ(result.storeId, "schema_device_id001");

    auto testStatus = kvStore->SetCapabilityEnabled(true);
    EXPECT_EQ(testStatus, Status::SUCCESS) << "set fail";
    manager.DeleteKvStore(appId, storeId, options.baseDir);
}

/**
* @tc.name: DeviceSync002
* @tc.desc: Test sync enable.
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, DeviceSync002, TestSize.Level1)
{
    std::shared_ptr<SingleKvStore> kvStore;
    DistributedKvDataManager manager;
    Options options;
    options.encrypt = true;
    options.area = EL1;
    options.baseDir = std::string("/data/service/el1/public/database/odmf");
    AppId appId = { "odmf" };
    StoreId storeId = { "schema_device_id002" };
    manager.GetSingleKvStore(options, appId, storeId, kvStore);
    ASSERT_NE(kvStore, nullptr) << "kvStore is null.";
    auto result = kvStore->GetStoreId();
    EXPECT_EQ(result.storeId, "schema_device_id002");

    std::vector<std::string> local = {"A", "B"};
    std::vector<std::string> remote = {"C", "D"};
    auto testStatus = kvStore->SetCapabilityRange(local, remote);
    EXPECT_EQ(testStatus, Status::SUCCESS) << "set range fail";
    manager.DeleteKvStore(appId, storeId, options.baseDir);
}

/**
* @tc.name: SyncWithCondition001
* @tc.desc: sync device data with condition;
* @tc.type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
*/
HWTEST_F(DeviceKvStoreTest, SyncWithCondition001, TestSize.Level1)
{
    EXPECT_NE(kvStore_, nullptr) << "kvStore is null.";
    std::vector<std::string> deviceIds = {"invalid_device_id1", "invalid_device_id2"};
    DataQuery dataQuery;
    dataQuery.KeyPrefix("name");
    auto syncStatus = kvStore_->Sync(deviceIds, SyncMode::PUSH, dataQuery, nullptr);
    EXPECT_NE(syncStatus, Status::SUCCESS) << "sync device should not return success";
}

/**
 * @tc.name: SubscribeWithQuery001
 * desc: subscribe and sync device data with query;
 * type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
 */
HWTEST_F(DeviceKvStoreTest, SubscribeWithQuery001, TestSize.Level1)
{
    EXPECT_NE(kvStore_, nullptr) << "kvStore is null.";
    std::vector<std::string> deviceIds = {"invalid_device_id1", "invalid_device_id2"};
    DataQuery dataQuery;
    dataQuery.KeyPrefix("name");
    auto syncStatus = kvStore_->SubscribeWithQuery(deviceIds, dataQuery);
    EXPECT_NE(syncStatus, Status::SUCCESS) << "sync device should not return success";
}

/**
 * @tc.name: UnSubscribeWithQuery001
 * desc: subscribe and sync device data with query;
 * type: FUNC
* @tc.require: I5DE2A
* @tc.author: Sven Wang
 */
HWTEST_F(DeviceKvStoreTest, UnSubscribeWithQuery001, TestSize.Level1)
{
    EXPECT_NE(kvStore_, nullptr) << "kvStore is nullptr.";
    std::vector<std::string> deviceIds = {"invalid_device_id1", "invalid_device_id2"};
    DataQuery dataQuery;
    dataQuery.KeyPrefix("name");
    auto unSubscribeStatus = kvStore_->UnsubscribeWithQuery(deviceIds, dataQuery);
    EXPECT_NE(unSubscribeStatus, Status::SUCCESS) << "sync device should not return success";
}