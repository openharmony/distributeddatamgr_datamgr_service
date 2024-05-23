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

#include "gtest/gtest.h"
#include "log_print.h"
#include "rdb_cloud.h"
#include "rdb_cloud_data_translate.h"

using namespace testing::ext;
using namespace OHOS::DistributedData;
using namespace OHOS::DistributedRdb;
using DBVBucket = DistributedDB::VBucket;
using DBStatus = DistributedDB::DBStatus;
namespace OHOS::Test {
namespace DistributedRDBTest {
class RdbCloudTest : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
protected:
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
    std::shared_ptr<CloudDB> cloudDB  = std::make_shared<CloudDB>();
    RdbCloud rdbCloud(cloudDB, &bindAssets);
    std::string tableName = "testTable";
    std::vector<DBVBucket> record = {
        {{"#gid", {"0000000"}}, {"#flag", {true}}, {"#value", {int64_t(100)}}, {"#float", {double(100)}},
        {"#_type", {int64_t(1)}}, {"#_query", {Bytes(bytes)}}}
    };
    std::vector<DBVBucket> extend = {
        {{"#gid", {"0000000"}}, {"#flag", {true}}, {"#value", {int64_t(100)}}, {"#float", {double(100)}},
        {"#_type", {int64_t(1)}}, {"#_query", {Bytes(bytes)}}}
    };
    auto result = rdbCloud.BatchInsert(tableName, std::move(record), extend);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
    result = rdbCloud.BatchUpdate(tableName, std::move(record), extend);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
    result = rdbCloud.BatchDelete(tableName, extend);
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
    Bytes bytes;
    std::shared_ptr<CloudDB> cloudDB  = std::make_shared<CloudDB>();
    RdbCloud rdbCloud(cloudDB, &bindAssets);
    std::string tableName = "testTable";
    std::vector<DBVBucket> record = {
        {{"#gid", {"0000000"}}, {"#flag", {true}}, {"#value", {int64_t(100)}}, {"#float", {double(100)}},
        {"#_type", {int64_t(1)}}, {"#_query", {Bytes(bytes)}}}
    };
    std::vector<DBVBucket> extend = {
        {{"#gid", {"0000000"}}, {"#flag", {true}}, {"#value", {int64_t(100)}}, {"#float", {double(100)}},
        {"#_type", {int64_t(1)}}, {"#_query", {Bytes(bytes)}}}
    };
    rdbCloud.Lock();
    auto result = rdbCloud.BatchInsert(tableName, std::move(record), extend);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
    DBVBucket extends = {
        {"#gid", {"0000000"}}, {"#flag", {true}}, {"#value", {int64_t(100)}}, {"#float", {double(100)}},
        {"#_type", {int64_t(1)}}, {"#_query", {Bytes(bytes)}}
    };
    std::vector<DBVBucket> data = {
        {{"#gid", {"0000000"}}, {"#flag", {true}}, {"#value", {int64_t(100)}}, {"#float", {double(100)}},
        {"#_type", {int64_t(1)}}, {"#_query", {Bytes(bytes)}}}
    };
    result = rdbCloud.Query(tableName, extends, data);
    EXPECT_EQ(result, DBStatus::CLOUD_ERROR);
    std::vector<VBucket> vBuckets = {
        {{"#gid", {"0000000"}}, {"#flag", {true}}, {"#value", {int64_t(100)}}, {"#float", {double(100)}},
        {"#_type", {int64_t(1)}}, {"#_query", {Bytes(bytes)}}}
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
    Bytes bytes;
    std::shared_ptr<CloudDB> cloudDB  = std::make_shared<CloudDB>();
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
        {"#_query", {Bytes(bytes)}}
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
* @tc.name: ConvertStatus
* @tc.desc: RdbCloud ConvertStatus function test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbCloudTest, ConvertStatus, TestSize.Level1)
{
    BindAssets bindAssets;
    std::shared_ptr<CloudDB> cloudDB  = std::make_shared<CloudDB>();
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
}

/**
* @tc.name: BlobToAssets
* @tc.desc: rdb_cloud_data_translate BlobToAsset error test.
* @tc.type: FUNC
* @tc.require:
* @tc.author: SQL
*/
HWTEST_F(RdbCloudTest, BlobToAssets, TestSize.Level1)
{
    RdbCloudDataTranslate rdbTranslate;
    DistributedDB::Asset asset = {
        .name = "", .assetId = "", .subpath = "", .uri = "",
        .modifyTime = "", .createTime = "", .size = "", .hash = ""
    };
    std::vector<uint8_t> blob;
    auto result = rdbTranslate.BlobToAsset(blob);
    EXPECT_EQ(result, asset);

    DistributedDB::Assets assets;
    blob = rdbTranslate.AssetsToBlob(assets);
    auto results = rdbTranslate.BlobToAssets(blob);
    EXPECT_EQ(results, assets);
}
} // namespace DistributedRDBTest
} // namespace OHOS::Test