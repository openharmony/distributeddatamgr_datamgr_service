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

#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "metadata/meta_data_manager.h"
#include "metadata/meta_data_saver.h"
#include "metadata/store_meta_data.h"
#include "mock/db_store_mock.h"

using namespace OHOS::DistributedData;

namespace OHOS::Test {
class MetaDataSaverTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

    static void InitMetaDataManager();
    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static std::vector<std::string> savedKeys_;
};

std::shared_ptr<DBStoreMock> MetaDataSaverTest::dbStoreMock_;
std::vector<std::string> MetaDataSaverTest::savedKeys_;

void MetaDataSaverTest::SetUpTestCase()
{
}

void MetaDataSaverTest::TearDownTestCase()
{
}

void MetaDataSaverTest::SetUp()
{
}

void MetaDataSaverTest::TearDown()
{
    for (const auto& key : savedKeys_) {
        MetaDataManager::GetInstance().DelMeta(key, false);
    }
    savedKeys_.clear();
}

void MetaDataSaverTest::InitMetaDataManager()
{
    if (dbStoreMock_ == nullptr) {
        dbStoreMock_ = std::make_shared<DBStoreMock>();
    }
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
}

/**
 * @tc.name: MetaDataSaver_Destructor_Uninitialized_001
 * @tc.desc: Test destructor when MetaDataManager is not initialized
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataSaverTest, MetaDataSaver_Destructor_Uninitialized_001, testing::ext::TestSize.Level0)
{
    // Do NOT call InitMetaDataManager() - test failure scenario
    std::string key = "uninitialized_test_key";
    {
        MetaDataSaver saver(true);
        saver.Add(key, "test_value");
    }
    StoreMetaData loadedMeta;
    auto loadSuccess = MetaDataManager::GetInstance().LoadMeta(key, loadedMeta, true);
    EXPECT_TRUE(!loadSuccess) << "String value should have not been saved";
}

/**
 * @tc.name: MetaDataSaver_Constructor_001
 * @tc.desc: Test MetaDataSaver constructor with async parameter
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataSaverTest, MetaDataSaver_Constructor_001, testing::ext::TestSize.Level0)
{
    InitMetaDataManager();
    // Test async mode
    MetaDataSaver asyncSaver(true);
    EXPECT_EQ(asyncSaver.Size(), 0u);

    // Test sync mode
    MetaDataSaver syncSaver(false);
    EXPECT_EQ(syncSaver.Size(), 0u);
}

/**
 * @tc.name: MetaDataSaver_Add_001
 * @tc.desc: Test Add method increases entry count
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataSaverTest, MetaDataSaver_Add_001, testing::ext::TestSize.Level0)
{
    MetaDataSaver saver(true);

    // Add metadata entries
    StoreMetaData meta1;
    meta1.bundleName = "bundle1";
    meta1.storeId = "store1";
    saver.Add(meta1.GetKey(), meta1);
    EXPECT_EQ(saver.Size(), 1u);

    StoreMetaData meta2;
    meta2.bundleName = "bundle2";
    meta2.storeId = "store2";
    saver.Add(meta2.GetKey(), meta2);
    EXPECT_EQ(saver.Size(), 2u);
}

/**
 * @tc.name: MetaDataSaver_Destructor_AutoFlush_001
 * @tc.desc: Test destructor automatically flushes all entries
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataSaverTest, MetaDataSaver_Destructor_AutoFlush_001, testing::ext::TestSize.Level0)
{
    // Create scope to trigger destructor
    std::string key;
    {
        MetaDataSaver saver(true);
        StoreMetaData meta;
        meta.bundleName = "auto_flush_bundle";
        meta.storeId = "auto_flush_store";
        key = meta.GetKey();
        saver.Add(key, meta);
    }

    // Verify: Load metadata from MetaDataManager to confirm destructor flushed successfully
    StoreMetaData loadedMeta;
    auto loadSuccess = MetaDataManager::GetInstance().LoadMeta(key, loadedMeta, true);
    EXPECT_TRUE(loadSuccess) << "Destructor should have flushed metadata to MetaDataManager";

    // Record key for cleanup
    savedKeys_.push_back(key);
}

/**
 * @tc.name: MetaDataSaver_MultipleAdd_001
 * @tc.desc: Test multiple Add calls result in single batch save
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataSaverTest, MetaDataSaver_MultipleAdd_001, testing::ext::TestSize.Level0)
{
    std::string key;
    {
        MetaDataSaver saver(true);
        StoreMetaData meta;
        meta.bundleName = "multiple_add_bundle";
        meta.storeId = "multiple_add_store";
        key = meta.GetKey();

        // Add multiple times - should batch save on destruction
        saver.Add(key, meta);
        saver.Add(key, meta);  // Duplicate add (same key)
    }

    // Verify metadata was saved
    StoreMetaData loadedMeta;
    auto loadSuccess = MetaDataManager::GetInstance().LoadMeta(key, loadedMeta, true);
    EXPECT_TRUE(loadSuccess) << "Destructor should have saved metadata";

    // Record key for cleanup
    savedKeys_.push_back(key);
}

/**
 * @tc.name: MetaDataSaver_Clear_001
 * @tc.desc: Test Clear method removes all entries
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataSaverTest, MetaDataSaver_Clear_001, testing::ext::TestSize.Level0)
{
    MetaDataSaver saver(true);

    // Add multiple entries
    StoreMetaData meta1;
    meta1.bundleName = "bundle1";
    meta1.storeId = "store1";
    saver.Add(meta1.GetKey(), meta1);

    StoreMetaData meta2;
    meta2.bundleName = "bundle2";
    meta2.storeId = "store2";
    saver.Add(meta2.GetKey(), meta2);

    EXPECT_EQ(saver.Size(), 2u);

    // Clear all entries
    saver.Clear();
    EXPECT_EQ(saver.Size(), 0u);
}

/**
 * @tc.name: MetaDataSaver_Add_String_001
 * @tc.desc: Test Add method with pre-serialized string value and auto-flush
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataSaverTest, MetaDataSaver_Add_String_001, testing::ext::TestSize.Level0)
{
    std::string key = "test_key";
    std::string value = "test_value";

    // Create scope to trigger auto-flush
    {
        MetaDataSaver saver(true);
        saver.Add(key, value);
    }

    // Verify metadata was saved
    StoreMetaData loadedMeta;
    auto loadSuccess = MetaDataManager::GetInstance().LoadMeta(key, loadedMeta, true);
    EXPECT_TRUE(loadSuccess) << "String value should have been saved";

    // Note: For pre-serialized strings, the value is stored as-is
    savedKeys_.push_back(key);
}
} // namespace OHOS::Test
