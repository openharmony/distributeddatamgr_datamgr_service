/*
* Copyright (c) 2026 Huawei Device Co., Ltd.
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
#define LOG_TAG "KvstoreMetaManagerSubscribeTest"

#include <atomic>
#include <chrono>
#include <list>
#include <memory>
#include <thread>
#include <vector>

#include "bootstrap.h"
#include "gtest/gtest.h"
#include "kvstore_meta_manager.h"
#include "log_print.h"
#include "store_types.h"
namespace {
using namespace testing::ext;
using namespace OHOS::DistributedData;
using KvStoreMetaManager = OHOS::DistributedKv::KvStoreMetaManager;
using ChangeFlag = OHOS::DistributedKv::CHANGE_FLAG;

constexpr int32_t THREAD_COUNT = 8;
constexpr int32_t REGISTER_COUNT = 20;
const std::string INSERT_KEY = "subscribe_test_insert_key";
const std::string UPDATE_KEY = "subscribe_test_update_key";
const std::string DELETE_KEY = "subscribe_test_delete_key";
const std::string CONCURRENT_PREFIX = "subscribe_test_concurrent_";

constexpr uint32_t FlagBit(ChangeFlag flag)
{
    return 1u << static_cast<uint32_t>(flag);
}

// The observers registered via SubscribeMeta outlive each test case, so the recorded state is
// kept alive through a shared_ptr and stays valid for notifications of later cases.
struct ObserverState {
    std::atomic<uint32_t> flagBits{ 0 };
    std::string key;
};

// KvStoreChangedData is an abstract interface, so a test-owned implementation feeds synthetic
// entries into KvStoreMetaObserver::OnChange. This drives the real dispatch path without
// opening the meta store, which on a live device is shared with the running distributeddata
// service and must not be touched by the test.
class TestChangedData final : public DistributedDB::KvStoreChangedData {
public:
    TestChangedData(std::list<DistributedDB::Entry> inserted, std::list<DistributedDB::Entry> updated,
        std::list<DistributedDB::Entry> deleted)
        : inserted_(std::move(inserted)), updated_(std::move(updated)), deleted_(std::move(deleted))
    {
    }

    ~TestChangedData() override
    {
    }

    const std::list<DistributedDB::Entry> &GetEntriesInserted() const override
    {
        return inserted_;
    }

    const std::list<DistributedDB::Entry> &GetEntriesUpdated() const override
    {
        return updated_;
    }

    const std::list<DistributedDB::Entry> &GetEntriesDeleted() const override
    {
        return deleted_;
    }

    bool IsCleared() const override
    {
        return false;
    }

private:
    std::list<DistributedDB::Entry> inserted_;
    std::list<DistributedDB::Entry> updated_;
    std::list<DistributedDB::Entry> deleted_;
};

DistributedDB::Entry MakeEntry(const std::string &key, const std::string &value = "")
{
    DistributedDB::Entry entry;
    entry.key.assign(key.begin(), key.end());
    entry.value.assign(value.begin(), value.end());
    return entry;
}

void SubscribeFor(const std::string &keyPrefix, const std::shared_ptr<ObserverState> &state)
{
    KvStoreMetaManager::GetInstance().SubscribeMeta(
        keyPrefix, [state](const std::vector<uint8_t> &key, const std::vector<uint8_t> &, ChangeFlag flag) {
            state->key.assign(key.begin(), key.end());
            state->flagBits |= FlagBit(flag);
        });
}

class KvstoreMetaManagerSubscribeTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        // Only the label and directory strategies are needed: the dispatch path under test runs
        // entirely in memory and never opens the meta store.
        Bootstrap::GetInstance().LoadComponents();
        Bootstrap::GetInstance().LoadDirectory();
    }

    static void TearDownTestCase()
    {
    }
};

/**
* @tc.name: SubscribeMeta_OnChangeInserted_ObserverNotifiedWithInsertFlag
* @tc.desc: dispatch an inserted entry and check the observer is notified with insert flag
* @tc.type: FUNC
* @tc.require:
* @tc.author: agent
*/
HWTEST_F(
    KvstoreMetaManagerSubscribeTest, SubscribeMeta_OnChangeInserted_ObserverNotifiedWithInsertFlag, TestSize.Level1)
{
    auto state = std::make_shared<ObserverState>();
    SubscribeFor(INSERT_KEY, state);

    TestChangedData data({ MakeEntry(INSERT_KEY, "insert") }, {}, {});
    KvStoreMetaManager::GetInstance().metaObserver_->OnChange(data);

    EXPECT_TRUE(state->flagBits.load() & FlagBit(ChangeFlag::INSERT));
    EXPECT_EQ(state->key, INSERT_KEY);
}

