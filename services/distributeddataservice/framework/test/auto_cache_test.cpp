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

#define LOG_TAG "AutoCacheTest"
#include "store/auto_cache.h"

#include <gmock/gmock.h>

#include <memory>
#include <utility>

#include "gtest/gtest.h"
#include "mock/account_delegate_mock.h"
#include "mock/account_delegate_mock_proxy.h"
#include "mock/general_store_mock.h"
#include "mock/screen_manager_mock.h"


using namespace testing;
using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
namespace DistributedDataAutoCacheTest {

class AutoCacheTestGeneralStoreMock : public GeneralStoreMock {
public:
    StoreMetaData meta;
    bool createRequired = false;
    std::atomic_int32_t ref = 0;
    AutoCacheTestGeneralStoreMock() : ref(1){};
    explicit AutoCacheTestGeneralStoreMock(const StoreMetaData &meta, bool createRequired = false)
        : meta(meta), createRequired(createRequired), ref(1){};
};

class AutoCacheTest : public testing::Test {
public:
    static constexpr int32_t MOCK_STORE_TYPE = 21;
    static AccountDelegateMockProxy *accountDelegatProxy;
    static std::shared_ptr<AccountDelegateMock> defaultAccountDelegateMock;
    static void SetUpTestCase(void)
    {
        testing::FLAGS_gmock_verbose = "error";
        auto result = AutoCache::GetInstance().RegCreator(MOCK_STORE_TYPE,
            [](const StoreMetaData &metaData,
                const AutoCache::StoreOption &option) -> std::pair<int32_t, GeneralStore *> {
                auto store = new (std::nothrow) AutoCacheTestGeneralStoreMock(metaData, option.createRequired);
                if (store == nullptr) {
                    return { GeneralError::E_ERROR, nullptr };
                }
                return { E_OK, store };
            });
        EXPECT_EQ(result, E_OK);
        auto pool = std::make_shared<ExecutorPool>(5, 2);
        AutoCache::GetInstance().Bind(pool);
        accountDelegatProxy = new AccountDelegateMockProxy();
        ASSERT_NE(accountDelegatProxy, nullptr);
        AccountDelegate::RegisterAccountInstance(accountDelegatProxy);
        defaultAccountDelegateMock = std::make_shared<AccountDelegateMock>();
        ON_CALL(*defaultAccountDelegateMock, IsDeactivating(_)).WillByDefault(Return(false));
        ON_CALL(*defaultAccountDelegateMock, IsVerified(_)).WillByDefault(Return(true));
    };
    static void TearDownTestCase(void)
    {
        if (accountDelegatProxy != nullptr) {
            delete accountDelegatProxy;
        }
        defaultAccountDelegateMock = nullptr;
    };
    void SetUp()
    {
        AutoCache::GetInstance().CloseStore([](const auto &) {
            return true;
        });
        AutoCache::GetInstance().Configure(1000 * 60);  // 1000 ms/sec * 60 sec/min = 1 minute timeout
    };
    void TearDown()
    {
        ScreenManager::RegisterInstance(nullptr);
        accountDelegatProxy->SetAccountDelegateMock(defaultAccountDelegateMock);
        AutoCache::GetInstance().CloseStore([](const auto &) {
            return true;
        });
        AutoCache::GetInstance().Configure(1000 * 60);  // 1000 ms/sec * 60 sec/min = 1 minute timeout
    };

