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

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "cloud/cloud_db.h"
#include "error/general_error.h"
#include "store/general_value.h"
#include "store/cursor.h"

#include "cloud_db_mock.h"
#include "cursor_mock.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "log_print.h"

using namespace testing::ext;
using namespace testing;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedRdb;

// Fake CloudDB implementation that returns E_NOT_SUPPORT (same as CloudDB base class)
// This avoids vtable issues and mock ambiguity problems
class FakeCloudDB : public CloudDB {
public:
    int32_t Execute(const std::string &table, const std::string &sql, const VBucket &extend) override
    {
        return GeneralError::E_NOT_SUPPORT;
    }
    int32_t BatchInsert(const std::string &table, VBuckets &&values, VBuckets &extends) override
    {
        return GeneralError::E_NOT_SUPPORT;
    }
    int32_t BatchUpdate(const std::string &table, VBuckets &&values, VBuckets &extends) override
    {
        return GeneralError::E_NOT_SUPPORT;
    }
    int32_t BatchUpdate(const std::string &table, VBuckets &&values, const VBuckets &extends) override
    {
        return GeneralError::E_NOT_SUPPORT;
    }
    int32_t BatchDelete(const std::string &table, VBuckets &extends) override
    {
        return GeneralError::E_NOT_SUPPORT;
    }
    std::pair<int32_t, std::shared_ptr<Cursor>> Query(const std::string &table, const VBucket &extend) override
    {
        return { GeneralError::E_NOT_SUPPORT, nullptr };
    }
    std::pair<int32_t, std::shared_ptr<Cursor>> Query(GenQuery &query, const VBucket &extend) override
    {
        return { GeneralError::E_NOT_SUPPORT, nullptr };
    }
    int32_t PreSharing(const std::string &table, VBuckets &extend) override
    {
        return GeneralError::E_NOT_SUPPORT;
    }
    int32_t Sync(const Devices &devices, int32_t mode, const GenQuery &query, Async async, int32_t wait) override
    {
        return GeneralError::E_NOT_SUPPORT;
    }
    int32_t Watch(int32_t origin, Watcher &watcher) override
    {
        return GeneralError::E_NOT_SUPPORT;
    }
    int32_t Unwatch(int32_t origin, Watcher &watcher) override
    {
        return GeneralError::E_NOT_SUPPORT;
    }
    int32_t Lock() override
    {
        return GeneralError::E_NOT_SUPPORT;
    }
    int32_t Heartbeat() override
    {
        return GeneralError::E_NOT_SUPPORT;
    }
    int32_t Unlock() override
    {
        return GeneralError::E_NOT_SUPPORT;
    }
    int64_t AliveTime() override
    {
        return -1;
    }
    int32_t Close() override
    {
        return GeneralError::E_NOT_SUPPORT;
    }
    std::pair<int32_t, std::string> GetEmptyCursor(const std::string &tableName) override
    {
        return { GeneralError::E_NOT_SUPPORT, "" };
    }
    void SetPrepareTraceId(const std::string &prepareTraceId) override {}
};
using DBVBucket = DistributedDB::VBucket;
using DBStatus = DistributedDB::DBStatus;
using DBAsset = DistributedDB::Asset;
using DBAssets = DistributedDB::Assets;
using AssetOpType = DistributedDB::AssetOpType;
using AssetStatus = DistributedDB::AssetStatus;
DBAsset assetValue1 = { .version = 1,
    .name = "texture_diffuse",
    .assetId = "123",
    .subpath = "textures/environment",
    .uri = "http://asset.com/textures/123.jpg",
    .modifyTime = "2025-04-05T12:30:00Z",
    .createTime = "2025-04-05T10:15:00Z",
    .size = "1024",
    .hash = "sha256-abc123",
    .flag = static_cast<uint32_t>(AssetOpType::INSERT),
    .status = static_cast<uint32_t>(AssetStatus::NORMAL),
    .timestamp = std::time(nullptr) };

DBAsset assetValue2 = { .version = 2,
    .name = "texture_diffuse",
    .assetId = "456",
    .subpath = "textures/environment",
    .uri = "http://asset.com/textures/456.jpg",
    .modifyTime = "2025-06-19T12:30:00Z",
    .createTime = "2025-04-05T10:15:00Z",
    .size = "1024",
    .hash = "sha256-abc456",
    .flag = static_cast<uint32_t>(AssetOpType::UPDATE),
    .status = static_cast<uint32_t>(AssetStatus::NORMAL),
    .timestamp = std::time(nullptr) };

