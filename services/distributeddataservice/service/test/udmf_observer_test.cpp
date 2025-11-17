/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
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

#define LOG_TAG "UdmfObserverTest"
#include "gtest/gtest.h"

#include "accesstoken_kit.h"
#include "bootstrap.h"
#include "observer_factory.h"
#include "drag_observer.h"
#include "data_handler.h"
#include "delay_data_acquire_container.h"
#include "device_manager_adapter.h"
#include "ipc_skeleton.h"
#include "kvstore_meta_manager.h"
#include "lifecycle_manager.h"
#include "log_print.h"
#include "nativetoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "udmf_notifier_proxy.h"
#include "udmf_service_impl.h"


using namespace OHOS::UDMF;
using namespace OHOS::Security::AccessToken;
using namespace testing::ext;
using namespace testing;
namespace OHOS::Test {
namespace DistributedDataTest {
constexpr size_t NUM_MIN = 1;
constexpr size_t NUM_MAX = 3;
static void GrantPermissionNative()
{
    const char **perms = new const char *[3];
    perms[0] = "ohos.permission.DISTRIBUTED_DATASYNC";
    perms[1] = "ohos.permission.ACCESS_SERVICE_DM";
    perms[2] = "ohos.permission.MONITOR_DEVICE_NETWORK_STATE"; // perms[2] is a permission parameter
    TokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 3,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "distributed_data_test",
        .aplStr = "system_basic",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    AccessTokenKit::ReloadNativeTokenInfo();
    delete[] perms;
}

class UdmfObserverTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        auto executors = std::make_shared<ExecutorPool>(NUM_MAX, NUM_MIN);
        LifeCycleManager::GetInstance().SetThreadPool(executors);
        GrantPermissionNative();
        DistributedData::Bootstrap::GetInstance().LoadComponents();
        DistributedData::Bootstrap::GetInstance().LoadDirectory();
        DistributedData::Bootstrap::GetInstance().LoadCheckers();
        OHOS::DistributedData::DeviceManagerAdapter::GetInstance().Init(executors);
        DistributedKv::KvStoreMetaManager::GetInstance().BindExecutor(executors);
        DistributedKv::KvStoreMetaManager::GetInstance().InitMetaParameter();
        DistributedKv::KvStoreMetaManager::GetInstance().InitMetaListener();
    }
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown()
    {
        LifeCycleManager::GetInstance().ClearUdKeys();
        DelayDataAcquireContainer::GetInstance().delayDataCallback_.clear();
    }
};

class StoreChangedData : public DistributedDB::KvStoreChangedData {
public:
    const std::list<DistributedDB::Entry> &GetEntriesInserted() const
    {
        return insertedEntries_;
    }

    const std::list<DistributedDB::Entry> &GetEntriesUpdated() const
    {
        return updatedEntries_;
    }

    const std::list<DistributedDB::Entry> &GetEntriesDeleted() const
    {
        return deletedEntries_;
    }

    bool IsCleared() const
    {
        return true;
    }

    std::list<DistributedDB::Entry> insertedEntries_;
    std::list<DistributedDB::Entry> updatedEntries_;
    std::list<DistributedDB::Entry> deletedEntries_;
};

