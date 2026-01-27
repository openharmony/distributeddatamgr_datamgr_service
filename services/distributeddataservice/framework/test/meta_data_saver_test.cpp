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

    static std::shared_ptr<DBStoreMock> dbStoreMock_;
    static std::vector<std::string> savedKeys_;
};

std::shared_ptr<DBStoreMock> MetaDataSaverTest::dbStoreMock_;
std::vector<std::string> MetaDataSaverTest::savedKeys_;

void MetaDataSaverTest::SetUpTestCase()
{
    dbStoreMock_ = std::make_shared<DBStoreMock>();
    MetaDataManager::GetInstance().Initialize(dbStoreMock_, nullptr, "");
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

/**
 * @tc.name: MetaDataSaver_Constructor_001
 * @tc.desc: Test MetaDataSaver constructor with async parameter
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataSaverTest, MetaDataSaver_Constructor_001, testing::ext::TestSize.Level0)
{
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
    MetaDataSaver saver(false);

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
 * @tc.name: MetaDataSaver_Flush_Empty_001
 * @tc.desc: Test Flush with empty entries returns true immediately
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataSaverTest, MetaDataSaver_Flush_Empty_001, testing::ext::TestSize.Level0)
{
    MetaDataSaver saver(false);

    // Flush with no entries should return true
    auto result = saver.Flush();
    EXPECT_TRUE(result);
    EXPECT_EQ(saver.Size(), 0u);

    // Second flush should also return true
    result = saver.Flush();
    EXPECT_TRUE(result);
}

/**
 * @tc.name: MetaDataSaver_Flush_WithEntries_001
 * @tc.desc: Test Flush with entries saves metadata
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataSaverTest, MetaDataSaver_Flush_WithEntries_001, testing::ext::TestSize.Level0)
{
    MetaDataSaver saver(false);

    // Add metadata
    StoreMetaData meta;
    meta.bundleName = "test_bundle";
    meta.storeId = "test_store";
    auto key = meta.GetKey();
    saver.Add(key, meta);

    EXPECT_EQ(saver.Size(), 1u);

    // Flush should succeed
    auto result = saver.Flush();
    EXPECT_TRUE(result);

    savedKeys_.push_back(key);

    // After flush, size should be same but flushed flag set
    EXPECT_EQ(saver.Size(), 1u);

    // Second flush should return true (already flushed)
    result = saver.Flush();
    EXPECT_TRUE(result);
}

/**
 * @tc.name: MetaDataSaver_Destructor_AutoFlush_001
 * @tc.desc: Test destructor automatically flushes when not explicitly called
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataSaverTest, MetaDataSaver_Destructor_AutoFlush_001, testing::ext::TestSize.Level0)
{
    // Create scope to trigger destructor
    std::string key;
    {
        MetaDataSaver saver(false);
        StoreMetaData meta;
        meta.bundleName = "auto_flush_bundle";
        meta.storeId = "auto_flush_store";
        key = meta.GetKey();
        saver.Add(key, meta);
    }

    StoreMetaData loadedMeta;
    auto loadSuccess = MetaDataManager::GetInstance().LoadMeta(key, loadedMeta, false);
    EXPECT_TRUE(loadSuccess) << "Destructor should have flushed metadata to MetaDataManager";

    // Record key for cleanup
    savedKeys_.push_back(key);
}

/**
 * @tc.name: MetaDataSaver_Destructor_NoFlush_WhenFlushed_001
 * @tc.desc: Test destructor does not flush again when already flushed
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataSaverTest, MetaDataSaver_Destructor_NoFlush_WhenFlushed_001, testing::ext::TestSize.Level0)
{
    // Create scope to trigger destructor
    std::string key;
    {
        MetaDataSaver saver(false);
        StoreMetaData meta;
        meta.bundleName = "already_flushed_bundle";
        meta.storeId = "already_flushed_store";
        key = meta.GetKey();
        saver.Add(key, meta);

        // Explicitly flush
        auto result = saver.Flush();
        EXPECT_TRUE(result);

        // Verify metadata was saved
        StoreMetaData loadedMeta;
        EXPECT_TRUE(MetaDataManager::GetInstance().LoadMeta(key, loadedMeta, false));
    }
    
    StoreMetaData loadedMeta;
    auto loadSuccess = MetaDataManager::GetInstance().LoadMeta(key, loadedMeta, false);
    EXPECT_TRUE(loadSuccess) << "Metadata should still exist after destructor (not flushed again)";

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
    MetaDataSaver saver(false);

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
 * @tc.desc: Test Add method with pre-serialized string value
 * @tc.type: FUNC
 */
HWTEST_F(MetaDataSaverTest, MetaDataSaver_Add_String_001, testing::ext::TestSize.Level0)
{
    MetaDataSaver saver(false);

    // Add with pre-serialized string
    std::string key = "test_key";
    std::string value = "test_value";
    saver.Add(key, value);

    EXPECT_EQ(saver.Size(), 1u);

    auto result = saver.Flush();
    EXPECT_TRUE(result);

    savedKeys_.push_back(key);
}

} // namespace OHOS::Test
