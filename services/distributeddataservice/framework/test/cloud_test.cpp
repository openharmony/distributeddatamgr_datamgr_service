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

#define LOG_TAG "CloudInfoTest"
#include <gtest/gtest.h>

#include "serializable/serializable.h"
#include "cloud/cloud_db.h"
#include "cloud/cloud_event.h"
#include "cloud/cloud_info.h"
#include "cloud/cloud_mark.h"
#include "cloud/cloud_server.h"
#include "cloud/schema_meta.h"
#include "utils/crypto.h"
#include "screen/screen_manager.h"
#include "store/general_store.h"
#include "store/general_value.h"
#include "store/general_watcher.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
namespace OHOS::Test {
class CloudInfoTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class ServicesCloudServerTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class ServicesCloudDBTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class CloudEventTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

class ScreenManagerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

void ScreenManagerTest::SetUpTestCase()
{
    ScreenManager::GetInstance()->BindExecutor(nullptr);
}

class MockGeneralWatcher : public DistributedData::GeneralWatcher {
public:
    int32_t OnChange(const Origin &origin, const PRIFields &primaryFields,
        ChangeInfo &&values) override
    {
        return GeneralError::E_OK;
    }

    int32_t OnChange(const Origin &origin, const Fields &fields, ChangeData &&datas) override
    {
        return GeneralError::E_OK;
    }
};

class MockQuery : public GenQuery {
public:
    static const uint64_t TYPE_ID = 1;

    bool IsEqual(uint64_t tid) override
    {
        return tid == TYPE_ID;
    }

    std::vector<std::string> GetTables() override
    {
        return {"table1", "table2"};
    }
};

/**
* @tc.name: GetSyncTables_002
* @tc.desc: get p2p sync table names w
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudEventTest, GetSyncTables_002, TestSize.Level0)
{
    Database database;
    auto tables = database.GetSyncTables();
    std::vector<std::string> tagTable;
    EXPECT_EQ(tables, tagTable);
    for (int i = 0; i < 5; ++i) {
        Table table;
        table.name = "table" + std::to_string(i);
        table.sharedTableName = "shared" + std::to_string(i);
        database.tables.push_back(std::move(table));
    }
    tables = database.GetSyncTables();
    EXPECT_EQ(tables, tagTable);
}

/**
* @tc.name: GetSyncTables_001
* @tc.desc: get p2p sync table names w
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudEventTest, GetCloudTables_001, TestSize.Level0)
{
    Database database;
    for (int i = 0; i < 5; ++i) {
        Table table;
        table.name = "table" + std::to_string(i);
        table.sharedTableName = "shared" + std::to_string(i);
        auto &fields = (i % 2 == 0) ? table.deviceSyncFields : table.cloudSyncFields;
        fields.push_back("field" + std::to_string(i));
        database.tables.push_back(std::move(table));
    }
    auto tables = database.GetCloudTables();
    std::vector<std::string> tagTable = { "table1", "shared1", "table3", "shared3" };
    EXPECT_EQ(tables, tagTable);
}

/**
* @tc.name: GetSyncTables_002
* @tc.desc: get p2p sync table names w
* @tc.type: FUNC
* @tc.require:
* @tc.author:
*/
HWTEST_F(CloudEventTest, GetCloudTables_002, TestSize.Level0)
{
    Database database;
    for (int i = 0; i < 5; ++i) {
        Table table;
        table.name = "table" + std::to_string(i);
        table.sharedTableName = "shared" + std::to_string(i);
        database.tables.push_back(std::move(table));
    }
    auto tables = database.GetCloudTables();
    std::vector<std::string> tagTable;
    EXPECT_EQ(tables, tagTable);
}
} // namespace OHOS::Test
