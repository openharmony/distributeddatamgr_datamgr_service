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
#include "store_util.h"
#include "distributed_major.h"
#include "distributed_test_helper.h"
#include "refbase.h"
#include "hilog/log.h"
#include "directory_ex.h"

using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::DistributedKv;
using namespace OHOS::DistributeSystemTest;
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL_TEST = {LOG_CORE, 0, "DistributedTest"};
constexpr HiLogLabel LABEL = {LOG_CORE, 0, "KvStoreSyncCallbackTestImpl"};

class KvStoreSyncCallbackTestImpl : public KvStoreSyncCallback {
public:
    void SyncCompleted(const std::map<std::string, Status> &results);
    std::condition_variable cv_;
    bool compeleted_ = false;
    std::mutex mtx_;
};

void KvStoreSyncCallbackTestImpl::SyncCompleted(const std::map<std::string, Status> &results)
{
    for (const auto &result : results) {
        HiLog::Info(LABEL, "uuid = %{public}s, status = %{public}d",
                    StoreUtil::Anonymous(result.first).c_str(), result.second);
    }
    std::lock_guard<std::mutex> lck(mtx_);
    compeleted_ = true;
    cv_.notify_one();
}

class DistributedTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    static std::shared_ptr<SingleKvStore> singleKvStore_; // declare kvstore instance.
    static DistributedKvDataManager manager_;
    static std::vector <DeviceInfo> deviceInfos_;
    DistributedTestHelper helper_;
    static constexpr int WAIT_FOR_TIME = 3; // wait for synccallback time is 3s.
    static constexpr int FILE_PERMISSION = 0777; // 0777 is to modify the permissions of the file.
};

std::shared_ptr<SingleKvStore> DistributedTest::singleKvStore_ = nullptr;
DistributedKvDataManager DistributedTest::manager_;
std::vector<DeviceInfo> DistributedTest::deviceInfos_;

void DistributedTest::SetUpTestCase(void)
{
    Options options = { .createIfMissing = true, .encrypt = false, .autoSync = false,
                        .kvStoreType = KvStoreType::SINGLE_VERSION };
    options.area = EL1;
    options.baseDir = std::string("/data/service/el1/public/database/odmf");
    AppId appId = { "odmf" };
    StoreId storeId = { "student_single" }; // define kvstore(database) name.
    mkdir(options.baseDir.c_str(), (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
    manager_.GetSingleKvStore(options, appId, storeId, singleKvStore_);
    manager_.GetDeviceList(deviceInfos_, DeviceFilterStrategy::NO_FILTER);
    OHOS::ChangeModeDirectory(options.baseDir, FILE_PERMISSION);
}

void DistributedTest::TearDownTestCase(void)
{
    (void)remove("/data/service/el1/public/database/odmf/key");
    (void)remove("/data/service/el1/public/database/odmf/kvdb");
    (void)remove("/data/service/el1/public/database/odmf");
}

void DistributedTest::SetUp(void)
{}

void DistributedTest::TearDown(void)
{}

HWTEST_F(DistributedTest, SyncData001, TestSize.Level1)
{
    EXPECT_NE(singleKvStore_, nullptr) << "kvStorePtr is null";
    Key key = { "single1_001" };
    Value val = { "value1_001" };
    auto status = singleKvStore_->Put(key, val);
    EXPECT_EQ(Status::SUCCESS, status)<< "Put failed";

    auto syncCallback = std::make_shared<KvStoreSyncCallbackTestImpl>();
    status = singleKvStore_->RegisterSyncCallback(syncCallback);
    EXPECT_EQ(Status::SUCCESS, status)<< "SyncCallback failed";

    std::vector<std::string> deviceIds;
    for (const auto &device : deviceInfos_) {
        deviceIds.push_back(device.deviceId);
    }
    std::unique_lock<std::mutex> lck(syncCallback->mtx_);
    status = singleKvStore_->Sync(deviceIds, SyncMode::PUSH);
    if (!syncCallback->cv_.wait_for(lck, std::chrono::seconds(WAIT_FOR_TIME),
        [&syncCallback]() { return syncCallback->compeleted_; })) {
        status = Status::TIME_OUT;
    }
    EXPECT_EQ(Status::SUCCESS, status)<< "Sync failed";

    Value value;
    status = helper_.GetRemote(key, value);
    HiLog::Info(LABEL_TEST, "value = %{public}s", value.ToString().c_str());
    EXPECT_EQ(Status::SUCCESS, status)<< "Get failed";
    EXPECT_STREQ(value.ToString().c_str(), val.ToString().c_str())<< "failed";
}

HWTEST_F(DistributedTest, SyncData002, TestSize.Level1)
{
    EXPECT_NE(singleKvStore_, nullptr) << "kvStorePtr is null";
    Key key = { "single1_002" };
    Value val = { "value1_002" };
    auto status = singleKvStore_->Put(key, val);
    EXPECT_EQ(Status::SUCCESS, status)<< "Put failed";

    SyncMode mode = SyncMode::PULL;
    EXPECT_EQ(Status::SUCCESS, helper_.SyncRemote(mode))<< "Sync failed";

    Value value;
    status = helper_.GetRemote(key, value);
    HiLog::Info(LABEL_TEST, "value = %{public}s", value.ToString().c_str());
    EXPECT_EQ(Status::SUCCESS, status)<< "Get failed";
    EXPECT_STREQ(value.ToString().c_str(), val.ToString().c_str())<< "failed";
}

HWTEST_F(DistributedTest, SyncData003, TestSize.Level1)
{
    EXPECT_NE(singleKvStore_, nullptr) << "kvStorePtr is null";
    Key key = { "single1_003" };
    Value val = { "value1_003" };
    auto status = singleKvStore_->Put(key, val);
    EXPECT_EQ(Status::SUCCESS, status)<< "Put failed";

    SyncMode mode = SyncMode::PULL;
    uint32_t delay = 200;
    EXPECT_EQ(Status::SUCCESS, helper_.SyncRemote(mode, delay))<< "Sync failed";

    Value value;
    status = helper_.GetRemote(key, value);
    HiLog::Info(LABEL_TEST, "value = %{public}s", value.ToString().c_str());
    EXPECT_EQ(Status::SUCCESS, status)<< "Get failed";
    EXPECT_STREQ(value.ToString().c_str(), val.ToString().c_str())<< "failed";
}
}

int main(int argc, char *argv[])
{
    g_pDistributetestEnv = new DistributeTestEnvironment("major.desc");
    testing::AddGlobalTestEnvironment(g_pDistributetestEnv);
    testing::GTEST_FLAG(output) = "xml:./";
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}