/**
* @tc.name: SubscribeMeta_OnChangeUpdated_ObserverNotifiedWithUpdateFlag
* @tc.desc: dispatch an updated entry and check the observer is notified with update flag
* @tc.type: FUNC
* @tc.require:
* @tc.author: agent
*/
HWTEST_F(KvstoreMetaManagerSubscribeTest, SubscribeMeta_OnChangeUpdated_ObserverNotifiedWithUpdateFlag, TestSize.Level1)
{
    auto state = std::make_shared<ObserverState>();
    SubscribeFor(UPDATE_KEY, state);

    TestChangedData data({}, { MakeEntry(UPDATE_KEY, "update") }, {});
    KvStoreMetaManager::GetInstance().metaObserver_->OnChange(data);

    EXPECT_TRUE(state->flagBits.load() & FlagBit(ChangeFlag::UPDATE));
    EXPECT_EQ(state->key, UPDATE_KEY);
}

/**
* @tc.name: SubscribeMeta_OnChangeDeleted_ObserverNotifiedWithDeleteFlag
* @tc.desc: dispatch a deleted entry and check the observer is notified with delete flag
* @tc.type: FUNC
* @tc.require:
* @tc.author: agent
*/
HWTEST_F(KvstoreMetaManagerSubscribeTest, SubscribeMeta_OnChangeDeleted_ObserverNotifiedWithDeleteFlag, TestSize.Level1)
{
    auto state = std::make_shared<ObserverState>();
    SubscribeFor(DELETE_KEY, state);

    TestChangedData data({}, {}, { MakeEntry(DELETE_KEY, "") });
    KvStoreMetaManager::GetInstance().metaObserver_->OnChange(data);

    EXPECT_TRUE(state->flagBits.load() & FlagBit(ChangeFlag::DELETE));
    EXPECT_EQ(state->key, DELETE_KEY);
}

/**
* @tc.name: SubscribeMeta_PrefixMismatch_ObserverNotNotified
* @tc.desc: dispatch an entry whose key does not match the registered prefix and check the
*           observer is not notified
* @tc.type: FUNC
* @tc.require:
* @tc.author: agent
*/
HWTEST_F(KvstoreMetaManagerSubscribeTest, SubscribeMeta_PrefixMismatch_ObserverNotNotified, TestSize.Level1)
{
    auto state = std::make_shared<ObserverState>();
    SubscribeFor("subscribe_test_other_prefix", state);

    TestChangedData data({ MakeEntry(INSERT_KEY, "mismatch") }, {}, {});
    KvStoreMetaManager::GetInstance().metaObserver_->OnChange(data);

    EXPECT_EQ(state->flagBits.load(), 0u);
}

/**
* @tc.name: SubscribeMeta_ConcurrentRegisterAndDispatch_ObserverStillWorks
* @tc.desc: register observers from multiple threads while changes are dispatched concurrently,
*           then check the observer registered after the churn still receives notifications
* @tc.type: FUNC
* @tc.require:
* @tc.author: agent
*/
HWTEST_F(
    KvstoreMetaManagerSubscribeTest, SubscribeMeta_ConcurrentRegisterAndDispatch_ObserverStillWorks, TestSize.Level1)
{
    std::atomic<bool> stop = false;
    std::thread dispatcher([&stop]() {
        while (!stop.load()) {
            TestChangedData data({ MakeEntry(CONCURRENT_PREFIX + "dispatch") }, {}, {});
            KvStoreMetaManager::GetInstance().metaObserver_->OnChange(data);
        }
    });

    std::vector<std::thread> registrars;
    for (int32_t i = 0; i < THREAD_COUNT; ++i) {
        registrars.emplace_back([]() {
            for (int32_t j = 0; j < REGISTER_COUNT; ++j) {
                auto state = std::make_shared<ObserverState>();
                SubscribeFor(CONCURRENT_PREFIX, state);
            }
        });
    }

    for (auto &registrar : registrars) {
        registrar.join();
    }
    stop.store(true);
    dispatcher.join();

    auto state = std::make_shared<ObserverState>();
    SubscribeFor(CONCURRENT_PREFIX, state);
    TestChangedData data({ MakeEntry(CONCURRENT_PREFIX + "final") }, {}, {});
    KvStoreMetaManager::GetInstance().metaObserver_->OnChange(data);

    EXPECT_TRUE(state->flagBits.load() & FlagBit(ChangeFlag::INSERT));
    EXPECT_EQ(state->key, CONCURRENT_PREFIX + "final");
}
} // namespace