DBAssets assetsValue = { assetValue1, assetValue2 };
std::vector<DBVBucket> g_DBVBuckets = { { { "#gid", { "0000000" } }, { "#flag", { true } },
    { "#value", { int64_t(100) } }, { "#float", { double(100) } }, { "#_type", { int64_t(1) } },
    { "#_error", { "E_ERROR" } }, { "#_error", { "INVALID" } }, { "#_query", { Bytes({ 1, 2, 3, 4 }) } },
    { "#asset", assetValue1 }, { "#assets", assetsValue } } };
DBVBucket g_DBVBucket = { { "#value", { int64_t(100) } } };

namespace OHOS::Test {
namespace DistributedRDBTest {
class RdbCloudTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp(){};
    void TearDown(){};
    static inline std::shared_ptr<MockCloudDB> mockCloudDB = nullptr;
    static constexpr int32_t count = 2;
};

void RdbCloudTest::SetUpTestCase()
{
    mockCloudDB = std::make_shared<MockCloudDB>();
}

void RdbCloudTest::TearDownTestCase()
{
    mockCloudDB = nullptr;
}

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
    RdbCloud rdbCloud(std::make_shared<FakeCloudDB>(), bindAssets);
    std::string tableName = "testTable";

    std::vector<DBVBucket> dataInsert = g_DBVBuckets;
    std::vector<DBVBucket> extendInsert = g_DBVBuckets;
    auto result = rdbCloud.BatchInsert(tableName, std::move(dataInsert), extendInsert);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);

    std::vector<DBVBucket> dataUpdate = g_DBVBuckets;
    std::vector<DBVBucket> extendUpdate = g_DBVBuckets;
    result = rdbCloud.BatchUpdate(tableName, std::move(dataUpdate), extendUpdate);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);

    std::vector<DBVBucket> extendDelete = g_DBVBuckets;
    result = rdbCloud.BatchDelete(tableName, extendDelete);
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
    RdbCloud rdbCloud(std::make_shared<FakeCloudDB>(), bindAssets);
    std::string tableName = "testTable";
    rdbCloud.Lock();
    std::string traceId = "id";
    rdbCloud.SetPrepareTraceId(traceId);
    std::vector<DBVBucket> dataInsert = g_DBVBuckets;
    std::vector<DBVBucket> extendInsert = g_DBVBuckets;
    auto result = rdbCloud.BatchInsert(tableName, std::move(dataInsert), extendInsert);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
    DBVBucket extends = { { "#gid", { "00000" } }, { "#flag", { true } }, { "#value", { int64_t(100) } },
        { "#float", { double(100) } }, { "#_type", { int64_t(1) } }, { "#_query", { Bytes({ 1, 2, 3, 4 }) } } };
    result = rdbCloud.Query(tableName, extends, g_DBVBuckets);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
    std::vector<VBucket> vBuckets = { { { "#gid", { "00000" } }, { "#flag", { true } }, { "#value", { int64_t(100) } },
        { "#float", { double(100) } }, { "#_type", { int64_t(1) } }, { "#_query", { Bytes({ 1, 2, 3, 4 }) } } } };
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
    BindAssets bindAssets = nullptr;
    RdbCloud rdbCloud(std::make_shared<FakeCloudDB>(), bindAssets);
    std::string tableName = "testTable";
    DBVBucket extends = { { "#gid", { "0000000" } }, { "#flag", { true } }, { "#value", { int64_t(100) } },
        { "#float", { double(100) } }, { "#_type", { int64_t(1) } } };
    std::vector<DBVBucket> data = { { { "#gid", { "0000000" } }, { "#flag", { true } }, { "#value", { int64_t(100) } },
        { "#float", { double(100) } } } };
    auto result = rdbCloud.Query(tableName, extends, data);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);

    extends = { { "#gid", { "0000000" } }, { "#flag", { true } }, { "#value", { int64_t(100) } },
        { "#float", { double(100) } }, { "#_query", { Bytes({ 1, 2, 3, 4 }) } } };
    result = rdbCloud.Query(tableName, extends, data);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);

    extends = { { "#gid", { "0000000" } }, { "#flag", { true } }, { "#value", { int64_t(100) } },
        { "#float", { double(100) } }, { "#_type", { int64_t(0) } } };
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
    RdbCloud rdbCloud(std::make_shared<FakeCloudDB>(), bindAssets);

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
    RdbCloud rdbCloud(std::make_shared<FakeCloudDB>(), bindAssets);
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
    result = rdbCloud.ConvertStatus(GeneralError::E_SKIP_WHEN_CLOUD_SPACE_INSUFFICIENT);
    EXPECT_EQ(result, DBStatus::SKIP_WHEN_CLOUD_SPACE_INSUFFICIENT);
    result = rdbCloud.ConvertStatus(GeneralError::E_EXPIRED_CURSOR);
    EXPECT_EQ(result, DBStatus::EXPIRED_CURSOR);
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
    DistributedDB::QueryNode node = { DistributedDB::QueryNodeType::IN, "", { int64_t(1) } };
    nodes.push_back(node);
    node = { DistributedDB::QueryNodeType::OR, "", { int64_t(1) } };
    nodes.push_back(node);
    node = { DistributedDB::QueryNodeType::AND, "", { int64_t(1) } };
    nodes.push_back(node);
    node = { DistributedDB::QueryNodeType::EQUAL_TO, "", { int64_t(1) } };
    nodes.push_back(node);
    node = { DistributedDB::QueryNodeType::BEGIN_GROUP, "", { int64_t(1) } };
    nodes.push_back(node);
    node = { DistributedDB::QueryNodeType::END_GROUP, "", { int64_t(1) } };
    nodes.push_back(node);

    auto result = RdbCloud::ConvertQuery(std::move(nodes));
    EXPECT_EQ(result.size(), 6);

    nodes.clear();
    node = { DistributedDB::QueryNodeType::ILLEGAL, "", { int64_t(1) } };
    nodes.push_back(node);
    result = RdbCloud::ConvertQuery(std::move(nodes));
    EXPECT_EQ(result.size(), 0);
}