    StoreMetaData GetStoreMetaData(const std::string &storeId)
    {
        StoreMetaData metaData;
        metaData.bundleName = "com.ohos.AutoCacheTest";
        metaData.storeId = storeId;
        metaData.storeType = MOCK_STORE_TYPE;
        return metaData;
    }
};

AccountDelegateMockProxy *AutoCacheTest::accountDelegatProxy = nullptr;
std::shared_ptr<AccountDelegateMock> AutoCacheTest::defaultAccountDelegateMock = nullptr;
/**
 * @tc.name: GetStore_IdenticalMetadata_CacheHit
 * @tc.desc: Test that AutoCache returns the same store instance when GetStore is called twice with identical metadata
 * Step 1: Register mock store creator with counter to track creation calls
 * Step 2: Create store metadata with valid parameters including store type 0
 * Step 3: Call GetStore with metadata for the first time and verify new instance creation
 * Step 4: Call GetStore again with exact same metadata
 * Step 5: Verify same store instance returned (cache hit) and creator called only once
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, GetStore_IdenticalMetadata_CacheHit, TestSize.Level1)
{
    auto mock = std::make_shared<AutoCacheTestGeneralStoreMock>();
    EXPECT_CALL(*mock, SetExecutor).Times(1);
    auto creationCount = 0;
    auto result = AutoCache::GetInstance().RegCreator(0,
        [mock, &creationCount](const StoreMetaData &,
            const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
            creationCount++;
            return { E_OK, mock.get() };
        });
    EXPECT_EQ(result, E_OK);
    auto meta = GetStoreMetaData("GetStore_IdenticalMetadata_CacheHit.db");
    meta.storeType = 0;

    // First call to GetStore with specific metadata should create a new store instance
    auto store1 = AutoCache::GetInstance().GetStore(meta, {});
    ASSERT_NE(store1, nullptr);
    EXPECT_EQ(store1.get(), mock.get());

    // Second call with identical metadata should hit cache and return the same instance
    auto store2 = AutoCache::GetInstance().GetStore(meta, {});
    ASSERT_NE(store2, nullptr);
    EXPECT_EQ(store1.get(), store2.get());
    EXPECT_EQ(store2.get(), mock.get());

    // Verify that the creator function was only called once, confirming cache effectiveness
    EXPECT_EQ(creationCount, 1);
}

/**
 * @tc.name: AutoCache_GetStore_WithInvalidMetaData
 * @tc.desc: Test AutoCache GetStore method with invalid metadata
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, AutoCache_GetStore_WithInvalidMetaData, TestSize.Level1)
{
    StoreMetaData meta;
    meta.storeType = -1; // Invalid type
    auto store = AutoCache::GetInstance().GetStore(meta, {});
    EXPECT_EQ(store, nullptr);
    meta.storeType = AutoCache::MAX_CREATOR_NUM; // Invalid type
    store = AutoCache::GetInstance().GetStore(meta, {});
    EXPECT_EQ(store, nullptr);
}

/**
 * @tc.name: AutoCache_RegCreator_InvalidType
 * @tc.desc: Test AutoCache RegCreator with invalid type
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, AutoCache_RegCreator_InvalidType, TestSize.Level1)
{
    auto result = AutoCache::GetInstance().RegCreator(-1,
        [](const StoreMetaData &metaData, const AutoCache::StoreOption &option) -> std::pair<int32_t, GeneralStore *> {
            return { E_OK, nullptr };
        });
    EXPECT_EQ(result, E_ERROR);

    result = AutoCache::GetInstance().RegCreator(AutoCache::MAX_CREATOR_NUM,
        [](const StoreMetaData &metaData, const AutoCache::StoreOption &option) -> std::pair<int32_t, GeneralStore *> {
            return { E_OK, nullptr };
        });
    EXPECT_EQ(result, E_ERROR);
}

/**
 * @tc.name: AutoCache_GarbageCollection_StoreNotReleasedBeforeTimeout
 * @tc.desc: Test AutoCache garbage collection behavior to verify stores are retained before timeout and released
 * after timeout
 * Step 1: Configure AutoCache with 100ms garbage collection interval
 * Step 2: Register mock store creator and create store metadata
 * Step 3: Get store instance and verify it's created successfully
 * Step 4: Sleep for less than garbage collection timeout (50ms < 100ms)
 * Step 5: Get store again and verify it's still cached (not garbage collected)
 * Step 6: Sleep for longer than timeout (200ms > 100ms)
 * Step 7: Verify store has been garbage collected and new instance is created
 * Step 8: Verify CloseStore with filter function works correctly
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, AutoCache_GarbageCollection_StoreNotReleasedBeforeTimeout, TestSize.Level1)
{
    auto mock = std::make_shared<AutoCacheTestGeneralStoreMock>();
    EXPECT_CALL(*mock, SetExecutor).Times(2);
    const uint32_t garbageInterval = 100; // garbage once 100ms
    AutoCache::GetInstance().Configure(garbageInterval);
    auto creationCount = 0;
    AutoCache::GetInstance().RegCreator(0,
        [mock, &creationCount](const StoreMetaData &,
            const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
            creationCount++;
            return { E_OK, mock.get() };
        });
    auto meta = GetStoreMetaData("AutoCache_GarbageCollection_StoreNotReleasedBeforeTimeout.db");
    meta.storeType = 0;

    // First call to GetStore with specific metadata should create a new store instance
    auto store = AutoCache::GetInstance().GetStore(meta, {});
    ASSERT_NE(store, nullptr);
    EXPECT_EQ(store.get(), mock.get());
    EXPECT_EQ(creationCount, 1);

    // Sleep for less than garbage collection timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(garbageInterval / 2));

    // Get store again and verify it's still cached (not garbage collected)
    store = AutoCache::GetInstance().GetStore(meta, {});
    ASSERT_NE(store, nullptr);
    EXPECT_EQ(store.get(), mock.get());

    // Sleep for longer than timeout to trigger garbage collection
    std::this_thread::sleep_for(std::chrono::milliseconds(garbageInterval * 2));

    // Get store again - should create new instance after garbage collection
    store = AutoCache::GetInstance().GetStore(meta, {});
    ASSERT_NE(store, nullptr);
    EXPECT_EQ(store.get(), mock.get());

    // Verify that the creator function was called twice, confirming garbage collection occurred
    EXPECT_EQ(creationCount, 2);

    StoreMetaData tmp;
    AutoCache::GetInstance().CloseStore([&tmp](const auto &data) {
        tmp = data;
        return true;
    });
    EXPECT_EQ(tmp, meta);
}

/**
 * @tc.name: AutoCache_GetStore_SameTypeDifferentDB_DifferentInstances
 * @tc.desc: Test that AutoCache creates different store instances for the same store type but different database IDs
 * Step 1: Create two mock store instances with same storeType but different storeIds
 * Step 2: Register creator for the shared store type (3)
 * Step 3: Create metadata for two different databases with same storeType but different storeId
 * Step 4: Get first database instance and verify it matches the first mock
 * Step 5: Get second database instance and verify it matches the second mock
 * Step 6: Verify the two store instances are different from each other even with same type
 * Step 7: Request the same databases again to test cache hit
 * Step 8: Verify cached instances are returned and creation count remains at 2
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, AutoCache_GetStore_SameTypeDifferentDB_DifferentInstances, TestSize.Level1)
{
    auto mock1 = std::make_shared<AutoCacheTestGeneralStoreMock>();
    auto mock2 = std::make_shared<AutoCacheTestGeneralStoreMock>();
    EXPECT_CALL(*mock1, SetExecutor).Times(1);
    EXPECT_CALL(*mock2, SetExecutor).Times(1);

    auto creationCount = 0;
    auto result = AutoCache::GetInstance().RegCreator(3,
        [&, mock1, mock2](const StoreMetaData &meta,
            const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
            creationCount++;
            if (meta.storeId == "db1_same_type.db") {
                return { E_OK, mock1.get() };
            } else if (meta.storeId == "db2_same_type.db") {
                return { E_OK, mock2.get() };
            }
            return { GeneralError::E_ERROR, nullptr };
        });
    EXPECT_EQ(result, E_OK);

    auto meta1 = GetStoreMetaData("db1_same_type.db");
    meta1.storeType = 3;

    auto meta2 = GetStoreMetaData("db2_same_type.db");
    meta2.storeType = 3;

    // Get first database instance
    auto store1 = AutoCache::GetInstance().GetStore(meta1, {});
    ASSERT_NE(store1, nullptr);
    EXPECT_EQ(store1.get(), mock1.get());

    // Get second database instance
    auto store2 = AutoCache::GetInstance().GetStore(meta2, {});
    ASSERT_NE(store2, nullptr);
    EXPECT_EQ(store2.get(), mock2.get());

    // Verify the two instances are different even with same store type
    EXPECT_NE(store1.get(), store2.get());

    // Get the same databases again to test cache hit
    auto store1Again = AutoCache::GetInstance().GetStore(meta1, {});
    ASSERT_NE(store1Again, nullptr);
    EXPECT_EQ(store1.get(), store1Again.get());

    auto store2Again = AutoCache::GetInstance().GetStore(meta2, {});
    ASSERT_NE(store2Again, nullptr);
    EXPECT_EQ(store2.get(), store2Again.get());

    // Verify creation function was only called twice (once for each different database)
    EXPECT_EQ(creationCount, 2);
}

/**
 * @tc.name: AutoCache_GetStore_MultipleStores_DifferentInstances
 * @tc.desc: Test that AutoCache creates different store instances for different store types and database IDs
 * Step 1: Create two mock store instances with different metadata
 * Step 2: Register creators for both store types (1 and 2)
 * Step 3: Create metadata for two different stores with different storeId and storeType
 * Step 4: Get first store instance and verify it matches the first mock
 * Step 5: Get second store instance and verify it matches the second mock
 * Step 6: Verify the two store instances are different from each other
 * Step 7: Request the same stores again to test cache hit
 * Step 8: Verify cached instances are returned and creation count remains at 2
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, AutoCache_GetStore_MultipleStores_DifferentInstances, TestSize.Level1)
{
    auto mock1 = std::make_shared<AutoCacheTestGeneralStoreMock>();
    auto mock2 = std::make_shared<AutoCacheTestGeneralStoreMock>();
    EXPECT_CALL(*mock1, SetExecutor).Times(1);
    EXPECT_CALL(*mock2, SetExecutor).Times(1);

    auto creationCount = 0;
    auto result1 = AutoCache::GetInstance().RegCreator(1,
        [mock1, &creationCount](const StoreMetaData &,
            const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
            creationCount++;
            return { E_OK, mock1.get() };
        });
    EXPECT_EQ(result1, E_OK);

    auto result2 = AutoCache::GetInstance().RegCreator(2,
        [mock2, &creationCount](const StoreMetaData &,
            const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
            creationCount++;
            return { E_OK, mock2.get() };
        });
    EXPECT_EQ(result2, E_OK);

    auto meta1 = GetStoreMetaData("db1.db");
    meta1.storeType = 1;

    auto meta2 = GetStoreMetaData("db2.db");
    meta2.storeType = 2;

    // Get first database instance
    auto store1 = AutoCache::GetInstance().GetStore(meta1, {});
    ASSERT_NE(store1, nullptr);
    EXPECT_EQ(store1.get(), mock1.get());

    // Get second database instance
    auto store2 = AutoCache::GetInstance().GetStore(meta2, {});
    ASSERT_NE(store2, nullptr);
    EXPECT_EQ(store2.get(), mock2.get());

    // Verify the two instances are different
    EXPECT_NE(store1.get(), store2.get());

    // Get the same stores again to test cache hit
    auto store1Again = AutoCache::GetInstance().GetStore(meta1, {});
    ASSERT_NE(store1Again, nullptr);
    EXPECT_EQ(store1.get(), store1Again.get());

    auto store2Again = AutoCache::GetInstance().GetStore(meta2, {});
    ASSERT_NE(store2Again, nullptr);
    EXPECT_EQ(store2.get(), store2Again.get());

    // Verify creation function was only called twice (once for each different store)
    EXPECT_EQ(creationCount, 2);
}

/**
 * @tc.name: AutoCache_GetStore_ConcurrentCalls_PerformanceTest
 * @tc.desc: Test AutoCache thread safety and performance under concurrent access to different stores
 * Step 1: Create multiple mock store instances to simulate real store creation
 * Step 2: Register creator for store type 4 that maps different store IDs to different mock instances
 * Step 3: Prepare metadata for 5 different databases with same store type but different store IDs
 * Step 4: Launch 5 concurrent threads, each requesting a different store instance simultaneously
 * Step 5: Measure total execution time to ensure concurrent access performs efficiently
 * Step 6: Validate that each thread receives the correct corresponding store instance
 * Step 7: Confirm that each unique store was created exactly once despite concurrent requests
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, AutoCache_GetStore_ConcurrentCalls_PerformanceTest, TestSize.Level1)
{
    const int numStores = 5;
    const int storeCreationDelayMs = 50;
    const int maxExpectedTimeMs = storeCreationDelayMs * numStores; // Allow some overhead

    // Create mocks for each store with construction delay
    auto holders = std::make_shared<std::vector<std::shared_ptr<AutoCacheTestGeneralStoreMock>>>();
    std::vector<std::shared_ptr<AutoCacheTestGeneralStoreMock>> &mocks = *holders;
    for (int i = 0; i < numStores; i++) {
        mocks.push_back(std::make_shared<AutoCacheTestGeneralStoreMock>());
        EXPECT_CALL(*mocks[i], SetExecutor).Times(1);
    }

    // Register creator that returns different mock based on storeId
    std::atomic<int> creationCount{ 0 };
    auto create = [&creationCount, holders, storeCreationDelayMs](const StoreMetaData &meta,
                      const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
        creationCount++;
        for (int i = 0; i < numStores; i++) {
            if (meta.storeId == "concurrent_db" + std::to_string(i) + ".db") {
                std::this_thread::sleep_for(std::chrono::milliseconds(storeCreationDelayMs));
                auto res = std::make_pair(E_OK, (*holders)[i].get());
                return res;
            }
        }
        return { GeneralError::E_ERROR, nullptr };
    };

    auto result = AutoCache::GetInstance().RegCreator(4, create);
    EXPECT_EQ(result, E_OK);

    // Launch concurrent threads to get stores
    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<GeneralStore>> stores(numStores);

    auto startTime = std::chrono::steady_clock::now();

    for (int i = 0; i < numStores; i++) {
        auto meta = GetStoreMetaData("concurrent_db" + std::to_string(i) + ".db");
        meta.storeType = 4;
        meta.tokenId = i;
        threads.emplace_back([meta, i, &stores, &mocks]() {
            stores[i] = AutoCache::GetInstance().GetStore(meta, {});
            EXPECT_EQ(stores[i].get(), mocks[i].get());
        });
    }

    // Wait for all threads to complete
    for (auto &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    // Verify all operations completed within reasonable time (benefit of concurrency)
    EXPECT_LT(duration, maxExpectedTimeMs);

    // Verify creation count (should be 5, one for each unique store)
    EXPECT_EQ(creationCount.load(), numStores);
}

/**
 * @tc.name: AutoCache_CloseStore_SameTokenId_MultipleStores
 * @tc.desc: Test AutoCache CloseStore method to verify selective closure of stores based on token ID and store ID
 * Step 1: Create metadata for 3 different stores with same token ID but different store IDs
 * Step 2: Get all 3 stores and verify they are created successfully and cached
 * Step 3: Close only one specific store by specifying its store ID while keeping other parameters empty
 * Step 4: Verify the specified store is closed but other stores remain accessible from cache
 * Step 5: Close all remaining stores by calling CloseStore with empty store ID (only token ID specified)
 * Step 6: Verify all stores under that token ID are closed and no instances remain in cache
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, AutoCache_CloseStore_SameTokenId_MultipleStores, TestSize.Level1)
{
    const uint32_t tokenId = 100;
    const int numStores = 3;

    // Create mock stores
    std::vector<std::shared_ptr<AutoCacheTestGeneralStoreMock>> mocks;
    for (int i = 0; i < numStores; i++) {
        mocks.push_back(std::make_shared<AutoCacheTestGeneralStoreMock>());
        EXPECT_CALL(*mocks[i], SetExecutor).Times(i == 0 ? 2 : 1); // First store should SetExecutor twice
    }

    // Register creator for store type 5
    auto create = [&mocks](const StoreMetaData &meta,
                      const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
        for (int i = 0; i < mocks.size(); i++) {
            std::string expectedStoreId = "same_token_db" + std::to_string(i) + ".db";
            if (meta.storeId == expectedStoreId) {
                auto res = std::make_pair(E_OK, mocks[i].get());
                return res;
            }
        }
        return { GeneralError::E_ERROR, nullptr };
    };
    auto result = AutoCache::GetInstance().RegCreator(5, create);
    EXPECT_EQ(result, E_OK);

    // Create metadata for multiple stores with same tokenId
    std::vector<StoreMetaData> metas;
    std::vector<AutoCache::Store> stores;

    for (int i = 0; i < numStores; i++) {
        auto meta = GetStoreMetaData("same_token_db" + std::to_string(i) + ".db");
        meta.storeType = 5;
        meta.tokenId = tokenId;
        metas.push_back(meta);

        // Get store instance
        auto store = AutoCache::GetInstance().GetStore(meta, {});
        ASSERT_NE(store, nullptr);
        stores.push_back(store);
        EXPECT_EQ(store.get(), mocks[i].get());
    }

    // Verify all stores exist by getting them again
    for (int i = 0; i < numStores; i++) {
        auto meta = metas[i];
        auto store = AutoCache::GetInstance().GetStore(meta, {});
        ASSERT_NE(store, nullptr);
        EXPECT_EQ(store.get(), mocks[i].get());
    }

    // Close only the first store
    AutoCache::GetInstance().CloseStore(tokenId, "", "same_token_db0.db");

    // Verify first store is closed but others remain
    auto closedStore = AutoCache::GetInstance().GetStore(metas[0], {});

    for (int i = 1; i < numStores; i++) {
        auto store = AutoCache::GetInstance().GetStore(metas[i], {});
        ASSERT_NE(store, nullptr);
        EXPECT_EQ(store.get(), mocks[i].get());
    }

    // Close all remaining stores by specifying empty storeId
    AutoCache::GetInstance().CloseStore(tokenId, "", "");

    auto existingStores = AutoCache::GetInstance().GetStoresIfPresent(tokenId);
    EXPECT_EQ(existingStores.size(), 0);
}

/**
 * @tc.name: AutoCache_CloseStore_DifferentTokenIds_MultipleStores
 * @tc.desc: Test AutoCache CloseStore method with multiple stores under different tokenIds to verify selective closure
 * Step 1: Create metadata for 4 stores with 2 different tokenIds and 2 stores per tokenId
 * Step 2: Get all 4 stores and verify they are created successfully
 * Step 3: Close stores for first tokenId only
 * Step 4: Verify stores of first tokenId are closed and stores of second tokenId remain
 * Step 5: Close stores for second tokenId
 * Step 6: Verify all stores are closed regardless of tokenId
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, AutoCache_CloseStore_DifferentTokenIds_MultipleStores, TestSize.Level1)
{
    const uint32_t storeType = 6;
    const uint32_t tokenId1 = 200;
    const uint32_t tokenId2 = 201;
    const int storesPerToken = 2;

    // Create mock stores
    auto holder = std::make_shared<std::map<std::string, std::shared_ptr<AutoCacheTestGeneralStoreMock>>>();
    std::map<std::string, std::shared_ptr<AutoCacheTestGeneralStoreMock>> &mocks = *holder;
    for (int i = 0; i < storesPerToken * 2; i++) {
        std::string storeId = "diff_token_db" + std::to_string(i) + ".db";
        mocks.insert_or_assign(storeId, std::make_shared<AutoCacheTestGeneralStoreMock>());
        EXPECT_CALL(*mocks[storeId], SetExecutor).Times(1);
    }

    // Register creator for store type 6
    int creationCount = 0;
    auto result = AutoCache::GetInstance().RegCreator(storeType,
        [holder, &creationCount](const StoreMetaData &meta,
            const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
            creationCount++;
            auto it = holder->find(meta.storeId);
            if (it != holder->end()) {
                return { E_OK, it->second.get() };
            }
            return { GeneralError::E_ERROR, nullptr };
        });
    ASSERT_EQ(result, E_OK);

    for (int i = 0; i < storesPerToken * 2; i++) {
        std::string storeId = "diff_token_db" + std::to_string(i) + ".db";
        auto meta = GetStoreMetaData(storeId);
        meta.storeType = 6;
        meta.tokenId = i < storesPerToken ? tokenId1 : tokenId2;
        auto store = AutoCache::GetInstance().GetStore(meta, {});
        EXPECT_EQ(store.get(), mocks[storeId].get());
    }

    // Verify all stores are created
    EXPECT_EQ(creationCount, storesPerToken * 2);

    // Close stores for tokenId1 only
    AutoCache::GetInstance().CloseStore(tokenId1, "", "");

    auto existingStores = AutoCache::GetInstance().GetStoresIfPresent(tokenId1);
    EXPECT_EQ(existingStores.size(), 0);

    existingStores = AutoCache::GetInstance().GetStoresIfPresent(tokenId2);
    EXPECT_EQ(existingStores.size(), 2);

    // Close stores for tokenId2
    AutoCache::GetInstance().CloseStore(tokenId2, "", "");
}

/**
 * @tc.name: AutoCache_CloseStore_FilterBased_Closure
 * @tc.desc: Test AutoCache CloseStore method with filter function to close stores based on custom conditions
 * Step 1: Create metadata for 5 stores with different area values
 * Step 2: Get all 5 stores and verify they are created successfully
 * Step 3: Use filter to close stores with area value greater than 2
 * Step 4: Verify only matching stores are closed and others remain
 * Step 5: Confirm that 3 stores with area <= 2 remain accessible
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, AutoCache_CloseStore_FilterBased_Closure, TestSize.Level1)
{
    const uint32_t tokenId = 300;
    const uint32_t storeNum = 5;

    // Create mock stores
    auto holder = std::make_shared<std::map<std::string, std::shared_ptr<AutoCacheTestGeneralStoreMock>>>();
    std::map<std::string, std::shared_ptr<AutoCacheTestGeneralStoreMock>> &mocks = *holder;
    for (int i = 0; i < storeNum; i++) {
        std::string storeId = "filter_db" + std::to_string(i) + ".db";
        mocks.insert_or_assign(storeId, std::make_shared<AutoCacheTestGeneralStoreMock>());
        EXPECT_CALL(*mocks[storeId], SetExecutor).Times(1);
    }

    // Register creator for store type 7
    int creationCount = 0;
    auto result = AutoCache::GetInstance().RegCreator(7,
        [holder, &creationCount](const StoreMetaData &meta,
            const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
            creationCount++;
            auto it = holder->find(meta.storeId);
            if (it != holder->end()) {
                return { E_OK, it->second.get() };
            }
            return { GeneralError::E_ERROR, nullptr };
        });
    EXPECT_EQ(result, E_OK);

    // Create metadata for stores with different bundle names
    std::vector<StoreMetaData> metas;
    for (int i = 0; i < storeNum; i++) {
        auto meta = GetStoreMetaData("filter_db" + std::to_string(i) + ".db");
        meta.storeType = 7;
        meta.tokenId = tokenId; // Different tokenIds to spread across map
        meta.area = i;
        metas.push_back(meta);

        // Get store instance
        auto store = AutoCache::GetInstance().GetStore(meta, {});
        EXPECT_EQ(store.get(), mocks[meta.storeId].get());
    }

    // Verify all stores are created
    EXPECT_EQ(creationCount, storeNum);

    // Use filter to close stores with bundle name "com.test.bundle1"
    AutoCache::GetInstance().CloseStore([](const StoreMetaData &meta) {
        return meta.area > 2;
    });

    auto existingStores = AutoCache::GetInstance().GetStoresIfPresent(tokenId);
    EXPECT_EQ(existingStores.size(), 3); // Only stores with area <= 2 should remain
}

/**
 * @tc.name: AutoCache_GetDBStore_DisableEnable_StoreAvailability
 * @tc.desc: Test AutoCache GetDBStore method behavior when store is disabled and then enabled
 * Step 1: Register creator for store type 10
 * Step 2: Create store metadata and get store instance successfully when enabled
 * Step 3: Disable the store using Disable method
 * Step 4: Verify GetDBStore returns error when store is disabled
 * Step 5: Enable the store using Enable method
 * Step 6: Verify GetDBStore succeeds again after store is enabled
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, AutoCache_GetDBStore_DisableEnable_StoreAvailability, TestSize.Level1)
{
    const uint32_t storeType = 10;
    const uint32_t tokenId = 400;
    const std::string storeId = "disable_enable_test.db";

    // Create mock store
    auto mock = std::make_shared<AutoCacheTestGeneralStoreMock>();
    EXPECT_CALL(*mock, SetExecutor).Times(2); // Called twice: once for initial get, once after enable

    // Register creator for store type
    int creationCount = 0;
    auto result = AutoCache::GetInstance().RegCreator(storeType,
        [mock, &creationCount](const StoreMetaData &meta,
            const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
            creationCount++;
            return { E_OK, mock.get() };
        });
    EXPECT_EQ(result, E_OK);

    // Create metadata for the store
    auto meta = GetStoreMetaData(storeId);
    meta.storeType = storeType;
    meta.tokenId = tokenId;

    // Initially, GetDBStore should succeed
    auto [err, store] = AutoCache::GetInstance().GetDBStore(meta, {}, {});
    EXPECT_EQ(err, E_OK);
    ASSERT_NE(store, nullptr);
    EXPECT_EQ(store.get(), mock.get());
    EXPECT_EQ(creationCount, 1);

    // Disable the store
    AutoCache::GetInstance().Disable(tokenId, "", storeId);

    // Now GetDBStore should fail because the store is in disables_ set
    auto [err2, store2] = AutoCache::GetInstance().GetDBStore(meta, {}, {});
    EXPECT_EQ(err2, E_ERROR);  // Should return error when store is disabled
    EXPECT_EQ(store2, nullptr);

    // Enable the store
    AutoCache::GetInstance().Enable(tokenId, "", storeId);

    // Now GetDBStore should succeed again
    auto [err3, store3] = AutoCache::GetInstance().GetDBStore(meta, {}, {});
    EXPECT_EQ(err3, E_OK);
    ASSERT_NE(store3, nullptr);
    EXPECT_EQ(store3.get(), mock.get());
    EXPECT_EQ(creationCount, 2);  // New instance should be created after enable
}

/**
 * @tc.name: AutoCache_GetDBStore_CheckStatusBeforeOpen_EL4ScreenLocked
 * @tc.desc: Test that GetDBStore returns E_SCREEN_LOCKED when area is EL4 and screen is locked
 * Step 1: Create mock ScreenManager that reports screen as locked
 * Step 2: Register the mock ScreenManager instance
 * Step 3: Create store metadata with area EL4
 * Step 4: Call GetDBStore and verify it returns E_SCREEN_LOCKED error
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, AutoCache_GetDBStore_CheckStatusBeforeOpen_EL4ScreenLocked, TestSize.Level1)
{
    const uint32_t storeType = 13;
    const uint32_t tokenId = 403;

    // Create mock screen manager that reports screen as locked
    auto screenManagerMock = std::make_shared<ScreenManagerMock>();
    EXPECT_CALL(*screenManagerMock, IsLocked()).WillOnce(Return(true));

    // Register the mock instance
    ScreenManager::RegisterInstance(screenManagerMock);

    // Create mock store
    auto mock = std::make_shared<AutoCacheTestGeneralStoreMock>();
    EXPECT_CALL(*mock, SetExecutor).Times(0); // Should not be called since it fails at CheckStatusBeforeOpen

    // Register creator for store type
    auto result = AutoCache::GetInstance().RegCreator(storeType,
        [mock](const StoreMetaData &meta, const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
            return { E_OK, mock.get() };
        });
    EXPECT_EQ(result, E_OK);

    // Create metadata for the store with EL4 area and screen locked scenario
    auto meta = GetStoreMetaData("el4_screen_locked_test.db");
    meta.storeType = storeType;
    meta.tokenId = tokenId;
    meta.area = GeneralStore::Area::EL4;  // This area checks for screen lock

    // Call GetDBStore and expect E_SCREEN_LOCKED error
    auto [err, store] = AutoCache::GetInstance().GetDBStore(meta, {}, {});
    EXPECT_EQ(err, E_SCREEN_LOCKED);
    EXPECT_EQ(store, nullptr);
}

/**
 * @tc.name: AutoCache_GetDBStore_CheckStatusBeforeOpen_UserDeactivating
 * @tc.desc: Test that GetDBStore returns E_USER_DEACTIVATING when user is deactivating
 * Step 1: Create store metadata with non-zero user ID and non-EL1 area
 * Step 2: Mock AccountDelegate to return true for IsDeactivating
 * Step 3: Register the mock AccountDelegate instance
 * Step 4: Call GetDBStore and verify it returns E_USER_DEACTIVATING error
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, AutoCache_GetDBStore_CheckStatusBeforeOpen_UserDeactivating, TestSize.Level1)
{
    const uint32_t storeType = 14;
    const uint32_t tokenId = 404;

    // Create mock AccountDelegate that reports user as not verified
    auto accountMock = std::make_shared<AccountDelegateMock>();
    ASSERT_NE(accountMock, nullptr);
    EXPECT_CALL(*accountMock, IsDeactivating(100)).WillOnce(Return(true)); // User ID 100 is deactivatin

    accountDelegatProxy->SetAccountDelegateMock(accountMock);
    // Create mock store
    auto mock = std::make_shared<AutoCacheTestGeneralStoreMock>();
    EXPECT_CALL(*mock, SetExecutor).Times(0); // Should not be called since it fails at CheckStatusBeforeOpen

    // Register creator for store type
    auto result = AutoCache::GetInstance().RegCreator(storeType,
        [mock](const StoreMetaData &meta, const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
            return { E_OK, mock.get() };
        });
    EXPECT_EQ(result, E_OK);

    // Create metadata for the store with non-zero user ID and non-EL1/EL4 area
    auto meta = GetStoreMetaData("user_deactivating_test.db");
    meta.storeType = storeType;
    meta.tokenId = tokenId;
    meta.user = "100";  // Non-zero user ID
    meta.area = GeneralStore::Area::EL2;  // Not EL1, so will check account status

    // Call GetDBStore and expect E_USER_DEACTIVATING error
    auto [err, store] = AutoCache::GetInstance().GetDBStore(meta, {}, {});
    EXPECT_EQ(err, E_USER_DEACTIVATING);
    EXPECT_EQ(store, nullptr);
}

/**
 * @tc.name: AutoCache_GetDBStore_CheckStatusBeforeOpen_UserNotVerified
 * @tc.desc: Test that GetDBStore returns E_USER_LOCKED when user is not verified
 * Step 1: Create store metadata with non-zero user ID and non-EL1 area
 * Step 2: Mock AccountDelegate to return false for IsVerified (user not verified)
 * Step 3: Register the mock AccountDelegate instance
 * Step 4: Call GetDBStore and verify it returns E_USER_LOCKED error
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, AutoCache_GetDBStore_CheckStatusBeforeOpen_UserNotVerified, TestSize.Level1)
{
    const uint32_t storeType = 15;
    const uint32_t tokenId = 405;

    // Create mock AccountDelegate that reports user as not verified
    auto accountMock = std::make_shared<AccountDelegateMock>();
    ASSERT_NE(accountMock, nullptr);
    EXPECT_CALL(*accountMock, IsDeactivating(100)).WillOnce(Return(false));  // User is not deactivating
    EXPECT_CALL(*accountMock, IsVerified(100)).WillOnce(Return(false));     // But user is not verified

    // Register the mock instance
    accountDelegatProxy->SetAccountDelegateMock(accountMock);

    // Create mock store
    auto mock = std::make_shared<AutoCacheTestGeneralStoreMock>();
    EXPECT_CALL(*mock, SetExecutor).Times(0); // Should not be called since it fails at CheckStatusBeforeOpen

    // Register creator for store type
    auto result = AutoCache::GetInstance().RegCreator(storeType,
        [mock](const StoreMetaData &meta, const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
            return { E_OK, mock.get() };
        });
    EXPECT_EQ(result, E_OK);

    // Create metadata for the store with non-zero user ID and non-EL1 area
    auto meta = GetStoreMetaData("user_not_verified_test.db");
    meta.storeType = storeType;
    meta.tokenId = tokenId;
    meta.user = "100";  // Non-zero user ID
    meta.area = GeneralStore::Area::EL3;  // Not EL1, so will check account status

    // Call GetDBStore and expect E_USER_LOCKED error
    auto [err, store] = AutoCache::GetInstance().GetDBStore(meta, {}, {});
    EXPECT_EQ(err, E_USER_LOCKED);
    EXPECT_EQ(store, nullptr);
}

/**
 * @tc.name: AutoCache_GetDBStore_CheckStatusBeforeOpen_EL4ScreenUnlocked
 * @tc.desc: Test that GetDBStore works when area is EL4 but screen is unlocked
 * Step 1: Create mock ScreenManager that reports screen as unlocked
 * Step 2: Register the mock ScreenManager instance
 * Step 3: Create store metadata with area EL4
 * Step 4: Call GetDBStore and verify it proceeds past the screen lock check
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, AutoCache_GetDBStore_CheckStatusBeforeOpen_EL4ScreenUnlocked, TestSize.Level1)
{
    const uint32_t storeType = 17;
    const uint32_t tokenId = 407;

    // Create mock screen manager that reports screen as unlocked
    auto screenManagerMock = std::make_shared<ScreenManagerMock>();
    EXPECT_CALL(*screenManagerMock, IsLocked()).WillOnce(Return(false));

    // Register the mock instance
    ScreenManager::RegisterInstance(screenManagerMock);

    // Create mock store
    auto mock = std::make_shared<AutoCacheTestGeneralStoreMock>();
    EXPECT_CALL(*mock, SetExecutor).Times(1); // Called once for successful attempt

    // Register creator for store type
    int creationCount = 0;
    auto result = AutoCache::GetInstance().RegCreator(storeType,
        [mock, &creationCount](
            const StoreMetaData &meta, const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
            creationCount++;
            return { E_OK, mock.get() };
        });
    EXPECT_EQ(result, E_OK);

    // Create metadata for the store with EL4 area
    auto meta = GetStoreMetaData("el4_screen_unlocked_test.db");
    meta.storeType = storeType;
    meta.tokenId = tokenId;
    meta.area = GeneralStore::Area::EL4;  // EL4 area checks for screen lock
    meta.user = "100";  // Non-zero user ID to test account verification too

    // This should succeed if the screen is unlocked
    auto [err, store] = AutoCache::GetInstance().GetDBStore(meta, {}, {});
    EXPECT_EQ(err, E_OK);
    ASSERT_NE(store, nullptr);
    EXPECT_EQ(store.get(), mock.get());
    EXPECT_EQ(creationCount, 1);
}

/**
 * @tc.name: AutoCache_GetDBStore_CheckStatusBeforeOpen_ValidUserVerified
 * @tc.desc: Test that GetDBStore works when user is verified and not deactivating
 * Step 1: Create mock AccountDelegate that reports user as verified and not deactivating
 * Step 2: Register the mock AccountDelegate instance
 * Step 3: Create store metadata with non-zero user ID and non-EL1 area
 * Step 4: Call GetDBStore and verify it succeeds
 * @tc.type: FUNC
 */
HWTEST_F(AutoCacheTest, AutoCache_GetDBStore_CheckStatusBeforeOpen_ValidUserVerified, TestSize.Level1)
{
    const uint32_t storeType = 18;
    const uint32_t tokenId = 408;

    // Create mock AccountDelegate that reports user as verified and not deactivating
    auto accountMock = std::make_shared<AccountDelegateMock>();
    ASSERT_NE(accountMock, nullptr);
    EXPECT_CALL(*accountMock, IsDeactivating(100)).WillOnce(Return(false));  // User is not deactivating
    EXPECT_CALL(*accountMock, IsVerified(100)).WillOnce(Return(true));       // And user is verified

    // Register the mock instance
    accountDelegatProxy->SetAccountDelegateMock(accountMock);

    // Create mock store
    auto mock = std::make_shared<AutoCacheTestGeneralStoreMock>();
    EXPECT_CALL(*mock, SetExecutor).Times(1); // Called once for successful attempt

    // Register creator for store type
    int creationCount = 0;
    auto result = AutoCache::GetInstance().RegCreator(storeType,
        [mock, &creationCount](
            const StoreMetaData &meta, const AutoCache::StoreOption &) -> std::pair<int32_t, GeneralStore *> {
            creationCount++;
            return { E_OK, mock.get() };
        });
    EXPECT_EQ(result, E_OK);

    // Create metadata for the store with non-zero user ID and non-EL1 area
    auto meta = GetStoreMetaData("valid_user_verified_test.db");
    meta.storeType = storeType;
    meta.tokenId = tokenId;
    meta.user = "100";  // Non-zero user ID
    meta.area = GeneralStore::Area::EL2;  // Not EL1, so will check account status

    // This should succeed if the user is verified and not deactivating
    auto [err, store] = AutoCache::GetInstance().GetDBStore(meta, {}, {});
    EXPECT_EQ(err, E_OK);
    ASSERT_NE(store, nullptr);
    EXPECT_EQ(store.get(), mock.get());
    EXPECT_EQ(creationCount, 1);
}
} // namespace DistributedDataAutoCacheTest
} // namespace OHOS::Test