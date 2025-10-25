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
* @tc.name: DataLoadCallbackTest001
* @tc.desc: Test Execute data load callback with return false
* @tc.type: FUNC
* @tc.require:
*/
HWTEST_F(UdmfStoreDataChangedObserverTest, DataLoadCallbackTest001, TestSize.Level1)
{
    DataLoadInfo info;
    info.udKey = "udmf://drag/com.example.test/11111";
    std::vector<DistributedDB::Entry> entries;
    DataHandler::MarshalDataLoadEntries(info, entries);
    std::list<DistributedDB::Entry> entriesList(entries.begin(), entries.end());
    StoreChangedData changedData;
    changedData.entries_ = entriesList;

    AcceptableInfoObserver observer;
    EXPECT_NO_FATAL_FAILURE(observer.OnChange(changedData));
}

} // DistributedDataTest
} // OHOS::UDMF