/**
* @tc.name: ObserverFactoryTest001
* @tc.desc: Test ObserverFactory with return invalid param
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfObserverTest, ObserverFactoryTest001, TestSize.Level1)
{
    auto observer = ObserverFactory::GetObserver("data_hub");
    EXPECT_EQ(observer, nullptr);
}

/**
* @tc.name: DragObserverTest001
* @tc.desc: Test DragObserver with register delay data, and get data
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfObserverTest, DragObserverTest001, TestSize.Level1)
{
    ZLOGI("UDMF DragObserverTest001, begin");
    auto observer = ObserverFactory::GetObserver("drag");
    EXPECT_NE(observer, nullptr);

    UnifiedData delayData;
    Runtime runtime;
    runtime.key = UnifiedKey("drag", "com.example.test", "1111");
    runtime.key.GetUnifiedKey();
    runtime.dataStatus = DataStatus::WAITING;
    runtime.tokenId = 456;
    delayData.SetRuntime(runtime);
    std::vector<DistributedDB::Entry> entries1;
    DataHandler::MarshalToEntries(delayData, entries1);
    std::list<DistributedDB::Entry> entriesList1(entries1.begin(), entries1.end());

    auto tokenId = IPCSkeleton::GetSelfTokenID();
    SetSelfTokenID(tokenId);
    auto record = std::make_shared<UnifiedRecord>();
    record->AddEntry("type1", "value1");
    record->AddEntry("type2", "value2");
    auto properties = std::make_shared<UnifiedDataProperties>();
    UnifiedData data(properties);
    Runtime runtime2;
    runtime2.key = UnifiedKey("drag", "com.example.test", "22222");
    runtime2.tokenId = tokenId;
    runtime2.recordTotalNum = 1;
    Privilege privilege;
    privilege.tokenId = tokenId;
    runtime2.privileges = { privilege };
    data.SetRuntime(runtime2);
    data.SetRecords({record});
    std::vector<DistributedDB::Entry> entries2;
    DataHandler::MarshalToEntries(data, entries2);

    std::list<DistributedDB::Entry> entriesList2(entries2.begin(), entries2.end());
    StoreChangedData changedData;
    changedData.insertedEntries_ = entriesList1;
    changedData.updatedEntries_ = entriesList2;

    UdmfServiceImpl::factory_.product_ = std::make_shared<UdmfServiceImpl>();
    auto store = StoreCache::GetInstance().GetStore("drag");
    store->Put(data);

    DelayGetDataInfo delayGetDataInfo;
    delayGetDataInfo.tokenId = tokenId;
    DelayDataAcquireContainer::GetInstance().RegisterDelayDataCallback(runtime2.key.GetUnifiedKey(), delayGetDataInfo);
    EXPECT_EQ(0, LifeCycleManager::GetInstance().udKeys_.Size());
    EXPECT_TRUE(DelayDataAcquireContainer::GetInstance().IsContainDelayData(runtime2.key.key));
    observer->OnChange(changedData);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(0, LifeCycleManager::GetInstance().udKeys_.Size());
    EXPECT_FALSE(DelayDataAcquireContainer::GetInstance().IsContainDelayData(runtime2.key.key));
}

/**
* @tc.name: DragObserverTest002
* @tc.desc: Test DragObserver with register delay data, and get no data
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfObserverTest, DragObserverTest002, TestSize.Level1)
{
    auto observer = ObserverFactory::GetObserver("drag");
    EXPECT_NE(observer, nullptr);

    UnifiedData delayData;
    Runtime runtime;
    runtime.key = UnifiedKey("drag", "com.example.test", "1111");
    runtime.key.GetUnifiedKey();
    runtime.dataStatus = DataStatus::WAITING;
    runtime.tokenId = 456;
    delayData.SetRuntime(runtime);
    std::vector<DistributedDB::Entry> entries1;
    DataHandler::MarshalToEntries(delayData, entries1);
    std::list<DistributedDB::Entry> entriesList1(entries1.begin(), entries1.end());

    auto record = std::make_shared<UnifiedRecord>();
    record->AddEntry("type1", "value1");
    record->AddEntry("type2", "value2");
    auto properties = std::make_shared<UnifiedDataProperties>();
    UnifiedData data(properties);
    Runtime runtime2;
    runtime2.key = UnifiedKey("drag", "com.example.test", "22222");
    runtime2.tokenId = 123;
    runtime2.recordTotalNum = 1;
    Privilege privilege;
    privilege.tokenId = 123;
    runtime2.privileges = { privilege };
    data.SetRuntime(runtime2);
    data.SetRecords({record});
    std::vector<DistributedDB::Entry> entries2;
    DataHandler::MarshalToEntries(data, entries2);

    std::list<DistributedDB::Entry> entriesList2(entries2.begin(), entries2.end());
    StoreChangedData changedData;
    changedData.insertedEntries_ = entriesList1;
    changedData.updatedEntries_ = entriesList2;

    DelayDataAcquireContainer::GetInstance().RegisterDelayDataCallback(runtime2.key.GetUnifiedKey(),
        DelayGetDataInfo());
    EXPECT_EQ(0, LifeCycleManager::GetInstance().udKeys_.Size());
    EXPECT_TRUE(DelayDataAcquireContainer::GetInstance().IsContainDelayData(runtime2.key.key));
    EXPECT_NO_FATAL_FAILURE(observer->OnChange(changedData));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(1, LifeCycleManager::GetInstance().udKeys_.Size());
    EXPECT_TRUE(LifeCycleManager::GetInstance().udKeys_.Contains(123));
    EXPECT_TRUE(DelayDataAcquireContainer::GetInstance().IsContainDelayData(runtime2.key.key));
}

/**
* @tc.name: DragObserverTest003
* @tc.desc: Test DragObserver with the normal delay data
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfObserverTest, DragObserverTest003, TestSize.Level1)
{
    auto observer = ObserverFactory::GetObserver("drag");
    EXPECT_NE(observer, nullptr);

    auto record = std::make_shared<UnifiedRecord>();
    record->AddEntry("type1", "value1");
    record->AddEntry("type2", "value2");
    auto properties = std::make_shared<UnifiedDataProperties>();
    UnifiedData data(properties);
    Runtime runtime;
    runtime.key = UnifiedKey("drag", "com.example.test", "22222");
    runtime.tokenId = 123;
    data.SetRuntime(runtime);
    data.SetRecords({record});
    std::vector<DistributedDB::Entry> entries;
    DataHandler::MarshalToEntries(data, entries);

    std::list<DistributedDB::Entry> entriesList(entries.begin(), entries.end());
    StoreChangedData changedData;
    changedData.insertedEntries_ = entriesList;

    EXPECT_EQ(0, LifeCycleManager::GetInstance().udKeys_.Size());
    EXPECT_FALSE(DelayDataAcquireContainer::GetInstance().IsContainDelayData(runtime.key.GetUnifiedKey()));
    EXPECT_NO_FATAL_FAILURE(observer->OnChange(changedData));
    EXPECT_EQ(1, LifeCycleManager::GetInstance().udKeys_.Size());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_TRUE(LifeCycleManager::GetInstance().udKeys_.Contains(123));
    EXPECT_FALSE(DelayDataAcquireContainer::GetInstance().IsContainDelayData(runtime.key.key));
}

/**
* @tc.name: DragObserverTest004
* @tc.desc: Test DragObserver with the delete data
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfObserverTest, DragObserverTest004, TestSize.Level1)
{
    auto observer = ObserverFactory::GetObserver("drag");
    EXPECT_NE(observer, nullptr);

    auto record = std::make_shared<UnifiedRecord>();
    record->AddEntry("type1", "value1");
    record->AddEntry("type2", "value2");
    auto properties = std::make_shared<UnifiedDataProperties>();
    UnifiedData data(properties);
    Runtime runtime;
    auto key = UnifiedKey("drag", "com.example.test", "22222");
    key.GetUnifiedKey();
    runtime.key = key;
    runtime.tokenId = 123;
    data.SetRuntime(runtime);
    data.SetRecords({record});
    std::vector<DistributedDB::Entry> entries;
    DataHandler::MarshalToEntries(data, entries);

    std::list<DistributedDB::Entry> entriesList(entries.begin(), entries.end());
    StoreChangedData changedData;
    changedData.deletedEntries_ = entriesList;
    
    LifeCycleManager::GetInstance().InsertUdKey(123, runtime.key.key);
    EXPECT_EQ(1, LifeCycleManager::GetInstance().udKeys_.Size());
    EXPECT_NO_FATAL_FAILURE(observer->OnChange(changedData));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EXPECT_EQ(0, LifeCycleManager::GetInstance().udKeys_.Size());
}

} // DistributedDataTest
} // OHOS::UDMF