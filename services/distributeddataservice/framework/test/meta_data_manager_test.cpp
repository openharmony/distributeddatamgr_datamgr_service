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
#define LOG_TAG "MetaDataManagerTest"

#include "metadata/meta_data_manager.h"
#include <gtest/gtest.h>

#include "log_print.h"
#include "mock/db_store_mock.h"
using namespace OHOS;
using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedKv;
namespace OHOS::Test {
class DBStoreMockImpl;
class MetaDataManagerTest : public testing::Test {
public:
    static const std::string INVALID_DEVICE_ID;
    static void SetUpTestCase(void);
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
    static std::shared_ptr<DBStoreMockImpl> metaStore;
};
const std::string MetaDataManagerTest::INVALID_DEVICE_ID = "1234567890";
std::shared_ptr<DBStoreMockImpl> MetaDataManagerTest::metaStore = nullptr;

class DBStoreMockImpl : public DBStoreMock {
public:
    using PutLocalBatchFunc = std::function<DBStatus(const std::vector<Entry> &)>;
    PutLocalBatchFunc putLocalBatchFunc;
    DBStatus PutLocalBatch(const std::vector<Entry> &entries) override
    {
        if (putLocalBatchFunc) {
            return putLocalBatchFunc(entries);
        }
        return DBStoreMock::PutLocalBatch(entries);
    }
    using DeleteLocalBatchFunc = std::function<DBStatus(const std::vector<Key> &)>;
    DeleteLocalBatchFunc deleteLocalBatchFunc;
    DBStatus DeleteLocalBatch(const std::vector<Key> &keys) override
    {
        if (deleteLocalBatchFunc) {
            return deleteLocalBatchFunc(keys);
        }
        return DBStoreMock::DeleteLocalBatch(keys);
    }
};

void MetaDataManagerTest::SetUpTestCase(void)
{
    metaStore = std::make_shared<DBStoreMockImpl>();
    MetaDataManager::GetInstance().Initialize(
        metaStore,
        [](const auto &store) {
            return 0;
        },
        "meta");
};
/**
 * @tc.name: FilterConstructorAndGetKeyTest
 * @tc.desc: FilterConstructor and GetKey.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: MengYao
 */
HWTEST_F(MetaDataManagerTest, FilterConstructorAndGetKeyTest, TestSize.Level1)
{
    std::string pattern = "test";
    MetaDataManager::Filter filter(pattern);

    std::vector<uint8_t> key = filter.GetKey();
    ASSERT_EQ(key.size(), 0);
}

/**
 * @tc.name: FilterOperatorTest
 * @tc.desc: FilterOperator.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: MengYao
 */
HWTEST_F(MetaDataManagerTest, FilterOperatorTest, TestSize.Level1)
{
    std::string pattern = "test";
    MetaDataManager::Filter filter(pattern);

    std::string key = "test_key";
    ASSERT_TRUE(filter(key));

    key = "another_key";
    ASSERT_FALSE(filter(key));
}

/**
 * @tc.name: SyncTest001
 * @tc.desc: devices is empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(MetaDataManagerTest, SyncTest001, TestSize.Level1)
{
    std::vector<std::string> devices;
    MetaDataManager::OnComplete complete;
    auto result = MetaDataManager::GetInstance().Sync(devices, complete);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: SyncTest002
 * @tc.desc: devices is invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(MetaDataManagerTest, SyncTest002, TestSize.Level1)
{
    std::vector<std::string> devices;
    devices.emplace_back(INVALID_DEVICE_ID);
    MetaDataManager::OnComplete complete;
    auto result = MetaDataManager::GetInstance().Sync(devices, complete);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: SyncTest003
 * @tc.desc: devices is invalid.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author: SQL
 */
HWTEST_F(MetaDataManagerTest, SyncTest003, TestSize.Level1)
{
    std::vector<std::string> devices;
    devices.emplace_back(INVALID_DEVICE_ID);
    MetaDataManager::OnComplete complete = [](auto &results) {};
    auto result = MetaDataManager::GetInstance().Sync(devices, complete);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: SaveMetaTest001
 * @tc.desc: Batch save empty.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, SaveMetaTest001, TestSize.Level1)
{
    std::vector<MetaDataManager::Entry> values;
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(values, true));
}

/**
 * @tc.name: SaveMetaTest002
 * @tc.desc: Return error code when saving.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, SaveMetaTest002, TestSize.Level1)
{
    std::vector<MetaDataManager::Entry> values;
    values.push_back({ "SaveMetaTest002", "value" });
    metaStore->putLocalBatchFunc = [](auto &entries) {
        return DistributedDB::DBStatus::DB_ERROR;
    };
    EXPECT_FALSE(MetaDataManager::GetInstance().SaveMeta(values, true));
    metaStore->putLocalBatchFunc = nullptr;
}

/**
 * @tc.name: SaveMetaTest004
 * @tc.desc: Trigger sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, SaveMetaTest003, TestSize.Level1)
{
    metaStore->putLocalBatchFunc = nullptr;
    std::vector<MetaDataManager::Entry> values;
    values.push_back({ "SaveMetaTest004", "value" });
    bool flag = false;
    MetaDataManager::GetInstance().SetCloudSyncer([&flag]() {
        flag = true;
        return;
    });
    EXPECT_TRUE(MetaDataManager::GetInstance().SaveMeta(values, false));
    EXPECT_TRUE(flag);
    MetaDataManager::GetInstance().SetCloudSyncer(nullptr);
}

/**
 * @tc.name: DelMetaTest001
 * @tc.desc: Return INVALID_PASSWD_OR_CORRUPTED_DB when del meta.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, DelMetaTest001, TestSize.Level1)
{
    metaStore->deleteLocalBatchFunc = nullptr;
    std::vector<std::string> keys{ "DelMetaTest001" };
    metaStore->deleteLocalBatchFunc = [](auto &keys) {
        return DistributedDB::DBStatus::DB_ERROR;
    };
    EXPECT_FALSE(MetaDataManager::GetInstance().DelMeta(keys, true));
    metaStore->deleteLocalBatchFunc = nullptr;
}

/**
 * @tc.name: DelMetaTest003
 * @tc.desc: Trigger sync.
 * @tc.type: FUNC
 * @tc.require:
 * @tc.author:
 */
HWTEST_F(MetaDataManagerTest, DelMetaTest002, TestSize.Level1)
{
    std::vector<std::string> keys{ "DelMetaTest003" };
    bool flag = false;
    MetaDataManager::GetInstance().SetCloudSyncer([&flag]() {
        flag = true;
        return;
    });
    EXPECT_TRUE(MetaDataManager::GetInstance().DelMeta(keys, false));
    EXPECT_TRUE(flag);
    MetaDataManager::GetInstance().SetCloudSyncer(nullptr);
}
} // namespace OHOS::Test