/**
* @tc.name: SetPrepareTraceId001
* @tc.desc: RdbCloud PostEvent test cloudDB_ is nullptr.
* @tc.type: FUNC
*/
HWTEST_F(RdbCloudTest, SetPrepareTraceId001, TestSize.Level1)
{
    std::string traceId = "testId";
    EXPECT_CALL(*mockCloudDB, SetPrepareTraceId(_)).Times(0);
    BindAssets bindAssets;
    std::shared_ptr<CloudDB> nullCloudDB = nullptr;
    RdbCloud rdbCloud(nullCloudDB, bindAssets);
    rdbCloud.SetPrepareTraceId(traceId);
}

/**
* @tc.name: PostEvent001
* @tc.desc: RdbCloud PostEvent test bindAssets is nullptr.
* @tc.type: FUNC
*/
HWTEST_F(RdbCloudTest, PostEvent001, TestSize.Level1)
{
    BindAssets bindAssets;
    RdbCloud rdbCloud(std::make_shared<FakeCloudDB>(), nullptr);
    std::string traceId = "testId";
    rdbCloud.SetPrepareTraceId(traceId);
    std::string tableName = "testTable";
    std::vector<DBVBucket> dataInsert = g_DBVBuckets;
    std::vector<DBVBucket> extendInsert = g_DBVBuckets;
    auto result = rdbCloud.BatchInsert(tableName, std::move(dataInsert), extendInsert);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
}

/**
* @tc.name: PostEvent002
* @tc.desc: RdbCloud PostEvent test snapshots contains asset.uri.
* @tc.type: FUNC
*/
HWTEST_F(RdbCloudTest, PostEvent002, TestSize.Level1)
{
    BindAssets snapshots = std::make_shared<std::map<std::string, std::shared_ptr<Snapshot>>>();
    std::shared_ptr<Snapshot> snapshot;
    snapshots->insert_or_assign(assetValue1.uri, snapshot);
    RdbCloud rdbCloud(std::make_shared<FakeCloudDB>(), snapshots);
    std::string tableName = "testTable";
    std::vector<DBVBucket> dataInsert = g_DBVBuckets;
    std::vector<DBVBucket> extendInsert = g_DBVBuckets;
    auto result = rdbCloud.BatchInsert(tableName, std::move(dataInsert), extendInsert);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
}

/**
* @tc.name: PostEvent003
* @tc.desc: RdbCloud PostEvent test snapshots does not contains asset.uri.
* @tc.type: FUNC
*/
HWTEST_F(RdbCloudTest, PostEvent003, TestSize.Level1)
{
    BindAssets snapshots = std::make_shared<std::map<std::string, std::shared_ptr<Snapshot>>>();
    std::shared_ptr<Snapshot> snapshot;
    std::string uri = "testuri";
    snapshots->insert_or_assign(uri, snapshot);
    RdbCloud rdbCloud(std::make_shared<FakeCloudDB>(), snapshots);
    std::string tableName = "testTable";
    std::vector<DBVBucket> dataInsert = g_DBVBuckets;
    std::vector<DBVBucket> extendInsert = g_DBVBuckets;
    auto result = rdbCloud.BatchInsert(tableName, std::move(dataInsert), extendInsert);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
}

