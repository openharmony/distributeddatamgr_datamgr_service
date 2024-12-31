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
#define LOG_TAG "RdbCloudTest"

#include "rdb_cloud.h"

#include "gtest/gtest.h"
#include "log_print.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedRdb;
using DBVBucket = DistributedDB::VBucket;
using DBStatus = DistributedDB::DBStatus;
std::vector<DBVBucket> g_DBVBucket = { { { "#gid", { "0000000" } }, { "#flag", { true } },
    { "#value", { int64_t(100) } }, { "#float", { double(100) } }, { "#_type", { int64_t(1) } },
    { "#_query", { Bytes({ 1, 2, 3, 4 }) } } } };
namespace OHOS::Test {
namespace DistributedRDBTest {
class RdbCloudTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
* @tc.name: RdbCloudTest001
* @tc.desc: RdbCloud BatchInsert BatchUpdate BatchDelete test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbCloudTest, RdbCloudTest001, TestSize.Level1)
{
    BindAssets bindAssets;
    Bytes bytes;
    std::shared_ptr<CloudDB> cloudDB = std::make_shared<CloudDB>();
    RdbCloud rdbCloud(cloudDB, &bindAssets);
    std::string tableName = "testTable";
    auto result = rdbCloud.BatchInsert(tableName, std::move(g_DBVBucket), g_DBVBucket);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
    result = rdbCloud.BatchUpdate(tableName, std::move(g_DBVBucket), g_DBVBucket);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
    result = rdbCloud.BatchDelete(tableName, g_DBVBucket);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
}

/**
* @tc.name: RdbCloudTest002
* @tc.desc: RdbCloud Query PreSharing HeartBeat Close test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbCloudTest, RdbCloudTest002, TestSize.Level1)
{
    BindAssets bindAssets;
    std::shared_ptr<CloudDB> cloudDB = std::make_shared<CloudDB>();
    RdbCloud rdbCloud(cloudDB, &bindAssets);
    std::string tableName = "testTable";
    rdbCloud.Lock();
    std::string traceId = "id";
    rdbCloud.SetPrepareTraceId(traceId);
    auto result = rdbCloud.BatchInsert(tableName, std::move(g_DBVBucket), g_DBVBucket);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
    DBVBucket extends = {
        {"#gid", {"0000000"}}, {"#flag", {true}}, {"#value", {int64_t(100)}}, {"#float", {double(100)}},
        {"#_type", {int64_t(1)}}, {"#_query", {Bytes({ 1, 2, 3, 4 })}}
    };
    result = rdbCloud.Query(tableName, extends, g_DBVBucket);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
    std::vector<VBucket> vBuckets = {
        {{"#gid", {"0000000"}}, {"#flag", {true}}, {"#value", {int64_t(100)}}, {"#float", {double(100)}},
        {"#_type", {int64_t(1)}}, {"#_query", {Bytes({ 1, 2, 3, 4 })}}}
    };
    rdbCloud.PreSharing(tableName, vBuckets);
    result = rdbCloud.HeartBeat();
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
    rdbCloud.UnLock();
    rdbCloud.GetEmptyCursor(tableName);
    result = rdbCloud.Close();
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
}

/**
* @tc.name: RdbCloudTest003
* @tc.desc: RdbCloud Query error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbCloudTest, RdbCloudTest003, TestSize.Level1)
{
    BindAssets bindAssets;
    bindAssets.bindAssets = nullptr;
    std::shared_ptr<CloudDB> cloudDB = std::make_shared<CloudDB>();
    RdbCloud rdbCloud(cloudDB, &bindAssets);
    std::string tableName = "testTable";
    DBVBucket extends = {
        {"#gid", {"0000000"}}, {"#flag", {true}}, {"#value", {int64_t(100)}}, {"#float", {double(100)}},
        {"#_type", {int64_t(1)}}
    };
    std::vector<DBVBucket> data = {
        {{"#gid", {"0000000"}}, {"#flag", {true}}, {"#value", {int64_t(100)}}, {"#float", {double(100)}}}
    };
    auto result = rdbCloud.Query(tableName, extends, data);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);

    extends = {
        {"#gid", {"0000000"}}, {"#flag", {true}}, {"#value", {int64_t(100)}}, {"#float", {double(100)}},
        {"#_query", {Bytes({ 1, 2, 3, 4 })}}
    };
    result = rdbCloud.Query(tableName, extends, data);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);

    extends = {
        {"#gid", {"0000000"}}, {"#flag", {true}}, {"#value", {int64_t(100)}}, {"#float", {double(100)}},
        {"#_type", {int64_t(0)}}
    };
    result = rdbCloud.Query(tableName, extends, data);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
}

/**
* @tc.name: RdbCloudTest004
* @tc.desc: RdbCloud UnLockCloudDB LockCloudDB InnerUnLock test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbCloudTest, RdbCloudTest004, TestSize.Level1)
{
    BindAssets bindAssets;
    std::shared_ptr<CloudDB> cloudDB = std::make_shared<CloudDB>();
    RdbCloud rdbCloud(cloudDB, &bindAssets);

    auto err = rdbCloud.UnLockCloudDB(OHOS::DistributedRdb::RdbCloud::FLAG::SYSTEM_ABILITY);
    EXPECT_EQ(err, GeneralError::E_NOT_SUPPORT);

    auto result = rdbCloud.LockCloudDB(OHOS::DistributedRdb::RdbCloud::FLAG::SYSTEM_ABILITY);
    EXPECT_EQ(result.first, GeneralError::E_NOT_SUPPORT);

    rdbCloud.flag_ = 1;
    err = rdbCloud.InnerUnLock(static_cast<OHOS::DistributedRdb::RdbCloud::FLAG>(0));
    EXPECT_EQ(err, GeneralError::E_OK);
}

/**
* @tc.name: ConvertStatus
* @tc.desc: RdbCloud ConvertStatus function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbCloudTest, ConvertStatus, TestSize.Level1)
{
    BindAssets bindAssets;
    std::shared_ptr<CloudDB> cloudDB = std::make_shared<CloudDB>();
    RdbCloud rdbCloud(cloudDB, &bindAssets);
    auto result = rdbCloud.ConvertStatus(GeneralError::E_OK);
    EXPECT_EQ(result, DBStatus::OK);
    result = rdbCloud.ConvertStatus(GeneralError::E_NETWORK_ERROR);
    EXPECT_EQ(result, DBStatus::CLOUD_NETWORK_ERROR);
    result = rdbCloud.ConvertStatus(GeneralError::E_LOCKED_BY_OTHERS);
    EXPECT_EQ(result, DBStatus::CLOUD_LOCK_ERROR);
    result = rdbCloud.ConvertStatus(GeneralError::E_RECODE_LIMIT_EXCEEDED);
    EXPECT_EQ(result, DBStatus::CLOUD_FULL_RECORDS);
    result = rdbCloud.ConvertStatus(GeneralError::E_NO_SPACE_FOR_ASSET);
    EXPECT_EQ(result, DBStatus::CLOUD_ASSET_SPACE_INSUFFICIENT);
    result = rdbCloud.ConvertStatus(GeneralError::E_RECORD_EXIST_CONFLICT);
    EXPECT_EQ(result, DBStatus::CLOUD_RECORD_EXIST_CONFLICT);
    result = rdbCloud.ConvertStatus(GeneralError::E_VERSION_CONFLICT);
    EXPECT_EQ(result, DBStatus::CLOUD_VERSION_CONFLICT);
    result = rdbCloud.ConvertStatus(GeneralError::E_RECORD_NOT_FOUND);
    EXPECT_EQ(result, DBStatus::CLOUD_RECORD_NOT_FOUND);
    result = rdbCloud.ConvertStatus(GeneralError::E_RECORD_ALREADY_EXISTED);
    EXPECT_EQ(result, DBStatus::CLOUD_RECORD_ALREADY_EXISTED);
    result = rdbCloud.ConvertStatus(GeneralError::E_FILE_NOT_EXIST);
    EXPECT_EQ(result, DBStatus::LOCAL_ASSET_NOT_FOUND);
    result = rdbCloud.ConvertStatus(GeneralError::E_TIME_OUT);
    EXPECT_EQ(result, DBStatus::TIME_OUT);
    result = rdbCloud.ConvertStatus(GeneralError::E_CLOUD_DISABLED);
    EXPECT_EQ(result, DBStatus::CLOUD_DISABLED);
}

/**
* @tc.name: ConvertQuery
* @tc.desc: RdbCloud ConvertQuery function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbCloudTest, ConvertQuery, TestSize.Level1)
{
    RdbCloud::DBQueryNodes nodes;
    DistributedDB::QueryNode node = { DistributedDB::QueryNodeType::IN, "",  {int64_t(1)} };
    nodes.push_back(node);
    node = { DistributedDB::QueryNodeType::OR, "",  {int64_t(1)} };
    nodes.push_back(node);
    node = { DistributedDB::QueryNodeType::AND, "",  {int64_t(1)} };
    nodes.push_back(node);
    node = { DistributedDB::QueryNodeType::EQUAL_TO, "",  {int64_t(1)} };
    nodes.push_back(node);
    node = { DistributedDB::QueryNodeType::BEGIN_GROUP, "",  {int64_t(1)} };
    nodes.push_back(node);
    node = { DistributedDB::QueryNodeType::END_GROUP, "",  {int64_t(1)} };
    nodes.push_back(node);
    
    auto result = RdbCloud::ConvertQuery(std::move(nodes));
    EXPECT_EQ(result.size(), 6);

    nodes.clear();
    node = { DistributedDB::QueryNodeType::ILLEGAL, "",  {int64_t(1)} };
    nodes.push_back(node);
    result = RdbCloud::ConvertQuery(std::move(nodes));
    EXPECT_EQ(result.size(), 0);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test