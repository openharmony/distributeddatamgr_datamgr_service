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

#define LOG_TAG "UdmfStoreDataChangedObserverTest"
#include "gtest/gtest.h"

#include "store_data_changed_observer.h"
#include "data_handler.h"
#include "delay_data_prepare_container.h"
#include "udmf_notifier_proxy.h"

using namespace OHOS::UDMF;
using namespace testing::ext;
using namespace testing;
namespace OHOS::Test {
namespace DistributedDataTest {

class UdmfStoreDataChangedObserverTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp() {}
    void TearDown() {}
};

class StoreChangedData : public DistributedDB::KvStoreChangedData {
public:
    const std::list<DistributedDB::Entry> &GetEntriesInserted() const
    {
        return entries_;
    }

    const std::list<DistributedDB::Entry> &GetEntriesUpdated() const
    {
        return entries_;
    }

    const std::list<DistributedDB::Entry> &GetEntriesDeleted() const
    {
        return entries_;
    }

    bool IsCleared() const
    {
        return true;
    }

    std::list<DistributedDB::Entry> entries_;
};

/**
* @tc.name: HandleRemoteDelayData002
* @tc.desc: Test HandleRemoteDelayData when data status is WORKING
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfStoreDataChangedObserverTest, HandleRemoteDelayData002, TestSize.Level1)
{
    UnifiedData delayData;
    std::vector<uint8_t> key = { 1, 2, 3 };
    std::vector<uint8_t> value = { 1, 2, 3 };
    DistributedDB::Entry entry = { key, value };
    std::list<DistributedDB::Entry> entriesList = { entry };
    StoreChangedData changedData;
    changedData.entries_ = entriesList;

    RuntimeObserver observer;
    EXPECT_NO_FATAL_FAILURE(observer.OnChange(changedData));
}
} // DistributedDataTest
} // OHOS::UDMF