/**
* @tc.name: PostEvent004
* @tc.desc: RdbCloud PostEvent test the snapshot contained in the snapshots is nullpt.
* @tc.type: FUNC
*/
HWTEST_F(RdbCloudTest, PostEvent004, TestSize.Level1)
{
    BindAssets snapshots = std::make_shared<std::map<std::string, std::shared_ptr<Snapshot>>>();
    std::shared_ptr<Snapshot> snapshot = nullptr;
    std::string uri = "testuri";
    snapshots->insert_or_assign(uri, snapshot);
    RdbCloud rdbCloud(std::make_shared<FakeCloudDB>(), snapshots);
    std::string tableName = "testTable";
    std::vector<DBVBucket> dataInsert = g_DBVBuckets;
    std::vector<DBVBucket> extendInsert = g_DBVBuckets;
    auto result = rdbCloud.BatchInsert(tableName, std::move(dataInsert), extendInsert);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
}

/**
* @tc.name: Query001
* @tc.desc: RdbCloud Query test the cursor is nullptr and code is not E_OK.
* @tc.type: FUNC
*/
HWTEST_F(RdbCloudTest, Query001, TestSize.Level1)
{
    std::shared_ptr<Cursor> cursor = nullptr;
    std::string tableName = "testTable";
    EXPECT_CALL(*mockCloudDB, Query(tableName, _)).WillOnce(Return(std::make_pair(E_NETWORK_ERROR, cursor)));
    BindAssets snapshots;
    RdbCloud rdbCloud(mockCloudDB, snapshots);
    auto result = rdbCloud.Query(tableName, g_DBVBucket, g_DBVBuckets);
    EXPECT_EQ(result, DBStatus::CLOUD_NETWORK_ERROR);
}

/**
* @tc.name: Query002
* @tc.desc: RdbCloud Query test code is QUERY_END.
* @tc.type: FUNC
*/
HWTEST_F(RdbCloudTest, Query002, TestSize.Level1)
{
    std::shared_ptr<MockCursor> mockCursor = std::make_shared<MockCursor>();
    std::string tableName = "testTable";
    EXPECT_CALL(*mockCloudDB, Query(tableName, _)).WillOnce(Return(std::make_pair(E_OK, mockCursor)));
    EXPECT_CALL(*mockCursor, GetCount()).WillOnce(Return(count));
    EXPECT_CALL(*mockCursor, GetEntry(_)).WillOnce(Return(E_OK)).WillOnce(Return(E_ERROR));
    EXPECT_CALL(*mockCursor, MoveToNext()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*mockCursor, IsEnd()).WillOnce(Return(true));

    BindAssets snapshots;
    RdbCloud rdbCloud(mockCloudDB, snapshots);
    auto result = rdbCloud.Query(tableName, g_DBVBucket, g_DBVBuckets);
    EXPECT_EQ(result, DBStatus::QUERY_END);
}

/**
* @tc.name: Query003
* @tc.desc: RdbCloud Query test code is CLOUD_ERROR.
* @tc.type: FUNC
*/
HWTEST_F(RdbCloudTest, Query003, TestSize.Level1)
{
    std::shared_ptr<MockCursor> mockCursor = std::make_shared<MockCursor>();
    std::string tableName = "testTable";
    EXPECT_CALL(*mockCloudDB, Query(tableName, _)).WillOnce(Return(std::make_pair(E_OK, mockCursor)));
    EXPECT_CALL(*mockCursor, GetCount()).WillOnce(Return(count));
    EXPECT_CALL(*mockCursor, GetEntry(_)).WillOnce(Return(E_OK)).WillOnce(Return(E_ERROR));
    EXPECT_CALL(*mockCursor, MoveToNext()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*mockCursor, IsEnd()).WillOnce(Return(false));

    BindAssets snapshots;
    RdbCloud rdbCloud(mockCloudDB, snapshots);
    auto result = rdbCloud.Query(tableName, g_DBVBucket, g_DBVBuckets);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
}

/**
* @tc.name: Query004
* @tc.desc: RdbCloud Query test code is CLOUD_ERROR.
* @tc.type: FUNC
*/
HWTEST_F(RdbCloudTest, Query004, TestSize.Level1) {
    std::shared_ptr<MockCursor> mockCursor = std::make_shared<MockCursor>();
    std::string tableName = "testTable";
    EXPECT_CALL(*mockCloudDB, Query(tableName, _)).WillOnce(Return(std::make_pair(E_ERROR, mockCursor)));

    BindAssets snapshots;
    RdbCloud rdbCloud(mockCloudDB, snapshots);
    auto result = rdbCloud.Query(tableName, g_DBVBucket, g_DBVBuckets);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
}

/**
* @tc.name: QueryAllGid_CursorIsNull
* @tc.desc: RdbCloud QueryAllGidy test the cursor is nullptr.
* @tc.type: FUNC
*/
HWTEST_F(RdbCloudTest, QueryAllGid_CursorIsNull, TestSize.Level1)
{
    std::shared_ptr<Cursor> cursor = nullptr;
    std::string tableName = "testTable";
    EXPECT_CALL(*mockCloudDB, Query(tableName, _)).WillOnce(Return(std::make_pair(E_OK, cursor)));
    BindAssets snapshots;
    RdbCloud rdbCloud(mockCloudDB, snapshots);
    auto result = rdbCloud.QueryAllGid(tableName, g_DBVBucket, g_DBVBuckets);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
}

/**
* @tc.name: QueryAllGid_Abormal
* @tc.desc: RdbCloud QueryAllGidy test the code is not E_OK.
* @tc.type: FUNC
*/
HWTEST_F(RdbCloudTest, QueryAllGid_Abormal, TestSize.Level1)
{
    std::shared_ptr<MockCursor> mockCursor = std::make_shared<MockCursor>();
    std::string tableName = "testTable";
    EXPECT_CALL(*mockCloudDB, Query(tableName, _)).WillOnce(Return(std::make_pair(E_NETWORK_ERROR, mockCursor)));
    BindAssets snapshots;
    RdbCloud rdbCloud(mockCloudDB, snapshots);
    auto result = rdbCloud.QueryAllGid(tableName, g_DBVBucket, g_DBVBuckets);
    EXPECT_EQ(result, DBStatus::CLOUD_NETWORK_ERROR);
}

/**
* @tc.name: QueryAllGid_End
* @tc.desc: RdbCloud QueryAllGid test code is QUERY_END.
* @tc.type: FUNC
*/
HWTEST_F(RdbCloudTest, QueryAllGid_End, TestSize.Level1)
{
    std::shared_ptr<MockCursor> mockCursor = std::make_shared<MockCursor>();
    std::string tableName = "testTable";
    EXPECT_CALL(*mockCloudDB, Query(tableName, _)).WillOnce(Return(std::make_pair(E_OK, mockCursor)));
    EXPECT_CALL(*mockCursor, GetCount()).WillOnce(Return(count));
    EXPECT_CALL(*mockCursor, GetEntry(_)).WillOnce(Return(E_OK)).WillOnce(Return(E_ERROR));
    EXPECT_CALL(*mockCursor, MoveToNext()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*mockCursor, IsEnd()).WillOnce(Return(true));

    BindAssets snapshots;
    RdbCloud rdbCloud(mockCloudDB, snapshots);
    auto result = rdbCloud.QueryAllGid(tableName, g_DBVBucket, g_DBVBuckets);
    EXPECT_EQ(result, DBStatus::QUERY_END);
}

/**
* @tc.name: QueryAllGid_NotEnd
* @tc.desc: RdbCloud QueryAllGid test code is not QUERY_END.
* @tc.type: FUNC
*/
HWTEST_F(RdbCloudTest, QueryAllGid_NotEnd, TestSize.Level1)
{
    std::shared_ptr<MockCursor> mockCursor = std::make_shared<MockCursor>();
    std::string tableName = "testTable";
    EXPECT_CALL(*mockCloudDB, Query(tableName, _)).WillOnce(Return(std::make_pair(E_OK, mockCursor)));
    EXPECT_CALL(*mockCursor, GetCount()).WillOnce(Return(count));
    EXPECT_CALL(*mockCursor, GetEntry(_)).WillOnce(Return(E_OK)).WillOnce(Return(E_OK));
    EXPECT_CALL(*mockCursor, MoveToNext()).WillRepeatedly(Return(E_OK));
    EXPECT_CALL(*mockCursor, IsEnd()).WillOnce(Return(false));

    BindAssets snapshots;
    RdbCloud rdbCloud(mockCloudDB, snapshots);
    auto result = rdbCloud.QueryAllGid(tableName, g_DBVBucket, g_DBVBuckets);
    EXPECT_EQ(result, DBStatus::OK);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test